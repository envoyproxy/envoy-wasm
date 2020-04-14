#include <stdio.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/config/wasm/v3/wasm.pb.validate.h"
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
#include "extensions/common/wasm/wasm_state.h"
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

class DeferAfterCallActions {
public:
  DeferAfterCallActions(Context* context) : wasm_(context->wasm()) {}
  ~DeferAfterCallActions() { wasm_->doAfterVmCallActions(); }

private:
  Wasm* const wasm_;
};

using HashPolicy = envoy::config::route::v3::RouteAction::HashPolicy;

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
      auto wasm = getThreadLocalWasmPtr(vm_id);
      if (wasm) {
        wasm->wasm()->queueReady(context_id, token);
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

Http::RequestTrailerMapPtr buildRequestTrailerMapFromPairs(const Pairs& pairs) {
  auto map = std::make_unique<Http::RequestTrailerMapImpl>();
  for (auto& p : pairs) {
    // Note: because of the lack of a string_view interface for addCopy and
    // the lack of an interface to add an entry with an empty value and return
    // the entry, there is no efficient way to prevent either a double copy
    // of the valueor a double lookup of the entry.
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
    // of the valueor a double lookup of the entry.
    map->addCopy(Http::LowerCaseString(std::string(p.first)), std::string(p.second));
  }
  return map;
}

} // namespace

// Test support.

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name) {
  return global_shared_data.resolveQueue(vm_id, queue_name);
}

Context::Context() : root_context_(this) {}

Context::Context(Wasm* wasm) : wasm_(wasm), root_context_(this) { wasm_->contexts_[id_] = this; }

Context::Context(Wasm* wasm, PluginSharedPtr plugin) { initializeRoot(wasm, plugin); }

Context::Context(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin)
    : wasm_(wasm), id_(wasm->allocContextId()), root_context_id_(root_context_id), plugin_(plugin) {
  wasm_->contexts_[id_] = this;
  root_context_ = wasm_->contexts_[root_context_id_];
}

void Context::initializeRoot(Wasm* wasm, PluginSharedPtr plugin) {
  wasm_ = wasm;
  id_ = wasm->allocContextId();
  root_id_ = plugin->root_id_, root_log_prefix_ = makeRootLogPrefix(plugin->vm_id_);
  root_local_info_ = &plugin->local_info_;
  root_context_ = this;
  wasm_->contexts_[id_] = this;
}

std::string Context::makeRootLogPrefix(absl::string_view vm_id) const {
  std::string prefix;
  if (!root_id_.empty()) {
    prefix = prefix + " " + std::string(root_id_);
  }
  if (vm_id.empty()) {
    prefix = prefix + " " + std::string(vm_id);
  }
  return prefix;
}

WasmVm* Context::wasmVm() const { return wasm_->wasm_vm(); }
Upstream::ClusterManager& Context::clusterManager() const { return wasm_->clusterManager(); }

void Context::addAfterVmCallAction(std::function<void()> f) { wasm_->addAfterVmCallAction(f); }

