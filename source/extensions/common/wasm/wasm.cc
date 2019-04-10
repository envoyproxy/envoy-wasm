#include "extensions/common/wasm/wasm.h"

#include <stdio.h>

#include <limits>
#include <memory>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/http/codes.h"
#include "envoy/server/wasm.h"
#include "envoy/thread_local/thread_local.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/c_smart_ptr.h"
#include "common/common/empty_string.h"
#include "common/common/enum_to_int.h"
#include "common/common/logger.h"
#include "common/config/datasource.h"
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"

#include "extensions/common/wasm/wavm/wavm.h"
#include "extensions/common/wasm/well_known_names.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "absl/synchronization/mutex.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

// Any currently executing Wasm call context.
thread_local Context* current_context_ = nullptr;
#define WASM_CONTEXT(_c) (static_cast<Context*>((void)_c, current_context_))

namespace {

class SharedData {
public:
  std::pair<std::string, uint32_t> get(absl::string_view id, const absl::string_view key) {
    absl::ReaderMutexLock l(&mutex);
    auto map = data.find(id);
    if (map == data.end())
      return {"", 0};
    auto it = map->second.find(key);
    if (it != map->second.end()) {
      return it->second;
    }
    return {"", 0};
  }

  bool set(absl::string_view id, absl::string_view key, absl::string_view value, uint32_t cas) {
    absl::WriterMutexLock l(&mutex);
    absl::flat_hash_map<std::string, std::pair<std::string, uint32_t>>* map;
    auto map_it = data.find(id);
    if (map_it == data.end()) {
      map = &data[id];
    } else {
      map = &map_it->second;
    }
    auto it = map->find(key);
    if (it != map->end()) {
      if (cas && cas != it->second.second)
        return false;
      it->second = std::make_pair(std::string(value), nextCas());
    } else {
      map->emplace(key, std::make_pair(std::string(value), nextCas()));
    }
    return true;
  }

  uint32_t nextCas() {
    auto result = cas;
    cas++;
    if (!cas)
      cas++;
    return result;
  }

private:
  absl::Mutex mutex;
  uint32_t cas = 1;
  absl::node_hash_map<std::string,
                      absl::flat_hash_map<std::string, std::pair<std::string, uint32_t>>>
      data;
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
  if (sizeof(uint32_t) + size * 2 * sizeof(uint32_t) > buffer.size())
    return {};
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
void getPairs(Context* context, const Pairs& result, uint32_t ptr_ptr, uint32_t size_ptr) {
  if (result.empty()) {
    context->wasm()->copyToPointerSize("", ptr_ptr, size_ptr);
    return;
  }
  uint32_t size = pairsSize(result);
  uint32_t ptr;
  char* buffer = static_cast<char*>(context->wasm()->allocMemory(size, &ptr));
  marshalPairs(result, buffer);
  context->wasmVm()->setMemory(ptr_ptr, sizeof(int32_t), &ptr);
  context->wasmVm()->setMemory(size_ptr, sizeof(int32_t), &size);
}

void exportPairs(Context* context, const Pairs& pairs, uint32_t* ptr_ptr, uint32_t* size_ptr) {
  if (pairs.empty()) {
    *ptr_ptr = 0;
    *size_ptr = 0;
    return;
  }
  uint32_t size = pairsSize(pairs);
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

//
// HTTP Handlers
//

// StreamInfo
void getRequestStreamInfoProtocolHandler(void* raw_context, uint32_t value_ptr_ptr,
                                         uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  context->wasm()->copyToPointerSize(context->getRequestStreamInfoProtocol(), value_ptr_ptr,
                                     value_size_ptr);
}

void getResponseStreamInfoProtocolHandler(void* raw_context, uint32_t value_ptr_ptr,
                                          uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  context->wasm()->copyToPointerSize(context->getResponseStreamInfoProtocol(), value_ptr_ptr,
                                     value_size_ptr);
}

void getRequestMetadataHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                               uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  context->wasm()->copyToPointerSize(
      context->getRequestMetadata(context->wasmVm()->getMemory(key_ptr, key_size)), value_ptr_ptr,
      value_size_ptr);
}

void setRequestMetadataHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                               uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->setRequestMetadata(context->wasmVm()->getMemory(key_ptr, key_size),
                              context->wasmVm()->getMemory(value_ptr, value_size));
}

void getRequestMetadataPairsHandler(void* raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  getPairs(context, context->getRequestMetadataPairs(), ptr_ptr, size_ptr);
}

void getResponseMetadataHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                                uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  context->wasm()->copyToPointerSize(
      context->getResponseMetadata(context->wasmVm()->getMemory(key_ptr, key_size)), value_ptr_ptr,
      value_size_ptr);
}

void setResponseMetadataHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                                uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->setResponseMetadata(context->wasmVm()->getMemory(key_ptr, key_size),
                               context->wasmVm()->getMemory(value_ptr, value_size));
}

void getResponseMetadataPairsHandler(void* raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  getPairs(context, context->getResponseMetadataPairs(), ptr_ptr, size_ptr);
}

// Continue
void continueRequestHandler(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->continueRequest();
}

void continueResponseHandler(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->continueResponse();
}

// SharedData
void getSharedDataHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                          uint32_t value_ptr_ptr, uint32_t value_size_ptr, uint32_t cas_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  auto p = context->getSharedData(key);
  context->wasm()->copyToPointerSize(p.first, value_ptr_ptr, value_size_ptr);
  context->wasmVm()->setMemory(cas_ptr, sizeof(uint32_t), &p.second);
}

uint32_t setSharedDataHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                              uint32_t value_ptr, uint32_t value_size, uint32_t cas) {
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr, key_size);
  auto value = context->wasmVm()->getMemory(value_ptr, value_size);
  auto ok = context->setSharedData(key, value, cas);
  return static_cast<uint32_t>(ok);
}

// Request Headers Handlers
void addRequestHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                             uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->addRequestHeader(context->wasmVm()->getMemory(key_ptr, key_size),
                            context->wasmVm()->getMemory(value_ptr, value_size));
}

void getRequestHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                             uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getRequestHeader(context->wasmVm()->getMemory(key_ptr, key_size));
  context->wasm()->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceRequestHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                                 uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->replaceRequestHeader(context->wasmVm()->getMemory(key_ptr, key_size),
                                context->wasmVm()->getMemory(value_ptr, value_size));
}

void removeRequestHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->removeRequestHeader(context->wasmVm()->getMemory(key_ptr, key_size));
}

void getRequestHeaderPairsHandler(void* raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getRequestHeaderPairs();
  getPairs(context, result, ptr_ptr, size_ptr);
}

// Request Trailers Handlers
void addRequestTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                              uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->addRequestTrailer(context->wasmVm()->getMemory(key_ptr, key_size),
                             context->wasmVm()->getMemory(value_ptr, value_size));
}

void getRequestTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                              uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getRequestTrailer(context->wasmVm()->getMemory(key_ptr, key_size));
  context->wasm()->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceRequestTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                                  uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->replaceRequestTrailer(context->wasmVm()->getMemory(key_ptr, key_size),
                                 context->wasmVm()->getMemory(value_ptr, value_size));
}

void removeRequestTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->removeRequestTrailer(context->wasmVm()->getMemory(key_ptr, key_size));
}

void getRequestTrailerPairsHandler(void* raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getRequestTrailerPairs();
  getPairs(context, result, ptr_ptr, size_ptr);
}

// Response Headers Handlers
void addResponseHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                              uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->addResponseHeader(context->wasmVm()->getMemory(key_ptr, key_size),
                             context->wasmVm()->getMemory(value_ptr, value_size));
}

void getResponseHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                              uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getResponseHeader(context->wasmVm()->getMemory(key_ptr, key_size));
  context->wasm()->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceResponseHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                                  uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->replaceResponseHeader(context->wasmVm()->getMemory(key_ptr, key_size),
                                 context->wasmVm()->getMemory(value_ptr, value_size));
}

void removeResponseHeaderHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->removeResponseHeader(context->wasmVm()->getMemory(key_ptr, key_size));
}

void getResponseHeaderPairsHandler(void* raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getResponseHeaderPairs();
  getPairs(context, result, ptr_ptr, size_ptr);
}

// Response Trailers Handlers
void addResponseTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                               uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->addResponseTrailer(context->wasmVm()->getMemory(key_ptr, key_size),
                              context->wasmVm()->getMemory(value_ptr, value_size));
}

void getResponseTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                               uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getResponseTrailer(context->wasmVm()->getMemory(key_ptr, key_size));
  context->wasm()->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceResponseTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size,
                                   uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->replaceResponseTrailer(context->wasmVm()->getMemory(key_ptr, key_size),
                                  context->wasmVm()->getMemory(value_ptr, value_size));
}

void removeResponseTrailerHandler(void* raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto context = WASM_CONTEXT(raw_context);
  context->removeResponseTrailer(context->wasmVm()->getMemory(key_ptr, key_size));
}

void getResponseTrailerPairsHandler(void* raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getResponseTrailerPairs();
  getPairs(context, result, ptr_ptr, size_ptr);
}

// Body Buffer
void getRequestBodyBufferBytesHandler(void* raw_context, uint32_t start, uint32_t length,
                                      uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getRequestBodyBufferBytes(start, length);
  context->wasm()->copyToPointerSize(result, ptr_ptr, size_ptr);
}

void getResponseBodyBufferBytesHandler(void* raw_context, uint32_t start, uint32_t length,
                                       uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getResponseBodyBufferBytes(start, length);
  context->wasm()->copyToPointerSize(result, ptr_ptr, size_ptr);
}

uint32_t httpCallHandler(void* raw_context, uint32_t uri_ptr, uint32_t uri_size,
                         uint32_t header_pairs_ptr, uint32_t header_pairs_size, uint32_t body_ptr,
                         uint32_t body_size, uint32_t trailer_pairs_ptr,
                         uint32_t trailer_pairs_size, uint32_t timeout_milliseconds) {
  auto context = WASM_CONTEXT(raw_context);
  auto uri = context->wasmVm()->getMemory(uri_ptr, uri_size);
  auto headers = toPairs(context->wasmVm()->getMemory(header_pairs_ptr, header_pairs_size));
  auto body = context->wasmVm()->getMemory(body_ptr, body_size);
  auto trailers = toPairs(context->wasmVm()->getMemory(trailer_pairs_ptr, trailer_pairs_size));
  return context->httpCall(uri, headers, body, trailers, timeout_milliseconds);
}

uint32_t getTotalMemoryHandler(void*) { return 0x7FFFFFFF; }
uint32_t _emscripten_get_heap_sizeHandler(void*) { return 0x7FFFFFFF; }
void _llvm_trapHandler(void*) { throw WasmException("emscripten llvm_trap"); }

void setTickPeriodMillisecondsHandler(void* raw_context, uint32_t tick_period_milliseconds) {
  WASM_CONTEXT(raw_context)->setTickPeriod(std::chrono::milliseconds(tick_period_milliseconds));
}

void logHandler(void* raw_context, uint32_t level, uint32_t address, uint32_t size) {
  auto context = WASM_CONTEXT(raw_context);
  context->scriptLog(static_cast<spdlog::level::level_enum>(level),
                     context->wasmVm()->getMemory(address, size));
}

// Headers/Trailer Helper Functions
void addHeader(Http::HeaderMap* map, absl::string_view key, absl::string_view value) {
  if (!map)
    return;
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  map->addCopy(lower_key, std::string(value));
}

absl::string_view getHeader(const Http::HeaderMap* map, absl::string_view key_view) {
  if (!map)
    return "";
  const Http::LowerCaseString lower_key(std::move(std::string(key_view)));
  auto entry = map->get(lower_key);
  if (!entry)
    return "";
  return entry->value().getStringView();
}

