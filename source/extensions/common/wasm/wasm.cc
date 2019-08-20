#include "extensions/common/wasm/wasm.h"

#include <stdio.h>

#include <limits>
#include <memory>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/grpc/status.h"
#include "envoy/http/codes.h"
#include "envoy/local_info/local_info.h"
#include "envoy/server/wasm.h"
#include "envoy/thread_local/thread_local.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/empty_string.h"
#include "common/common/enum_to_int.h"
#include "common/common/logger.h"
#include "common/config/datasource.h"
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/common/wasm/well_known_names.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "absl/synchronization/mutex.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

// Any currently executing Wasm call context.
#define WASM_CONTEXT(_c)                                                                           \
  (ContextOrEffectiveContext(static_cast<Context*>((void)_c, current_context_)))
// The id of the context which should be used for calls out of the VM in place of current_context_
// above.

namespace {

inline Word wasmResultToWord(WasmResult r) { return Word(static_cast<uint64_t>(r)); }

inline uint32_t convertWordToUint32(Word w) { return static_cast<uint32_t>(w.u64); }

// Convert a function of the form Word(Word...) to one of the form uint32_t(uint32_t...).
template <typename F, F* fn> struct ConvertFunctionWordToUint32 {
  static void convertFunctionWordToUint32() {}
};
template <typename R, typename... Args, auto (*F)(Args...)->R>
struct ConvertFunctionWordToUint32<R(Args...), F> {
  static auto convertFunctionWordToUint32(typename ConvertWordTypeToUint32<Args>::type... args) {
    return convertWordToUint32(F(std::forward<Args>(args)...));
  }
};
template <typename... Args, auto (*F)(Args...)->void>
struct ConvertFunctionWordToUint32<void(Args...), F> {
  static void convertFunctionWordToUint32(typename ConvertWordTypeToUint32<Args>::type... args) {
    F(std::forward<Args>(args)...);
  }
};

class SharedData {
public:
  WasmResult get(absl::string_view vm_id, const absl::string_view key,
                 std::pair<std::string, uint32_t>* result) {
    absl::ReaderMutexLock l(&mutex);
    auto map = data.find(vm_id);
    if (map == data.end()) {
      return WasmResult::NotFound;
    }
    auto it = map->second.find(key);
    if (it != map->second.end()) {
      *result = it->second;
      return WasmResult::Ok;
    }
    return WasmResult::NotFound;
  }

  WasmResult set(absl::string_view vm_id, absl::string_view key, absl::string_view value,
                 uint32_t cas) {
    absl::WriterMutexLock l(&mutex);
    absl::flat_hash_map<std::string, std::pair<std::string, uint32_t>>* map;
    auto map_it = data.find(vm_id);
    if (map_it == data.end()) {
      map = &data[vm_id];
    } else {
      map = &map_it->second;
    }
    auto it = map->find(key);
    if (it != map->end()) {
      if (cas && cas != it->second.second) {
        return WasmResult::CasMismatch;
      }
      it->second = std::make_pair(std::string(value), nextCas());
    } else {
      map->emplace(key, std::make_pair(std::string(value), nextCas()));
    }
    return WasmResult::Ok;
  }

  uint32_t registerQueue(absl::string_view vm_id, absl::string_view queue_name, uint32_t context_id,
                         Event::Dispatcher& dispatcher) {
    absl::WriterMutexLock l(&mutex);
    auto key = std::make_pair(std::string(vm_id), std::string(queue_name));
    auto it = queue_tokens.insert(std::make_pair(key, static_cast<uint32_t>(0)));
    if (it.second) {
      it.first->second = nextQueueToken();
      queue_token_set.insert(it.first->second);
    }
    uint32_t token = it.first->second;
    auto& q = queues[token];
    q.vm_id = std::string(vm_id);
    q.context_id = context_id;
    q.dispatcher = &dispatcher;
    // Preserve any existing data.
    return token;
  }

  uint32_t resolveQueue(absl::string_view vm_id, absl::string_view queue_name) {
    absl::WriterMutexLock l(&mutex);
    auto key = std::make_pair(std::string(vm_id), std::string(queue_name));
    auto it = queue_tokens.find(key);
    if (it != queue_tokens.end()) {
      return it->second;
    }
    return 0; // N.B. zero indicates that the queue was not found.
  }

  WasmResult dequeue(uint32_t token, std::string* data) {
    absl::ReaderMutexLock l(&mutex);
    auto it = queues.find(token);
    if (it == queues.end()) {
      return WasmResult::NotFound;
    }
    if (it->second.queue.empty()) {
      return WasmResult::Empty;
    }
    *data = it->second.queue.front();
    it->second.queue.pop_front();
    return WasmResult::Ok;
  }

  WasmResult enqueue(uint32_t token, absl::string_view value) {
    absl::WriterMutexLock l(&mutex);
    auto it = queues.find(token);
    if (it == queues.end()) {
      return WasmResult::NotFound;
    }
    it->second.queue.push_back(std::string(value));
    auto vm_id = it->second.vm_id;
    auto context_id = it->second.context_id;
    it->second.dispatcher->post([vm_id, context_id, token] {
      auto wasm = getThreadLocalWasmOrNull(vm_id);
      if (wasm) {
        wasm->queueReady(context_id, token);
      }
    });
    return WasmResult::Ok;
  }

  uint32_t nextCas() {
    auto result = cas;
    cas++;
    if (!cas) { // 0 is not a valid CAS value.
      cas++;
    }
    return result;
  }

private:
  uint32_t nextQueueToken() {
    while (true) {
      uint32_t token = next_queue_token++;
      if (token == 0) {
        continue; // 0 is an illegal token.
      }
      if (queue_token_set.find(token) == queue_token_set.end()) {
        return token;
      }
    }
  }

  struct Queue {
    std::string vm_id;
    uint32_t context_id;
    Event::Dispatcher* dispatcher;
    std::deque<std::string> queue;
  };

  absl::Mutex mutex;
  uint32_t cas = 1;
  uint32_t next_queue_token = 1;
  absl::node_hash_map<std::string,
                      absl::flat_hash_map<std::string, std::pair<std::string, uint32_t>>>
      data;
  absl::node_hash_map<uint32_t, Queue> queues;
  struct pair_hash {
    template <class T1, class T2> std::size_t operator()(const std::pair<T1, T2>& pair) const {
      return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
  };
  absl::flat_hash_map<std::pair<std::string, std::string>, uint32_t, pair_hash> queue_tokens;
  absl::flat_hash_set<uint32_t> queue_token_set;
};

SharedData global_shared_data;

// Map from Wasm ID to the local Wasm instance.
thread_local absl::flat_hash_map<std::string, std::shared_ptr<Wasm>> local_wasms;

const std::string INLINE_STRING = "<inline>";

template <typename Pairs> size_t pairsSize(const Pairs& result) {
  size_t size = 4; // number of headers
  for (auto& p : result) {
    size += 8;                   // size of key, size of value
    size += p.first.size() + 1;  // null terminated key
    size += p.second.size() + 1; // null terminated value
  }
  return size;
}

template <typename Pairs> void marshalPairs(const Pairs& result, char* buffer) {
  char* b = buffer;
  *reinterpret_cast<uint32_t*>(b) = result.size();
  b += sizeof(uint32_t);
  for (auto& p : result) {
    *reinterpret_cast<uint32_t*>(b) = p.first.size();
    b += sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(b) = p.second.size();
    b += sizeof(uint32_t);
  }
  for (auto& p : result) {
    memcpy(b, p.first.data(), p.first.size());
    b += p.first.size();
    *b++ = 0;
    memcpy(b, p.second.data(), p.second.size());
    b += p.second.size();
    *b++ = 0;
  }
}

Pairs toPairs(absl::string_view buffer) {
  Pairs result;
  const char* b = buffer.data();
  if (buffer.size() < sizeof(uint32_t))
    return {};
  auto size = *reinterpret_cast<const uint32_t*>(b);
  b += sizeof(uint32_t);
  if (sizeof(uint32_t) + size * 2 * sizeof(uint32_t) > buffer.size()) {
    return {};
  }
  result.resize(size);
  for (uint32_t i = 0; i < size; i++) {
    result[i].first = absl::string_view(nullptr, *reinterpret_cast<const uint32_t*>(b));
    b += sizeof(uint32_t);
    result[i].second = absl::string_view(nullptr, *reinterpret_cast<const uint32_t*>(b));
    b += sizeof(uint32_t);
  }
  for (auto& p : result) {
    p.first = absl::string_view(b, p.first.size());
    b += p.first.size() + 1;
    p.second = absl::string_view(b, p.second.size());
    b += p.second.size() + 1;
  }
  return result;
}

template <typename Pairs>
bool getPairs(Context* context, const Pairs& result, uint64_t ptr_ptr, uint64_t size_ptr) {
  if (result.empty()) {
    return context->wasm()->copyToPointerSize("", ptr_ptr, size_ptr);
  }
  uint64_t size = pairsSize(result);
  uint64_t ptr;
  char* buffer = static_cast<char*>(context->wasm()->allocMemory(size, &ptr));
  marshalPairs(result, buffer);
  if (!context->wasmVm()->setWord(ptr_ptr, Word(ptr))) {
    return false;
  }
  if (!context->wasmVm()->setWord(size_ptr, Word(size))) {
    return false;
  }
  return true;
}

void exportPairs(Context* context, const Pairs& pairs, uint64_t* ptr_ptr, uint64_t* size_ptr) {
  if (pairs.empty()) {
    *ptr_ptr = 0;
    *size_ptr = 0;
    return;
  }
  uint64_t size = pairsSize(pairs);
  char* buffer = static_cast<char*>(context->wasm()->allocMemory(size, ptr_ptr));
  marshalPairs(pairs, buffer);
  *size_ptr = size;
}

Http::HeaderMapPtr buildHeaderMapFromPairs(const Pairs& pairs) {
  auto map = std::make_unique<Http::HeaderMapImpl>();
  for (auto& p : pairs) {
    // Note: because of the lack of a string_view interface for addCopy and
    // the lack of an interface to add an entry with an empty value and return
    // the entry, there is no efficient way to prevent either a double copy
    // of the valueor a double lookup of the entry.
    map->addCopy(Http::LowerCaseString(std::string(p.first)), std::string(p.second));
  }
  return map;
}

const ProtobufWkt::Struct*
getStructProtoFromMetadata(const envoy::api::v2::core::Metadata& metadata,
                           absl::string_view name = "") {
  if (name.empty()) {
    name = HttpFilters::HttpFilterNames::get().Wasm;
  }
  const auto filter_it = metadata.filter_metadata().find(std::string(name));
  if (filter_it == metadata.filter_metadata().end()) {
    return nullptr;
  }
  return &filter_it->second;
}

const ProtobufWkt::Struct* getRouteMetadataStructProto(Http::StreamFilterCallbacks* callbacks) {
  if (callbacks == nullptr || callbacks->route() == nullptr ||
      callbacks->route()->routeEntry() == nullptr) {
    return nullptr;
  }
  return getStructProtoFromMetadata(callbacks->route()->routeEntry()->metadata());
}

const uint8_t* decodeVarint(const uint8_t* pos, const uint8_t* end, uint32_t* out) {
  uint32_t ret = 0;
  int shift = 0;
  while (pos < end && (*pos & 0x80)) {
    ret |= (*pos & 0x7f) << shift;
    shift += 7;
    pos++;
  }
  if (pos < end) {
    ret |= *pos << shift;
    pos++;
  }
  *out = ret;
  return pos;
}

Context* ContextOrEffectiveContext(Context* context) {
  if (effective_context_id_ == 0) {
    return context;
  }
  auto effective_context = context->wasm()->getContext(effective_context_id_);
  if (effective_context) {
    return effective_context;
  }
  // The effective_context_id_ no longer exists, revert to the true context.
  return context;
}

} // namespace

// Test support.

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name) {
  return global_shared_data.resolveQueue(vm_id, queue_name);
}