WasmResult Context::setTickPeriod(std::chrono::milliseconds tick_period) {
  wasm_->setTickPeriod(root_context_id_ ? root_context_id_ : id_, tick_period);
  return WasmResult::Ok;
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

uint64_t Context::getCurrentTimeNanoseconds() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             wasm_->time_source_.systemTime().time_since_epoch())
      .count();
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
    // Warning: loss of precision to nano-seconds
    int64_t out = absl::ToInt64Nanoseconds(value.DurationOrDie());
    result->assign(reinterpret_cast<const char*>(&out), sizeof(int64_t));
    return WasmResult::Ok;
  }
  case CelValue::Type::kTimestamp: {
    // Warning: loss of precision to nano-seconds
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

// An expression wrapper for the WASM state
class WasmStateWrapper : public google::api::expr::runtime::CelMap {
public:
  WasmStateWrapper(const StreamInfo::FilterState& filter_state,
                   const StreamInfo::FilterState* upstream_connection_filter_state)
      : filter_state_(filter_state),
        upstream_connection_filter_state_(upstream_connection_filter_state) {}
  absl::optional<google::api::expr::runtime::CelValue>
  operator[](google::api::expr::runtime::CelValue key) const override {
    if (!key.IsString()) {
      return {};
    }
    auto value = key.StringOrDie().value();
    if (filter_state_.hasData<WasmState>(value)) {
      const WasmState& result = filter_state_.getDataReadOnly<WasmState>(value);
      return google::api::expr::runtime::CelValue::CreateBytes(&result.value());
    }

    if (upstream_connection_filter_state_ &&
        upstream_connection_filter_state_->hasData<WasmState>(value)) {
      const WasmState& result =
          upstream_connection_filter_state_->getDataReadOnly<WasmState>(value);
      return google::api::expr::runtime::CelValue::CreateBytes(&result.value());
    }
    return {};
  }

  int size() const override { NOT_IMPLEMENTED_GCOVR_EXCL_LINE; }
  bool empty() const override { NOT_IMPLEMENTED_GCOVR_EXCL_LINE; }
  const google::api::expr::runtime::CelList* ListKeys() const override {
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }

private:
  const StreamInfo::FilterState& filter_state_;
  const StreamInfo::FilterState* upstream_connection_filter_state_;
};

#define PROPERTY_TOKENS(_f)                                                                        \
  _f(METADATA) _f(FILTER_STATE) _f(REQUEST) _f(RESPONSE) _f(CONNECTION) _f(UPSTREAM) _f(NODE)      \
      _f(SOURCE) _f(DESTINATION) _f(LISTENER_DIRECTION) _f(LISTENER_METADATA) _f(CLUSTER_NAME)     \
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
Context::FindValue(absl::string_view name, Protobuf::Arena* arena) const {
  using google::api::expr::runtime::CelValue;

  // Convert into a dense token to enable a jump table implementation.
  auto part_token = property_tokens.find(name);
  if (part_token == property_tokens.end()) {
    return {};
  }

  const StreamInfo::StreamInfo* info = getConstRequestStreamInfo();
  switch (part_token->second) {
  case PropertyToken::METADATA:
    if (info) {
      return CelValue::CreateMessage(&info->dynamicMetadata(), arena);
    }
    break;
  case PropertyToken::FILTER_STATE:
    if (info) {
      return CelValue::CreateMap(Protobuf::Arena::Create<WasmStateWrapper>(
          arena, info->filterState(), info->upstreamFilterState().get()));
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
      return CelValue::CreateMessage(&plugin_->local_info_.node(), arena);
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
      return CelValue::CreateInt64(plugin_->direction_);
    }
    break;
  case PropertyToken::LISTENER_METADATA:
    if (plugin_) {
      return CelValue::CreateMessage(plugin_->listener_metadata_, arena);
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
      return CelValue::CreateStringView(plugin_->name_);
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
      auto top_value = FindValue(part, &arena);
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
  return global_shared_data.get(wasm_->vm_id(), key, data);
}

WasmResult Context::setSharedData(absl::string_view key, absl::string_view value, uint32_t cas) {
  return global_shared_data.set(wasm_->vm_id(), key, value, cas);
}

// Shared Queue

uint32_t Context::registerSharedQueue(absl::string_view queue_name) {
  // Get the id of the root context if this is a stream context because onQueueReady is on the
  // root.
  return global_shared_data.registerQueue(
      wasm_->vm_id(), queue_name, isRootContext() ? id_ : root_context_id_, wasm_->dispatcher_);
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
  case HeaderMapType::GrpcReceiveInitialMetadata:
    return rootContext()->grpc_receive_initial_metadata_.get();
  case HeaderMapType::GrpcReceiveTrailingMetadata:
    return rootContext()->grpc_receive_trailing_metadata_.get();
  case HeaderMapType::HttpCallResponseHeaders: {
    Envoy::Http::ResponseMessagePtr* response = rootContext()->http_call_response_;
    if (response) {
      return &(*response)->headers();
    }
    return nullptr;
  }
  case HeaderMapType::HttpCallResponseTrailers: {
    Envoy::Http::ResponseMessagePtr* response = rootContext()->http_call_response_;
    if (response) {
      return (*response)->trailers();
    }
    return nullptr;
  }
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
  if (!map) {
    return;
  }
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  map->setCopy(lower_key, value);
}

uint32_t Context::getHeaderMapSize(HeaderMapType type) {
  auto map = getMap(type);
  if (!map) {
    return 0;
  }
  return map->byteSize();
}

// Buffer

const Buffer::Instance* Context::getBuffer(BufferType type) {
  switch (type) {
  case BufferType::HttpRequestBody:
    if (buffering_request_body_) {
      return decoder_callbacks_->decodingBuffer();
    }
    return request_body_buffer_;
  case BufferType::HttpResponseBody:
    if (buffering_response_body_) {
      return encoder_callbacks_->encodingBuffer();
    }
    return response_body_buffer_;
  case BufferType::NetworkDownstreamData:
    return network_downstream_data_buffer_;
  case BufferType::NetworkUpstreamData:
    return network_upstream_data_buffer_;
  case BufferType::HttpCallResponseBody: {
    Envoy::Http::ResponseMessagePtr* response = rootContext()->http_call_response_;
    if (response) {
      return (*response)->body().get();
    }
  } break;
  case BufferType::GrpcReceiveBuffer:
    return rootContext()->grpc_receive_buffer_.get();
  default:
    break;
  }
  return nullptr;
}

WasmResult Context::setBuffer(BufferType type, std::function<void(Buffer::Instance&)> callback) {
  switch (type) {
  case BufferType::HttpRequestBody:
    if (buffering_request_body_) {
      decoder_callbacks_->modifyDecodingBuffer(callback);
    } else {
      callback(*request_body_buffer_);
    }
    return WasmResult::Ok;
  case BufferType::HttpResponseBody:
    if (buffering_response_body_) {
      encoder_callbacks_->modifyEncodingBuffer(callback);
    } else {
      callback(*response_body_buffer_);
    }
    return WasmResult::Ok;
  case BufferType::NetworkDownstreamData:
    callback(*network_downstream_data_buffer_);
    return WasmResult::Ok;
  case BufferType::NetworkUpstreamData:
    callback(*network_upstream_data_buffer_);
    return WasmResult::Ok;
  default:
    return WasmResult::BadArgument;
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
    message->body().reset(new Buffer::OwnedImpl(request_body.data(), request_body.size()));
    message->headers().setContentLength(request_body.size());
  }

  if (request_trailers.size() > 0) {
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

WasmResult Context::grpcCall(const GrpcService& grpc_service, absl::string_view service_name,
                             absl::string_view method_name, const Pairs& initial_metadata,
                             absl::string_view request,
                             const absl::optional<std::chrono::milliseconds>& timeout,
                             uint32_t* token_ptr) {
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
          .factoryForGrpcService(grpc_service, *wasm()->scope_, true /* skip_cluster_check */)
          ->create();
  grpc_initial_metadata_ = buildRequestHeaderMapFromPairs(initial_metadata);

  // set default hash policy to be based on :authority to enable consistent hash
  Http::AsyncClient::RequestOptions options;
  options.setTimeout(timeout);
  Protobuf::RepeatedPtrField<HashPolicy> hash_policy;
  hash_policy.Add()->mutable_header()->set_header_name(Http::Headers::get().Host.get());
  options.setHashPolicy(hash_policy);

  auto grpc_request =
      grpc_client->sendRaw(service_name, method_name, std::make_unique<Buffer::OwnedImpl>(request),
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

WasmResult Context::grpcStream(const GrpcService& grpc_service, absl::string_view service_name,
                               absl::string_view method_name, const Pairs& initial_metadata,
                               uint32_t* token_ptr) {
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
          .factoryForGrpcService(grpc_service, *wasm()->scope_, true /* skip_cluster_check */)
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
void Context::onCreateInitialMetadata(uint32_t /* token */,
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

WasmResult Context::setProperty(absl::string_view key, absl::string_view serialized_value) {
  auto* stream_info = getRequestStreamInfo();
  if (!stream_info) {
    return WasmResult::NotFound;
  }
  stream_info->filterState()->setData(key, std::make_unique<WasmState>(serialized_value),
                                      StreamInfo::FilterState::StateType::Mutable);
  return WasmResult::Ok;
}

void Context::scriptLog(spdlog::level::level_enum level, absl::string_view message) {
  switch (level) {
  case spdlog::level::trace:
    ENVOY_LOG(trace, "wasm log{}: {}", log_prefix(), message);
    return;
  case spdlog::level::debug:
    ENVOY_LOG(debug, "wasm log{}: {}", log_prefix(), message);
    return;
  case spdlog::level::info:
    ENVOY_LOG(info, "wasm log{}: {}", log_prefix(), message);
    return;
  case spdlog::level::warn:
    ENVOY_LOG(warn, "wasm log{}: {}", log_prefix(), message);
    return;
  case spdlog::level::err:
    ENVOY_LOG(error, "wasm log{}: {}", log_prefix(), message);
    return;
  case spdlog::level::critical:
    ENVOY_LOG(critical, "wasm log{}: {}", log_prefix(), message);
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
bool Context::onStart(absl::string_view vm_configuration, PluginSharedPtr plugin) {
  DeferAfterCallActions actions(this);
  bool result = 0;
  if (wasm_->on_context_create_) {
    plugin_ = plugin;
    wasm_->on_context_create_(this, id_, 0);
    plugin_.reset();
  }
  if (wasm_->on_vm_start_) {
    configuration_ = vm_configuration;
    plugin_ = plugin;
    result =
        wasm_->on_vm_start_(this, id_, static_cast<uint32_t>(vm_configuration.size())).u64_ != 0;
    plugin_.reset();
    configuration_ = "";
  }
  return result;
}

bool Context::validateConfiguration(absl::string_view configuration, PluginSharedPtr plugin) {
  if (!wasm_->validate_configuration_) {
    return true;
  }
  configuration_ = configuration;
  plugin_ = plugin;
  auto result =
      wasm_->validate_configuration_(this, id_, static_cast<uint32_t>(configuration.size())).u64_ !=
      0;
  plugin_.reset();
  configuration_ = "";
  return result;
}

bool Context::onConfigure(absl::string_view plugin_configuration, PluginSharedPtr plugin) {
  if (!wasm_->on_configure_) {
    return true;
  }
  DeferAfterCallActions actions(this);
  configuration_ = plugin_configuration;
  plugin_ = plugin;
  auto result =
      wasm_->on_configure_(this, id_, static_cast<uint32_t>(plugin_configuration.size())).u64_ != 0;
  plugin_.reset();
  configuration_ = "";
  return result;
}

absl::string_view Context::getConfiguration() { return configuration_; }

std::pair<uint32_t, absl::string_view> Context::getStatus() {
  return std::make_pair(status_code_, status_message_);
}

void Context::onTick() {
  if (wasm_->on_tick_) {
    DeferAfterCallActions actions(this);
    wasm_->on_tick_(this, id_);
  }
}

void Context::onCreate(uint32_t parent_context_id) {
  if (wasm_->on_context_create_) {
    DeferAfterCallActions actions(this);
    wasm_->on_context_create_(this, id_, parent_context_id);
  }
}

Network::FilterStatus Context::onNetworkNewConnection() {
  DeferAfterCallActions actions(this);
  onCreate(root_context_id_);
  if (!wasm_->on_new_connection_) {
    return Network::FilterStatus::Continue;
  }
  if (wasm_->on_new_connection_(this, id_).u64_ == 0) {
    return Network::FilterStatus::Continue;
  }
  return Network::FilterStatus::StopIteration;
}

Network::FilterStatus Context::onDownstreamData(int data_length, bool end_of_stream) {
  if (!wasm_->on_downstream_data_) {
    return Network::FilterStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  end_of_stream_ = end_of_stream;
  auto result = wasm_->on_downstream_data_(this, id_, static_cast<uint32_t>(data_length),
                                           static_cast<uint32_t>(end_of_stream));
  // TODO(PiotrSikora): pull Proxy-WASM's FilterStatus values.
  return result.u64_ == 0 ? Network::FilterStatus::Continue : Network::FilterStatus::StopIteration;
}

Network::FilterStatus Context::onUpstreamData(int data_length, bool end_of_stream) {
  if (!wasm_->on_upstream_data_) {
    return Network::FilterStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  end_of_stream_ = end_of_stream;
  auto result = wasm_->on_upstream_data_(this, id_, static_cast<uint32_t>(data_length),
                                         static_cast<uint32_t>(end_of_stream));
  // TODO(PiotrSikora): pull Proxy-WASM's FilterStatus values.
  return result.u64_ == 0 ? Network::FilterStatus::Continue : Network::FilterStatus::StopIteration;
}

void Context::onDownstreamConnectionClose(PeerType peer_type) {
  if (wasm_->on_downstream_connection_close_) {
    DeferAfterCallActions actions(this);
    wasm_->on_downstream_connection_close_(this, id_, static_cast<uint32_t>(peer_type));
  }
  downstream_closed_ = true;
  // Call close on TCP connection, if upstream connection closed or there was a failure seen in this
  // connection.
  if (upstream_closed_ || getRequestStreamInfo()->hasAnyResponseFlag()) {
    onCloseTCP();
  }
}

void Context::onUpstreamConnectionClose(PeerType peer_type) {
  if (wasm_->on_upstream_connection_close_) {
    DeferAfterCallActions actions(this);
    wasm_->on_upstream_connection_close_(this, id_, static_cast<uint32_t>(peer_type));
  }
  upstream_closed_ = true;
  if (downstream_closed_) {
    onCloseTCP();
  }
}

// Empty headers/trailers have zero size.
template <typename P> static uint32_t headerSize(const P& p) { return p ? p->size() : 0; }

Http::FilterHeadersStatus Context::onRequestHeaders() {
  DeferAfterCallActions actions(this);
  onCreate(root_context_id_);
  in_vm_context_created_ = true;
  if (!wasm_->on_request_headers_) {
    return Http::FilterHeadersStatus::Continue;
  }
  if (wasm_->on_request_headers_(this, id_, headerSize(request_headers_)).u64_ == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Context::onRequestBody(bool end_of_stream) {
  if (!wasm_->on_request_body_) {
    return Http::FilterDataStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  const auto buffer = getBuffer(BufferType::HttpRequestBody);
  const auto buffer_length = (buffer == nullptr) ? 0 : buffer->length();
  switch (wasm_
              ->on_request_body_(this, id_, static_cast<uint32_t>(buffer_length),
                                 static_cast<uint32_t>(end_of_stream))
              .u64_) {
  case 0:
    buffering_request_body_ = false;
    return Http::FilterDataStatus::Continue;
  case 1:
    buffering_request_body_ = true;
    return Http::FilterDataStatus::StopIterationAndBuffer;
  case 2:
    buffering_request_body_ = false;
    return Http::FilterDataStatus::StopIterationAndWatermark;
  default:
    buffering_request_body_ = false;
    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
}

Http::FilterTrailersStatus Context::onRequestTrailers() {
  if (!wasm_->on_request_trailers_) {
    return Http::FilterTrailersStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  if (wasm_->on_request_trailers_(this, id_, headerSize(request_trailers_)).u64_ == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return Http::FilterTrailersStatus::StopIteration;
}

Http::FilterMetadataStatus Context::onRequestMetadata() {
  if (!wasm_->on_request_metadata_) {
    return Http::FilterMetadataStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  if (wasm_->on_request_metadata_(this, id_, headerSize(request_metadata_)).u64_ == 0) {
    return Http::FilterMetadataStatus::Continue;
  }
  return Http::FilterMetadataStatus::Continue; // This is currently the only return code.
}

Http::FilterHeadersStatus Context::onResponseHeaders() {
  DeferAfterCallActions actions(this);
  if (!in_vm_context_created_) {
    // If the request is invalid then onRequestHeaders() will not be called and neither will
    // onCreate() then sendLocalReply be called which will call this function. In this case we
    // need to call onCreate() so that the Context inside the VM is created before the
    // onResponseHeaders() call.
    onCreate(root_context_id_);
    in_vm_context_created_ = true;
  }
  if (!wasm_->on_response_headers_) {
    return Http::FilterHeadersStatus::Continue;
  }
  if (wasm_->on_response_headers_(this, id_, headerSize(response_headers_)).u64_ == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Context::onResponseBody(bool end_of_stream) {
  if (!wasm_->on_response_body_) {
    return Http::FilterDataStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  const auto buffer = getBuffer(BufferType::HttpResponseBody);
  const auto buffer_length = (buffer == nullptr) ? 0 : buffer->length();
  switch (wasm_
              ->on_response_body_(this, id_, static_cast<uint32_t>(buffer_length),
                                  static_cast<uint32_t>(end_of_stream))
              .u64_) {
  case 0:
    buffering_response_body_ = false;
    return Http::FilterDataStatus::Continue;
  case 1:
    buffering_response_body_ = true;
    return Http::FilterDataStatus::StopIterationAndBuffer;
  case 2:
    buffering_response_body_ = false;
    return Http::FilterDataStatus::StopIterationAndWatermark;
  default:
    buffering_response_body_ = false;
    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
}

Http::FilterTrailersStatus Context::onResponseTrailers() {
  if (!wasm_->on_response_trailers_) {
    return Http::FilterTrailersStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  if (wasm_->on_response_trailers_(this, id_, headerSize(response_trailers_)).u64_ == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return Http::FilterTrailersStatus::StopIteration;
}

Http::FilterMetadataStatus Context::onResponseMetadata() {
  if (!wasm_->on_response_metadata_) {
    return Http::FilterMetadataStatus::Continue;
  }
  DeferAfterCallActions actions(this);
  if (wasm_->on_response_metadata_(this, id_, headerSize(response_metadata_)).u64_ == 0) {
    return Http::FilterMetadataStatus::Continue;
  }
  return Http::FilterMetadataStatus::Continue; // This is currently the only return code.
}

void Context::onHttpCallResponse(uint32_t token, uint32_t headers, uint32_t body_size,
                                 uint32_t trailers) {
  if (!wasm_->on_http_call_response_) {
    return;
  }
  DeferAfterCallActions actions(this);
  wasm_->on_http_call_response_(this, id_, token, headers, body_size, trailers);
}

void Context::onQueueReady(uint32_t token) {
  if (wasm_->on_queue_ready_) {
    DeferAfterCallActions actions(this);
    wasm_->on_queue_ready_(this, id_, token);
  }
}

void Context::onGrpcReceiveInitialMetadata(uint32_t token, Http::HeaderMapPtr&& metadata) {
  if (!wasm_->on_grpc_receive_initial_metadata_) {
    return;
  }
  DeferAfterCallActions actions(this);
  grpc_receive_initial_metadata_ = std::move(metadata);
  wasm_->on_grpc_receive_initial_metadata_(this, id_, token,
                                           headerSize(grpc_receive_initial_metadata_));
  grpc_receive_initial_metadata_ = nullptr;
}

void Context::onGrpcReceiveTrailingMetadata(uint32_t token, Http::HeaderMapPtr&& metadata) {
  if (!wasm_->on_grpc_receive_trailing_metadata_) {
    return;
  }
  DeferAfterCallActions actions(this);
  grpc_receive_trailing_metadata_ = std::move(metadata);
  wasm_->on_grpc_receive_trailing_metadata_(this, id_, token,
                                            headerSize(grpc_receive_trailing_metadata_));
  grpc_receive_trailing_metadata_ = nullptr;
}

WasmResult Context::defineMetric(MetricType type, absl::string_view name, uint32_t* metric_id_ptr) {
  // TODO: Consider rethinking the scoping policy as it does not help in this case.
  Stats::StatNameManagedStorage storage(name, wasm()->scope_->symbolTable());
  Stats::StatName stat_name = storage.statName();
  if (type == MetricType::Counter) {
    auto id = wasm_->nextCounterMetricId();
    auto c = &wasm_->scope_->counterFromStatName(stat_name);
    wasm_->counters_.emplace(id, c);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  } else if (type == MetricType::Gauge) {
    auto id = wasm_->nextGaugeMetricId();
    auto g = &wasm_->scope_->gaugeFromStatName(stat_name, Stats::Gauge::ImportMode::Accumulate);
    wasm_->gauges_.emplace(id, g);
    *metric_id_ptr = id;
    return WasmResult::Ok;
  } else if (type == MetricType::Histogram) {
    auto id = wasm_->nextHistogramMetricId();
    auto h = &wasm_->scope_->histogramFromStatName(stat_name, Stats::Histogram::Unit::Unspecified);
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

std::string Plugin::makeLogPrefix() const {
  std::string prefix;
  if (!name_.empty()) {
    prefix = prefix + " " + name_;
  }
  if (!root_id_.empty()) {
    prefix = prefix + " " + std::string(root_id_);
  }
  if (vm_id_.empty()) {
    prefix = prefix + " " + std::string(vm_id_);
  }
  return prefix;
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
  // Do not remove vm or root contexts which have the same lifetime as wasm_.
  if (root_context_id_) {
    wasm_->contexts_.erase(id_);
  }
}

Network::FilterStatus Context::onNewConnection() { return onNetworkNewConnection(); };

Network::FilterStatus Context::onData(Buffer::Instance& data, bool end_stream) {
  DeferAfterCallActions actions(this);
  network_downstream_data_buffer_ = &data;
  auto result = onDownstreamData(data.length(), end_stream);
  if (result == Network::FilterStatus::Continue) {
    network_downstream_data_buffer_ = nullptr;
  }
  return result;
}

Network::FilterStatus Context::onWrite(Buffer::Instance& data, bool end_stream) {
  DeferAfterCallActions actions(this);
  network_upstream_data_buffer_ = &data;
  auto result = onUpstreamData(data.length(), end_stream);
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
  DeferAfterCallActions actions(this);
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

void Wasm::log(absl::string_view root_id, const Http::RequestHeaderMap* request_headers,
               const Http::ResponseHeaderMap* response_headers,
               const Http::ResponseTrailerMap* response_trailers,
               const StreamInfo::StreamInfo& stream_info) {
  auto context = getRootContext(root_id);
  context->log(request_headers, response_headers, response_trailers, stream_info);
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

bool Context::onDone() {
  DeferAfterCallActions actions(this);
  if (wasm_->on_done_) {
    return wasm_->on_done_(this, id_).u64_ != 0;
  }
  return true;
}

void Context::onLog() {
  DeferAfterCallActions actions(this);
  if (wasm_->on_log_) {
    wasm_->on_log_(this, id_);
  }
}

void Context::onDelete() {
  DeferAfterCallActions actions(this);
  if (wasm_->on_delete_) {
    wasm_->on_delete_(this, id_);
  }
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

Http::FilterHeadersStatus Context::decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) {
  request_headers_ = &headers;
  end_of_stream_ = end_stream;
  auto result = onRequestHeaders();
  if (result == Http::FilterHeadersStatus::Continue) {
    request_headers_ = nullptr;
  }
  return result;
}

Http::FilterDataStatus Context::decodeData(Buffer::Instance& data, bool end_stream) {
  request_body_buffer_ = &data;
  end_of_stream_ = end_stream;
  auto result = onRequestBody(end_stream);
  if (result == Http::FilterDataStatus::Continue) {
    request_body_buffer_ = nullptr;
  }
  return result;
}

Http::FilterTrailersStatus Context::decodeTrailers(Http::RequestTrailerMap& trailers) {
  request_trailers_ = &trailers;
  auto result = onRequestTrailers();
  if (result == Http::FilterTrailersStatus::Continue) {
    request_trailers_ = nullptr;
  }
  return result;
}

Http::FilterMetadataStatus Context::decodeMetadata(Http::MetadataMap& request_metadata) {
  request_metadata_ = &request_metadata;
  auto result = onRequestMetadata();
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
  auto result = onResponseHeaders();
  if (result == Http::FilterHeadersStatus::Continue) {
    response_headers_ = nullptr;
  }
  return result;
}

Http::FilterDataStatus Context::encodeData(Buffer::Instance& data, bool end_stream) {
  response_body_buffer_ = &data;
  end_of_stream_ = end_stream;
  auto result = onResponseBody(end_stream);
  if (result == Http::FilterDataStatus::Continue) {
    response_body_buffer_ = nullptr;
  }
  return result;
}

Http::FilterTrailersStatus Context::encodeTrailers(Http::ResponseTrailerMap& trailers) {
  response_trailers_ = &trailers;
  auto result = onResponseTrailers();
  if (result == Http::FilterTrailersStatus::Continue) {
    response_trailers_ = nullptr;
  }
  return result;
}

Http::FilterMetadataStatus Context::encodeMetadata(Http::MetadataMap& response_metadata) {
  response_metadata_ = &response_metadata;
  auto result = onResponseMetadata();
  if (result == Http::FilterMetadataStatus::Continue) {
    response_metadata_ = nullptr;
  }
  return result;
}

//  Http::FilterMetadataStatus::Continue;

void Context::setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

void Context::onHttpCallSuccess(uint32_t token, Envoy::Http::ResponseMessagePtr& response) {
  http_call_response_ = &response;
  onHttpCallResponse(token, response->headers().size(), response->body()->length(),
                     headerSize(response->trailers()));
  http_call_response_ = nullptr;
  http_request_.erase(token);
}

void Context::onHttpCallFailure(uint32_t token, Http::AsyncClient::FailureReason reason) {
  status_code_ = static_cast<uint32_t>(WasmResult::BrokenConnection);
  // This is the only value currently.
  ASSERT(reason == Http::AsyncClient::FailureReason::Reset);
  status_message_ = "reset";
  onHttpCallResponse(token, 0, 0, 0);
  status_message_ = "";
  http_request_.erase(token);
}

void Context::onGrpcReceive(uint32_t token, Buffer::InstancePtr response) {
  if (wasm_->on_grpc_receive_) {
    DeferAfterCallActions actions(this);
    grpc_receive_buffer_ = std::move(response);
    uint32_t response_size = grpc_receive_buffer_->length();
    wasm_->on_grpc_receive_(this, id_, token, response_size);
    grpc_receive_buffer_.reset();
  }
  if (IsGrpcCallToken(token)) {
    grpc_call_request_.erase(token);
  }
}

void Context::onGrpcClose(uint32_t token, const Grpc::Status::GrpcStatus& status,
                          const absl::string_view message) {
  if (wasm_->on_grpc_close_) {
    DeferAfterCallActions actions(this);
    status_code_ = static_cast<uint32_t>(status);
    status_message_ = message;
    wasm_->on_grpc_close_(this, id_, token, static_cast<uint64_t>(status));
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
    it->second.stream_->sendMessageRaw(
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