Pairs getHeaderPairs(const Http::HeaderMap* map) {
  if (!map)
    return {};
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

void removeHeader(Http::HeaderMap* map, absl::string_view key) {
  if (!map)
    return;
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  map->remove(lower_key);
}

void replaceHeader(Http::HeaderMap* map, absl::string_view key, absl::string_view value) {
  if (!map)
    return;
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  auto entry = map->get(lower_key);
  if (entry != nullptr)
    entry->value(value.data(), value.size());
  else
    map->addCopy(lower_key, std::string(value));
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

} // namespace

void Context::setTickPeriod(std::chrono::milliseconds tick_period) {
  wasm_->setTickPeriod(tick_period);
}

// Shared Data
std::pair<std::string, uint32_t> Context::getSharedData(absl::string_view key) {
  return global_shared_data.get(wasm_->id(), key);
}

bool Context::setSharedData(absl::string_view key, absl::string_view value, uint32_t cas) {
  return global_shared_data.set(wasm_->id(), key, value, cas);
}

// Request Headers
void Context::addRequestHeader(absl::string_view key, absl::string_view value) {
  addHeader(request_headers_, key, value);
}

absl::string_view Context::getRequestHeader(absl::string_view key) {
  if (access_log_request_headers_)
    return getHeader(access_log_request_headers_, key);
  return getHeader(request_headers_, key);
}

Pairs Context::getRequestHeaderPairs() {
  if (access_log_request_headers_)
    return getHeaderPairs(access_log_request_headers_);
  return getHeaderPairs(request_headers_);
}

void Context::removeRequestHeader(absl::string_view key) { removeHeader(request_headers_, key); }

void Context::replaceRequestHeader(absl::string_view key, absl::string_view value) {
  replaceHeader(request_headers_, key, value);
}

// Request Trailers
void Context::addRequestTrailer(absl::string_view key, absl::string_view value) {
  addHeader(request_trailers_, key, value);
}

absl::string_view Context::getRequestTrailer(absl::string_view key) {
  if (access_log_request_trailers_)
    return getHeader(access_log_request_trailers_, key);
  return getHeader(request_trailers_, key);
}

Pairs Context::getRequestTrailerPairs() {
  if (access_log_request_trailers_)
    return getHeaderPairs(access_log_request_trailers_);
  return getHeaderPairs(request_trailers_);
}

void Context::removeRequestTrailer(absl::string_view key) { removeHeader(request_trailers_, key); }

void Context::replaceRequestTrailer(absl::string_view key, absl::string_view value) {
  replaceHeader(request_trailers_, key, value);
}

// Response Headers
void Context::addResponseHeader(absl::string_view key, absl::string_view value) {
  addHeader(response_headers_, key, value);
}

absl::string_view Context::getResponseHeader(absl::string_view key) {
  if (access_log_response_headers_)
    return getHeader(access_log_response_headers_, key);
  return getHeader(response_headers_, key);
}

Pairs Context::getResponseHeaderPairs() {
  if (access_log_response_headers_)
    return getHeaderPairs(access_log_response_headers_);
  return getHeaderPairs(response_headers_);
}

void Context::removeResponseHeader(absl::string_view key) { removeHeader(response_headers_, key); }

void Context::replaceResponseHeader(absl::string_view key, absl::string_view value) {
  replaceHeader(response_headers_, key, value);
}

// Response Trailers
void Context::addResponseTrailer(absl::string_view key, absl::string_view value) {
  addHeader(response_trailers_, key, value);
}

absl::string_view Context::getResponseTrailer(absl::string_view key) {
  if (access_log_response_trailers_)
    return getHeader(access_log_response_trailers_, key);
  return getHeader(response_trailers_, key);
}

Pairs Context::getResponseTrailerPairs() {
  if (access_log_response_trailers_)
    return getHeaderPairs(access_log_response_trailers_);
  return getHeaderPairs(response_trailers_);
}

void Context::removeResponseTrailer(absl::string_view key) {
  removeHeader(response_trailers_, key);
}

void Context::replaceResponseTrailer(absl::string_view key, absl::string_view value) {
  replaceHeader(response_trailers_, key, value);
}

// Body Buffer

absl::string_view Context::getRequestBodyBufferBytes(uint32_t start, uint32_t length) {
  if (!requestBodyBuffer_)
    return "";
  if (requestBodyBuffer_->length() < static_cast<uint64_t>((start + length)))
    return "";
  return absl::string_view(
      static_cast<char*>(requestBodyBuffer_->linearize(start + length)) + start, length);
}

absl::string_view Context::getResponseBodyBufferBytes(uint32_t start, uint32_t length) {
  if (!responseBodyBuffer_)
    return "";
  if (responseBodyBuffer_->length() < static_cast<uint64_t>((start + length)))
    return "";
  return absl::string_view(
      static_cast<char*>(responseBodyBuffer_->linearize(start + length)) + start, length);
}

// Async call via HTTP
uint32_t Context::httpCall(absl::string_view cluster, const Pairs& request_headers,
                           absl::string_view request_body, const Pairs& request_trailers,
                           int timeout_milliseconds) {
  if (timeout_milliseconds < 0)
    return 0;
  auto cluster_string = std::string(cluster);
  if (clusterManager().get(cluster_string) == nullptr)
    return 0;

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

  auto token = next_async_token_++;
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

void Context::httpRespond(const Pairs& response_headers, absl::string_view body,
                          const Pairs& response_trailers) {
  (void)response_headers;
  (void)body;
  (void)response_trailers;
}

const StreamInfo::StreamInfo& Context::streamInfo() const {
  if (access_log_stream_info_)
    return *access_log_stream_info_;
  return decoder_callbacks_->streamInfo();
}

// StreamInfo
std::string Context::getRequestStreamInfoProtocol() {
  if (!decoder_callbacks_)
    return "";
  return Http::Utility::getProtocolString(streamInfo().protocol().value());
}

std::string Context::getResponseStreamInfoProtocol() {
  if (!encoder_callbacks_)
    return "";
  return Http::Utility::getProtocolString(streamInfo().protocol().value());
}

// Metadata: the values are serialized ProtobufWkt::Struct
std::string Context::getRequestMetadata(absl::string_view key) {
  if (!decoder_callbacks_)
    return "";
  auto& proto_struct = getMetadata(decoder_callbacks_);
  auto it = proto_struct.fields().find(std::string(key));
  if (it == proto_struct.fields().end())
    return "";
  std::string result;
  it->second.SerializeToString(&result);
  return result;
}

void Context::setRequestMetadata(absl::string_view key, absl::string_view serialized_proto_struct) {
  if (!decoder_callbacks_)
    return;
  decoder_callbacks_->streamInfo().setDynamicMetadata(
      HttpFilters::HttpFilterNames::get().Wasm,
      MessageUtil::keyValueStruct(std::string(key), std::string(serialized_proto_struct)));
}

PairsWithStringValues Context::getRequestMetadataPairs() {
  PairsWithStringValues result;
  if (!encoder_callbacks_)
    return {};
  auto& proto_struct = getMetadata(encoder_callbacks_);
  for (auto& p : proto_struct.fields()) {
    std::string value;
    p.second.SerializeToString(&value);
    result.emplace_back(p.first, std::move(value));
  }
  return result;
}

std::string Context::getResponseMetadata(absl::string_view key) {
  if (!encoder_callbacks_)
    return "";
  auto& proto_struct = getMetadata(encoder_callbacks_);
  auto it = proto_struct.fields().find(std::string(key));
  if (it == proto_struct.fields().end())
    return "";
  std::string result;
  it->second.SerializeToString(&result);
  return result;
}

void Context::setResponseMetadata(absl::string_view key,
                                  absl::string_view serialized_proto_struct) {
  if (!encoder_callbacks_)
    return;
  encoder_callbacks_->streamInfo().setDynamicMetadata(
      HttpFilters::HttpFilterNames::get().Wasm,
      MessageUtil::keyValueStruct(std::string(key), std::string(serialized_proto_struct)));
}

PairsWithStringValues Context::getResponseMetadataPairs() {
  PairsWithStringValues result;
  if (!encoder_callbacks_)
    return {};
  auto& proto_struct = getMetadata(encoder_callbacks_);
  for (auto& p : proto_struct.fields()) {
    std::string value;
    p.second.SerializeToString(&value);
    result.emplace_back(p.first, std::move(value));
  }
  return result;
}

void Context::scriptLog(spdlog::level::level_enum level, absl::string_view message) {
  switch (level) {
  case spdlog::level::trace:
    ENVOY_LOG(trace, "wasm log: {}", message);
    return;
  case spdlog::level::debug:
    ENVOY_LOG(debug, "wasm log: {}", message);
    return;
  case spdlog::level::info:
    ENVOY_LOG(info, "wasm log: {}", message);
    return;
  case spdlog::level::warn:
    ENVOY_LOG(warn, "wasm log: {}", message);
    return;
  case spdlog::level::err:
    ENVOY_LOG(error, "wasm log: {}", message);
    return;
  case spdlog::level::critical:
    ENVOY_LOG(critical, "wasm log: {}", message);
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
void Context::onStart() {
  if (wasm_->onStart_) {
    wasm_->onStart_(this);
  }
}

void Context::onConfigure(absl::string_view configuration) {
  if (!wasm_->onConfigure_)
    return;
  if (configuration.empty())
    return;
  auto address = wasm_->copyString(configuration);
  wasm_->onConfigure_(this, address, static_cast<uint32_t>(configuration.size()));
}

void Context::onCreate() {
  if (wasm_->onCreate_) {
    wasm_->onCreate_(this, id_);
  }
}

Http::FilterHeadersStatus Context::onRequestHeaders() {
  onCreate();
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
  if (!wasm_->onResponseTrailers_)
    return Http::FilterTrailersStatus::Continue;
  if (wasm_->onResponseTrailers_(this, id_) == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return Http::FilterTrailersStatus::StopIteration;
}

Http::FilterMetadataStatus Context::onResponseMetadata() {
  if (!wasm_->onResponseMetadata_)
    return Http::FilterMetadataStatus::Continue;
  if (wasm_->onResponseMetadata_(this, id_) == 0) {
    return Http::FilterMetadataStatus::Continue;
  }
  return Http::FilterMetadataStatus::Continue; // This is currently the only return code.
}

void Context::onHttpCallResponse(uint32_t token, const Pairs& response_headers,
                                 absl::string_view response_body, const Pairs& response_trailers) {
  if (!wasm_->onHttpCallResponse_)
    return;
  uint32_t headers_ptr, headers_size, trailers_ptr, trailers_size;
  exportPairs(this, response_headers, &headers_ptr, &headers_size);
  exportPairs(this, response_trailers, &trailers_ptr, &trailers_size);
  uint32_t body_ptr = wasm_->copyString(response_body);
  uint32_t body_size = response_body.size();
  wasm_->onHttpCallResponse_(this, id_, token, headers_ptr, headers_size, body_ptr, body_size,
                             trailers_ptr, trailers_size);
}

Wasm::Wasm(absl::string_view vm, absl::string_view id, absl::string_view initial_configuration,
           Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher)
    : cluster_manager_(cluster_manager), dispatcher_(dispatcher),
      initial_configuration_(initial_configuration) {
  wasm_vm_ = Common::Wasm::createWasmVm(vm);
  id_ = std::string(id);
}

void Wasm::registerCallbacks() {
#define _REGISTER(_fn) wasm_vm_->registerCallback("envoy", #_fn, &_fn##Handler);
  if (is_emscripten_) {
    _REGISTER(getTotalMemory);
    _REGISTER(_emscripten_get_heap_size);
    _REGISTER(_llvm_trap);
  }
#undef _REGISTER

  // Calls with the "_proxy_" prefix.
#define _REGISTER_PROXY(_fn) wasm_vm_->registerCallback("envoy", "_proxy_" #_fn, &_fn##Handler);
  _REGISTER_PROXY(log);

  _REGISTER_PROXY(getRequestStreamInfoProtocol);
  _REGISTER_PROXY(getResponseStreamInfoProtocol);

  _REGISTER_PROXY(getRequestMetadata);
  _REGISTER_PROXY(setRequestMetadata);
  _REGISTER_PROXY(getRequestMetadataPairs);
  _REGISTER_PROXY(getResponseMetadata);
  _REGISTER_PROXY(setResponseMetadata);
  _REGISTER_PROXY(getResponseMetadataPairs);

  _REGISTER_PROXY(continueRequest);
  _REGISTER_PROXY(continueResponse);

  _REGISTER_PROXY(getSharedData);
  _REGISTER_PROXY(setSharedData);

  _REGISTER_PROXY(getRequestHeader);
  _REGISTER_PROXY(addRequestHeader);
  _REGISTER_PROXY(replaceRequestHeader);
  _REGISTER_PROXY(removeRequestHeader);
  _REGISTER_PROXY(getRequestHeaderPairs);

  _REGISTER_PROXY(getRequestTrailer);
  _REGISTER_PROXY(addRequestTrailer);
  _REGISTER_PROXY(replaceRequestTrailer);
  _REGISTER_PROXY(removeRequestTrailer);
  _REGISTER_PROXY(getRequestTrailerPairs);

  _REGISTER_PROXY(getResponseHeader);
  _REGISTER_PROXY(addResponseHeader);
  _REGISTER_PROXY(replaceResponseHeader);
  _REGISTER_PROXY(removeResponseHeader);
  _REGISTER_PROXY(getResponseHeaderPairs);

  _REGISTER_PROXY(getResponseTrailer);
  _REGISTER_PROXY(addResponseTrailer);
  _REGISTER_PROXY(replaceResponseTrailer);
  _REGISTER_PROXY(removeResponseTrailer);
  _REGISTER_PROXY(getResponseTrailerPairs);

  _REGISTER_PROXY(getRequestBodyBufferBytes);
  _REGISTER_PROXY(getResponseBodyBufferBytes);

  _REGISTER_PROXY(httpCall);

  _REGISTER_PROXY(setTickPeriodMilliseconds);
#undef _REGISTER_PROXY
}

void Wasm::establishEnvironment() {
  if (is_emscripten_) {
    wasm_vm_->makeModule("global");
    emscripten_NaN_ = wasm_vm_->makeGlobal("global", "NaN", std::nan("0"));
    emscripten_Infinity_ =
        wasm_vm_->makeGlobal("global", "Infinity", std::numeric_limits<double>::infinity());
  }
}

void Wasm::getFunctions() {
#define _GET(_fn) wasm_vm_->getFunction("_" #_fn, &_fn##_);
  _GET(malloc);
  _GET(free);
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
  _GET_PROXY(onDone);
  _GET_PROXY(onLog);
  _GET_PROXY(onDelete);
#undef _GET_PROXY

  if (!malloc_ || !free_) {
    throw WasmException("WAVM missing malloc/free");
  }
}

Wasm::Wasm(const Wasm& wasm, Event::Dispatcher& dispatcher)
    : std::enable_shared_from_this<Wasm>(wasm), cluster_manager_(wasm.cluster_manager_),
      dispatcher_(dispatcher) {
  wasm_vm_ = wasm.wasmVm()->clone();
  general_context_ = createContext();
  getFunctions();
}

bool Wasm::initialize(const std::string& code, absl::string_view name, bool allow_precompiled) {
  if (!wasm_vm_)
    return false;
  auto ok = wasm_vm_->load(code, allow_precompiled);
  if (!ok)
    return false;
  auto metadata = wasm_vm_->getUserSection("emscripten_metadata");
  if (!metadata.empty()) {
    is_emscripten_ = true;
    auto start = reinterpret_cast<const uint8_t*>(metadata.data());
    auto end = reinterpret_cast<const uint8_t*>(metadata.data() + metadata.size());
    start = decodeVarint(start, end, &emscripten_major_version_);
    start = decodeVarint(start, end, &emscripten_minor_version_);
    start = decodeVarint(start, end, &emscripten_abi_major_version_);
    start = decodeVarint(start, end, &emscripten_abi_minor_version_);
    start = decodeVarint(start, end, &emscripten_memory_size_);
    decodeVarint(start, end, &emscripten_table_size_);
  }
  registerCallbacks();
  establishEnvironment();
  wasm_vm_->link(name, is_emscripten_);
  general_context_ = createContext();
  wasm_vm_->start(general_context_.get());
  if (is_emscripten_) {
    ASSERT(std::isnan(emscripten_NaN_->get()));
    ASSERT(std::isinf(emscripten_Infinity_->get()));
  }
  code_ = code;
  allow_precompiled_ = allow_precompiled;
  getFunctions();
  return true;
}

void Wasm::configure(absl::string_view configuration) {
  if (onConfigure_ && !configuration.empty()) {
    auto address = copyString(configuration);
    onConfigure_(general_context_.get(), address, configuration.size());
  }
}

void Wasm::start() { general_context_->onStart(); }

void Wasm::setTickPeriod(std::chrono::milliseconds tick_period) {
  bool was_running = timer_ && tick_period_.count() > 0;
  tick_period_ = tick_period;
  if (tick_period_.count() > 0 && !was_running) {
    timer_ = dispatcher_.createTimer([weak = std::weak_ptr<Wasm>(shared_from_this())]() {
      auto shared = weak.lock();
      if (shared)
        shared->tickHandler();
    });
    timer_->enableTimer(tick_period_);
  }
}

void Wasm::tickHandler() {
  if (onTick_) {
    onTick_(general_context_.get());
    if (timer_ && tick_period_.count() > 0) {
      timer_->enableTimer(tick_period_);
    }
  }
}

uint32_t Wasm::allocContextId() { return next_context_id_++; }

void Wasm::log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
               const Http::HeaderMap* response_trailers,
               const StreamInfo::StreamInfo& stream_info) {
  general_context_->log(request_headers, response_headers, response_trailers, stream_info);
}

void Context::log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
                  const Http::HeaderMap* response_trailers,
                  const StreamInfo::StreamInfo& stream_info) {
  access_log_request_headers_ = request_headers;
  // ? request_trailers  ?
  access_log_response_headers_ = response_headers;
  access_log_response_headers_ = response_trailers;
  access_log_stream_info_ = &stream_info;

  onLog();

  access_log_request_headers_ = nullptr;
  // ? request_trailers  ?
  access_log_response_headers_ = nullptr;
  access_log_response_headers_ = nullptr;
  access_log_stream_info_ = nullptr;

  onDelete();
}

void Context::onDestroy() {
  if (destroyed_)
    return;
  destroyed_ = true;
  onDone();
}

void Context::onDone() {
  if (wasm_->onDone_)
    wasm_->onDone_(this, id_);
}

void Context::onLog() {
  if (wasm_->onLog_)
    wasm_->onLog_(this, id_);
}

void Context::onDelete() {
  if (wasm_->onDelete_)
    wasm_->onDelete_(this, id_);
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

Http::FilterMetadataStatus Context::decodeMetadata(Http::MetadataMap&& response_metadata) {
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

void Context::onAsyncClientSuccess(uint32_t token, Envoy::Http::MessagePtr& response) {
  auto body =
      absl::string_view(static_cast<char*>(response->body()->linearize(response->body()->length())),
                        response->body()->length());
  onHttpCallResponse(token, getHeaderPairs(&response->headers()), body,
                     getHeaderPairs(response->trailers()));
  http_request_.erase(token);
}

void Context::onAsyncClientFailure(uint32_t token, Http::AsyncClient::FailureReason /* reason */) {
  onHttpCallResponse(token, {}, "", {});
  http_request_.erase(token);
}

void AsyncClientHandler::onSuccess(Envoy::Http::MessagePtr&& response) {
  context->onAsyncClientSuccess(token, response);
}

void AsyncClientHandler::onFailure(Http::AsyncClient::FailureReason reason) {
  context->onAsyncClientFailure(token, reason);
}

std::unique_ptr<WasmVm> createWasmVm(absl::string_view wasm_vm) {
  if (wasm_vm == WasmVmNames::get().Wavm) {
    return Wavm::createWavm();
  } else {
    return nullptr;
  }
}

std::shared_ptr<Wasm> createWasm(absl::string_view id,
                                 const envoy::config::wasm::v2::VmConfig& vm_config,
                                 Upstream::ClusterManager& cluster_manager,
                                 Event::Dispatcher& dispatcher, Api::Api& api) {
  auto wasm = std::make_shared<Wasm>(vm_config.vm(), id, vm_config.initial_configuration(),
                                     cluster_manager, dispatcher);
  const auto& code = Config::DataSource::read(vm_config.code(), true, api);
  const auto& path = Config::DataSource::getPath(vm_config.code())
                         .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  if (code.empty()) {
    throw WasmException(fmt::format("Failed to load WASM code from {}", path));
  }
  if (!wasm->initialize(code, id, vm_config.allow_precompiled())) {
    throw WasmException(fmt::format("Failed to initialize WASM code from {}", path));
  }
  wasm->configure(vm_config.initial_configuration());
  return wasm;
}

std::shared_ptr<Wasm> createThreadLocalWasm(Wasm& base_wasm, absl::string_view configuration,
                                            Event::Dispatcher& dispatcher) {
  std::shared_ptr<Wasm> wasm;
  if (base_wasm.wasmVm()->clonable()) {
    wasm = std::make_shared<Wasm>(base_wasm, dispatcher);
  } else {
    wasm = std::make_shared<Wasm>(base_wasm.wasmVm()->vm(), base_wasm.id(),
                                  base_wasm.initial_configuration(), base_wasm.clusterManager(),
                                  dispatcher);
    if (!wasm->initialize(base_wasm.code(), base_wasm.id(), base_wasm.allow_precompiled())) {
      throw WasmException("Failed to initialize WASM code");
    }
    wasm->configure(base_wasm.initial_configuration());
  }
  wasm->configure(configuration);
  wasm->start();
  if (!wasm->id().empty())
    local_wasms[wasm->id()] = wasm;
  return wasm;
}

std::shared_ptr<Wasm> getThreadLocalWasm(absl::string_view id, absl::string_view configuration) {
  auto it = local_wasms.find(id);
  if (it == local_wasms.end()) {
    throw WasmException(fmt::format("Failed to find WASM id {}", id));
  }
  auto wasm = it->second;
  wasm->configure(configuration);
  return wasm;
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
