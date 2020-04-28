#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/extensions/wasm/v3/wasm.pb.validate.h"
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
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/common/wasm/well_known_names.h"
#include "extensions/filters/common/expr/context.h"

#include "absl/base/casts.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "eval/eval/field_access.h"
#include "eval/eval/field_backed_list_impl.h"
#include "eval/eval/field_backed_map_impl.h"
#include "eval/public/cel_value.h"
#include "eval/public/value_export_util.h"
#include "openssl/bytestring.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

namespace {

using HashPolicy = envoy::config::route::v3::RouteAction::HashPolicy;

class SharedData {
public:
  WasmResult get(absl::string_view vm_id, const absl::string_view key,
                 std::pair<std::string, uint32_t>* result) {
    absl::ReaderMutexLock l(&mutex_);
    auto map = data_.find(vm_id);
    if (map == data_.end()) {
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
    absl::WriterMutexLock l(&mutex_);
    absl::flat_hash_map<std::string, std::pair<std::string, uint32_t>>* map;
    auto map_it = data_.find(vm_id);
    if (map_it == data_.end()) {
      map = &data_[vm_id];
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
    absl::WriterMutexLock l(&mutex_);
    auto key = std::make_pair(std::string(vm_id), std::string(queue_name));
    auto it = queue_tokens_.insert(std::make_pair(key, static_cast<uint32_t>(0)));
    if (it.second) {
      it.first->second = nextQueueToken();
      queue_token_set_.insert(it.first->second);
    }
    uint32_t token = it.first->second;
    auto& q = queues_[token];
    q.vm_id = std::string(vm_id);
    q.context_id = context_id;
    q.dispatcher = &dispatcher;
    // Preserve any existing data.
    return token;
  }

  uint32_t resolveQueue(absl::string_view vm_id, absl::string_view queue_name) {
    absl::WriterMutexLock l(&mutex_);
    auto key = std::make_pair(std::string(vm_id), std::string(queue_name));
    auto it = queue_tokens_.find(key);
    if (it != queue_tokens_.end()) {
      return it->second;
    }
    return 0; // N.B. zero indicates that the queue was not found.
  }

  WasmResult dequeue(uint32_t token, std::string* data) {
    absl::ReaderMutexLock l(&mutex_);
    auto it = queues_.find(token);
    if (it == queues_.end()) {
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
    absl::WriterMutexLock l(&mutex_);
    auto it = queues_.find(token);
    if (it == queues_.end()) {
      return WasmResult::NotFound;
    }
    it->second.queue.emplace_back(value);
    auto vm_id = it->second.vm_id;
    auto context_id = it->second.context_id;
    it->second.dispatcher->post([vm_id, context_id, token] {
      auto wasm = proxy_wasm::getThreadLocalWasm(vm_id);
      if (wasm) {
        wasm->wasm()->queueReady(context_id, token);
      }
    });
    return WasmResult::Ok;
  }

  uint32_t nextCas() {
    auto result = cas_;
    cas_++;
    if (!cas_) { // 0 is not a valid CAS value.
      cas_++;
    }
    return result;
  }

private:
  uint32_t nextQueueToken() {
    while (true) {
      uint32_t token = next_queue_token_++;
      if (token == 0) {
        continue; // 0 is an illegal token.
      }
      if (queue_token_set_.find(token) == queue_token_set_.end()) {
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

  absl::Mutex mutex_;
  uint32_t cas_ = 1;
  uint32_t next_queue_token_ = 1;
  absl::node_hash_map<std::string,
                      absl::flat_hash_map<std::string, std::pair<std::string, uint32_t>>>
      data_;
  absl::node_hash_map<uint32_t, Queue> queues_;
  struct PairHash {
    template <class T1, class T2> std::size_t operator()(const std::pair<T1, T2>& pair) const {
      return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
  };
  absl::flat_hash_map<std::pair<std::string, std::string>, uint32_t, PairHash> queue_tokens_;
  absl::flat_hash_set<uint32_t> queue_token_set_;
};

SharedData global_shared_data;

Http::RequestTrailerMapPtr buildRequestTrailerMapFromPairs(const Pairs& pairs) {
  auto map = std::make_unique<Http::RequestTrailerMapImpl>();
  for (auto& p : pairs) {
    // Note: because of the lack of a string_view interface for addCopy and
    // the lack of an interface to add an entry with an empty value and return
    // the entry, there is no efficient way to prevent either a double copy
    // of the value or a double lookup of the entry.
    map->addCopy(Http::LowerCaseString(std::string(p.first)), std::string(p.second));
  }
  return map;
}

Http::RequestHeaderMapPtr buildRequestHeaderMapFromPairs(const Pairs& pairs) {
  auto map = std::make_unique<Http::RequestHeaderMapImpl>();
  for (auto& p : pairs) {
    // Note: because of the lack of a string_view interface for addCopy and
    // the lack of an interface to add an entry with an empty value and return
    // the entry, there is no efficient way to prevent either a double copy
    // of the value or a double lookup of the entry.
    map->addCopy(Http::LowerCaseString(std::string(p.first)), std::string(p.second));
  }
  return map;
}

template <typename P> static uint32_t headerSize(const P& p) { return p ? p->size() : 0; }

} // namespace

// Test support.

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name) {
  return global_shared_data.resolveQueue(vm_id, queue_name);
}

size_t Buffer::size() const {
  if (buffer_instance_) {
    return buffer_instance_->length();
  }
  return data_.size();
}

WasmResult Buffer::copyTo(WasmBase* wasm, size_t start, size_t length, uint64_t ptr_ptr,
                          uint64_t size_ptr) const {
  if (const_buffer_instance_) {
    uint64_t pointer;
    auto p = wasm->allocMemory(length, &pointer);
    if (!p) {
      return WasmResult::InvalidMemoryAccess;
    }
    const_buffer_instance_->copyOut(start, length, p);
    if (!wasm->wasm_vm()->setWord(ptr_ptr, Word(pointer))) {
      return WasmResult::InvalidMemoryAccess;
    }
    if (!wasm->wasm_vm()->setWord(size_ptr, Word(length))) {
      return WasmResult::InvalidMemoryAccess;
    }
    return WasmResult::Ok;
  }
  absl::string_view s = data_.substr(start, length);
  if (!wasm->copyToPointerSize(s, ptr_ptr, size_ptr)) {
    return WasmResult::InvalidMemoryAccess;
  }
  return WasmResult::Ok;
}

WasmResult Buffer::copyFrom(size_t start, size_t length, absl::string_view data) {
  if (buffer_instance_) {
    if (start == 0) {
      if (length == 0) {
        buffer_instance_->prepend(data);
        return WasmResult::Ok;
      } else if (length >= buffer_instance_->length()) {
        buffer_instance_->drain(buffer_instance_->length());
        buffer_instance_->add(data);
        return WasmResult::Ok;
      } else {
        return WasmResult::BadArgument;
      }
    } else if (start >= buffer_instance_->length()) {
      buffer_instance_->add(data);
      return WasmResult::Ok;
    } else {
      return WasmResult::BadArgument;
    }
  }
  if (const_buffer_instance_) { // This buffer is immutable.
    return WasmResult::BadArgument;
  }
  // Setting a string buffer not supported (no use case).
  return WasmResult::BadArgument;
}

Context::Context() = default;
Context::Context(Wasm* wasm) : ContextBase(wasm) {}

Context::Context(Wasm* wasm, const PluginSharedPtr& plugin) : ContextBase(wasm, plugin) {}

Context::Context(Wasm* wasm, uint32_t root_context_id, const PluginSharedPtr& plugin)
    : ContextBase(wasm, root_context_id, plugin) {}

Wasm* Context::wasm() const { return static_cast<Wasm*>(wasm_); }
Plugin* Context::plugin() const { return static_cast<Plugin*>(plugin_.get()); }
Context* Context::rootContext() const { return static_cast<Context*>(root_context()); }
Upstream::ClusterManager& Context::clusterManager() const { return wasm()->clusterManager(); }

void Context::error(absl::string_view message) { throw WasmException(std::string(message)); }

void Context::initializeRoot(WasmBase* wasm, std::shared_ptr<PluginBase> plugin) {
  ContextBase::initializeRoot(wasm, plugin);
  root_local_info_ = &std::static_pointer_cast<Plugin>(plugin)->local_info_;
}

WasmResult Context::setTickPeriod(std::chrono::milliseconds tick_period) {
  wasm()->setTickPeriod(root_context_id_ ? root_context_id_ : id_, tick_period);
  return WasmResult::Ok;
}

uint64_t Context::getCurrentTimeNanoseconds() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             wasm()->time_source_.systemTime().time_since_epoch())
      .count();
}

void Context::onCloseTCP() {
  if (tcp_connection_closed_) {
    return;
  }
  tcp_connection_closed_ = true;
  onDone();
  onLog();
  onDelete();
}

// Native serializer carrying over bit representation from CEL value to the extension
WasmResult serializeValue(Filters::Common::Expr::CelValue value, std::string* result) {
  using Filters::Common::Expr::CelValue;
  switch (value.type()) {
  case CelValue::Type::kString:
    result->assign(value.StringOrDie().value().data(), value.StringOrDie().value().size());
    return WasmResult::Ok;
  case CelValue::Type::kBytes:
    result->assign(value.BytesOrDie().value().data(), value.BytesOrDie().value().size());
    return WasmResult::Ok;
  case CelValue::Type::kInt64: {
    auto out = value.Int64OrDie();
    result->assign(reinterpret_cast<const char*>(&out), sizeof(int64_t));
    return WasmResult::Ok;
  }
  case CelValue::Type::kUint64: {
    auto out = value.Uint64OrDie();
    result->assign(reinterpret_cast<const char*>(&out), sizeof(uint64_t));
    return WasmResult::Ok;
  }
  case CelValue::Type::kDouble: {
    auto out = value.DoubleOrDie();
    result->assign(reinterpret_cast<const char*>(&out), sizeof(double));
    return WasmResult::Ok;
  }
  case CelValue::Type::kBool: {
    auto out = value.BoolOrDie();
    result->assign(reinterpret_cast<const char*>(&out), sizeof(bool));
    return WasmResult::Ok;
  }
  case CelValue::Type::kDuration: {
    // Warning: loss of precision to nanoseconds
    int64_t out = absl::ToInt64Nanoseconds(value.DurationOrDie());
    result->assign(reinterpret_cast<const char*>(&out), sizeof(int64_t));
    return WasmResult::Ok;
  }
  case CelValue::Type::kTimestamp: {
    // Warning: loss of precision to nanoseconds
    int64_t out = absl::ToUnixNanos(value.TimestampOrDie());
    result->assign(reinterpret_cast<const char*>(&out), sizeof(int64_t));
    return WasmResult::Ok;
  }
  case CelValue::Type::kMessage: {
    auto out = value.MessageOrDie();
    if (out == nullptr) {
      result->clear();
      return WasmResult::Ok;
    }
    if (out->SerializeToString(result)) {
      return WasmResult::Ok;
    }
    return WasmResult::SerializationFailure;
  }
  // Slow path using protobuf value conversion
  case CelValue::Type::kMap: {
    ProtobufWkt::Value out;
    if (!google::api::expr::runtime::ExportAsProtoValue(value, &out).ok()) {
      return WasmResult::SerializationFailure;
    }
    if (!out.struct_value().SerializeToString(result)) {
      return WasmResult::SerializationFailure;
    }
    return WasmResult::Ok;
  }
  case CelValue::Type::kList: {
    ProtobufWkt::Value out;
    if (!google::api::expr::runtime::ExportAsProtoValue(value, &out).ok()) {
      return WasmResult::SerializationFailure;
    }
    if (!out.list_value().SerializeToString(result)) {
      return WasmResult::SerializationFailure;
    }
    return WasmResult::Ok;
  }
  default:
    return WasmResult::SerializationFailure;
  }
  return WasmResult::SerializationFailure;
}

#define PROPERTY_TOKENS(_f)                                                                        \
  _f(METADATA) _f(REQUEST) _f(RESPONSE) _f(CONNECTION) _f(UPSTREAM) _f(NODE) _f(SOURCE)            \
      _f(DESTINATION) _f(LISTENER_DIRECTION) _f(LISTENER_METADATA) _f(CLUSTER_NAME)                \
          _f(CLUSTER_METADATA) _f(ROUTE_NAME) _f(ROUTE_METADATA) _f(PLUGIN_NAME)                   \
              _f(PLUGIN_ROOT_ID) _f(PLUGIN_VM_ID)

static inline std::string downCase(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

#define _DECLARE(_t) _t,
enum class PropertyToken { PROPERTY_TOKENS(_DECLARE) };
#undef _DECLARE

#define _PAIR(_t) {downCase(#_t), PropertyToken::_t},
static absl::flat_hash_map<std::string, PropertyToken> property_tokens = {PROPERTY_TOKENS(_PAIR)};
#undef _PARI

absl::optional<google::api::expr::runtime::CelValue>
Context::findValue(absl::string_view name, Protobuf::Arena* arena, bool last) const {
  using google::api::expr::runtime::CelValue;

  const StreamInfo::StreamInfo* info = getConstRequestStreamInfo();

  // Convert into a dense token to enable a jump table implementation.
  auto part_token = property_tokens.find(name);
  if (part_token == property_tokens.end()) {
    if (info) {
      std::string key;
      absl::StrAppend(&key, WasmStateKeyPrefix, name);
      const WasmState* state;
      if (info->filterState().hasData<WasmState>(key)) {
        state = &info->filterState().getDataReadOnly<WasmState>(key);
      } else if (info->upstreamFilterState() &&
                 info->upstreamFilterState()->hasData<WasmState>(key)) {
        state = &info->upstreamFilterState()->getDataReadOnly<WasmState>(key);
      } else {
        return {};
      }
      if (last) {
        return CelValue::CreateBytes(&state->value());
      }
      return state->exprValue(arena);
    }
    return {};
  }

  switch (part_token->second) {
  case PropertyToken::METADATA:
    if (info) {
      return CelValue::CreateMessage(&info->dynamicMetadata(), arena);
    }
    break;
  case PropertyToken::REQUEST:
    if (info) {
      return CelValue::CreateMap(Protobuf::Arena::Create<Filters::Common::Expr::RequestWrapper>(
          arena, request_headers_ ? request_headers_ : access_log_request_headers_, *info));
    }
    break;
  case PropertyToken::RESPONSE:
    if (info) {
      return CelValue::CreateMap(Protobuf::Arena::Create<Filters::Common::Expr::ResponseWrapper>(
          arena, response_headers_ ? response_headers_ : access_log_response_headers_,
          response_trailers_ ? response_trailers_ : access_log_response_trailers_, *info));
    }
    break;
  case PropertyToken::CONNECTION:
    if (info) {
      return CelValue::CreateMap(
          Protobuf::Arena::Create<Filters::Common::Expr::ConnectionWrapper>(arena, *info));
    }
    break;
  case PropertyToken::UPSTREAM:
    if (info) {
      return CelValue::CreateMap(
          Protobuf::Arena::Create<Filters::Common::Expr::UpstreamWrapper>(arena, *info));
    }
    break;
  case PropertyToken::NODE:
    if (root_local_info_) {
      return CelValue::CreateMessage(&root_local_info_->node(), arena);
    } else if (plugin_) {
      return CelValue::CreateMessage(&plugin()->local_info_.node(), arena);
    }
    break;
  case PropertyToken::SOURCE:
    if (info) {
      return CelValue::CreateMap(
          Protobuf::Arena::Create<Filters::Common::Expr::PeerWrapper>(arena, *info, false));
    }
    break;
  case PropertyToken::DESTINATION:
    if (info) {
      return CelValue::CreateMap(
          Protobuf::Arena::Create<Filters::Common::Expr::PeerWrapper>(arena, *info, true));
    }
    break;
  case PropertyToken::LISTENER_DIRECTION:
    if (plugin_) {
      return CelValue::CreateInt64(plugin()->direction_);
    }
    break;
  case PropertyToken::LISTENER_METADATA:
    if (plugin_) {
      return CelValue::CreateMessage(plugin()->listener_metadata_, arena);
    }
    break;
  case PropertyToken::CLUSTER_NAME:
    if (info && info->upstreamHost()) {
      return CelValue::CreateString(&info->upstreamHost()->cluster().name());
    } else if (info && info->routeEntry()) {
      return CelValue::CreateString(&info->routeEntry()->clusterName());
    } else if (info && info->upstreamClusterInfo().has_value() &&
               info->upstreamClusterInfo().value()) {
      return CelValue::CreateString(&info->upstreamClusterInfo().value()->name());
    }
    break;
  case PropertyToken::CLUSTER_METADATA:
    if (info && info->upstreamHost()) {
      return CelValue::CreateMessage(&info->upstreamHost()->cluster().metadata(), arena);
    }
    break;
  case PropertyToken::ROUTE_NAME:
    if (info) {
      return CelValue::CreateString(&info->getRouteName());
    }
    break;
  case PropertyToken::ROUTE_METADATA:
    if (info && info->routeEntry()) {
      return CelValue::CreateMessage(&info->routeEntry()->metadata(), arena);
    }
    break;
  case PropertyToken::PLUGIN_NAME:
    if (plugin_) {
      return CelValue::CreateStringView(plugin()->name_);
    }
    break;
  case PropertyToken::PLUGIN_ROOT_ID:
    return CelValue::CreateStringView(root_id());
  case PropertyToken::PLUGIN_VM_ID:
    return CelValue::CreateStringView(wasm()->vm_id());
  }
  return {};
}

WasmResult Context::getProperty(absl::string_view path, std::string* result) {
  using google::api::expr::runtime::CelValue;

  bool first = true;
  CelValue value;
  Protobuf::Arena arena;

  size_t start = 0;
  while (true) {
    if (start >= path.size()) {
      break;
    }

    size_t end = path.find('\0', start);
    if (end == absl::string_view::npos) {
      end = start + path.size();
    }
    auto part = path.substr(start, end - start);
    start = end + 1;

    if (first) {
      // top-level ident
      first = false;
      auto top_value = findValue(part, &arena, start >= path.size());
      if (!top_value.has_value()) {
        return WasmResult::NotFound;
      }
      value = top_value.value();
    } else if (value.IsMap()) {
      auto& map = *value.MapOrDie();
      auto field = map[CelValue::CreateStringView(part)];
      if (!field.has_value()) {
        return WasmResult::NotFound;
      }
      value = field.value();
    } else if (value.IsMessage()) {
      auto msg = value.MessageOrDie();
      if (msg == nullptr) {
        return WasmResult::NotFound;
      }
      const Protobuf::Descriptor* desc = msg->GetDescriptor();
      const Protobuf::FieldDescriptor* field_desc = desc->FindFieldByName(std::string(part));
      if (field_desc == nullptr) {
        return WasmResult::NotFound;
      }
      if (field_desc->is_map()) {
        value = CelValue::CreateMap(
            Protobuf::Arena::Create<google::api::expr::runtime::FieldBackedMapImpl>(
                &arena, msg, field_desc, &arena));
      } else if (field_desc->is_repeated()) {
        value = CelValue::CreateList(
            Protobuf::Arena::Create<google::api::expr::runtime::FieldBackedListImpl>(
                &arena, msg, field_desc, &arena));
      } else {
        auto status =
            google::api::expr::runtime::CreateValueFromSingleField(msg, field_desc, &arena, &value);
        if (!status.ok()) {
          return WasmResult::InternalFailure;
        }
      }
    } else {
      return WasmResult::NotFound;
    }
  }

  return serializeValue(value, result);
}

// Shared Data
WasmResult Context::getSharedData(absl::string_view key, std::pair<std::string, uint32_t>* data) {
  return global_shared_data.get(wasm()->vm_id(), key, data);
}

WasmResult Context::setSharedData(absl::string_view key, absl::string_view value, uint32_t cas) {
  return global_shared_data.set(wasm()->vm_id(), key, value, cas);
}

// Shared Queue

uint32_t Context::registerSharedQueue(absl::string_view queue_name) {
  // Get the id of the root context if this is a stream context because onQueueReady is on the
  // root.
  return global_shared_data.registerQueue(
      wasm()->vm_id(), queue_name, isRootContext() ? id_ : root_context_id_, wasm()->dispatcher_);
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
Http::HeaderMap* Context::getMap(WasmHeaderMapType type) {
  switch (type) {
  case WasmHeaderMapType::RequestHeaders:
    return request_headers_;
  case WasmHeaderMapType::RequestTrailers:
    return request_trailers_;
  case WasmHeaderMapType::ResponseHeaders:
    return response_headers_;
  case WasmHeaderMapType::ResponseTrailers:
    return response_trailers_;
  default:
    return nullptr;
  }
}

const Http::HeaderMap* Context::getConstMap(WasmHeaderMapType type) {
  switch (type) {
  case WasmHeaderMapType::RequestHeaders:
    if (access_log_request_headers_) {
      return access_log_request_headers_;
    }
    return request_headers_;
  case WasmHeaderMapType::RequestTrailers:
    if (access_log_request_trailers_) {
      return access_log_request_trailers_;
    }
    return request_trailers_;
  case WasmHeaderMapType::ResponseHeaders:
    if (access_log_response_headers_) {
      return access_log_response_headers_;
    }
    return response_headers_;
  case WasmHeaderMapType::ResponseTrailers:
    if (access_log_response_trailers_) {
      return access_log_response_trailers_;
    }
    return response_trailers_;
  case WasmHeaderMapType::GrpcReceiveInitialMetadata:
    return rootContext()->grpc_receive_initial_metadata_.get();
  case WasmHeaderMapType::GrpcReceiveTrailingMetadata:
    return rootContext()->grpc_receive_trailing_metadata_.get();
  case WasmHeaderMapType::HttpCallResponseHeaders: {
    Envoy::Http::ResponseMessagePtr* response = rootContext()->http_call_response_;
    if (response) {
      return &(*response)->headers();
    }
    return nullptr;
  }
  case WasmHeaderMapType::HttpCallResponseTrailers: {
    Envoy::Http::ResponseMessagePtr* response = rootContext()->http_call_response_;
    if (response) {
      return (*response)->trailers();
    }
    return nullptr;
  }
  }
  return nullptr;
}

void Context::addHeaderMapValue(WasmHeaderMapType type, absl::string_view key,
                                absl::string_view value) {
  auto map = getMap(type);
  if (!map) {
    return;
  }
  const Http::LowerCaseString lower_key{std::string(key)};
  map->addCopy(lower_key, std::string(value));
}

absl::string_view Context::getHeaderMapValue(WasmHeaderMapType type, absl::string_view key) {
  auto map = getConstMap(type);
  if (!map) {
    return "";
  }
  const Http::LowerCaseString lower_key{std::string(key)};
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

Pairs Context::getHeaderMapPairs(WasmHeaderMapType type) {
  return headerMapToPairs(getConstMap(type));
}

void Context::setHeaderMapPairs(WasmHeaderMapType type, const Pairs& pairs) {
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
    const Http::LowerCaseString lower_key{k};
    map->remove(lower_key);
  }
  for (auto& p : pairs) {
    const Http::LowerCaseString lower_key{std::string(p.first)};
    map->addCopy(lower_key, std::string(p.second));
  }
}

void Context::removeHeaderMapValue(WasmHeaderMapType type, absl::string_view key) {
  auto map = getMap(type);
  if (!map) {
    return;
  }
  const Http::LowerCaseString lower_key{std::string(key)};
  map->remove(lower_key);
}

void Context::replaceHeaderMapValue(WasmHeaderMapType type, absl::string_view key,
                                    absl::string_view value) {
  auto map = getMap(type);
  if (!map) {
    return;
  }
  const Http::LowerCaseString lower_key{std::string(key)};
  map->setCopy(lower_key, value);
}

uint32_t Context::getHeaderMapSize(WasmHeaderMapType type) {
  auto map = getMap(type);
  if (!map) {
    return 0;
  }
  return map->byteSize();
}

// Buffer

BufferInterface* Context::getBuffer(WasmBufferType type) {
  switch (type) {
  case WasmBufferType::VmConfiguration:
    return buffer_.set(wasm()->vm_configuration());
  case WasmBufferType::PluginConfiguration:
    if (plugin_) {
      return buffer_.set(plugin_->plugin_configuration_);
    }
    break;
  case WasmBufferType::HttpRequestBody:
    if (buffering_request_body_) {
      // We need the mutable version, so capture it using a callback.
      // TODO: consider adding a mutableDecodingBuffer() interface.
      ::Envoy::Buffer::Instance* buffer_instance{};
      decoder_callbacks_->modifyDecodingBuffer(
          [&buffer_instance](::Envoy::Buffer::Instance& buffer) { buffer_instance = &buffer; });
      return buffer_.set(buffer_instance);
    }
    return buffer_.set(request_body_buffer_);
  case WasmBufferType::HttpResponseBody:
    if (buffering_response_body_) {
      // TODO: consider adding a mutableDecodingBuffer() interface.
      ::Envoy::Buffer::Instance* buffer_instance{};
      encoder_callbacks_->modifyEncodingBuffer(
          [&buffer_instance](::Envoy::Buffer::Instance& buffer) { buffer_instance = &buffer; });
      return buffer_.set(buffer_instance);
    }
    return buffer_.set(response_body_buffer_);
  case WasmBufferType::NetworkDownstreamData:
    return buffer_.set(network_downstream_data_buffer_);
  case WasmBufferType::NetworkUpstreamData:
    return buffer_.set(network_upstream_data_buffer_);
  case WasmBufferType::HttpCallResponseBody: {
    Envoy::Http::ResponseMessagePtr* response = rootContext()->http_call_response_;
    if (response) {
      return buffer_.set((*response)->body().get());
    }
  } break;
  case WasmBufferType::GrpcReceiveBuffer:
    return buffer_.set(rootContext()->grpc_receive_buffer_.get());
  default:
    break;
  }
  return nullptr;
}

void Context::onDownstreamConnectionClose(PeerType peer_type) {
  ContextBase::onDownstreamConnectionClose(peer_type);
  downstream_closed_ = true;
  // Call close on TCP connection, if upstream connection closed or there was a failure seen in this
  // connection.
  if (upstream_closed_ || getRequestStreamInfo()->hasAnyResponseFlag()) {
    onCloseTCP();
  }
}

void Context::onUpstreamConnectionClose(PeerType peer_type) {
  ContextBase::onUpstreamConnectionClose(peer_type);
  upstream_closed_ = true;
  if (downstream_closed_) {
    onCloseTCP();
  }
}

// Async call via HTTP
WasmResult Context::httpCall(absl::string_view cluster, const Pairs& request_headers,
                             absl::string_view request_body, const Pairs& request_trailers,
                             int timeout_milliseconds, uint32_t* token_ptr) {
  if (timeout_milliseconds < 0) {
    return WasmResult::BadArgument;
  }
  auto cluster_string = std::string(cluster);
  if (clusterManager().get(cluster_string) == nullptr) {
    return WasmResult::BadArgument;
  }

  Http::RequestMessagePtr message(
      new Http::RequestMessageImpl(buildRequestHeaderMapFromPairs(request_headers)));

  // Check that we were provided certain headers.
  if (message->headers().Path() == nullptr || message->headers().Method() == nullptr ||
      message->headers().Host() == nullptr) {
    return WasmResult::BadArgument;
  }

  if (!request_body.empty()) {
    message->body() =
        std::make_unique<::Envoy::Buffer::OwnedImpl>(request_body.data(), request_body.size());
    message->headers().setContentLength(request_body.size());
  }

  if (!request_trailers.empty()) {
    message->trailers(buildRequestTrailerMapFromPairs(request_trailers));
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

  // set default hash policy to be based on :authority to enable consistent hash
  Http::AsyncClient::RequestOptions options;
  options.setTimeout(timeout);
  Protobuf::RepeatedPtrField<HashPolicy> hash_policy;
  hash_policy.Add()->mutable_header()->set_header_name(Http::Headers::get().Host.get());
  options.setHashPolicy(hash_policy);
  auto http_request = clusterManager()
                          .httpAsyncClientForCluster(cluster_string)
                          .send(std::move(message), handler, options);
  if (!http_request) {
    http_request_.erase(token);
    return WasmResult::InternalFailure;
  }
  handler.context_ = this;
  handler.token_ = token;
  handler.request_ = http_request;
  *token_ptr = token;
  return WasmResult::Ok;
}

WasmResult Context::grpcCall(absl::string_view grpc_service, absl::string_view service_name,
                             absl::string_view method_name, const Pairs& initial_metadata,
                             absl::string_view request,
                             const absl::optional<std::chrono::milliseconds>& timeout,
                             uint32_t* token_ptr) {
  GrpcService service_proto;
  if (!service_proto.ParseFromArray(grpc_service.data(), grpc_service.size())) {
    return WasmResult::ParseFailure;
  }
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
  handler.context_ = this;
  handler.token_ = token;
  auto grpc_client =
      clusterManager()
          .grpcAsyncClientManager()
          .factoryForGrpcService(service_proto, *wasm()->scope_, true /* skip_cluster_check */)
          ->create();
  grpc_initial_metadata_ = buildRequestHeaderMapFromPairs(initial_metadata);

  // set default hash policy to be based on :authority to enable consistent hash
  Http::AsyncClient::RequestOptions options;
  options.setTimeout(timeout);
  Protobuf::RepeatedPtrField<HashPolicy> hash_policy;
  hash_policy.Add()->mutable_header()->set_header_name(Http::Headers::get().Host.get());
  options.setHashPolicy(hash_policy);

  auto grpc_request = grpc_client->sendRaw(service_name, method_name,
                                           std::make_unique<::Envoy::Buffer::OwnedImpl>(request),
                                           handler, Tracing::NullSpan::instance(), options);
  if (!grpc_request) {
    grpc_call_request_.erase(token);
    return WasmResult::InternalFailure;
  }
  handler.client_ = std::move(grpc_client);
  handler.request_ = grpc_request;
  *token_ptr = token;
  return WasmResult::Ok;
}

WasmResult Context::grpcStream(absl::string_view grpc_service, absl::string_view service_name,
                               absl::string_view method_name, const Pairs& initial_metadata,
                               uint32_t* token_ptr) {
  GrpcService service_proto;
  if (!service_proto.ParseFromArray(grpc_service.data(), grpc_service.size())) {
    return WasmResult::ParseFailure;
  }
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
  handler.context_ = this;
  handler.token_ = token;
  auto grpc_client =
      clusterManager()
          .grpcAsyncClientManager()
          .factoryForGrpcService(service_proto, *wasm()->scope_, true /* skip_cluster_check */)
          ->create();
  grpc_initial_metadata_ = buildRequestHeaderMapFromPairs(initial_metadata);

  // set default hash policy to be based on :authority to enable consistent hash
  Http::AsyncClient::StreamOptions options;
  Protobuf::RepeatedPtrField<HashPolicy> hash_policy;
  hash_policy.Add()->mutable_header()->set_header_name(Http::Headers::get().Host.get());
  options.setHashPolicy(hash_policy);

  auto grpc_stream = grpc_client->startRaw(service_name, method_name, handler, options);
  if (!grpc_stream) {
    grpc_stream_.erase(token);
    return WasmResult::InternalFailure;
  }
  handler.client_ = std::move(grpc_client);
  handler.stream_ = grpc_stream;
  *token_ptr = token;
  return WasmResult::Ok;
}

// NB: this is currently called inline, so the token is known to be that of the currently
// executing grpcCall or grpcStream.
void Context::onGrpcCreateInitialMetadata(uint32_t /* token */,
                                          Http::RequestHeaderMap& initial_metadata) {
  if (grpc_initial_metadata_) {
    initial_metadata = std::move(*grpc_initial_metadata_);
    grpc_initial_metadata_.reset();
  }
}

void Context::httpRespond(const Pairs& response_headers, absl::string_view body,
                          const Pairs& response_trailers) {
  (void)response_headers;
  (void)body;
  (void)response_trailers;
}

// StreamInfo
const StreamInfo::StreamInfo* Context::getConstRequestStreamInfo() const {
  if (encoder_callbacks_) {
    return &encoder_callbacks_->streamInfo();
  } else if (decoder_callbacks_) {
    return &decoder_callbacks_->streamInfo();
  } else if (access_log_stream_info_) {
    return access_log_stream_info_;
  } else if (network_read_filter_callbacks_) {
    return &network_read_filter_callbacks_->connection().streamInfo();
  } else if (network_write_filter_callbacks_) {
    return &network_write_filter_callbacks_->connection().streamInfo();
  }
  return nullptr;
}

StreamInfo::StreamInfo* Context::getRequestStreamInfo() const {
  if (encoder_callbacks_) {
    return &encoder_callbacks_->streamInfo();
  } else if (decoder_callbacks_) {
    return &decoder_callbacks_->streamInfo();
  } else if (network_read_filter_callbacks_) {
    return &network_read_filter_callbacks_->connection().streamInfo();
  } else if (network_write_filter_callbacks_) {
    return &network_write_filter_callbacks_->connection().streamInfo();
  }
  return nullptr;
}

const Network::Connection* Context::getConnection() const {
  if (encoder_callbacks_) {
    return encoder_callbacks_->connection();
  } else if (decoder_callbacks_) {
    return decoder_callbacks_->connection();
  } else if (network_read_filter_callbacks_) {
    return &network_read_filter_callbacks_->connection();
  } else if (network_write_filter_callbacks_) {
    return &network_write_filter_callbacks_->connection();
  }
  return nullptr;
}

WasmResult Context::setProperty(absl::string_view path, absl::string_view value) {
  auto* stream_info = getRequestStreamInfo();
  if (!stream_info) {
    return WasmResult::NotFound;
  }
  std::string key;
  absl::StrAppend(&key, WasmStateKeyPrefix, path);
  WasmState* state;
  if (stream_info->filterState()->hasData<WasmState>(key)) {
    state = &stream_info->filterState()->getDataMutable<WasmState>(key);
  } else {
    const auto& it = rootContext()->state_prototypes_.find(path);
    const WasmStatePrototype& prototype = it == rootContext()->state_prototypes_.end()
                                              ? DefaultWasmStatePrototype::get()
                                              : *it->second.get();
    auto state_ptr = std::make_unique<WasmState>(prototype);
    state = state_ptr.get();
    stream_info->filterState()->setData(key, std::move(state_ptr),
                                        StreamInfo::FilterState::StateType::Mutable,
                                        prototype.life_span_);
  }
  if (!state->setValue(value)) {
    return WasmResult::BadArgument;
  }
  return WasmResult::Ok;
}

WasmResult Context::declareProperty(absl::string_view path,
                                    std::unique_ptr<const WasmStatePrototype> state_prototype) {
  state_prototypes_[path] = std::move(state_prototype);
  return WasmResult::Ok;
}

WasmResult Context::log(uint64_t level, absl::string_view message) {
  switch (static_cast<spdlog::level::level_enum>(level)) {
  case spdlog::level::trace:
    ENVOY_LOG(trace, "wasm log{}: {}", log_prefix(), message);
    return WasmResult::Ok;
  case spdlog::level::debug:
    ENVOY_LOG(debug, "wasm log{}: {}", log_prefix(), message);
    return WasmResult::Ok;
  case spdlog::level::info:
    ENVOY_LOG(info, "wasm log{}: {}", log_prefix(), message);
    return WasmResult::Ok;
  case spdlog::level::warn:
    ENVOY_LOG(warn, "wasm log{}: {}", log_prefix(), message);
    return WasmResult::Ok;
  case spdlog::level::err:
    ENVOY_LOG(error, "wasm log{}: {}", log_prefix(), message);
    return WasmResult::Ok;
  case spdlog::level::critical:
    ENVOY_LOG(critical, "wasm log{}: {}", log_prefix(), message);
    return WasmResult::Ok;
  case spdlog::level::off:
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
}

//
// Calls into the WASM code.
//
bool Context::validateConfiguration(absl::string_view configuration,
                                    const std::shared_ptr<PluginBase>& plugin_base) {
  auto plugin = std::static_pointer_cast<Plugin>(plugin_base);
  if (!wasm()->validate_configuration_) {
    return true;
  }
  configuration_ = configuration;
  plugin_ = plugin_base;
  auto result =
      wasm()
          ->validate_configuration_(this, id_, static_cast<uint32_t>(configuration.size()))
          .u64_ != 0;
  plugin_.reset();
  configuration_ = "";
  return result;
}

absl::string_view Context::getConfiguration() { return configuration_; }

std::pair<uint32_t, absl::string_view> Context::getStatus() {
  return std::make_pair(status_code_, status_message_);
}

void Context::onGrpcReceiveInitialMetadataWrapper(uint32_t token, Http::HeaderMapPtr&& metadata) {
  grpc_receive_initial_metadata_ = std::move(metadata);
  onGrpcReceiveInitialMetadata(token, headerSize(grpc_receive_initial_metadata_));
  grpc_receive_initial_metadata_ = nullptr;
}

void Context::onGrpcReceiveTrailingMetadataWrapper(uint32_t token, Http::HeaderMapPtr&& metadata) {
  grpc_receive_trailing_metadata_ = std::move(metadata);
  onGrpcReceiveTrailingMetadata(token, headerSize(grpc_receive_trailing_metadata_));
  grpc_receive_trailing_metadata_ = nullptr;
}

WasmResult Context::defineMetric(MetricType type, absl::string_view name, uint32_t* metric_id_ptr) {
  // TODO: Consider rethinking the scoping policy as it does not help in this case.
  Stats::StatNameManagedStorage storage(name, wasm()->scope_->symbolTable());
  Stats::StatName stat_name = storage.statName();
  if (type == MetricType::Counter) {
    auto id = wasm()->nextCounterMetricId();
    auto c = &wasm()->scope_->counterFromStatName(stat_name);
    wasm()->counters_.emplace(id, c);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  } else if (type == MetricType::Gauge) {
    auto id = wasm()->nextGaugeMetricId();
    auto g = &wasm()->scope_->gaugeFromStatName(stat_name, Stats::Gauge::ImportMode::Accumulate);
    wasm()->gauges_.emplace(id, g);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  } else if (type == MetricType::Histogram) {
    auto id = wasm()->nextHistogramMetricId();
    auto h = &wasm()->scope_->histogramFromStatName(stat_name, Stats::Histogram::Unit::Unspecified);
    wasm()->histograms_.emplace(id, h);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  }
  return WasmResult::BadArgument;
}

WasmResult Context::incrementMetric(uint32_t metric_id, int64_t offset) {
  auto type = static_cast<MetricType>(metric_id & Wasm::kMetricTypeMask);
  if (type == MetricType::Counter) {
    auto it = wasm()->counters_.find(metric_id);
    if (it != wasm()->counters_.end()) {
      if (offset > 0) {
        it->second->add(offset);
        return WasmResult::Ok;
      } else {
        return WasmResult::BadArgument;
      }
      return WasmResult::NotFound;
    }
  } else if (type == MetricType::Gauge) {
    auto it = wasm()->gauges_.find(metric_id);
    if (it != wasm()->gauges_.end()) {
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
    auto it = wasm()->counters_.find(metric_id);
    if (it != wasm()->counters_.end()) {
      it->second->add(value);
      return WasmResult::Ok;
    }
  } else if (type == MetricType::Gauge) {
    auto it = wasm()->gauges_.find(metric_id);
    if (it != wasm()->gauges_.end()) {
      it->second->set(value);
      return WasmResult::Ok;
    }
  } else if (type == MetricType::Histogram) {
    auto it = wasm()->histograms_.find(metric_id);
    if (it != wasm()->histograms_.end()) {
      it->second->recordValue(value);
      return WasmResult::Ok;
    }
  }
  return WasmResult::NotFound;
}

WasmResult Context::getMetric(uint32_t metric_id, uint64_t* result_uint64_ptr) {
  auto type = static_cast<MetricType>(metric_id & Wasm::kMetricTypeMask);
  if (type == MetricType::Counter) {
    auto it = wasm()->counters_.find(metric_id);
    if (it != wasm()->counters_.end()) {
      *result_uint64_ptr = it->second->value();
      return WasmResult::Ok;
    }
    return WasmResult::NotFound;
  } else if (type == MetricType::Gauge) {
    auto it = wasm()->gauges_.find(metric_id);
    if (it != wasm()->gauges_.end()) {
      *result_uint64_ptr = it->second->value();
      return WasmResult::Ok;
    }
    return WasmResult::NotFound;
  }
  return WasmResult::BadArgument;
}

Context::~Context() {
  // Cancel any outstanding requests.
  for (auto& p : http_request_) {
    p.second.request_->cancel();
  }
  for (auto& p : grpc_call_request_) {
    p.second.request_->cancel();
  }
  for (auto& p : grpc_stream_) {
    p.second.stream_->resetStream();
  }
}

Network::FilterStatus convertNetworkFilterStatus(proxy_wasm::FilterStatus status) {
  switch (status) {
  case proxy_wasm::FilterStatus::Continue:
    return Network::FilterStatus::Continue;
  case proxy_wasm::FilterStatus::StopIteration:
    return Network::FilterStatus::StopIteration;
  }
};

Http::FilterHeadersStatus convertFilterHeadersStatus(proxy_wasm::FilterHeadersStatus status) {
  switch (status) {
  case proxy_wasm::FilterHeadersStatus::Continue:
    return Http::FilterHeadersStatus::Continue;
  case proxy_wasm::FilterHeadersStatus::StopIteration:
    return Http::FilterHeadersStatus::StopIteration;
  }
};

Http::FilterTrailersStatus convertFilterTrailersStatus(proxy_wasm::FilterTrailersStatus status) {
  switch (status) {
  case proxy_wasm::FilterTrailersStatus::Continue:
    return Http::FilterTrailersStatus::Continue;
  case proxy_wasm::FilterTrailersStatus::StopIteration:
    return Http::FilterTrailersStatus::StopIteration;
  }
};

Http::FilterMetadataStatus convertFilterMetadataStatus(proxy_wasm::FilterMetadataStatus status) {
  switch (status) {
  case proxy_wasm::FilterMetadataStatus::Continue:
    return Http::FilterMetadataStatus::Continue;
  }
};

Http::FilterDataStatus convertFilterDataStatus(proxy_wasm::FilterDataStatus status) {
  switch (status) {
  case proxy_wasm::FilterDataStatus::Continue:
    return Http::FilterDataStatus::Continue;
  case proxy_wasm::FilterDataStatus::StopIterationAndBuffer:
    return Http::FilterDataStatus::StopIterationAndBuffer;
  case proxy_wasm::FilterDataStatus::StopIterationAndWatermark:
    return Http::FilterDataStatus::StopIterationAndWatermark;
  case proxy_wasm::FilterDataStatus::StopIterationNoBuffer:
    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
};

Network::FilterStatus Context::onNewConnection() {
  return convertNetworkFilterStatus(onNetworkNewConnection());
};

Network::FilterStatus Context::onData(::Envoy::Buffer::Instance& data, bool end_stream) {
  network_downstream_data_buffer_ = &data;
  auto result = convertNetworkFilterStatus(onDownstreamData(data.length(), end_stream));
  if (result == Network::FilterStatus::Continue) {
    network_downstream_data_buffer_ = nullptr;
  }
  return result;
}

Network::FilterStatus Context::onWrite(::Envoy::Buffer::Instance& data, bool end_stream) {
  network_upstream_data_buffer_ = &data;
  auto result = convertNetworkFilterStatus(onUpstreamData(data.length(), end_stream));
  if (result == Network::FilterStatus::Continue) {
    network_upstream_data_buffer_ = nullptr;
  }
  if (end_stream) {
    // This is called when seeing end_stream=true and not on an upstream connection event,
    // because registering for latter requires replicating the whole TCP proxy extension.
    onUpstreamConnectionClose(PeerType::Unknown);
  }
  return result;
}

void Context::onEvent(Network::ConnectionEvent event) {
  switch (event) {
  case Network::ConnectionEvent::LocalClose:
    onDownstreamConnectionClose(PeerType::Local);
    break;
  case Network::ConnectionEvent::RemoteClose:
    onDownstreamConnectionClose(PeerType::Remote);
    break;
  default:
    break;
  }
}

void Context::initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) {
  network_read_filter_callbacks_ = &callbacks;
  network_read_filter_callbacks_->connection().addConnectionCallbacks(*this);
}

void Context::initializeWriteFilterCallbacks(Network::WriteFilterCallbacks& callbacks) {
  network_write_filter_callbacks_ = &callbacks;
}

void Context::log(const Http::RequestHeaderMap* request_headers,
                  const Http::ResponseHeaderMap* response_headers,
                  const Http::ResponseTrailerMap* response_trailers,
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

void Context::continueRequest() {
  if (decoder_callbacks_) {
    decoder_callbacks_->continueDecoding();
  }
  request_headers_ = nullptr;
  request_body_buffer_ = nullptr;
  request_trailers_ = nullptr;
  request_metadata_ = nullptr;
}

void Context::continueResponse() {
  if (encoder_callbacks_) {
    encoder_callbacks_->continueEncoding();
  }
  response_headers_ = nullptr;
  response_body_buffer_ = nullptr;
  response_trailers_ = nullptr;
  response_metadata_ = nullptr;
}

void Context::sendLocalResponse(uint64_t response_code, absl::string_view body_text,
                                Pairs additional_headers, uint64_t grpc_status,
                                absl::string_view details) {

  auto modify_headers = [additional_headers](Http::HeaderMap& headers) {
    for (auto& p : additional_headers) {
      const Http::LowerCaseString lower_key{std::string(p.first)};
      headers.addCopy(lower_key, std::string(p.second));
    }
  };
  if (decoder_callbacks_) {
    decoder_callbacks_->sendLocalReply(static_cast<Envoy::Http::Code>(response_code), body_text,
                                       modify_headers, grpc_status, details);
  }
}

Http::FilterHeadersStatus Context::decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) {
  request_headers_ = &headers;
  end_of_stream_ = end_stream;
  auto result = convertFilterHeadersStatus(onRequestHeaders(headerSize(&headers)));
  if (result == Http::FilterHeadersStatus::Continue) {
    request_headers_ = nullptr;
  }
  return result;
}

Http::FilterDataStatus Context::decodeData(::Envoy::Buffer::Instance& data, bool end_stream) {
  request_body_buffer_ = &data;
  end_of_stream_ = end_stream;
  auto result = convertFilterDataStatus(onRequestBody(data.length(), end_stream));
  buffering_request_body_ = false;
  switch (result) {
  case Http::FilterDataStatus::Continue:
    request_body_buffer_ = nullptr;
    break;
  case Http::FilterDataStatus::StopIterationAndBuffer:
    buffering_request_body_ = true;
    break;
  case Http::FilterDataStatus::StopIterationAndWatermark:
  case Http::FilterDataStatus::StopIterationNoBuffer:
    break;
  }
  return result;
}

Http::FilterTrailersStatus Context::decodeTrailers(Http::RequestTrailerMap& trailers) {
  request_trailers_ = &trailers;
  auto result = convertFilterTrailersStatus(onRequestTrailers(headerSize(&trailers)));
  if (result == Http::FilterTrailersStatus::Continue) {
    request_trailers_ = nullptr;
  }
  return result;
}

Http::FilterMetadataStatus Context::decodeMetadata(Http::MetadataMap& request_metadata) {
  request_metadata_ = &request_metadata;
  auto result = convertFilterMetadataStatus(onRequestMetadata(headerSize(&request_metadata)));
  if (result == Http::FilterMetadataStatus::Continue) {
    request_metadata_ = nullptr;
  }
  return result;
}

void Context::setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) {
  decoder_callbacks_ = &callbacks;
}

Http::FilterHeadersStatus Context::encode100ContinueHeaders(Http::ResponseHeaderMap&) {
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterHeadersStatus Context::encodeHeaders(Http::ResponseHeaderMap& headers,
                                                 bool end_stream) {
  response_headers_ = &headers;
  end_of_stream_ = end_stream;
  auto result = convertFilterHeadersStatus(onResponseHeaders(headerSize(&headers)));
  if (result == Http::FilterHeadersStatus::Continue) {
    response_headers_ = nullptr;
  }
  return result;
}

Http::FilterDataStatus Context::encodeData(::Envoy::Buffer::Instance& data, bool end_stream) {
  response_body_buffer_ = &data;
  end_of_stream_ = end_stream;
  auto result = convertFilterDataStatus(onResponseBody(data.length(), end_stream));
  buffering_response_body_ = false;
  switch (result) {
  case Http::FilterDataStatus::Continue:
    request_body_buffer_ = nullptr;
    break;
  case Http::FilterDataStatus::StopIterationAndBuffer:
    buffering_response_body_ = true;
    break;
  case Http::FilterDataStatus::StopIterationAndWatermark:
  case Http::FilterDataStatus::StopIterationNoBuffer:
    break;
  }
  return result;
}

Http::FilterTrailersStatus Context::encodeTrailers(Http::ResponseTrailerMap& trailers) {
  response_trailers_ = &trailers;
  auto result = convertFilterTrailersStatus(onResponseTrailers(headerSize(&trailers)));
  if (result == Http::FilterTrailersStatus::Continue) {
    response_trailers_ = nullptr;
  }
  return result;
}

Http::FilterMetadataStatus Context::encodeMetadata(Http::MetadataMap& response_metadata) {
  response_metadata_ = &response_metadata;
  auto result = convertFilterMetadataStatus(onResponseMetadata(headerSize(&response_metadata)));
  if (result == Http::FilterMetadataStatus::Continue) {
    response_metadata_ = nullptr;
  }
  return result;
}

//  Http::FilterMetadataStatus::Continue;

void Context::setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

void Context::onHttpCallSuccess(uint32_t token, Envoy::Http::ResponseMessagePtr&& response) {
  // TODO: convert this into a function in proxy-wasm-cpp-host and use here.
  if (proxy_wasm::current_context_ != nullptr) {
    // We are in a reentrant call, so defer.
    wasm()->addAfterVmCallAction([this, token, response = response.release()] {
      onHttpCallSuccess(token, std::unique_ptr<Envoy::Http::ResponseMessage>(response));
    });
    return;
  }
  http_call_response_ = &response;
  uint32_t body_size = response->body() ? response->body()->length() : 0;
  onHttpCallResponse(token, response->headers().size(), body_size,
                     headerSize(response->trailers()));
  http_call_response_ = nullptr;
  http_request_.erase(token);
}

void Context::onHttpCallFailure(uint32_t token, Http::AsyncClient::FailureReason reason) {
  if (proxy_wasm::current_context_ != nullptr) {
    // We are in a reentrant call, so defer.
    wasm()->addAfterVmCallAction([this, token, reason] { onHttpCallFailure(token, reason); });
    return;
  }
  status_code_ = static_cast<uint32_t>(WasmResult::BrokenConnection);
  // This is the only value currently.
  ASSERT(reason == Http::AsyncClient::FailureReason::Reset);
  status_message_ = "reset";
  onHttpCallResponse(token, 0, 0, 0);
  status_message_ = "";
  http_request_.erase(token);
}

void Context::onGrpcReceiveWrapper(uint32_t token, ::Envoy::Buffer::InstancePtr response) {
  ASSERT(proxy_wasm::current_context_ == nullptr); // Non-reentrant.
  if (wasm()->on_grpc_receive_) {
    grpc_receive_buffer_ = std::move(response);
    uint32_t response_size = grpc_receive_buffer_->length();
    ContextBase::onGrpcReceive(token, response_size);
    grpc_receive_buffer_.reset();
  }
  if (IsGrpcCallToken(token)) {
    grpc_call_request_.erase(token);
  }
}

void Context::onGrpcCloseWrapper(uint32_t token, const Grpc::Status::GrpcStatus& status,
                                 const absl::string_view message) {
  if (proxy_wasm::current_context_ != nullptr) {
    // We are in a reentrant call, so defer.
    wasm()->addAfterVmCallAction([this, token, status, message = std::string(message)] {
      onGrpcCloseWrapper(token, status, message);
    });
    return;
  }
  if (wasm()->on_grpc_close_) {
    status_code_ = static_cast<uint32_t>(status);
    status_message_ = message;
    onGrpcClose(token, status_code_);
    status_message_ = "";
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
  if (it != grpc_stream_.end() && it->second.stream_) {
    it->second.stream_->sendMessageRaw(::Envoy::Buffer::InstancePtr(new ::Envoy::Buffer::OwnedImpl(
                                           message.data(), message.size())),
                                       end_stream);
  }
  return WasmResult::Ok;
}

WasmResult Context::grpcClose(uint32_t token) {
  if (IsGrpcCallToken(token)) {
    auto it = grpc_call_request_.find(token);
    if (it == grpc_call_request_.end()) {
      return WasmResult::NotFound;
    }
    if (it != grpc_call_request_.end() && it->second.request_) {
      it->second.request_->cancel();
    }
    grpc_call_request_.erase(token);
  } else {
    auto it = grpc_stream_.find(token);
    if (it == grpc_stream_.end()) {
      return WasmResult::NotFound;
    }
    if (it != grpc_stream_.end() && it->second.stream_) {
      it->second.stream_->closeStream();
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
    if (it != grpc_call_request_.end() && it->second.request_) {
      it->second.request_->cancel();
    }
    grpc_call_request_.erase(token);
  } else {
    auto it = grpc_stream_.find(token);
    if (it == grpc_stream_.end()) {
      return WasmResult::NotFound;
    }
    if (it != grpc_stream_.end() && it->second.stream_) {
      it->second.stream_->resetStream();
    }
    grpc_stream_.erase(token);
  }
  return WasmResult::Ok;
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
