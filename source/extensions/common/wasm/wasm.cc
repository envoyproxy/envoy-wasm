#include "extensions/common/wasm/wasm.h"

#include <stdio.h>

#include <algorithm>
#include <cctype>
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
#include "common/common/base64.h"
#include "common/common/empty_string.h"
#include "common/common/enum_to_int.h"
#include "common/common/logger.h"
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"
#include "common/tracing/http_tracer_impl.h"

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

std::atomic<int64_t> active_wasm_;

std::string base64Sha256(absl::string_view data) {
  std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);
  EVP_MD_CTX* ctx(EVP_MD_CTX_new());
  auto rc = EVP_DigestInit(ctx, EVP_sha256());
  RELEASE_ASSERT(rc == 1, "Failed to init digest context");
  rc = EVP_DigestUpdate(ctx, data.data(), data.size());
  RELEASE_ASSERT(rc == 1, "Failed to update digest");
  rc = EVP_DigestFinal(ctx, digest.data(), nullptr);
  RELEASE_ASSERT(rc == 1, "Failed to finalize digest");
  EVP_MD_CTX_free(ctx);
  return Base64::encode(reinterpret_cast<const char*>(&digest[0]), digest.size());
}

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
thread_local absl::flat_hash_map<std::string, std::weak_ptr<Wasm>> local_wasms;

const std::string INLINE_STRING = "<inline>";

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

// Test support.

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name) {
  return global_shared_data.resolveQueue(vm_id, queue_name);
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
    auto out = value.DurationOrDie();
    result->assign(reinterpret_cast<const char*>(&out), sizeof(absl::Duration));
    return WasmResult::Ok;
  }
  case CelValue::Type::kTimestamp: {
    auto out = value.TimestampOrDie();
    result->assign(reinterpret_cast<const char*>(&out), sizeof(absl::Time));
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
  WasmStateWrapper(const StreamInfo::FilterState& filter_state) : filter_state_(filter_state) {}
  absl::optional<google::api::expr::runtime::CelValue>
  operator[](google::api::expr::runtime::CelValue key) const override {
    if (!key.IsString()) {
      return {};
    }
    auto value = key.StringOrDie().value();
    try {
      const WasmState& result = filter_state_.getDataReadOnly<WasmState>(value);
      return google::api::expr::runtime::CelValue::CreateBytes(&result.value());
    } catch (const EnvoyException& e) {
      return {};
    }
  }
  int size() const override { NOT_IMPLEMENTED_GCOVR_EXCL_LINE; }
  bool empty() const override { NOT_IMPLEMENTED_GCOVR_EXCL_LINE; }
  const google::api::expr::runtime::CelList* ListKeys() const override {
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }

private:
  const StreamInfo::FilterState& filter_state_;
};

#define PROPERTY_TOKENS(_f)                                                                        \
  _f(METADATA) _f(FILTER_STATE) _f(REQUEST) _f(RESPONSE) _f(CONNECTION) _f(UPSTREAM) _f(NODE)      \
      _f(SOURCE) _f(DESTINATION) _f(REQUEST_PROTOCOL) _f(LISTENER_DIRECTION) _f(LISTENER_METADATA) \
          _f(CLUSTER_NAME) _f(CLUSTER_METADATA) _f(ROUTE_NAME) _f(ROUTE_METADATA) _f(PLUGIN_NAME)  \
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