//
// HTTP Handlers
//

// Metadata
Word getMetadataHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                        Word value_ptr_ptr, Word value_size_ptr) {
  if (type > static_cast<int>(MetadataType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  std::string value;
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  if (!key) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto result = context->getMetadata(static_cast<MetadataType>(type.u64), key.value(), &value);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->copyToPointerSize(value, value_ptr_ptr, value_size_ptr)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(result);
}

Word setMetadataHandler(void* raw_context, Word type, Word key_ptr, Word key_size, Word value_ptr,
                        Word value_size) {
  if (type > static_cast<int>(MetadataType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  auto value = context->wasmVm()->getMemory(value_ptr, value_size);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(
      context->setMetadata(static_cast<MetadataType>(type.u64), key.value(), value.value()));
}

Word getMetadataPairsHandler(void* raw_context, Word type, Word ptr_ptr, Word size_ptr) {
  if (type > static_cast<int>(MetadataType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  PairsWithStringValues pairs;
  auto result = context->getMetadataPairs(static_cast<MetadataType>(type.u64), &pairs);
  if (!getPairs(context, pairs, ptr_ptr, size_ptr)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(result);
}

Word getMetadataStructHandler(void* raw_context, Word type, Word name_ptr, Word name_size,
                              Word value_ptr_ptr, Word value_size_ptr) {
  if (type > static_cast<int>(MetadataType::MAX)) {
    return Word(static_cast<uint64_t>(WasmResult::BadArgument));
  }
  auto context = WASM_CONTEXT(raw_context);
  std::string value;
  auto name = context->wasmVm()->getMemory(name_ptr, name_size);
  if (!name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto result =
      context->getMetadataStruct(static_cast<MetadataType>(type.u64), name.value(), &value);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->copyToPointerSize(value, value_ptr_ptr, value_size_ptr)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word setMetadataStructHandler(void* raw_context, Word type, Word name_ptr, Word name_size,
                              Word value_ptr, Word value_size) {
  if (type > static_cast<int>(MetadataType::MAX)) {
    return Word(static_cast<uint64_t>(WasmResult::BadArgument));
  }
  auto context = WASM_CONTEXT(raw_context);
  auto name = context->wasmVm()->getMemory(name_ptr, name_size);
  auto value = context->wasmVm()->getMemory(value_ptr, value_size);
  if (!name || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return Word(static_cast<uint64_t>(context->setMetadataStruct(static_cast<MetadataType>(type.u64),
                                                               name.value(), value.value())));
}

// Continue/Reply/Route
Word continueRequestHandler(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->continueRequest();
  return wasmResultToWord(WasmResult::Ok);
}

Word continueResponseHandler(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->continueResponse();
  return wasmResultToWord(WasmResult::Ok);
}

Word sendLocalResponseHandler(void* raw_context, Word response_code, Word response_code_details_ptr,
                              Word response_code_details_size, Word body_ptr, Word body_size,
                              Word additional_response_header_pairs_ptr,
                              Word additional_response_header_pairs_size, Word grpc_code) {
  auto context = WASM_CONTEXT(raw_context);
  auto details =
      context->wasmVm()->getMemory(response_code_details_ptr, response_code_details_size);
  auto body = context->wasmVm()->getMemory(body_ptr, body_size);
  auto additional_response_header_pairs = context->wasmVm()->getMemory(
      additional_response_header_pairs_ptr, additional_response_header_pairs_size);
  if (!details || !body || !additional_response_header_pairs) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto additional_headers = toPairs(additional_response_header_pairs.value());
  auto modify_headers = [additional_headers](Http::HeaderMap& headers) {
    for (auto& p : additional_headers) {
      const Http::LowerCaseString lower_key(std::move(std::string(p.first)));
      headers.addCopy(lower_key, std::string(p.second));
    }
  };
  auto grpc_status = static_cast<Grpc::Status::GrpcStatus>(grpc_code.u64);
  auto grpc_status_opt = (grpc_status != Grpc::Status::GrpcStatus::InvalidCode)
                             ? absl::optional<Grpc::Status::GrpcStatus>(grpc_status)
                             : absl::optional<Grpc::Status::GrpcStatus>();
  context->sendLocalResponse(static_cast<Envoy::Http::Code>(response_code.u64), body.value(),
                             modify_headers, grpc_status_opt, details.value());
  return wasmResultToWord(WasmResult::Ok);
}

Word setEffectiveContextHandler(void* raw_context, Word context_id) {
  auto context = WASM_CONTEXT(raw_context);
  uint32_t cid = static_cast<uint32_t>(context_id.u64);
  auto c = context->wasm()->getContext(cid);
  if (!c) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  effective_context_id_ = cid;
  return wasmResultToWord(WasmResult::Ok);
}

Word clearRouteCacheHandler(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->clearRouteCache();
  return wasmResultToWord(WasmResult::Ok);
}

// SharedData
Word getSharedDataHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr_ptr,
                          Word value_size_ptr, Word cas_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  if (!key) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  std::pair<std::string, uint32_t> data;
  WasmResult result = context->getSharedData(key.value(), &data);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->copyToPointerSize(data.first, value_ptr_ptr, value_size_ptr)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  if (!context->wasmVm()->setMemory(cas_ptr, sizeof(uint32_t), &data.second)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word setSharedDataHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr,
                          Word value_size, Word cas) {
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  auto value = context->wasmVm()->getMemory(value_ptr, value_size);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(context->setSharedData(key.value(), value.value(), cas));
}

Word registerSharedQueueHandler(void* raw_context, Word queue_name_ptr, Word queue_name_size,
                                Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto queue_name = context->wasmVm()->getMemory(queue_name_ptr, queue_name_size);
  if (!queue_name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  uint32_t token = context->registerSharedQueue(queue_name.value());
  if (!context->wasm()->setDatatype(token_ptr, token)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word dequeueSharedQueueHandler(void* raw_context, Word token, Word data_ptr_ptr,
                               Word data_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  std::string data;
  WasmResult result = context->dequeueSharedQueue(token.u32(), &data);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->copyToPointerSize(data, data_ptr_ptr, data_size_ptr)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word resolveSharedQueueHandler(void* raw_context, Word vm_id_ptr, Word vm_id_size,
                               Word queue_name_ptr, Word queue_name_size, Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto vm_id = context->wasmVm()->getMemory(vm_id_ptr, vm_id_size);
  auto queue_name = context->wasmVm()->getMemory(queue_name_ptr, queue_name_size);
  if (!vm_id || !queue_name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  uint32_t token = 0;
  auto result = context->resolveSharedQueue(vm_id.value(), queue_name.value(), &token);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(token_ptr, token)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word enqueueSharedQueueHandler(void* raw_context, Word token, Word data_ptr, Word data_size) {
  auto context = WASM_CONTEXT(raw_context);
  auto data = context->wasmVm()->getMemory(data_ptr, data_size);
  if (!data) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(context->enqueueSharedQueue(token.u32(), data.value()));
}

// Header/Trailer/Metadata Maps
Word addHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr, Word value_size) {
  if (type > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  auto value = context->wasmVm()->getMemory(value_ptr, value_size);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->addHeaderMapValue(static_cast<HeaderMapType>(type.u64), key.value(), value.value());
  return wasmResultToWord(WasmResult::Ok);
}

Word getHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr_ptr, Word value_size_ptr) {
  if (type > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  if (!key) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto result = context->getHeaderMapValue(static_cast<HeaderMapType>(type.u64), key.value());
  context->wasm()->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
  return wasmResultToWord(WasmResult::Ok);
}

Word replaceHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                                  Word value_ptr, Word value_size) {
  if (type > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  auto value = context->wasmVm()->getMemory(value_ptr, value_size);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->replaceHeaderMapValue(static_cast<HeaderMapType>(type.u64), key.value(), value.value());
  return wasmResultToWord(WasmResult::Ok);
}

Word removeHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size) {
  if (type > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  if (!key) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->removeHeaderMapValue(static_cast<HeaderMapType>(type.u64), key.value());
  return wasmResultToWord(WasmResult::Ok);
}

Word getHeaderMapPairsHandler(void* raw_context, Word type, Word ptr_ptr, Word size_ptr) {
  if (type > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getHeaderMapPairs(static_cast<HeaderMapType>(type.u64));
  if (!getPairs(context, result, ptr_ptr, size_ptr)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word setHeaderMapPairsHandler(void* raw_context, Word type, Word ptr, Word size) {
  if (type > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto data = context->wasmVm()->getMemory(ptr, size);
  if (!data) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->setHeaderMapPairs(static_cast<HeaderMapType>(type.u64), toPairs(data.value()));
  return wasmResultToWord(WasmResult::Ok);
}

Word getHeaderMapSizeHandler(void* raw_context, Word type, Word result_ptr) {
  if (type > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  size_t result = context->getHeaderMapSize(static_cast<HeaderMapType>(type.u64));
  if (!context->wasmVm()->setWord(result_ptr, Word(result))) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

// Body Buffer
Word getRequestBodyBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                      Word size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getRequestBodyBufferBytes(start, length);
  context->wasm()->copyToPointerSize(result, ptr_ptr, size_ptr);
  return wasmResultToWord(WasmResult::Ok);
}

Word getResponseBodyBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                       Word size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getResponseBodyBufferBytes(start, length);
  context->wasm()->copyToPointerSize(result, ptr_ptr, size_ptr);
  return wasmResultToWord(WasmResult::Ok);
}

Word httpCallHandler(void* raw_context, Word uri_ptr, Word uri_size, Word header_pairs_ptr,
                     Word header_pairs_size, Word body_ptr, Word body_size, Word trailer_pairs_ptr,
                     Word trailer_pairs_size, Word timeout_milliseconds) {
  auto context = WASM_CONTEXT(raw_context)->root_context();
  auto uri = context->wasmVm()->getMemory(uri_ptr, uri_size);
  auto body = context->wasmVm()->getMemory(body_ptr, body_size);
  auto header_pairs = context->wasmVm()->getMemory(header_pairs_ptr, header_pairs_size);
  auto trailer_pairs = context->wasmVm()->getMemory(trailer_pairs_ptr, trailer_pairs_size);
  if (!uri || !body || !header_pairs || !trailer_pairs) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto headers = toPairs(header_pairs.value());
  auto trailers = toPairs(trailer_pairs.value());
  return context->httpCall(uri.value(), headers, body.value(), trailers, timeout_milliseconds);
}

Word defineMetricHandler(void* raw_context, Word metric_type, Word name_ptr, Word name_size,
                         Word metric_id_ptr) {
  if (metric_type > static_cast<uint64_t>(Context::MetricType::Max))
    return 0;
  auto context = WASM_CONTEXT(raw_context);
  auto name = context->wasmVm()->getMemory(name_ptr, name_size);
  if (!name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  uint32_t metric_id = 0;
  auto result = context->defineMetric(static_cast<Context::MetricType>(metric_type.u64),
                                      name.value(), &metric_id);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(metric_id_ptr, metric_id)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word incrementMetricHandler(void* raw_context, Word metric_id, int64_t offset) {
  auto context = WASM_CONTEXT(raw_context);
  return wasmResultToWord(context->incrementMetric(metric_id, offset));
}

Word recordMetricHandler(void* raw_context, Word metric_id, uint64_t value) {
  auto context = WASM_CONTEXT(raw_context);
  return wasmResultToWord(context->recordMetric(metric_id, value));
}

Word getMetricHandler(void* raw_context, Word metric_id, Word result_uint64_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  uint64_t value = 0;
  auto result = context->getMetric(metric_id, &value);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(result_uint64_ptr, value)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word grpcCallHandler(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
                     Word service_name_size, Word method_name_ptr, Word method_name_size,
                     Word request_ptr, Word request_size, Word timeout_milliseconds) {
  auto context = WASM_CONTEXT(raw_context)->root_context();
  auto service = context->wasmVm()->getMemory(service_ptr, service_size);
  auto service_name = context->wasmVm()->getMemory(service_name_ptr, service_name_size);
  auto method_name = context->wasmVm()->getMemory(method_name_ptr, method_name_size);
  auto request = context->wasmVm()->getMemory(request_ptr, request_size);
  if (!service || !service_name || !method_name || !request) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  envoy::api::v2::core::GrpcService service_proto;
  if (!service_proto.ParseFromArray(service.value().data(), service.value().size())) {
    return false;
  }
  return context->grpcCall(service_proto, service_name.value(), method_name.value(),
                           request.value(), std::chrono::milliseconds(timeout_milliseconds));
}

Word grpcStreamHandler(void* raw_context, Word service_ptr, Word service_size,
                       Word service_name_ptr, Word service_name_size, Word method_name_ptr,
                       Word method_name_size) {
  auto context = WASM_CONTEXT(raw_context)->root_context();
  auto service = context->wasmVm()->getMemory(service_ptr, service_size);
  auto service_name = context->wasmVm()->getMemory(service_name_ptr, service_name_size);
  auto method_name = context->wasmVm()->getMemory(method_name_ptr, method_name_size);
  if (!service || !service_name || !method_name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  envoy::api::v2::core::GrpcService service_proto;
  if (!service_proto.ParseFromArray(service.value().data(), service.value().size())) {
    return false;
  }
  return context->grpcStream(service_proto, service_name.value(), method_name.value());
}

Word grpcCancelHandler(void* raw_context, Word token) {
  auto context = WASM_CONTEXT(raw_context)->root_context();
  return wasmResultToWord(context->grpcCancel(token));
}

Word grpcCloseHandler(void* raw_context, Word token) {
  auto context = WASM_CONTEXT(raw_context)->root_context();
  return wasmResultToWord(context->grpcClose(token));
}

Word grpcSendHandler(void* raw_context, Word token, Word message_ptr, Word message_size,
                     Word end_stream) {
  auto context = WASM_CONTEXT(raw_context)->root_context();
  auto message = context->wasmVm()->getMemory(message_ptr, message_size);
  if (!message) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(context->grpcSend(token, message.value(), end_stream));
}

Word _emscripten_get_heap_sizeHandler(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  return context->wasmVm()->getMemorySize();
}

Word _emscripten_memcpy_bigHandler(void* raw_context, Word dst, Word src, Word size) {
  auto context = WASM_CONTEXT(raw_context);
  auto data = context->wasmVm()->getMemory(src, size);
  if (!data) {
    return 0;
  }
  context->wasmVm()->setMemory(dst, size, data.value().data());
  return dst;
}

Word _emscripten_resize_heapHandler(void*, Word) {
  throw WasmException("emscripten emscripten_resize_heap");
}

Word abortOnCannotGrowMemoryAbi00Handler(void*) {
  throw WasmException("emscripten abortOnCannotGrowMemory");
}

Word abortOnCannotGrowMemoryAbi02Handler(void*, Word) {
  throw WasmException("emscripten abortOnCannotGrowMemory");
}

void abortHandler(void*, Word) { throw WasmException("emscripten abort"); }

void _abortHandler(void*) { throw WasmException("emscripten abort"); }

void _llvm_trapHandler(void*) { throw WasmException("emscripten llvm_trap"); }

void ___assert_failHandler(void*, Word, Word, Word, Word) {
  throw WasmException("emscripten assert_fail");
}

void ___cxa_throwHandler(void*, Word, Word, Word) { throw WasmException("emscripten cxa_throw"); }

void ___cxa_pure_virtualHandler(void*) { throw WasmException("emscripten cxa_pure_virtual"); }

Word ___call_mainHandler(void*, Word, Word) { throw WasmException("emscripten call_main"); }

Word ___cxa_allocate_exceptionHandler(void*, Word) {
  throw WasmException("emscripten cxa_allocate_exception");
}

Word ___cxa_uncaught_exceptionHandler(void*) {
  throw WasmException("emscripten cxa_uncaught_exception");
}

Word ___cxa_uncaught_exceptionsHandler(void*) {
  throw WasmException("emscripten cxa_uncaught_exceptions");
}

Word ___clock_gettimeHandler(void*, Word, Word) { throw WasmException("emscripten clock_gettime"); }

void ___lockHandler(void*, Word) { throw WasmException("emscripten lock"); }

void ___unlockHandler(void*, Word) { throw WasmException("emscripten unlock"); }

Word ___syscall6Handler(void*, Word, Word) { throw WasmException("emscripten syscall6"); }

Word ___syscall54Handler(void*, Word, Word) { throw WasmException("emscripten syscall54"); }

Word ___syscall140Handler(void*, Word, Word) { throw WasmException("emscripten syscall140"); }

// Implementation of writev() syscall that redirects stdout/stderr to Envoy logs.
// ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
Word ___syscall146Handler(void* raw_context, Word, Word syscall_args_ptr) {
  auto context = WASM_CONTEXT(raw_context);

  // Read syscall args.
  auto memslice = context->wasmVm()->getMemory(syscall_args_ptr, 3 * sizeof(uint32_t));
  if (!memslice) {
    context->wasm()->setErrno(EINVAL);
    return -1;
  }
  const uint32_t* syscall_args = reinterpret_cast<const uint32_t*>(memslice.value().data());

  spdlog::level::level_enum log_level;
  switch (syscall_args[0] /* fd */) {
  case 1 /* stdout */:
    log_level = spdlog::level::info;
    break;
  case 2 /* stderr */:
    log_level = spdlog::level::err;
    break;
  default:
    throw WasmException("emscripten syscall146 (writev)");
  }

  std::string s;
  for (size_t i = 0; i < syscall_args[2] /* iovcnt */; i++) {
    memslice = context->wasmVm()->getMemory(syscall_args[1] /* iov */ + i * 2 * sizeof(uint32_t),
                                            2 * sizeof(uint32_t));
    if (!memslice) {
      context->wasm()->setErrno(EINVAL);
      return -1;
    }
    const uint32_t* iovec = reinterpret_cast<const uint32_t*>(memslice.value().data());
    if (iovec[1] /* size */) {
      memslice = context->wasmVm()->getMemory(iovec[0] /* data */, iovec[1] /* size */);
      if (!memslice) {
        context->wasm()->setErrno(EINVAL);
        return -1;
      }
      s.append(memslice.value().data(), memslice.value().size());
    }
  }

  size_t written = s.size();
  if (written) {
    // Remove trailing newline from the logs, if any.
    if (s[written - 1] == '\n') {
      s.erase(written - 1);
    }
    context->scriptLog(log_level, s);
  }
  return written;
}

void ___setErrNoHandler(void*, Word) { throw WasmException("emscripten setErrNo"); }

Word _pthread_equalHandler(void*, Word left, Word right) { return left == right; }
// NB: pthread_mutex_destroy is required to return 0 by the protobuf libarary.
Word _pthread_mutex_destroyHandler(void*, Word) { return 0; }
Word _pthread_cond_waitHandler(void*, Word, Word) {
  throw WasmException("emscripten pthread_cond_wait");
}
Word _pthread_getspecificHandler(void*, Word) {
  throw WasmException("emscripten pthread_getspecific");
}
Word _pthread_key_createHandler(void*, Word, Word) {
  throw WasmException("emscripten pthread_key_create");
}
Word _pthread_onceHandler(void*, Word, Word) { throw WasmException("emscripten pthread_once"); }
Word _pthread_setspecificHandler(void*, Word, Word) {
  throw WasmException("emscripten pthread_setspecific");
}
void setTempRet0Handler(void*, Word) { throw WasmException("emscripten setTempRet0"); }

Word setTickPeriodMillisecondsHandler(void* raw_context, Word tick_period_milliseconds) {
  return wasmResultToWord(WASM_CONTEXT(raw_context)
                              ->setTickPeriod(std::chrono::milliseconds(tick_period_milliseconds)));
}

Word getCurrentTimeNanosecondsHandler(void* raw_context, Word result_uint64_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  uint64_t result = context->getCurrentTimeNanoseconds();
  if (!context->wasm()->setDatatype(result_uint64_ptr.u64, result)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word logHandler(void* raw_context, Word level, Word address, Word size) {
  auto context = WASM_CONTEXT(raw_context);
  auto message = context->wasmVm()->getMemory(address, size);
  if (!message) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->scriptLog(static_cast<spdlog::level::level_enum>(level.u64), message.value());
  return wasmResultToWord(WasmResult::Ok);
}

WasmResult Context::setTickPeriod(std::chrono::milliseconds tick_period) {
  wasm_->setTickPeriod(root_context_id_ ? root_context_id_ : id_, tick_period);
  return WasmResult::Ok;
}

uint64_t Context::getCurrentTimeNanoseconds() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             wasm_->time_source_.systemTime().time_since_epoch())
      .count();
}

// Shared Data
WasmResult Context::getSharedData(absl::string_view key, std::pair<std::string, uint32_t>* data) {
  return global_shared_data.get(wasm_->id(), key, data);
}

WasmResult Context::setSharedData(absl::string_view key, absl::string_view value, uint32_t cas) {
  return global_shared_data.set(wasm_->id(), key, value, cas);
}

// Shared Queue

uint32_t Context::registerSharedQueue(absl::string_view queue_name) {
  // Get the id of the root context if this is a stream context because onQueueReady is on the root.
  return global_shared_data.registerQueue(
      wasm_->id(), queue_name, isRootContext() ? id_ : root_context_id_, wasm_->dispatcher_);
}

WasmResult Context::resolveSharedQueue(absl::string_view vm_id, absl::string_view queue_name,
                                       uint32_t* token_ptr) {
  uint32_t token = global_shared_data.resolveQueue(vm_id, queue_name);
  if (!token) {
    return WasmResult::NotFound;
  }
  *token_ptr = token;
  return WasmResult::Ok;
}

WasmResult Context::dequeueSharedQueue(uint32_t token, std::string* data) {
  return global_shared_data.dequeue(token, data);
}

WasmResult Context::enqueueSharedQueue(uint32_t token, absl::string_view value) {
  return global_shared_data.enqueue(token, value);
}

// Header/Trailer/Metadata Maps.
Http::HeaderMap* Context::getMap(HeaderMapType type) {
  switch (type) {
  case HeaderMapType::RequestHeaders:
    return request_headers_;
  case HeaderMapType::RequestTrailers:
    return request_trailers_;
  case HeaderMapType::ResponseHeaders:
    return response_headers_;
  case HeaderMapType::ResponseTrailers:
    return response_trailers_;
  case HeaderMapType::GrpcCreateInitialMetadata:
    return grpc_create_initial_metadata_;
  default:
    return nullptr;
  }
}

const Http::HeaderMap* Context::getConstMap(HeaderMapType type) {
  switch (type) {
  case HeaderMapType::RequestHeaders:
    if (access_log_request_headers_) {
      return access_log_request_headers_;
    }
    return request_headers_;
  case HeaderMapType::RequestTrailers:
    if (access_log_request_trailers_) {
      return access_log_request_trailers_;
    }
    return request_trailers_;
  case HeaderMapType::ResponseHeaders:
    if (access_log_response_headers_) {
      return access_log_response_headers_;
    }
    return response_headers_;
  case HeaderMapType::ResponseTrailers:
    if (access_log_response_trailers_) {
      return access_log_response_trailers_;
    }
    return response_trailers_;
  case HeaderMapType::GrpcCreateInitialMetadata:
    return grpc_create_initial_metadata_;
  case HeaderMapType::GrpcReceiveInitialMetadata:
    return grpc_receive_initial_metadata_.get();
  case HeaderMapType::GrpcReceiveTrailingMetadata:
    return grpc_receive_trailing_metadata_.get();
  }
  return nullptr;
}

void Context::addHeaderMapValue(HeaderMapType type, absl::string_view key,
                                absl::string_view value) {
  auto map = getMap(type);
  if (!map) {
    return;
  }
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  map->addCopy(lower_key, std::string(value));
}

absl::string_view Context::getHeaderMapValue(HeaderMapType type, absl::string_view key) {
  auto map = getConstMap(type);
  if (!map) {
    return "";
  }
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  auto entry = map->get(lower_key);
  if (!entry) {
    return "";
  }
  return entry->value().getStringView();
}

Pairs headerMapToPairs(const Http::HeaderMap* map) {
  if (!map) {
    return {};
  }
  Pairs pairs;
  pairs.reserve(map->size());
  map->iterate(
      [](const Http::HeaderEntry& header, void* pairs) -> Http::HeaderMap::Iterate {
        (static_cast<Pairs*>(pairs))
            ->push_back(
                std::make_pair(header.key().getStringView(), header.value().getStringView()));
        return Http::HeaderMap::Iterate::Continue;
      },
      &pairs);
  return pairs;
}

Pairs Context::getHeaderMapPairs(HeaderMapType type) { return headerMapToPairs(getConstMap(type)); }

void Context::setHeaderMapPairs(HeaderMapType type, const Pairs& pairs) {
  auto map = getMap(type);
  if (!map) {
    return;
  }
  std::vector<std::string> keys;
  map->iterate(
      [](const Http::HeaderEntry& header, void* keys) -> Http::HeaderMap::Iterate {
        (static_cast<std::vector<std::string>*>(keys))
            ->push_back(std::string(header.key().getStringView()));
        return Http::HeaderMap::Iterate::Continue;
      },
      &keys);
  for (auto& k : keys) {
    const Http::LowerCaseString lower_key(std::move(k));
    map->remove(lower_key);
  }
  for (auto& p : pairs) {
    const Http::LowerCaseString lower_key(std::move(std::string(p.first)));
    map->addCopy(lower_key, std::move(std::string(p.second)));
  }
}

void Context::removeHeaderMapValue(HeaderMapType type, absl::string_view key) {
  auto map = getMap(type);
  if (!map) {
    return;
  }
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  map->remove(lower_key);
}

void Context::replaceHeaderMapValue(HeaderMapType type, absl::string_view key,
                                    absl::string_view value) {
  auto map = getMap(type);
  if (!map)
    return;
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  auto entry = map->get(lower_key);
  if (entry != nullptr)
    entry->value(value.data(), value.size());
  else
    map->addCopy(lower_key, std::string(value));
}

uint32_t Context::getHeaderMapSize(HeaderMapType type) {
  auto map = getMap(type);
  if (!map) {
    return 0;
  }
  return map->byteSize();
}

// Body Buffer

absl::string_view Context::getRequestBodyBufferBytes(uint32_t start, uint32_t length) {
  if (!requestBodyBuffer_) {
    return "";
  }
  if (requestBodyBuffer_->length() < static_cast<uint64_t>((start + length))) {
    return "";
  }
  return absl::string_view(
      static_cast<char*>(requestBodyBuffer_->linearize(start + length)) + start, length);
}

absl::string_view Context::getResponseBodyBufferBytes(uint32_t start, uint32_t length) {
  if (!responseBodyBuffer_) {
    return "";
  }
  if (responseBodyBuffer_->length() < static_cast<uint64_t>((start + length))) {
    return "";
  }
  return absl::string_view(
      static_cast<char*>(responseBodyBuffer_->linearize(start + length)) + start, length);
}

// Async call via HTTP
uint32_t Context::httpCall(absl::string_view cluster, const Pairs& request_headers,
                           absl::string_view request_body, const Pairs& request_trailers,
                           int timeout_milliseconds) {
  if (timeout_milliseconds < 0) {
    return 0;
  }
  auto cluster_string = std::string(cluster);
  if (clusterManager().get(cluster_string) == nullptr) {
    return 0;
  }

  Http::MessagePtr message(new Http::RequestMessageImpl(buildHeaderMapFromPairs(request_headers)));

  // Check that we were provided certain headers.
  if (message->headers().Path() == nullptr || message->headers().Method() == nullptr ||
      message->headers().Host() == nullptr) {
    return 0;
  }

  if (!request_body.empty()) {
    message->body().reset(new Buffer::OwnedImpl(request_body.data(), request_body.size()));
    message->headers().insertContentLength().value(request_body.size());
  }

  if (request_trailers.size() > 0) {
    message->trailers(buildHeaderMapFromPairs(request_trailers));
  }

  absl::optional<std::chrono::milliseconds> timeout;
  if (timeout_milliseconds > 0) {
    timeout = std::chrono::milliseconds(timeout_milliseconds);
  }

  auto token = next_http_call_token_++;
  // Handle rollover.
  for (;;) {
    if (token == 0) {
      token = next_http_call_token_++;
    }
    if (!http_request_.count(token)) {
      break;
    }
    token = next_http_call_token_++;
  }
  auto& handler = http_request_[token];
  auto http_request = clusterManager()
                          .httpAsyncClientForCluster(cluster_string)
                          .send(std::move(message), handler,
                                Http::AsyncClient::RequestOptions().setTimeout(timeout));
  if (!http_request) {
    http_request_.erase(token);
    return 0;
  }
  handler.context = this;
  handler.token = token;
  handler.request = http_request;
  return token;
}

uint32_t Context::grpcCall(const envoy::api::v2::core::GrpcService& grpc_service,
                           absl::string_view service_name, absl::string_view method_name,
                           absl::string_view request,
                           const absl::optional<std::chrono::milliseconds>& timeout) {
  auto token = next_grpc_token_++;
  if (IsGrpcStreamToken(token)) {
    token = next_grpc_token_++;
  }
  // Handle rollover.
  for (;;) {
    if (token == 0) {
      token = next_grpc_token_ += 2;
    }
    if (!grpc_call_request_.count(token)) {
      break;
    }
    token = next_grpc_token_ += 2;
  }
  auto& handler = grpc_call_request_[token];
  handler.context = this;
  handler.token = token;
  auto grpc_client =
      clusterManager()
          .grpcAsyncClientManager()
          .factoryForGrpcService(grpc_service, wasm_->scope_, true /* skip_cluster_check */)
          ->create();
  // NB: this call causes the onCreateInitialMetadata callback to occur inline *before* this call
  // returns. Consequently the grpc_request is not available. Attempting to close or reset from that
  // callback will fail.
  auto grpc_request =
      grpc_client->sendRaw(service_name, method_name, std::make_unique<Buffer::OwnedImpl>(request),
                           handler, Tracing::NullSpan::instance(), timeout);
  if (!grpc_request) {
    grpc_call_request_.erase(token);
    return 0;
  }
  handler.client = std::move(grpc_client);
  handler.request = grpc_request;
  return token;
}

uint32_t Context::grpcStream(const envoy::api::v2::core::GrpcService& grpc_service,
                             absl::string_view service_name, absl::string_view method_name) {
  auto token = next_grpc_token_++;
  if (IsGrpcCallToken(token)) {
    token = next_grpc_token_++;
  }
  // Handle rollover.
  for (;;) {
    if (token == 0) {
      token = next_grpc_token_ += 2;
    }
    if (!grpc_stream_.count(token)) {
      break;
    }
    token = next_grpc_token_ += 2;
  }
  auto& handler = grpc_stream_[token];
  handler.context = this;
  handler.token = token;
  auto grpc_client =
      clusterManager()
          .grpcAsyncClientManager()
          .factoryForGrpcService(grpc_service, wasm_->scope_, true /* skip_cluster_check */)
          ->create();
  // NB: this call causes the onCreateInitialMetadata callback to occur inline *before* this call
  // returns. Consequently the grpc_stream is not available. Attempting to close or reset from that
  // callback will fail.
  auto grpc_stream = grpc_client->startRaw(service_name, method_name, handler);
  if (!grpc_stream) {
    grpc_stream_.erase(token);
    return 0;
  }
  handler.client = std::move(grpc_client);
  handler.stream = grpc_stream;
  return token;
}

void Context::httpRespond(const Pairs& response_headers, absl::string_view body,
                          const Pairs& response_trailers) {
  (void)response_headers;
  (void)body;
  (void)response_trailers;
}

// StreamInfo
StreamInfo::StreamInfo* Context::getStreamInfo(MetadataType type) const {
  switch (type) {
  case MetadataType::Request:
    if (decoder_callbacks_) {
      return &decoder_callbacks_->streamInfo();
    }
    break;
  case MetadataType::Response:
    if (encoder_callbacks_) {
      return &encoder_callbacks_->streamInfo();
    }
    break;
    // Note: Log is always const.
  default:
    break;
  }
  return nullptr;
}

const StreamInfo::StreamInfo* Context::getConstStreamInfo(MetadataType type) const {
  switch (type) {
  case MetadataType::Request:
    if (decoder_callbacks_) {
      return &decoder_callbacks_->streamInfo();
    }
    break;
  case MetadataType::Response:
    if (encoder_callbacks_) {
      return &encoder_callbacks_->streamInfo();
    }
    break;
  case MetadataType::Log:
    if (access_log_stream_info_) {
      return access_log_stream_info_;
    }
    break;
  case MetadataType::Cluster:
    if (decoder_callbacks_) {
      return &decoder_callbacks_->streamInfo();
    }
    if (encoder_callbacks_) {
      return &encoder_callbacks_->streamInfo();
    }
    if (access_log_stream_info_) {
      return access_log_stream_info_;
    }
    break;
  default:
    break;
  }
  return nullptr;
}

std::string Context::getProtocol(StreamType type) {
  auto streamInfo = getConstStreamInfo(StreamType2MetadataType(type));
  if (!streamInfo || !streamInfo->protocol().has_value()) {
    return "";
  }
  return Http::Utility::getProtocolString(streamInfo->protocol().value());
}

uint32_t Context::getDestinationPort(StreamType type) {
  auto streamInfo = getConstStreamInfo(StreamType2MetadataType(type));
  if (!streamInfo)
    return 0;
  auto address = streamInfo->upstreamHost()->address();
  if (!address) {
    return 0;
  }
  auto ip = address->ip();
  if (!ip) {
    return 0;
  }
  return ip->port();
}

uint32_t Context::getResponseCode(StreamType type) {
  auto streamInfo = getConstStreamInfo(StreamType2MetadataType(type));
  if (!streamInfo)
    return 0;
  return streamInfo->responseCode().value_or(0);
}

std::string Context::getTlsVersion(StreamType type) {
  auto streamInfo = getConstStreamInfo(StreamType2MetadataType(type));
  if (!streamInfo || !streamInfo->downstreamSslConnection()) {
    return "";
  }
  return streamInfo->downstreamSslConnection()->tlsVersion();
}

absl::optional<bool> Context::peerCertificatePresented(StreamType type) {
  auto streamInfo = getConstStreamInfo(StreamType2MetadataType(type));
  if (!streamInfo || !streamInfo->downstreamSslConnection()) {
    return absl::nullopt;
  }
  return streamInfo->downstreamSslConnection()->peerCertificatePresented();
}

const ProtobufWkt::Struct* Context::getMetadataStructProto(MetadataType type,
                                                           absl::string_view name) {
  switch (type) {
  case MetadataType::Expression:
    return nullptr;
  case MetadataType::RequestRoute:
    return getRouteMetadataStructProto(decoder_callbacks_);
  case MetadataType::ResponseRoute:
    return getRouteMetadataStructProto(encoder_callbacks_);
  case MetadataType::Node:
    if (name == ".") {
      temporary_metadata_.Clear();
      (*temporary_metadata_.mutable_fields())["id"].set_string_value(
          wasm_->local_info_.node().id());
      (*temporary_metadata_.mutable_fields())["cluster"].set_string_value(
          wasm_->local_info_.node().cluster());
      (*temporary_metadata_.mutable_fields())["locality.region"].set_string_value(
          wasm_->local_info_.node().locality().region());
      (*temporary_metadata_.mutable_fields())["locality.zone"].set_string_value(
          wasm_->local_info_.node().locality().zone());
      (*temporary_metadata_.mutable_fields())["locality.sub_zone"].set_string_value(
          wasm_->local_info_.node().locality().sub_zone());
      (*temporary_metadata_.mutable_fields())["build_version"].set_string_value(
          wasm_->local_info_.node().build_version());
      return &temporary_metadata_;
    }
    return &wasm_->local_info_.node().metadata();
  case MetadataType::Listener:
    if (wasm_->listener_metadata_) {
      return getStructProtoFromMetadata(*wasm_->listener_metadata_, name);
    }
    return nullptr;
  case MetadataType::Cluster: {
    std::string cluster_name;
    auto stream_info = getConstStreamInfo(type);
    if (!stream_info) {
      return nullptr;
    }
    auto host = stream_info->upstreamHost();
    if (!host) {
      return nullptr;
    }
    cluster_name = host->cluster().name();
    if (name == ".") {
      temporary_metadata_.Clear();
      (*temporary_metadata_.mutable_fields())["cluster_name"].set_string_value(cluster_name);
      return &temporary_metadata_;
    }
    return getStructProtoFromMetadata(host->cluster().metadata(), name);
  }
  default: {
    auto stream_info = getConstStreamInfo(type);
    if (!stream_info) {
      return nullptr;
    }
    return getStructProtoFromMetadata(stream_info->dynamicMetadata(), name);
  }
  }
}

WasmResult Context::getMetadata(MetadataType type, absl::string_view key, std::string* result_ptr) {
  if (type == MetadataType::Expression) {
    static absl::flat_hash_map<std::string, std::function<WasmResult(Context*, std::string*)>>
        handlers = {
            {"request.protocol",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto protocol = context->getProtocol(StreamType::Request);
               *result_ptr = protocol;
               if (protocol.empty()) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"response.protocol",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto protocol = context->getProtocol(StreamType::Response);
               *result_ptr = protocol;
               if (protocol.empty()) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"request.destination_port",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto port = context->getDestinationPort(StreamType::Request);
               result_ptr->assign(reinterpret_cast<const char*>(&port), sizeof(port));
               if (!port) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"response.destination_port",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto port = context->getDestinationPort(StreamType::Response);
               result_ptr->assign(reinterpret_cast<const char*>(&port), sizeof(port));
               if (!port) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"request.response_code",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto response_code = context->getResponseCode(StreamType::Request);
               result_ptr->assign(reinterpret_cast<const char*>(&response_code),
                                  sizeof(response_code));
               if (!response_code) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"response.response_code",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto response_code = context->getResponseCode(StreamType::Response);
               result_ptr->assign(reinterpret_cast<const char*>(&response_code),
                                  sizeof(response_code));
               if (!response_code) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"request.tls_version",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto tls_version = context->getTlsVersion(StreamType::Request);
               *result_ptr = tls_version;
               if (tls_version.empty()) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"response.tls_version",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto tls_version = context->getTlsVersion(StreamType::Response);
               *result_ptr = tls_version;
               if (tls_version.empty()) {
                 return WasmResult::NotFound;
               }
               return WasmResult::Ok;
             }},
            {"request.peer_certificate_presented",
             [](Context* context, bool* result_ptr) -> WasmResult {
               auto peer_certificate_presented =
                   context->peerCertificatePresented(StreamType::Request);
               if (!peer_certificate_presented) {
                 return WasmResult::NotFound;
               }
               *result_ptr = peer_certificate_presented.value();
               return WasmResult::Ok;
             }},
            {"response.peer_certificate_presented",
             [](Context* context, bool* result_ptr) -> WasmResult {
               auto peer_certificate_presented =
                   context->peerCertificatePresented(StreamType::Response);
               if (!peer_certificate_presented) {
                 return WasmResult::NotFound;
               }
               *result_ptr = peer_certificate_presented.value();
               return WasmResult::Ok;
             }},
            {"plugin.direction",
             [](Context* context, std::string* result_ptr) -> WasmResult {
               auto direction = context->wasm_->direction_;
               result_ptr->assign(reinterpret_cast<const char*>(&direction), sizeof(direction));
               return WasmResult::Ok;
             }},
        };
    auto it = handlers.find(key);
    if (it == handlers.end()) {
      return WasmResult::BadExpression;
    }
    return it->second(this, result_ptr);
  }
  auto proto_struct = getMetadataStructProto(type);
  if (!proto_struct) {
    return WasmResult::NotFound;
  }
  auto it = proto_struct->fields().find(std::string(key));
  if (it == proto_struct->fields().end()) {
    return WasmResult::NotFound;
  }
  std::string result;
  if (!it->second.SerializeToString(&result)) {
    return WasmResult::SerializationFailure;
  }
  *result_ptr = std::move(result);
  return WasmResult::Ok;
}

WasmResult Context::setMetadata(MetadataType type, absl::string_view key,
                                absl::string_view serialized_proto_struct) {
  auto streamInfo = getStreamInfo(type);
  if (!streamInfo) {
    return WasmResult::NotFound;
  }
  streamInfo->setDynamicMetadata(
      HttpFilters::HttpFilterNames::get().Wasm,
      MessageUtil::keyValueStruct(std::string(key), std::string(serialized_proto_struct)));
  return WasmResult::Ok;
}

WasmResult Context::getMetadataPairs(MetadataType type, PairsWithStringValues* result_ptr) {
  auto proto_struct = getMetadataStructProto(type);
  if (!proto_struct) {
    return WasmResult::NotFound;
  }
  PairsWithStringValues result;
  for (auto& p : proto_struct->fields()) {
    std::string value;
    if (!p.second.SerializeToString(&value)) {
      return WasmResult::SerializationFailure;
    }
    result.emplace_back(p.first, std::move(value));
  }
  *result_ptr = std::move(result);
  return WasmResult::Ok;
}

WasmResult Context::getMetadataStruct(MetadataType type, absl::string_view name,
                                      std::string* result_ptr) {
  auto proto_struct = getMetadataStructProto(type, name);
  if (!proto_struct) {
    return WasmResult::NotFound;
  }
  std::string result;
  if (!proto_struct->SerializeToString(&result)) {
    return WasmResult::SerializationFailure;
  }
  *result_ptr = std::move(result);
  return WasmResult::Ok;
}

WasmResult Context::setMetadataStruct(MetadataType type, absl::string_view name,
                                      absl::string_view serialized_proto_struct) {
  auto streamInfo = getStreamInfo(type);
  if (!streamInfo) {
    return WasmResult::NotFound;
  }
  ProtobufWkt::Struct proto_struct;
  if (!proto_struct.ParseFromArray(serialized_proto_struct.data(),
                                   serialized_proto_struct.size())) {
    return WasmResult::ParseFailure;
  }
  streamInfo->setDynamicMetadata(std::string(name), proto_struct);
  return WasmResult::Ok;
}

void Context::scriptLog(spdlog::level::level_enum level, absl::string_view message) {
  switch (level) {
  case spdlog::level::trace:
    ENVOY_LOG(trace, "wasm log{}: {}", log_prefix_, message);
    return;
  case spdlog::level::debug:
    ENVOY_LOG(debug, "wasm log{}: {}", log_prefix_, message);
    return;
  case spdlog::level::info:
    ENVOY_LOG(info, "wasm log{}: {}", log_prefix_, message);
    return;
  case spdlog::level::warn:
    ENVOY_LOG(warn, "wasm log{}: {}", log_prefix_, message);
    return;
  case spdlog::level::err:
    ENVOY_LOG(error, "wasm log{}: {}", log_prefix_, message);
    return;
  case spdlog::level::critical:
    ENVOY_LOG(critical, "wasm log{}: {}", log_prefix_, message);
    return;
  case spdlog::level::off:
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
}

// Connection
bool Context::isSsl() { return decoder_callbacks_->connection()->ssl() != nullptr; }

//
// Calls into the WASM code.
//
void Context::onStart(absl::string_view root_id, absl::string_view vm_configuration) {
  if (wasm_->onStart_) {
    auto root_id_addr = wasm_->copyString(root_id);
    auto config_addr = wasm_->copyString(vm_configuration);
    wasm_->onStart_(this, id_, root_id_addr, root_id.size(), config_addr, vm_configuration.size());
  }
}

void Context::onConfigure(absl::string_view configuration) {
  if (!wasm_->onConfigure_)
    return;
  if (configuration.empty())
    return;
  auto address = wasm_->copyString(configuration);
  wasm_->onConfigure_(this, id_, address, configuration.size());
}

void Context::onCreate(uint32_t root_context_id) {
  if (wasm_->onCreate_) {
    wasm_->onCreate_(this, id_, root_context_id);
  }
}

Http::FilterHeadersStatus Context::onRequestHeaders() {
  onCreate(root_context_id_);
  // Store the stream id so that we can use it in log().
  auto& stream_info = decoder_callbacks_->streamInfo();
  auto& metadata = (*stream_info.dynamicMetadata()
                         .mutable_filter_metadata())[HttpFilters::HttpFilterNames::get().Wasm];
  (*metadata.mutable_fields())[std::string("_stream_id_" + std::string(root_id()))]
      .set_number_value(id_);
  if (!wasm_->onRequestHeaders_)
    return Http::FilterHeadersStatus::Continue;
  if (wasm_->onRequestHeaders_(this, id_) == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Context::onRequestBody(int body_buffer_length, bool end_of_stream) {
  if (!wasm_->onRequestBody_)
    return Http::FilterDataStatus::Continue;
  switch (wasm_->onRequestBody_(this, id_, static_cast<uint32_t>(body_buffer_length),
                                static_cast<uint32_t>(end_of_stream))) {
  case 0:
    return Http::FilterDataStatus::Continue;
  case 1:
    return Http::FilterDataStatus::StopIterationAndBuffer;
  case 2:
    return Http::FilterDataStatus::StopIterationAndWatermark;
  default:
    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
}

Http::FilterTrailersStatus Context::onRequestTrailers() {
  if (!wasm_->onRequestTrailers_)
    return Http::FilterTrailersStatus::Continue;
  if (wasm_->onRequestTrailers_(this, id_) == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return Http::FilterTrailersStatus::StopIteration;
}

Http::FilterMetadataStatus Context::onRequestMetadata() {
  if (!wasm_->onRequestMetadata_)
    return Http::FilterMetadataStatus::Continue;
  if (wasm_->onRequestMetadata_(this, id_) == 0) {
    return Http::FilterMetadataStatus::Continue;
  }
  return Http::FilterMetadataStatus::Continue; // This is currently the only return code.
}

Http::FilterHeadersStatus Context::onResponseHeaders() {
  if (!wasm_->onResponseHeaders_)
    return Http::FilterHeadersStatus::Continue;
  if (wasm_->onResponseHeaders_(this, id_) == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Context::onResponseBody(int body_buffer_length, bool end_of_stream) {
  if (!wasm_->onResponseBody_)
    return Http::FilterDataStatus::Continue;
  switch (wasm_->onResponseBody_(this, id_, static_cast<uint32_t>(body_buffer_length),
                                 static_cast<uint32_t>(end_of_stream))) {
  case 0:
    return Http::FilterDataStatus::Continue;
  case 1:
    return Http::FilterDataStatus::StopIterationAndBuffer;
  case 2:
    return Http::FilterDataStatus::StopIterationAndWatermark;
  default:
    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
}

Http::FilterTrailersStatus Context::onResponseTrailers() {
  if (!wasm_->onResponseTrailers_) {
    return Http::FilterTrailersStatus::Continue;
  }
  if (wasm_->onResponseTrailers_(this, id_) == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return Http::FilterTrailersStatus::StopIteration;
}

Http::FilterMetadataStatus Context::onResponseMetadata() {
  if (!wasm_->onResponseMetadata_) {
    return Http::FilterMetadataStatus::Continue;
  }
  if (wasm_->onResponseMetadata_(this, id_) == 0) {
    return Http::FilterMetadataStatus::Continue;
  }
  return Http::FilterMetadataStatus::Continue; // This is currently the only return code.
}

void Context::onHttpCallResponse(uint32_t token, const Pairs& response_headers,
                                 absl::string_view response_body, const Pairs& response_trailers) {
  if (!wasm_->onHttpCallResponse_) {
    return;
  }
  uint64_t headers_ptr, headers_size, trailers_ptr, trailers_size;
  exportPairs(this, response_headers, &headers_ptr, &headers_size);
  exportPairs(this, response_trailers, &trailers_ptr, &trailers_size);
  auto body_ptr = wasm_->copyString(response_body);
  auto body_size = response_body.size();
  wasm_->onHttpCallResponse_(this, id_, token, headers_ptr, headers_size, body_ptr, body_size,
                             trailers_ptr, trailers_size);
}

void Context::onQueueReady(uint32_t token) {
  if (wasm_->onQueueReady_) {
    wasm_->onQueueReady_(this, id_, token);
  }
}

void Context::onGrpcCreateInitialMetadata(uint32_t token, Http::HeaderMap& metadata) {
  if (!wasm_->onGrpcCreateInitialMetadata_) {
    return;
  }
  grpc_create_initial_metadata_ = &metadata;
  wasm_->onGrpcCreateInitialMetadata_(this, id_, token);
  grpc_create_initial_metadata_ = nullptr;
}

void Context::onGrpcReceiveInitialMetadata(uint32_t token, Http::HeaderMapPtr&& metadata) {
  if (!wasm_->onGrpcReceiveInitialMetadata_) {
    return;
  }
  grpc_receive_initial_metadata_ = std::move(metadata);
  wasm_->onGrpcReceiveInitialMetadata_(this, id_, token);
  grpc_receive_initial_metadata_ = nullptr;
}

void Context::onGrpcReceiveTrailingMetadata(uint32_t token, Http::HeaderMapPtr&& metadata) {
  if (!wasm_->onGrpcReceiveTrailingMetadata_) {
    return;
  }
  grpc_receive_trailing_metadata_ = std::move(metadata);
  wasm_->onGrpcReceiveTrailingMetadata_(this, id_, token);
  grpc_receive_trailing_metadata_ = nullptr;
}

WasmResult Context::defineMetric(MetricType type, absl::string_view name, uint32_t* metric_id_ptr) {
  auto stat_name = wasm_->stat_name_set_.getStatName(name);
  if (type == MetricType::Counter) {
    auto id = wasm_->nextCounterMetricId();
    auto c = &wasm_->scope_.counterFromStatName(stat_name);
    wasm_->counters_.emplace(id, c);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  } else if (type == MetricType::Gauge) {
    auto id = wasm_->nextGaugeMetricId();
    auto g = &wasm_->scope_.gaugeFromStatName(stat_name, Stats::Gauge::ImportMode::Accumulate);
    wasm_->gauges_.emplace(id, g);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  } else if (type == MetricType::Histogram) {
    auto id = wasm_->nextHistogramMetricId();
    auto h = &wasm_->scope_.histogramFromStatName(stat_name);
    wasm_->histograms_.emplace(id, h);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  }
  return WasmResult::BadArgument;
}

WasmResult Context::incrementMetric(uint32_t metric_id, int64_t offset) {
  auto type = static_cast<MetricType>(metric_id & Wasm::kMetricTypeMask);
  if (type == MetricType::Counter) {
    auto it = wasm_->counters_.find(metric_id);
    if (it != wasm_->counters_.end()) {
      if (offset > 0) {
        it->second->add(offset);
        return WasmResult::Ok;
      } else {
        return WasmResult::BadArgument;
      }
      return WasmResult::NotFound;
    }
  } else if (type == MetricType::Gauge) {
    auto it = wasm_->gauges_.find(metric_id);
    if (it != wasm_->gauges_.end()) {
      if (offset > 0) {
        it->second->add(offset);
        return WasmResult::Ok;
      } else {
        it->second->sub(-offset);
        return WasmResult::Ok;
      }
    }
    return WasmResult::NotFound;
  }
  return WasmResult::BadArgument;
}

WasmResult Context::recordMetric(uint32_t metric_id, uint64_t value) {
  auto type = static_cast<MetricType>(metric_id & Wasm::kMetricTypeMask);
  if (type == MetricType::Counter) {
    auto it = wasm_->counters_.find(metric_id);
    if (it != wasm_->counters_.end()) {
      it->second->add(value);
      return WasmResult::Ok;
    }
  } else if (type == MetricType::Gauge) {
    auto it = wasm_->gauges_.find(metric_id);
    if (it != wasm_->gauges_.end()) {
      it->second->set(value);
      return WasmResult::Ok;
    }
  } else if (type == MetricType::Histogram) {
    auto it = wasm_->histograms_.find(metric_id);
    if (it != wasm_->histograms_.end()) {
      it->second->recordValue(value);
      return WasmResult::Ok;
    }
  }
  return WasmResult::NotFound;
}

WasmResult Context::getMetric(uint32_t metric_id, uint64_t* result_uint64_ptr) {
  auto type = static_cast<MetricType>(metric_id & Wasm::kMetricTypeMask);
  if (type == MetricType::Counter) {
    auto it = wasm_->counters_.find(metric_id);
    if (it != wasm_->counters_.end()) {
      *result_uint64_ptr = it->second->value();
      return WasmResult::Ok;
    }
    return WasmResult::NotFound;
  } else if (type == MetricType::Gauge) {
    auto it = wasm_->gauges_.find(metric_id);
    if (it != wasm_->gauges_.end()) {
      *result_uint64_ptr = it->second->value();
      return WasmResult::Ok;
    }
    return WasmResult::NotFound;
  }
  return WasmResult::BadArgument;
}

Wasm::Wasm(absl::string_view vm, absl::string_view id, absl::string_view vm_configuration,
           Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher,
           Stats::Scope& scope, PluginDirection direction, const LocalInfo::LocalInfo& local_info,
           const envoy::api::v2::core::Metadata* listener_metadata,
           Stats::ScopeSharedPtr owned_scope)
    : cluster_manager_(cluster_manager), dispatcher_(dispatcher), scope_(scope),
      direction_(direction), local_info_(local_info), listener_metadata_(listener_metadata),
      owned_scope_(owned_scope), time_source_(dispatcher.timeSource()),
      vm_configuration_(vm_configuration), stat_name_set_(scope_.symbolTable()) {
  wasm_vm_ = Common::Wasm::createWasmVm(vm);
  id_ = std::string(id);
}

std::string Context::makeLogPrefix() const {
  std::string id;
  if (!wasm()->id().empty()) {
    id = id + " " + std::string(wasm()->id());
  }
  if (!root_id().empty()) {
    id = id + " " + std::string(root_id());
  }
  return id;
}

void Wasm::registerCallbacks() {
#define _REGISTER_ABI(_fn, _abi)                                                                   \
  wasm_vm_->registerCallback(                                                                      \
      "envoy", #_fn, &_fn##_abi##Handler,                                                          \
      &ConvertFunctionWordToUint32<decltype(_fn##_abi##Handler),                                   \
                                   _fn##_abi##Handler>::convertFunctionWordToUint32)
#define _REGISTER(_fn) _REGISTER_ABI(_fn, )

  if (is_emscripten_) {
    if (emscripten_abi_major_version_ > 0 || emscripten_abi_minor_version_ > 1) {
      // abi 0.2 - abortOnCannotGrowMemory() changed signature to (param i32) (result i32).
      _REGISTER_ABI(abortOnCannotGrowMemory, Abi02);
    } else {
      _REGISTER_ABI(abortOnCannotGrowMemory, Abi00);
    }

    _REGISTER(_emscripten_memcpy_big);
    _REGISTER(_emscripten_get_heap_size);
    _REGISTER(_emscripten_resize_heap);
    _REGISTER(abort);
    _REGISTER(_abort);
    _REGISTER(_llvm_trap);
    _REGISTER(___assert_fail);
    _REGISTER(___cxa_throw);
    _REGISTER(___cxa_pure_virtual);
    _REGISTER(___cxa_allocate_exception);
    _REGISTER(___cxa_uncaught_exception);
    _REGISTER(___cxa_uncaught_exceptions);
    _REGISTER(___call_main);
    _REGISTER(___clock_gettime);
    _REGISTER(___lock);
    _REGISTER(___unlock);
    _REGISTER(___syscall6);
    _REGISTER(___syscall54);
    _REGISTER(___syscall140);
    _REGISTER(___syscall146);
    _REGISTER(___setErrNo);
    _REGISTER(_pthread_equal);
    _REGISTER(_pthread_mutex_destroy);
    _REGISTER(_pthread_cond_wait);
    _REGISTER(_pthread_getspecific);
    _REGISTER(_pthread_key_create);
    _REGISTER(_pthread_once);
    _REGISTER(_pthread_setspecific);
    _REGISTER(setTempRet0);
  }
#undef _REGISTER
#undef _REGISTER_ABI

  // Calls with the "_proxy_" prefix.
#define _REGISTER_PROXY(_fn)                                                                       \
  wasm_vm_->registerCallback(                                                                      \
      "envoy", "_proxy_" #_fn, &_fn##Handler,                                                      \
      &ConvertFunctionWordToUint32<decltype(_fn##Handler),                                         \
                                   _fn##Handler>::convertFunctionWordToUint32);
  _REGISTER_PROXY(log);

  _REGISTER_PROXY(getMetadata);
  _REGISTER_PROXY(setMetadata);
  _REGISTER_PROXY(getMetadataPairs);

  _REGISTER_PROXY(getMetadataStruct);
  _REGISTER_PROXY(setMetadataStruct);

  _REGISTER_PROXY(continueRequest);
  _REGISTER_PROXY(continueResponse);
  _REGISTER_PROXY(sendLocalResponse);
  _REGISTER_PROXY(clearRouteCache);

  _REGISTER_PROXY(getSharedData);
  _REGISTER_PROXY(setSharedData);

  _REGISTER_PROXY(registerSharedQueue);
  _REGISTER_PROXY(resolveSharedQueue);
  _REGISTER_PROXY(dequeueSharedQueue);
  _REGISTER_PROXY(enqueueSharedQueue);

  _REGISTER_PROXY(getHeaderMapValue);
  _REGISTER_PROXY(addHeaderMapValue);
  _REGISTER_PROXY(replaceHeaderMapValue);
  _REGISTER_PROXY(removeHeaderMapValue);
  _REGISTER_PROXY(getHeaderMapPairs);
  _REGISTER_PROXY(setHeaderMapPairs);
  _REGISTER_PROXY(getHeaderMapSize);

  _REGISTER_PROXY(getRequestBodyBufferBytes);
  _REGISTER_PROXY(getResponseBodyBufferBytes);

  _REGISTER_PROXY(httpCall);

  _REGISTER_PROXY(grpcCall);
  _REGISTER_PROXY(grpcStream);
  _REGISTER_PROXY(grpcClose);
  _REGISTER_PROXY(grpcCancel);
  _REGISTER_PROXY(grpcSend);

  _REGISTER_PROXY(setTickPeriodMilliseconds);
  _REGISTER_PROXY(getCurrentTimeNanoseconds);

  _REGISTER_PROXY(defineMetric);
  _REGISTER_PROXY(incrementMetric);
  _REGISTER_PROXY(recordMetric);
  _REGISTER_PROXY(getMetric);

  _REGISTER_PROXY(setEffectiveContext);
#undef _REGISTER_PROXY
}

void Wasm::establishEnvironment() {
  if (is_emscripten_) {
    wasm_vm_->setMemoryLayout(emscripten_stack_base_, emscripten_dynamic_base_,
                              emscripten_dynamictop_ptr_);

    global_table_base_ = wasm_vm_->makeGlobal("env", "__table_base", Word(0));
    global_dynamictop_ =
        wasm_vm_->makeGlobal("env", "DYNAMICTOP_PTR", Word(emscripten_dynamictop_ptr_));

    wasm_vm_->makeModule("global");
    global_NaN_ = wasm_vm_->makeGlobal("global", "NaN", std::nan("0"));
    global_Infinity_ =
        wasm_vm_->makeGlobal("global", "Infinity", std::numeric_limits<double>::infinity());
  }
}

void Wasm::getFunctions() {
#define _GET(_fn) wasm_vm_->getFunction("_" #_fn, &_fn##_);
  _GET(malloc);
  _GET(free);
  _GET(__errno_location);
#undef _GET

#define _GET_PROXY(_fn) wasm_vm_->getFunction("_proxy_" #_fn, &_fn##_);
  _GET_PROXY(onStart);
  _GET_PROXY(onConfigure);
  _GET_PROXY(onTick);

  _GET_PROXY(onCreate);
  _GET_PROXY(onRequestHeaders);
  _GET_PROXY(onRequestBody);
  _GET_PROXY(onRequestTrailers);
  _GET_PROXY(onRequestMetadata);
  _GET_PROXY(onResponseHeaders);
  _GET_PROXY(onResponseBody);
  _GET_PROXY(onResponseTrailers);
  _GET_PROXY(onResponseMetadata);
  _GET_PROXY(onHttpCallResponse);
  _GET_PROXY(onGrpcReceive);
  _GET_PROXY(onGrpcClose);
  _GET_PROXY(onGrpcCreateInitialMetadata);
  _GET_PROXY(onGrpcReceiveInitialMetadata);
  _GET_PROXY(onGrpcReceiveTrailingMetadata);
  _GET_PROXY(onQueueReady);
  _GET_PROXY(onDone);
  _GET_PROXY(onLog);
  _GET_PROXY(onDelete);
#undef _GET_PROXY

  if (!malloc_ || !free_) {
    throw WasmException("WASM missing malloc/free");
  }
}

Wasm::Wasm(const Wasm& wasm, Event::Dispatcher& dispatcher)
    : std::enable_shared_from_this<Wasm>(wasm), cluster_manager_(wasm.cluster_manager_),
      dispatcher_(dispatcher), scope_(wasm.scope_), direction_(wasm.direction_),
      local_info_(wasm.local_info_), listener_metadata_(wasm.listener_metadata_), id_(wasm.id_),
      owned_scope_(wasm.owned_scope_), time_source_(dispatcher.timeSource()),
      stat_name_set_(scope_.symbolTable()) {
  wasm_vm_ = wasm.wasmVm()->clone();
  vm_context_ = std::make_shared<Context>(this);
  getFunctions();
}

bool Wasm::initialize(const std::string& code, absl::string_view name, bool allow_precompiled) {
  if (!wasm_vm_) {
    return false;
  }
  auto ok = wasm_vm_->load(code, allow_precompiled);
  if (!ok) {
    return false;
  }
  auto metadata = wasm_vm_->getUserSection("emscripten_metadata");
  if (!metadata.empty()) {
    is_emscripten_ = true;
    auto start = reinterpret_cast<const uint8_t*>(metadata.data());
    auto end = reinterpret_cast<const uint8_t*>(metadata.data() + metadata.size());
    start = decodeVarint(start, end, &emscripten_metadata_major_version_);
    start = decodeVarint(start, end, &emscripten_metadata_minor_version_);
    start = decodeVarint(start, end, &emscripten_abi_major_version_);
    start = decodeVarint(start, end, &emscripten_abi_minor_version_);
    start = decodeVarint(start, end, &emscripten_memory_size_);
    start = decodeVarint(start, end, &emscripten_table_size_);
    if (emscripten_metadata_major_version_ > 0 || emscripten_metadata_minor_version_ > 0) {
      // metadata 0.1 - added: global_base, dynamic_base, dynamictop_ptr and tempdouble_ptr.
      start = decodeVarint(start, end, &emscripten_global_base_);
      start = decodeVarint(start, end, &emscripten_dynamic_base_);
      start = decodeVarint(start, end, &emscripten_dynamictop_ptr_);
      decodeVarint(start, end, &emscripten_tempdouble_ptr_);
    } else {
      // Workaround for Emscripten versions without heap (dynamic) base in metadata.
      emscripten_stack_base_ = 64 * 64 * 1024;      // 4MB
      emscripten_dynamic_base_ = 128 * 64 * 1024;   // 8MB
      emscripten_dynamictop_ptr_ = 128 * 64 * 1024; // 8MB
    }
  }
  registerCallbacks();
  establishEnvironment();
  wasm_vm_->link(name, is_emscripten_);
  vm_context_ = std::make_shared<Context>(this);
  getFunctions();
  wasm_vm_->start(vm_context_.get());
  if (is_emscripten_) {
    ASSERT(std::isnan(global_NaN_->get()));
    ASSERT(std::isinf(global_Infinity_->get()));
  }
  code_ = code;
  allow_precompiled_ = allow_precompiled;
  return true;
}

void Wasm::configure(Context* root_context, absl::string_view configuration) {
  if (onConfigure_ && !configuration.empty()) {
    auto address = copyString(configuration);
    onConfigure_(root_context, root_context->id(), address, configuration.size());
  }
}

Context* Wasm::start(absl::string_view root_id, absl::string_view vm_configuration) {
  auto it = root_contexts_.find(root_id);
  if (it != root_contexts_.end()) {
    it->second->onStart(root_id, vm_configuration);
    return it->second.get();
  }
  auto context = std::make_unique<Context>(this, root_id);
  auto context_ptr = context.get();
  root_contexts_[root_id] = std::move(context);
  context_ptr->onStart(root_id, vm_configuration);
  return context_ptr;
};

void Wasm::startForTesting(std::unique_ptr<Context> context) {
  auto context_ptr = context.get();
  if (!context->wasm_) {
    // Initialization was delayed till the Wasm object was created.
    context->wasm_ = this;
    context->id_ = allocContextId();
    context->makeLogPrefix();
    contexts_[context->id_] = context.get();
  }
  root_contexts_[""] = std::move(context);
  context_ptr->onStart("", "");
}

void Wasm::setErrno(int32_t err) {
  if (!__errno_location_) {
    return;
  }
  Word location = __errno_location_(vmContext());
  setDatatype(location.u64, err);
}

void Wasm::setTickPeriod(uint32_t context_id, std::chrono::milliseconds new_tick_period) {
  auto& tick_period = tick_period_[context_id];
  auto& timer = timer_[context_id];
  bool was_running = timer && tick_period.count() > 0;
  tick_period = new_tick_period;
  if (tick_period.count() > 0 && !was_running) {
    timer = dispatcher_.createTimer([weak = std::weak_ptr<Wasm>(shared_from_this()), context_id]() {
      auto shared = weak.lock();
      if (shared)
        shared->tickHandler(context_id);
    });
    timer->enableTimer(tick_period);
  }
}

void Wasm::tickHandler(uint32_t root_context_id) {
  auto& tick_period = tick_period_[root_context_id];
  auto& timer = timer_[root_context_id];
  if (onTick_) {
    onTick_(getContext(root_context_id), root_context_id);
    if (timer && tick_period.count() > 0) {
      timer->enableTimer(tick_period);
    }
  }
}

uint32_t Wasm::allocContextId() {
  while (true) {
    auto id = next_context_id_++;
    // Prevent reuse.
    if (contexts_.find(id) == contexts_.end()) {
      return id;
    }
  }
}

void Wasm::queueReady(uint32_t root_context_id, uint32_t token) {
  auto it = contexts_.find(root_context_id);
  if (it == contexts_.end() || !it->second->isRootContext()) {
    return;
  }
  it->second->onQueueReady(token);
}

void Wasm::log(absl::string_view root_id, const Http::HeaderMap* request_headers,
               const Http::HeaderMap* response_headers, const Http::HeaderMap* response_trailers,
               const StreamInfo::StreamInfo& stream_info) {
  // Check dynamic metadata for the id_ of the stream for this root_id.
  Context* context = nullptr;
  auto metadata_it = stream_info.dynamicMetadata().filter_metadata().find(
      HttpFilters::HttpFilterNames::get().Wasm);
  if (metadata_it != stream_info.dynamicMetadata().filter_metadata().end()) {
    auto find_id =
        metadata_it->second.fields().find(std::string("_stream_id_" + std::string(root_id)));
    if (find_id != metadata_it->second.fields().end()) {
      context = getContext(static_cast<uint32_t>(find_id->second.number_value()));
    }
  }
  if (!context) {
    context = getRootContext(root_id);
  }
  context->log(request_headers, response_headers, response_trailers, stream_info);
}

void Context::log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
                  const Http::HeaderMap* response_trailers,
                  const StreamInfo::StreamInfo& stream_info) {
  access_log_request_headers_ = request_headers;
  // ? request_trailers  ?
  access_log_response_headers_ = response_headers;
  access_log_response_trailers_ = response_trailers;
  access_log_stream_info_ = &stream_info;

  onLog();

  access_log_request_headers_ = nullptr;
  // ? request_trailers  ?
  access_log_response_headers_ = nullptr;
  access_log_response_trailers_ = nullptr;
  access_log_stream_info_ = nullptr;

  onDelete();
}

void Context::onDestroy() {
  if (destroyed_) {
    return;
  }
  destroyed_ = true;
  onDone();
}

void Context::onDone() {
  if (wasm_->onDone_) {
    wasm_->onDone_(this, id_);
  }
}

void Context::onLog() {
  if (wasm_->onLog_) {
    wasm_->onLog_(this, id_);
  }
}

void Context::onDelete() {
  if (wasm_->onDelete_) {
    wasm_->onDelete_(this, id_);
  }
}

Http::FilterHeadersStatus Context::decodeHeaders(Http::HeaderMap& headers, bool end_stream) {
  request_headers_ = &headers;
  request_end_of_stream_ = end_stream;
  auto result = onRequestHeaders();
  request_headers_ = nullptr;
  return result;
}

Http::FilterDataStatus Context::decodeData(Buffer::Instance& data, bool end_stream) {
  requestBodyBuffer_ = &data;
  auto result = onRequestBody(data.length(), end_stream);
  requestBodyBuffer_ = nullptr;
  return result;
}

Http::FilterTrailersStatus Context::decodeTrailers(Http::HeaderMap& trailers) {
  request_trailers_ = &trailers;
  auto result = onRequestTrailers();
  request_trailers_ = nullptr;
  return result;
}

Http::FilterMetadataStatus Context::decodeMetadata(Http::MetadataMap& response_metadata) {
  response_metadata_ = &response_metadata;
  auto result = onRequestMetadata();
  response_metadata_ = nullptr;
  return result;
}

void Context::setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) {
  decoder_callbacks_ = &callbacks;
}

Http::FilterHeadersStatus Context::encode100ContinueHeaders(Http::HeaderMap&) {
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterHeadersStatus Context::encodeHeaders(Http::HeaderMap& headers, bool end_stream) {
  response_headers_ = &headers;
  response_end_of_stream_ = end_stream;
  auto result = onResponseHeaders();
  response_headers_ = nullptr;
  return result;
}

Http::FilterDataStatus Context::encodeData(Buffer::Instance& data, bool end_stream) {
  responseBodyBuffer_ = &data;
  auto result = onResponseBody(data.length(), end_stream);
  responseBodyBuffer_ = nullptr;
  return result;
}

Http::FilterTrailersStatus Context::encodeTrailers(Http::HeaderMap& trailers) {
  response_trailers_ = &trailers;
  auto result = onResponseTrailers();
  response_trailers_ = nullptr;
  return result;
}

Http::FilterMetadataStatus Context::encodeMetadata(Http::MetadataMap& response_metadata) {
  response_metadata_ = &response_metadata;
  auto result = onResponseMetadata();
  response_metadata_ = nullptr;
  return result;
}

//  Http::FilterMetadataStatus::Continue;

void Context::setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

void Context::onHttpCallSuccess(uint32_t token, Envoy::Http::MessagePtr& response) {
  auto body =
      absl::string_view(static_cast<char*>(response->body()->linearize(response->body()->length())),
                        response->body()->length());
  onHttpCallResponse(token, headerMapToPairs(&response->headers()), body,
                     headerMapToPairs(response->trailers()));
  http_request_.erase(token);
}

void Context::onHttpCallFailure(uint32_t token, Http::AsyncClient::FailureReason /* reason */) {
  onHttpCallResponse(token, {}, "", {});
  http_request_.erase(token);
}

void AsyncClientHandler::onSuccess(Envoy::Http::MessagePtr&& response) {
  context->onHttpCallSuccess(token, response);
}

void AsyncClientHandler::onFailure(Http::AsyncClient::FailureReason reason) {
  context->onHttpCallFailure(token, reason);
}

void GrpcCallClientHandler::onCreateInitialMetadata(Http::HeaderMap& metadata) {
  context->onGrpcCreateInitialMetadata(token, metadata);
}

void GrpcStreamClientHandler::onCreateInitialMetadata(Http::HeaderMap& metadata) {
  context->onGrpcCreateInitialMetadata(token, metadata);
}

void GrpcStreamClientHandler::onReceiveInitialMetadata(Http::HeaderMapPtr&& metadata) {
  context->onGrpcReceiveInitialMetadata(token, std::move(metadata));
}

void GrpcStreamClientHandler::onReceiveTrailingMetadata(Http::HeaderMapPtr&& metadata) {
  context->onGrpcReceiveTrailingMetadata(token, std::move(metadata));
}

void Context::onGrpcReceive(uint32_t token, Buffer::InstancePtr response) {
  if (wasm_->onGrpcReceive_) {
    auto response_size = response->length();
    auto response_ptr = wasm_->copyBuffer(*response);
    wasm_->onGrpcReceive_(this, id_, token, response_ptr, response_size);
  }
  if (IsGrpcCallToken(token)) {
    grpc_call_request_.erase(token);
  }
}

void Context::onGrpcClose(uint32_t token, const Grpc::Status::GrpcStatus& status,
                          const absl::string_view message) {
  if (wasm_->onGrpcClose_) {
    auto message_ptr = wasm_->copyString(message);
    wasm_->onGrpcClose_(this, id_, token, static_cast<uint64_t>(status), message_ptr,
                        message.size());
  }
  if (IsGrpcCallToken(token)) {
    grpc_call_request_.erase(token);
  } else {
    grpc_stream_.erase(token);
  }
}

WasmResult Context::grpcSend(uint32_t token, absl::string_view message, bool end_stream) {
  if (IsGrpcCallToken(token)) {
    return WasmResult::BadArgument;
  }
  auto it = grpc_stream_.find(token);
  if (it == grpc_stream_.end()) {
    return WasmResult::NotFound;
  }
  if (it != grpc_stream_.end() && it->second.stream) {
    it->second.stream->sendMessageRaw(
        Buffer::InstancePtr(new Buffer::OwnedImpl(message.data(), message.size())), end_stream);
  }
  return WasmResult::Ok;
}

WasmResult Context::grpcClose(uint32_t token) {
  if (IsGrpcCallToken(token)) {
    auto it = grpc_call_request_.find(token);
    if (it == grpc_call_request_.end()) {
      return WasmResult::NotFound;
    }
    if (it != grpc_call_request_.end() && it->second.request) {
      it->second.request->cancel();
    }
    grpc_call_request_.erase(token);
  } else {
    auto it = grpc_stream_.find(token);
    if (it == grpc_stream_.end()) {
      return WasmResult::NotFound;
    }
    if (it != grpc_stream_.end() && it->second.stream) {
      it->second.stream->closeStream();
    }
    grpc_stream_.erase(token);
  }
  return WasmResult::Ok;
}

WasmResult Context::grpcCancel(uint32_t token) {
  if (IsGrpcCallToken(token)) {
    auto it = grpc_call_request_.find(token);
    if (it == grpc_call_request_.end()) {
      return WasmResult::NotFound;
    }
    if (it != grpc_call_request_.end() && it->second.request) {
      it->second.request->cancel();
    }
    grpc_call_request_.erase(token);
  } else {
    auto it = grpc_stream_.find(token);
    if (it == grpc_stream_.end()) {
      return WasmResult::NotFound;
    }
    if (it != grpc_stream_.end() && it->second.stream) {
      it->second.stream->resetStream();
    }
    grpc_stream_.erase(token);
  }
  return WasmResult::Ok;
}

void GrpcCallClientHandler::onSuccessRaw(Buffer::InstancePtr&& response, Tracing::Span&) {
  context->onGrpcReceive(token, std::move(response));
}

void GrpcCallClientHandler::onFailure(Grpc::Status::GrpcStatus status, const std::string& message,
                                      Tracing::Span&) {
  context->onGrpcClose(token, status, message);
}

bool GrpcStreamClientHandler::onReceiveMessageRaw(Buffer::InstancePtr&& response) {
  context->onGrpcReceive(token, std::move(response));
  return true;
}

void GrpcStreamClientHandler::onRemoteClose(Grpc::Status::GrpcStatus status,
                                            const std::string& message) {
  context->onGrpcClose(token, status, message);
}

static std::shared_ptr<Wasm> createWasmInternal(
    absl::string_view vm_id, const envoy::config::wasm::v2::VmConfig& vm_config,
    absl::string_view root_id, // e.g. filter instance id
    Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher, Api::Api& api,
    Stats::Scope& scope, PluginDirection direction, const LocalInfo::LocalInfo& local_info,
    const envoy::api::v2::core::Metadata* listener_metadata, Stats::ScopeSharedPtr scope_ptr,
    std::unique_ptr<Context> root_context_for_testing) {
  auto wasm = std::make_shared<Wasm>(vm_config.vm(), vm_id, vm_config.configuration(),
                                     cluster_manager, dispatcher, scope, direction, local_info,
                                     listener_metadata, scope_ptr);
  const auto& code = Config::DataSource::read(vm_config.code(), true, api);
  const auto& path = Config::DataSource::getPath(vm_config.code())
                         .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  if (code.empty()) {
    throw WasmException(fmt::format("Failed to load WASM code from {}", path));
  }
  if (!wasm->initialize(code, vm_id, vm_config.allow_precompiled())) {
    throw WasmException(fmt::format("Failed to initialize WASM code from {}", path));
  }
  Context* context;
  if (!root_context_for_testing) {
    context = wasm->start(root_id, vm_config.configuration());
  } else {
    context = root_context_for_testing.get();
    wasm->startForTesting(std::move(root_context_for_testing));
  }
  return wasm;
}

std::shared_ptr<Wasm> createWasm(absl::string_view vm_id,
                                 const envoy::config::wasm::v2::VmConfig& vm_config,
                                 absl::string_view root_id, // e.g. filter instance id
                                 Upstream::ClusterManager& cluster_manager,
                                 Event::Dispatcher& dispatcher, Api::Api& api, Stats::Scope& scope,
                                 PluginDirection direction, const LocalInfo::LocalInfo& local_info,
                                 const envoy::api::v2::core::Metadata* listener_metadata,
                                 Stats::ScopeSharedPtr scope_ptr) {
  return createWasmInternal(vm_id, vm_config, root_id, cluster_manager, dispatcher, api, scope,
                            direction, local_info, listener_metadata, scope_ptr, nullptr);
}

std::shared_ptr<Wasm> createWasmForTesting(
    absl::string_view vm_id, const envoy::config::wasm::v2::VmConfig& vm_config,
    absl::string_view root_id, // e.g. filter instance id
    Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher, Api::Api& api,
    Stats::Scope& scope, PluginDirection direction, const LocalInfo::LocalInfo& local_info,
    const envoy::api::v2::core::Metadata* listener_metadata, Stats::ScopeSharedPtr scope_ptr,
    std::unique_ptr<Context> root_context_for_testing) {
  return createWasmInternal(vm_id, vm_config, root_id, cluster_manager, dispatcher, api, scope,
                            direction, local_info, listener_metadata, scope_ptr,
                            std::move(root_context_for_testing));
}

std::shared_ptr<Wasm> createThreadLocalWasm(Wasm& base_wasm, absl::string_view root_id,
                                            absl::string_view configuration,
                                            Event::Dispatcher& dispatcher) {
  std::shared_ptr<Wasm> wasm;
  Context* root_context;
  if (base_wasm.wasmVm()->clonable()) {
    wasm = std::make_shared<Wasm>(base_wasm, dispatcher);
    root_context = wasm->start(root_id, base_wasm.vm_configuration());
  } else {
    wasm = std::make_shared<Wasm>(
        base_wasm.wasmVm()->vm(), base_wasm.id(), base_wasm.vm_configuration(),
        base_wasm.clusterManager(), dispatcher, base_wasm.scope(), base_wasm.direction(),
        base_wasm.localInfo(), base_wasm.listenerMetadata(), nullptr /* owned scope */);
    if (!wasm->initialize(base_wasm.code(), base_wasm.id(), base_wasm.allow_precompiled())) {
      throw WasmException("Failed to initialize WASM code");
    }
    root_context = wasm->start(root_id, base_wasm.vm_configuration());
  }
  wasm->configure(root_context, configuration);
  if (!wasm->id().empty()) {
    local_wasms[wasm->id()] = wasm;
  }
  return wasm;
}

std::shared_ptr<Wasm> getThreadLocalWasm(absl::string_view vm_id, absl::string_view id,
                                         absl::string_view configuration) {
  auto it = local_wasms.find(vm_id);
  if (it == local_wasms.end()) {
    throw WasmException(fmt::format("Failed to find WASM vm_id {}", vm_id));
  }
  auto wasm = it->second;
  auto root_context = wasm->start(id, wasm->vm_configuration());
  wasm->configure(root_context, configuration);
  return wasm;
}

std::shared_ptr<Wasm> getThreadLocalWasmOrNull(absl::string_view id) {
  auto it = local_wasms.find(id);
  if (it == local_wasms.end()) {
    return nullptr;
  }
  return it->second;
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