WasmResult Context::getProperty(absl::string_view path, std::string* result) {
  using google::api::expr::runtime::CelValue;
  using google::api::expr::runtime::FieldBackedListImpl;
  using google::api::expr::runtime::FieldBackedMapImpl;

  bool first = true;
  CelValue value;
  Protobuf::Arena arena;
  const StreamInfo::StreamInfo* info = getConstRequestStreamInfo();
  const auto request_headers = request_headers_ ? request_headers_ : access_log_request_headers_;
  const auto response_headers =
      response_headers_ ? response_headers_ : access_log_response_headers_;
  const auto response_trailers =
      response_trailers_ ? response_trailers_ : access_log_response_trailers_;

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

    // top-level ident
    if (first) {
      first = false;
      // Convert into a dense token to enable a jump table implementation.
      auto part_token = property_tokens.find(part);
      if (part_token == property_tokens.end()) {
        return WasmResult::NotFound;
      }
      switch (part_token->second) {
      case PropertyToken::METADATA:
        value = CelValue::CreateMessage(&info->dynamicMetadata(), &arena);
        break;
      case PropertyToken::FILTER_STATE:
        value = CelValue::CreateMap(
            Protobuf::Arena::Create<WasmStateWrapper>(&arena, info->filterState()));
        break;
      case PropertyToken::REQUEST:
        value = CelValue::CreateMap(Protobuf::Arena::Create<Filters::Common::Expr::RequestWrapper>(
            &arena, request_headers, *info));
        break;
      case PropertyToken::RESPONSE:
        value = CelValue::CreateMap(Protobuf::Arena::Create<Filters::Common::Expr::ResponseWrapper>(
            &arena, response_headers, response_trailers, *info));
        break;
      case PropertyToken::CONNECTION:
        value = CelValue::CreateMap(
            Protobuf::Arena::Create<Filters::Common::Expr::ConnectionWrapper>(&arena, *info));
        break;
      case PropertyToken::UPSTREAM:
        value = CelValue::CreateMap(
            Protobuf::Arena::Create<Filters::Common::Expr::UpstreamWrapper>(&arena, *info));
        break;
      case PropertyToken::NODE:
        if (!plugin_) {
          return WasmResult::NotFound;
        }
        value = CelValue::CreateMessage(&plugin_->local_info_.node(), &arena);
        break;
      case PropertyToken::SOURCE:
        value = CelValue::CreateMap(
            Protobuf::Arena::Create<Filters::Common::Expr::PeerWrapper>(&arena, *info, false));
        break;
      case PropertyToken::DESTINATION:
        value = CelValue::CreateMap(
            Protobuf::Arena::Create<Filters::Common::Expr::PeerWrapper>(&arena, *info, true));
        break;
      case PropertyToken::REQUEST_PROTOCOL:
        // TODO(kyessenov) move this upstream to CEL context
        if (info->protocol().has_value()) {
          value =
              CelValue::CreateString(&Http::Utility::getProtocolString(info->protocol().value()));
          break;
        } else {
          return WasmResult::NotFound;
        }
      case PropertyToken::LISTENER_DIRECTION:
        if (!plugin_) {
          return WasmResult::NotFound;
        }
        value = CelValue::CreateInt64(plugin_->direction_);
        break;
      case PropertyToken::LISTENER_METADATA:
        if (!plugin_) {
          return WasmResult::NotFound;
        }
        value = CelValue::CreateMessage(plugin_->listener_metadata_, &arena);
        break;
      case PropertyToken::CLUSTER_NAME:
        value = CelValue::CreateString(&info->upstreamHost()->cluster().name());
        break;
      case PropertyToken::CLUSTER_METADATA:
        value = CelValue::CreateMessage(&info->upstreamHost()->cluster().metadata(), &arena);
        break;
      case PropertyToken::ROUTE_NAME:
        value = CelValue::CreateString(&info->getRouteName());
        break;
      case PropertyToken::ROUTE_METADATA:
        value = CelValue::CreateMessage(&info->routeEntry()->metadata(), &arena);
        break;
      case PropertyToken::PLUGIN_NAME:
        if (!plugin_) {
          return WasmResult::NotFound;
        }
        value = CelValue::CreateString(plugin_->name_);
        break;
      case PropertyToken::PLUGIN_ROOT_ID:
        value = CelValue::CreateString(root_id());
        break;
      case PropertyToken::PLUGIN_VM_ID:
        value = CelValue::CreateString(wasm()->vm_id());
        break;
      }
      continue;
    }

    if (value.IsMap()) {
      auto& map = *value.MapOrDie();
      auto field = map[CelValue::CreateString(part)];
      if (field.has_value()) {
        value = field.value();
      } else {
        return {};
      }
    } else if (value.IsMessage()) {
      auto msg = value.MessageOrDie();
      if (msg == nullptr) {
        return {};
      }
      const Protobuf::Descriptor* desc = msg->GetDescriptor();
      const Protobuf::FieldDescriptor* field_desc = desc->FindFieldByName(std::string(part));
      if (field_desc == nullptr) {
        return {};
      } else if (field_desc->is_map()) {
        value = CelValue::CreateMap(
            Protobuf::Arena::Create<FieldBackedMapImpl>(&arena, msg, field_desc, &arena));
      } else if (field_desc->is_repeated()) {
        value = CelValue::CreateList(
            Protobuf::Arena::Create<FieldBackedListImpl>(&arena, msg, field_desc, &arena));
      } else {
        auto status =
            google::api::expr::runtime::CreateValueFromSingleField(msg, field_desc, &arena, &value);
        if (!status.ok()) {
          return {};
        }
      }
    } else {
      return {};
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
  case HeaderMapType::HttpCallResponseHeaders:
    if (http_call_response_)
      return &(*http_call_response_)->headers();
    return nullptr;
  case HeaderMapType::HttpCallResponseTrailers:
    if (http_call_response_)
      return (*http_call_response_)->trailers();
    return nullptr;
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
  auto entry = map->get(lower_key);
  if (entry != nullptr) {
    entry->value(value.data(), value.size());
  } else {
    map->addCopy(lower_key, std::string(value));
  }
}

uint32_t Context::getHeaderMapSize(HeaderMapType type) {
  auto map = getMap(type);
  if (!map) {
    return 0;
  }
  return map->refreshByteSize();
}

// Buffer

Buffer::Instance* Context::getBuffer(BufferType type) {
  switch (type) {
  case BufferType::HttpRequestBody:
    return request_body_buffer_;
  case BufferType::HttpResponseBody:
    return response_body_buffer_;
  case BufferType::NetworkDownstreamData:
    return network_downstream_data_buffer_;
  case BufferType::NetworkUpstreamData:
    return network_upstream_data_buffer_;
  case BufferType::HttpCallResponseBody:
    if (http_call_response_) {
      return (*http_call_response_)->body().get();
    }
    break;
  case BufferType::GrpcReceiveBuffer:
    return grpc_receive_buffer_.get();
  default:
    break;
  }
  return nullptr;
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

  Http::MessagePtr message(new Http::RequestMessageImpl(buildHeaderMapFromPairs(request_headers)));

  // Check that we were provided certain headers.
  if (message->headers().Path() == nullptr || message->headers().Method() == nullptr ||
      message->headers().Host() == nullptr) {
    return WasmResult::BadArgument;
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

  // set default hash policy to be based on :authority to enable consistent hash
  Http::AsyncClient::RequestOptions options;
  options.setTimeout(timeout);
  Protobuf::RepeatedPtrField<envoy::api::v2::route::RouteAction::HashPolicy> hash_policy;
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

WasmResult Context::grpcCall(const envoy::api::v2::core::GrpcService& grpc_service,
                             absl::string_view service_name, absl::string_view method_name,
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

  // set default hash policy to be based on :authority to enable consistent hash
  Http::AsyncClient::RequestOptions options;
  options.setTimeout(timeout);
  Protobuf::RepeatedPtrField<envoy::api::v2::route::RouteAction::HashPolicy> hash_policy;
  hash_policy.Add()->mutable_header()->set_header_name(Http::Headers::get().Host.get());
  options.setHashPolicy(hash_policy);

  // NB: this call causes the onCreateInitialMetadata callback to occur inline *before* this call
  // returns. Consequently the grpc_request is not available. Attempting to close or reset from
  // that callback will fail.
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

WasmResult Context::grpcStream(const envoy::api::v2::core::GrpcService& grpc_service,
                               absl::string_view service_name, absl::string_view method_name,
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

  // set default hash policy to be based on :authority to enable consistent hash
  Http::AsyncClient::StreamOptions options;
  Protobuf::RepeatedPtrField<envoy::api::v2::route::RouteAction::HashPolicy> hash_policy;
  hash_policy.Add()->mutable_header()->set_header_name(Http::Headers::get().Host.get());
  options.setHashPolicy(hash_policy);

  // NB: this call causes the onCreateInitialMetadata callback to occur inline *before* this call
  // returns. Consequently the grpc_stream is not available. Attempting to close or reset from
  // that callback will fail.
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

WasmResult Context::setProperty(absl::string_view key, absl::string_view serialized_value) {
  auto* stream_info = getRequestStreamInfo();
  if (!stream_info) {
    return WasmResult::NotFound;
  }
  stream_info->filterState().setData(key, std::make_unique<WasmState>(serialized_value),
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
bool Context::onStart(absl::string_view vm_configuration) {
  bool result = 0;
  if (wasm_->onStart_) {
    configuration_ = vm_configuration;
    result = wasm_->onStart_(this, id_, static_cast<uint32_t>(vm_configuration.size())).u64_ != 0;
    configuration_ = "";
  }
  return result;
}

bool Context::validateConfiguration(absl::string_view configuration) {
  if (!wasm_->validateConfiguration_) {
    return true;
  }
  configuration_ = configuration;
  auto result =
      wasm_->validateConfiguration_(this, id_, static_cast<uint32_t>(configuration.size())).u64_ !=
      0;
  configuration_ = "";
  return result;
}

bool Context::onConfigure(absl::string_view plugin_configuration) {
  if (!wasm_->onConfigure_) {
    return true;
  }
  configuration_ = plugin_configuration;
  auto result =
      wasm_->onConfigure_(this, id_, static_cast<uint32_t>(plugin_configuration.size())).u64_ != 0;
  configuration_ = "";
  return result;
}

absl::string_view Context::getConfiguration() { return configuration_; }

std::pair<uint32_t, absl::string_view> Context::getStatus() {
  return std::make_pair(status_code_, status_message_);
}

void Context::onCreate(uint32_t root_context_id) {
  if (wasm_->onCreate_) {
    wasm_->onCreate_(this, id_, root_context_id);
  }
}

Network::FilterStatus Context::onNetworkNewConnection() {
  onCreate(root_context_id_);
  if (!wasm_->onNewConnection_) {
    return Network::FilterStatus::Continue;
  }
  if (wasm_->onNewConnection_(this, id_).u64_ == 0) {
    return Network::FilterStatus::Continue;
  }
  return Network::FilterStatus::StopIteration;
}

Network::FilterStatus Context::onDownstreamData(int data_length, bool end_of_stream) {
  if (!wasm_->onDownstreamData_) {
    return Network::FilterStatus::Continue;
  }
  end_of_stream_ = end_of_stream;
  auto result = wasm_->onDownstreamData_(this, id_, static_cast<uint32_t>(data_length),
                                         static_cast<uint32_t>(end_of_stream));
  // TODO(PiotrSikora): pull Proxy-WASM's FilterStatus values.
  return result.u64_ == 0 ? Network::FilterStatus::Continue : Network::FilterStatus::StopIteration;
}

Network::FilterStatus Context::onUpstreamData(int data_length, bool end_of_stream) {
  if (!wasm_->onUpstreamData_) {
    return Network::FilterStatus::Continue;
  }
  end_of_stream_ = end_of_stream;
  auto result = wasm_->onUpstreamData_(this, id_, static_cast<uint32_t>(data_length),
                                       static_cast<uint32_t>(end_of_stream));
  // TODO(PiotrSikora): pull Proxy-WASM's FilterStatus values.
  return result.u64_ == 0 ? Network::FilterStatus::Continue : Network::FilterStatus::StopIteration;
}

void Context::onDownstreamConnectionClose(PeerType peer_type) {
  if (wasm_->onDownstreamConnectionClose_) {
    wasm_->onDownstreamConnectionClose_(this, id_, static_cast<uint32_t>(peer_type));
  }
}

void Context::onUpstreamConnectionClose(PeerType peer_type) {
  if (wasm_->onUpstreamConnectionClose_) {
    wasm_->onUpstreamConnectionClose_(this, id_, static_cast<uint32_t>(peer_type));
  }
}

// Empty headers/trailers have zero size.
template <typename P> static uint32_t headerSize(const P& p) { return p ? p->size() : 0; }

Http::FilterHeadersStatus Context::onRequestHeaders() {
  onCreate(root_context_id_);
  in_vm_context_created_ = true;
  if (!wasm_->onRequestHeaders_) {
    return Http::FilterHeadersStatus::Continue;
  }
  if (wasm_->onRequestHeaders_(this, id_, headerSize(request_headers_)).u64_ == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Context::onRequestBody(int body_buffer_length, bool end_of_stream) {
  if (!wasm_->onRequestBody_) {
    return Http::FilterDataStatus::Continue;
  }
  switch (wasm_
              ->onRequestBody_(this, id_, static_cast<uint32_t>(body_buffer_length),
                               static_cast<uint32_t>(end_of_stream))
              .u64_) {
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
  if (!wasm_->onRequestTrailers_) {
    return Http::FilterTrailersStatus::Continue;
  }
  if (wasm_->onRequestTrailers_(this, id_, headerSize(request_trailers_)).u64_ == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return Http::FilterTrailersStatus::StopIteration;
}

Http::FilterMetadataStatus Context::onRequestMetadata() {
  if (!wasm_->onRequestMetadata_) {
    return Http::FilterMetadataStatus::Continue;
  }
  if (wasm_->onRequestMetadata_(this, id_, headerSize(request_metadata_)).u64_ == 0) {
    return Http::FilterMetadataStatus::Continue;
  }
  return Http::FilterMetadataStatus::Continue; // This is currently the only return code.
}

Http::FilterHeadersStatus Context::onResponseHeaders() {
  if (!in_vm_context_created_) {
    // If the request is invalid then onRequestHeaders() will not be called and neither will
    // onCreate() then sendLocalReply be called which will call this function. In this case we
    // need to call onCreate() so that the Context inside the VM is created before the
    // onResponseHeaders() call.
    onCreate(root_context_id_);
    in_vm_context_created_ = true;
  }
  if (!wasm_->onResponseHeaders_) {
    return Http::FilterHeadersStatus::Continue;
  }
  if (wasm_->onResponseHeaders_(this, id_, headerSize(response_headers_)).u64_ == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Context::onResponseBody(int body_buffer_length, bool end_of_stream) {
  if (!wasm_->onResponseBody_) {
    return Http::FilterDataStatus::Continue;
  }
  switch (wasm_
              ->onResponseBody_(this, id_, static_cast<uint32_t>(body_buffer_length),
                                static_cast<uint32_t>(end_of_stream))
              .u64_) {
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
  if (wasm_->onResponseTrailers_(this, id_, headerSize(response_trailers_)).u64_ == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return Http::FilterTrailersStatus::StopIteration;
}

Http::FilterMetadataStatus Context::onResponseMetadata() {
  if (!wasm_->onResponseMetadata_) {
    return Http::FilterMetadataStatus::Continue;
  }
  if (wasm_->onResponseMetadata_(this, id_, headerSize(response_metadata_)).u64_ == 0) {
    return Http::FilterMetadataStatus::Continue;
  }
  return Http::FilterMetadataStatus::Continue; // This is currently the only return code.
}

void Context::onHttpCallResponse(uint32_t token, uint32_t headers, uint32_t body_size,
                                 uint32_t trailers) {
  if (!wasm_->onHttpCallResponse_) {
    return;
  }
  wasm_->onHttpCallResponse_(this, id_, token, headers, body_size, trailers);
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
  wasm_->onGrpcCreateInitialMetadata_(this, id_, token, headerSize(grpc_create_initial_metadata_));
  grpc_create_initial_metadata_ = nullptr;
}

void Context::onGrpcReceiveInitialMetadata(uint32_t token, Http::HeaderMapPtr&& metadata) {
  if (!wasm_->onGrpcReceiveInitialMetadata_) {
    return;
  }
  grpc_receive_initial_metadata_ = std::move(metadata);
  wasm_->onGrpcReceiveInitialMetadata_(this, id_, token,
                                       headerSize(grpc_receive_initial_metadata_));
  grpc_receive_initial_metadata_ = nullptr;
}

void Context::onGrpcReceiveTrailingMetadata(uint32_t token, Http::HeaderMapPtr&& metadata) {
  if (!wasm_->onGrpcReceiveTrailingMetadata_) {
    return;
  }
  grpc_receive_trailing_metadata_ = std::move(metadata);
  wasm_->onGrpcReceiveTrailingMetadata_(this, id_, token,
                                        headerSize(grpc_receive_trailing_metadata_));
  grpc_receive_trailing_metadata_ = nullptr;
}

WasmResult Context::defineMetric(MetricType type, absl::string_view name, uint32_t* metric_id_ptr) {
  auto stat_name = wasm_->stat_name_set_->getDynamic(name);
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

Wasm::Wasm(absl::string_view runtime, absl::string_view vm_id, absl::string_view vm_configuration,
           Stats::ScopeSharedPtr scope, Upstream::ClusterManager& cluster_manager,
           Event::Dispatcher& dispatcher)
    : vm_id_(std::string(vm_id)), wasm_vm_(Common::Wasm::createWasmVm(runtime, scope)),
      scope_(scope), cluster_manager_(cluster_manager), dispatcher_(dispatcher),
      time_source_(dispatcher.timeSource()), vm_configuration_(vm_configuration),
      wasm_stats_(WasmStats{
          ALL_WASM_STATS(POOL_COUNTER_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")),
                         POOL_GAUGE_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")))}),
      stat_name_set_(scope_->symbolTable().makeSet("Wasm").release()) {
  active_wasm_++;
  wasm_stats_.active_.set(active_wasm_);
  wasm_stats_.created_.inc();
  ENVOY_LOG(debug, "Base Wasm created {} now active", active_wasm_);
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

void Wasm::registerCallbacks() {
#define _REGISTER(_fn)                                                                             \
  wasm_vm_->registerCallback(                                                                      \
      "env", #_fn, &Exports::_fn,                                                                  \
      &ConvertFunctionWordToUint32<decltype(Exports::_fn),                                         \
                                   Exports::_fn>::convertFunctionWordToUint32)
  if (is_emscripten_) {
    _REGISTER(pthread_equal);
  }
#undef _REGISTER

#define _REGISTER_WASI(_fn)                                                                        \
  wasm_vm_->registerCallback(                                                                      \
      "wasi_unstable", #_fn, &Exports::wasi_unstable_##_fn,                                        \
      &ConvertFunctionWordToUint32<decltype(Exports::wasi_unstable_##_fn),                         \
                                   Exports::wasi_unstable_##_fn>::convertFunctionWordToUint32)
  if (is_emscripten_) {
    _REGISTER_WASI(fd_write);
    _REGISTER_WASI(fd_seek);
    _REGISTER_WASI(fd_close);
    _REGISTER_WASI(environ_get);
    _REGISTER_WASI(environ_sizes_get);
    _REGISTER_WASI(proc_exit);
  }
#undef _REGISTER_WASI

  // Calls with the "proxy_" prefix.
#define _REGISTER_PROXY(_fn)                                                                       \
  wasm_vm_->registerCallback(                                                                      \
      "env", "proxy_" #_fn, &Exports::_fn,                                                         \
      &ConvertFunctionWordToUint32<decltype(Exports::_fn),                                         \
                                   Exports::_fn>::convertFunctionWordToUint32);
  _REGISTER_PROXY(log);

  _REGISTER_PROXY(getStatus);
  _REGISTER_PROXY(getConfiguration);

  _REGISTER_PROXY(setProperty);
  _REGISTER_PROXY(getProperty);

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

  _REGISTER_PROXY(getBufferStatus);
  _REGISTER_PROXY(getBufferBytes);

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

void Wasm::getFunctions() {
#define _GET(_fn) wasm_vm_->getFunction(#_fn, &_fn##_);
  _GET(_start);
  _GET(__wasm_call_ctors);

  _GET(malloc);
  _GET(free);
#undef _GET

#define _GET_PROXY(_fn) wasm_vm_->getFunction("proxy_" #_fn, &_fn##_);
  _GET_PROXY(validateConfiguration);
  _GET_PROXY(onStart);
  _GET_PROXY(onConfigure);
  _GET_PROXY(onTick);

  _GET_PROXY(onCreate);

  _GET_PROXY(onNewConnection);
  _GET_PROXY(onDownstreamData);
  _GET_PROXY(onUpstreamData);
  _GET_PROXY(onDownstreamConnectionClose);
  _GET_PROXY(onUpstreamConnectionClose);

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
    : std::enable_shared_from_this<Wasm>(wasm), vm_id_(wasm.vm_id_),
      vm_id_with_hash_(wasm.vm_id_with_hash_), scope_(wasm.scope_),
      cluster_manager_(wasm.cluster_manager_), dispatcher_(dispatcher),
      time_source_(dispatcher.timeSource()), wasm_stats_(wasm.wasm_stats_),
      stat_name_set_(wasm.stat_name_set_) {
  if (wasm.wasm_vm()->cloneable()) {
    wasm_vm_ = wasm.wasm_vm()->clone();
    vm_context_ = std::make_shared<Context>(this);
    getFunctions();
  } else {
    wasm_vm_ = Common::Wasm::createWasmVm(wasm.wasm_vm()->runtime(), scope_);
    if (!initialize(wasm.code(), wasm.allow_precompiled())) {
      throw WasmException("Failed to initialize WASM code");
    }
  }
  active_wasm_++;
  wasm_stats_.active_.set(active_wasm_);
  wasm_stats_.created_.inc();
  ENVOY_LOG(debug, "Thread-Local Wasm created {} now active", active_wasm_);
}

Wasm::~Wasm() {
  active_wasm_--;
  wasm_stats_.active_.set(active_wasm_);
  ENVOY_LOG(debug, "~Wasm {} remaining active", active_wasm_);
  for (auto& p : timer_) {
    if (p.second && p.second->enabled()) {
      p.second->disableTimer();
    }
  }
}

bool Wasm::initialize(const std::string& code, bool allow_precompiled) {
  if (!wasm_vm_) {
    return false;
  }

  // Construct a unique identifier for the VM based on the provided vm_id and a hash of the
  // code.
  vm_id_with_hash_ = vm_id_ + ":" + base64Sha256(code);

  auto ok = wasm_vm_->load(code, allow_precompiled);
  if (!ok) {
    return false;
  }
  auto metadata = wasm_vm_->getCustomSection("emscripten_metadata");
  if (!metadata.empty()) {
    // See https://github.com/emscripten-core/emscripten/blob/incoming/tools/shared.py#L3059
    is_emscripten_ = true;
    auto start = reinterpret_cast<const uint8_t*>(metadata.data());
    auto end = reinterpret_cast<const uint8_t*>(metadata.data() + metadata.size());
    start = decodeVarint(start, end, &emscripten_metadata_major_version_);
    start = decodeVarint(start, end, &emscripten_metadata_minor_version_);
    start = decodeVarint(start, end, &emscripten_abi_major_version_);
    start = decodeVarint(start, end, &emscripten_abi_minor_version_);
    uint32_t temp;
    if (emscripten_metadata_major_version_ > 0 || emscripten_metadata_minor_version_ > 1) {
      // metadata 0.2 - added: wasm_backend.
      start = decodeVarint(start, end, &temp);
    }
    start = decodeVarint(start, end, &temp);
    start = decodeVarint(start, end, &temp);
    if (emscripten_metadata_major_version_ > 0 || emscripten_metadata_minor_version_ > 0) {
      // metadata 0.1 - added: global_base, dynamic_base, dynamictop_ptr and tempdouble_ptr.
      start = decodeVarint(start, end, &temp);
      start = decodeVarint(start, end, &temp);
      start = decodeVarint(start, end, &temp);
      decodeVarint(start, end, &temp);
      if (emscripten_metadata_major_version_ > 0 || emscripten_metadata_minor_version_ > 2) {
        // metadata 0.3 - added: standalone_wasm.
        start = decodeVarint(start, end, &emscripten_standalone_wasm_);
      }
    }
  }
  registerCallbacks();
  wasm_vm_->link(vm_id_);
  vm_context_ = std::make_shared<Context>(this);
  getFunctions();
  startVm(vm_context_.get());
  code_ = code;
  allow_precompiled_ = allow_precompiled;
  return true;
}

void Wasm::startVm(Context* root_context) {
  /* Call "_start" function, and fallback to "__wasm_call_ctors" if the former is not available. */
  if (_start_) {
    _start_(root_context);
  } else if (__wasm_call_ctors_) {
    __wasm_call_ctors_(root_context);
  }
}

bool Wasm::configure(Context* root_context, absl::string_view configuration) {
  return root_context->onConfigure(configuration);
}

Context* Wasm::start(PluginSharedPtr plugin) {
  auto root_id = plugin->root_id_;
  auto it = root_contexts_.find(root_id);
  if (it != root_contexts_.end()) {
    it->second->onStart(vm_configuration());
    return it->second.get();
  }
  auto context = std::make_unique<Context>(this, root_id, plugin);
  auto context_ptr = context.get();
  root_contexts_[root_id] = std::move(context);
  context_ptr->onStart(vm_configuration());
  return context_ptr;
};

void Wasm::startForTesting(std::unique_ptr<Context> context, PluginSharedPtr plugin) {
  auto context_ptr = context.get();
  if (!context->wasm_) {
    // Initialization was delayed till the Wasm object was created.
    context->wasm_ = this;
    context->plugin_ = plugin;
    context->id_ = allocContextId();
    contexts_[context->id_] = context.get();
  }
  root_contexts_[""] = std::move(context);
  context_ptr->onStart("");
}

void Wasm::setTickPeriod(uint32_t context_id, std::chrono::milliseconds new_tick_period) {
  auto& tick_period = tick_period_[context_id];
  auto& timer = timer_[context_id];
  bool was_running = timer && tick_period.count() > 0;
  tick_period = new_tick_period;
  if (tick_period.count() > 0 && !was_running) {
    timer = dispatcher_.createTimer([weak = std::weak_ptr<Wasm>(shared_from_this()), context_id]() {
      auto shared = weak.lock();
      if (shared) {
        shared->tickHandler(context_id);
      }
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

Network::FilterStatus Context::onNewConnection() { return onNetworkNewConnection(); };

Network::FilterStatus Context::onData(Buffer::Instance& data, bool end_stream) {
  network_downstream_data_buffer_ = &data;
  auto result = onDownstreamData(data.length(), end_stream);
  network_downstream_data_buffer_ = nullptr;
  return result;
}

Network::FilterStatus Context::onWrite(Buffer::Instance& data, bool end_stream) {
  network_upstream_data_buffer_ = &data;
  auto result = onUpstreamData(data.length(), end_stream);
  network_upstream_data_buffer_ = nullptr;
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

void Wasm::log(absl::string_view root_id, const Http::HeaderMap* request_headers,
               const Http::HeaderMap* response_headers, const Http::HeaderMap* response_trailers,
               const StreamInfo::StreamInfo& stream_info) {
  auto context = getRootContext(root_id);
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
  end_of_stream_ = end_stream;
  auto result = onRequestHeaders();
  request_headers_ = nullptr;
  return result;
}

Http::FilterDataStatus Context::decodeData(Buffer::Instance& data, bool end_stream) {
  request_body_buffer_ = &data;
  end_of_stream_ = end_stream;
  auto result = onRequestBody(data.length(), end_stream);
  request_body_buffer_ = nullptr;
  return result;
}

Http::FilterTrailersStatus Context::decodeTrailers(Http::HeaderMap& trailers) {
  request_trailers_ = &trailers;
  auto result = onRequestTrailers();
  request_trailers_ = nullptr;
  return result;
}

Http::FilterMetadataStatus Context::decodeMetadata(Http::MetadataMap& request_metadata) {
  request_metadata_ = &request_metadata;
  auto result = onRequestMetadata();
  request_metadata_ = nullptr;
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
  end_of_stream_ = end_stream;
  auto result = onResponseHeaders();
  response_headers_ = nullptr;
  return result;
}

Http::FilterDataStatus Context::encodeData(Buffer::Instance& data, bool end_stream) {
  response_body_buffer_ = &data;
  end_of_stream_ = end_stream;
  auto result = onResponseBody(data.length(), end_stream);
  response_body_buffer_ = nullptr;
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
  if (wasm_->onGrpcReceive_) {
    grpc_receive_buffer_ = std::move(response);
    uint32_t response_size = grpc_receive_buffer_->length();
    wasm_->onGrpcReceive_(this, id_, token, response_size);
    grpc_receive_buffer_.reset();
  }
  if (IsGrpcCallToken(token)) {
    grpc_call_request_.erase(token);
  }
}

void Context::onGrpcClose(uint32_t token, const Grpc::Status::GrpcStatus& status,
                          const absl::string_view message) {
  if (wasm_->onGrpcClose_) {
    status_code_ = static_cast<uint32_t>(status);
    status_message_ = message;
    wasm_->onGrpcClose_(this, id_, token, static_cast<uint64_t>(status));
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

static void createWasmInternal(const envoy::config::wasm::v2::VmConfig& vm_config,
                               PluginSharedPtr plugin, Stats::ScopeSharedPtr scope,
                               Upstream::ClusterManager& cluster_manager,
                               Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                               Api::Api& api, std::unique_ptr<Context> root_context_for_testing,
                               Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                               CreateWasmCallback&& cb) {
  auto wasm = std::make_shared<Wasm>(vm_config.runtime(), vm_config.vm_id(),
                                     vm_config.configuration(), scope, cluster_manager, dispatcher);

  std::string source, code;
  if (vm_config.code().has_remote()) {
    source = vm_config.code().remote().http_uri().uri();
  } else if (vm_config.code().has_local()) {
    code = Config::DataSource::read(vm_config.code().local(), true, api);
    source = Config::DataSource::getPath(vm_config.code().local())
                 .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  }

  auto callback = [wasm, plugin, cb, source, allow_precompiled = vm_config.allow_precompiled(),
                   context_ptr = root_context_for_testing ? root_context_for_testing.release()
                                                          : nullptr](const std::string& code) {
    std::unique_ptr<Context> context(context_ptr);
    if (code.empty()) {
      throw WasmException(fmt::format("Failed to load WASM code from {}", source));
    }
    if (!wasm->initialize(code, allow_precompiled)) {
      throw WasmException(fmt::format("Failed to initialize WASM code from {}", source));
    }
    if (!context) {
      wasm->start(plugin);
    } else {
      wasm->startForTesting(std::move(context), plugin);
    }
    cb(wasm);
  };

  if (vm_config.code().has_remote()) {
    remote_data_provider = std::make_unique<Config::DataSource::RemoteAsyncDataProvider>(
        cluster_manager, init_manager, vm_config.code().remote(), true, std::move(callback));
  } else if (vm_config.code().has_local()) {
    callback(code);
  } else {
    callback(EMPTY_STRING);
  }
}

void createWasm(const envoy::config::wasm::v2::VmConfig& vm_config, PluginSharedPtr plugin,
                Stats::ScopeSharedPtr scope, Upstream::ClusterManager& cluster_manager,
                Init::Manager& init_manager, Event::Dispatcher& dispatcher, Api::Api& api,
                Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, api,
                     nullptr /* root_context_for_testing */, remote_data_provider, std::move(cb));
}

void createWasmForTesting(const envoy::config::wasm::v2::VmConfig& vm_config,
                          PluginSharedPtr plugin, Stats::ScopeSharedPtr scope,
                          Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                          Event::Dispatcher& dispatcher, Api::Api& api,
                          std::unique_ptr<Context> root_context_for_testing,
                          Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                          CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, api,
                     std::move(root_context_for_testing), remote_data_provider, std::move(cb));
}

std::shared_ptr<Wasm> createThreadLocalWasm(Wasm& base_wasm, PluginSharedPtr plugin,
                                            absl::string_view configuration,
                                            Event::Dispatcher& dispatcher) {
  auto wasm = std::make_shared<Wasm>(base_wasm, dispatcher);
  Context* root_context = wasm->start(plugin);
  if (!wasm->configure(root_context, configuration)) {
    throw WasmException("Failed to configure WASM code");
  }
  local_wasms[wasm->vm_id_with_hash()] = wasm;
  return wasm;
}

std::shared_ptr<Wasm> getThreadLocalWasmPtr(absl::string_view vm_id) {
  auto it = local_wasms.find(vm_id);
  if (it == local_wasms.end()) {
    return nullptr;
  }
  auto wasm = it->second.lock();
  if (!wasm) {
    local_wasms.erase(vm_id);
  }
  return wasm;
}

std::shared_ptr<Wasm> getOrCreateThreadLocalWasm(Wasm& base_wasm, PluginSharedPtr plugin,
                                                 absl::string_view configuration,
                                                 Event::Dispatcher& dispatcher) {
  auto wasm = getThreadLocalWasmPtr(base_wasm.vm_id_with_hash());
  if (wasm) {
    auto root_context = wasm->start(plugin);
    if (!wasm->configure(root_context, configuration)) {
      throw WasmException("Failed to configure WASM code");
    }
    return wasm;
  }
  return createThreadLocalWasm(base_wasm, plugin, configuration, dispatcher);
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
