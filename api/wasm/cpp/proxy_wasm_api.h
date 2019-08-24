/*
 * Intrinsic high-level support functions available to WASM modules.
 */
// NOLINT(namespace-envoy)
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#define CHECK_RESULT(_c) do { if ((_c) != WasmResult::Ok) { proxy_log(LogLevel::critical, #_c, sizeof( #_c )-1); abort(); }} while(0)

//
// High Level C++ API.
//
class ContextBase;
class RootContext;
class Context;

// Note: exceptions are currently not supported.
#define WASM_EXCEPTIONS 0
#if WASM_EXCEPTIONS
class ProxyException : std::runtime_error {
public:
  ProxyException(const std::string& message) : std::runtime_error(message) {}
};
#endif

inline WasmResult logTrace(StringView logMessage) {
  return proxy_log(LogLevel::trace, logMessage.data(), logMessage.size());
}
inline WasmResult logDebug(StringView logMessage) {
  return proxy_log(LogLevel::debug, logMessage.data(), logMessage.size());
}
inline WasmResult logInfo(StringView logMessage) {
  return proxy_log(LogLevel::info, logMessage.data(), logMessage.size());
}
inline WasmResult logWarn(StringView logMessage) {
  return proxy_log(LogLevel::warn, logMessage.data(), logMessage.size());
}
inline WasmResult logError(StringView logMessage) {
  return proxy_log(LogLevel::error, logMessage.data(), logMessage.size());
}
inline WasmResult logCritical(StringView logMessage) {
  return proxy_log(LogLevel::critical, logMessage.data(), logMessage.size());
}
inline void logAbort(StringView logMessag) {
  logCritical(logMessag);
  abort();
}

// Buffers coming into the WASM filter.
class WasmData {
public:
  WasmData(const char* data, size_t size) : data_(data), size_(size) {}
  ~WasmData() { ::free(const_cast<char*>(data_)); }
  const char* data() { return data_; }
  size_t size() { return size_; }
  StringView view() { return {data_, size_}; }
  std::string toString() { return std::string(view()); }
  std::vector<std::pair<StringView, StringView>> pairs();
  template<typename T> T proto() {
    T p;
    p.ParseFromArray(data_, size_);
    return p;
  }

  WasmData& operator=(const WasmData&) = delete;
  WasmData(const WasmData&) = delete;

private:
  const char* data_;
  size_t size_;
};
typedef std::unique_ptr<WasmData> WasmDataPtr;

inline std::vector<std::pair<StringView, StringView>> WasmData::pairs() {
  std::vector<std::pair<StringView, StringView>> result;
  if (!data())
    return result;
  auto p = data();
  int n = *reinterpret_cast<const int*>(p);
  p += sizeof(int);
  result.resize(n);
  auto s = p + n * 8;
  for (int i = 0; i < n; i++) {
    int size = *reinterpret_cast<const int*>(p);
    p += sizeof(int);
    result[i].first = StringView(s, size);
    s += size + 1;
    size = *reinterpret_cast<const int*>(p);
    p += sizeof(int);
    result[i].second = StringView(s, size);
    s += size + 1;
  }
  return result;
}

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

template <typename Pairs> void exportPairs(const Pairs& pairs, const char** ptr, size_t* size_ptr) {
  if (pairs.empty()) {
    *ptr = nullptr;
    *size_ptr = 0;
    return;
  }
  size_t size = pairsSize(pairs);
  char* buffer = static_cast<char*>(::malloc(size));
  marshalPairs(pairs, buffer);
  *size_ptr = size;
}

struct PairHash {
  template <typename T, typename U> std::size_t operator()(const std::pair<T, U>& x) const {
    return std::hash<T>()(x.first) + std::hash<U>()(x.second);
  }
};

struct Tuple3Hash {
  template <typename T, typename U, typename V> std::size_t operator()(const std::tuple<T, U, V>& x) const {
    return std::hash<T>()(std::get<0>(x)) + std::hash<U>()(std::get<1>(x)) + std::hash<V>()(std::get<2>(x));
  }
};

using HeaderStringPairs = std::vector<std::pair<std::string, std::string>>;

class GrpcCallHandlerBase {
public:
  GrpcCallHandlerBase() {}
  virtual ~GrpcCallHandlerBase() {}

  RootContext* context() { return context_; }

  void cancel();

  virtual void onCreateInitialMetadata() {}
  virtual void onSuccess(std::unique_ptr<WasmData> message) = 0;
  virtual void onFailure(GrpcStatus status, std::unique_ptr<WasmData> error_message) = 0;

private:
  friend class RootContext;

  RootContext* context_{nullptr};
  uint32_t token_;
};

template<typename Message>
class GrpcCallHandler : public GrpcCallHandlerBase {
public:
  GrpcCallHandler() : GrpcCallHandlerBase() {}
  virtual ~GrpcCallHandler() {}

  virtual void onSuccess(Message&& response) = 0;

private:
  void onSuccess(std::unique_ptr<WasmData> message) override {
    onSuccess(message->proto<Message>());
  }
};

class GrpcStreamHandlerBase {
public:
  GrpcStreamHandlerBase() {}
  virtual ~GrpcStreamHandlerBase() {}

  RootContext* context() { return context_; }

  // NB: with end_of_stream == true, callbacks can still occur: reset() to prevent further callbacks.
  void send(StringView message, bool end_of_stream);
  void close(); // NB: callbacks can still occur: reset() to prevent further callbacks.
  void reset();

  virtual void onCreateInitialMetadata() {}
  virtual void onReceiveInitialMetadata() {}
  virtual void onReceiveTrailingMetadata() {}
  virtual void onReceive(std::unique_ptr<WasmData> message) = 0;
  virtual void onRemoteClose(GrpcStatus status, std::unique_ptr<WasmData> error_message) = 0;

protected:
  friend class RootContext;

  void doRemoteClose(GrpcStatus status, std::unique_ptr<WasmData> error_message);

  bool local_close_{false};
  bool remote_close_{false};
  RootContext*  context_{nullptr};
  uint32_t token_;
};

template<typename Request, typename Response>
class GrpcStreamHandler : public GrpcStreamHandlerBase {
public:
  GrpcStreamHandler() : GrpcStreamHandlerBase() {}
  virtual ~GrpcStreamHandler() {}

  void send(const Request& message, bool end_of_stream) {
    std::string output;
    if (!message.SerializeToString(&output)) {
      return;
    }
    GrpcStreamHandlerBase::send(output, end_of_stream);
    local_close_ = local_close_ || end_of_stream;
  }

  virtual void onReceive(Response&& message) = 0;

private:
  void onReceive(std::unique_ptr<WasmData> message) override {
    onReceive(message->proto<Response>());
  }
};

// Behavior supported by all contexts.
class ContextBase {
public:
  explicit ContextBase(uint32_t id) : id_(id) {}
  virtual ~ContextBase() {}

  uint32_t id() { return id_; }

  // Make this context the effective context for calls out of the VM.
  WasmResult setEffectiveContext();

  virtual RootContext* asRoot() { return nullptr; }
  virtual Context* asContext() { return nullptr; }

  // Metadata
  virtual bool isRootCachable(MetadataType type);  // Cache in the root.
  virtual bool isProactivelyCachable(MetadataType type);  // Cache all keys on any read.
  WasmResult nodeMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult nodeMetadataStruct(google::protobuf::Struct* struct_ptr);
  WasmResult namedMetadataValue(MetadataType type, StringView name, StringView key,  google::protobuf::Value* value_ptr);

  using HttpCallCallback = std::function<void(std::unique_ptr<WasmData> header_pairs,
      std::unique_ptr<WasmData> body, std::unique_ptr<WasmData> trailer_pairs)>;
  using GrpcSimpleCallCallback = std::function<void(GrpcStatus status, std::unique_ptr<WasmData> message)>;

protected:
  WasmResult metadataValue(MetadataType type, StringView key, google::protobuf::Value* value_ptr);
  WasmResult metadataStruct(MetadataType type, StringView name, google::protobuf::Struct* struct_ptr);
  WasmResult metadataStruct(MetadataType type, google::protobuf::Struct* struct_ptr) {
    return metadataStruct(type, "", struct_ptr);
  }

  std::unordered_map<std::pair<int32_t, std::string>, google::protobuf::Value, PairHash> value_cache_;
  std::unordered_map<std::tuple<int32_t, std::string, std::string>, google::protobuf::Value, Tuple3Hash> name_value_cache_;
  std::unordered_map<std::pair<int32_t, std::string>, google::protobuf::Struct, PairHash> struct_cache_;

private:
  uint32_t id_;
};

// A context unique for each root_id for a use-case (e.g. filter) compiled into module.
class RootContext : public ContextBase {
public:
  RootContext(uint32_t id, StringView root_id) : ContextBase(id), root_id_(root_id) {}
  ~RootContext() {}

  StringView root_id() { return root_id_; }

  RootContext* asRoot() override { return this; }
  Context* asContext() override { return nullptr; }

  // Called once when the VM loads and once when each hook loads and whenever configuration changes.
  virtual void onConfigure(std::unique_ptr<WasmData> /* configuration */) {}
  // Called when each hook loads.
  virtual void onStart(WasmDataPtr /* vm_configuration */) {}
  // Called when the timer goes off.
  virtual void onTick() {}
  // Called when data arrives on a SharedQueue.
  virtual void onQueueReady(uint32_t /* token */) {}

  // Low level HTTP/gRPC interface.
  virtual void onHttpCallResponse(uint32_t token, std::unique_ptr<WasmData> header_pairs,
                                  std::unique_ptr<WasmData> body,
                                  std::unique_ptr<WasmData> trailer_pairs);
  virtual void onGrpcCreateInitialMetadata(uint32_t token);
  virtual void onGrpcReceiveInitialMetadata(uint32_t token);
  virtual void onGrpcReceiveTrailingMetadata(uint32_t token);
  virtual void onGrpcReceive(uint32_t token, std::unique_ptr<WasmData> message);
  virtual void onGrpcClose(uint32_t token, GrpcStatus status, std::unique_ptr<WasmData> message);

  // Default high level HTTP/gRPC interface.  NB: overriding the low level interface will disable this interface.
  // Returns false on setup error.
  bool httpCall(StringView uri, const HeaderStringPairs& request_headers,
      StringView request_body, const HeaderStringPairs& request_trailers,
      uint32_t timeout_milliseconds, HttpCallCallback callback);
  // NB: the message is the response if status == OK and an error message otherwise.
  // Returns false on setup error.
  bool grpcSimpleCall(StringView service, StringView service_name, StringView method_name,
      const google::protobuf::MessageLite &request, uint32_t timeout_milliseconds, GrpcSimpleCallCallback callback);
  template<typename Response> void grpcSimpleCall(StringView service, StringView service_name,
      StringView method_name, const google::protobuf::MessageLite &request, uint32_t timeout_milliseconds,
      std::function<void(Response&& response)> success_callback,
      std::function<void(GrpcStatus status, StringView error_message)> failure_callback) {
    auto callback = [success_callback, failure_callback](GrpcStatus status, std::unique_ptr<WasmData> message) {
      if (status == GrpcStatus::Ok) {
        success_callback(message->proto<Response>());
      } else {
        failure_callback(status, message->view());
      }
    };
    grpcSimpleCall(service, service_name, method_name, request, timeout_milliseconds, callback);
  }
  // Returns false on setup error.
  bool grpcCallHandler(StringView service, StringView service_name,
      StringView method_name, const google::protobuf::MessageLite &request, uint32_t timeout_milliseconds,
      std::unique_ptr<GrpcCallHandlerBase> handler);
  // Returns false on setup error.
  bool grpcStreamHandler(StringView service, StringView service_name,
      StringView method_name, std::unique_ptr<GrpcStreamHandlerBase> handler);

private:
  friend class GrpcCallHandlerBase;
  friend class GrpcStreamHandlerBase;

  const std::string root_id_;
  std::unordered_map<uint32_t, HttpCallCallback> http_calls_;
  std::unordered_map<uint32_t, GrpcSimpleCallCallback> simple_grpc_calls_;
  std::unordered_map<uint32_t, std::unique_ptr<GrpcCallHandlerBase>> grpc_calls_;
  std::unordered_map<uint32_t, std::unique_ptr<GrpcStreamHandlerBase>> grpc_streams_;
};

RootContext* getRoot(StringView root_id);

// Context for a stream.  The distinguished context id == 0 is used for non-stream calls.
class Context : public ContextBase {
public:
  Context(uint32_t id, RootContext* root) : ContextBase(id), root_(root) {}
  virtual ~Context() {}

  RootContext* root() { return root_; }

  RootContext* asRoot() override { return nullptr; }
  Context* asContext() override { return this; }

  // Called on individual requests/response streams.
  virtual void onCreate() {}
  virtual FilterHeadersStatus onRequestHeaders() { return FilterHeadersStatus::Continue; }
  virtual FilterMetadataStatus onRequestMetadata() { return FilterMetadataStatus::Continue; }
  virtual FilterDataStatus onRequestBody(size_t /* body_buffer_length */, bool /* end_of_stream */) {
    return FilterDataStatus::Continue;
  }
  virtual FilterTrailersStatus onRequestTrailers() { return FilterTrailersStatus::Continue; }
  virtual FilterHeadersStatus onResponseHeaders() { return FilterHeadersStatus::Continue; }
  virtual FilterMetadataStatus onResponseMetadata() { return FilterMetadataStatus::Continue; }
  virtual FilterDataStatus onResponseBody(size_t /* body_buffer_length */, bool /* end_of_stream */) {
    return FilterDataStatus::Continue;
  }
  virtual FilterTrailersStatus onResponseTrailers() { return FilterTrailersStatus::Continue; }
  virtual void onDone() {}  // Called when the stream has completed.
  virtual void onLog() {}  // Called after onDone when logging is requested.
  virtual void onDelete() {}  // Called to indicate that no more calls will come and this context is being deleted.

  // Metadata
  bool isImmutable(MetadataType type);
  // Caching Metadata calls.  Note: "name" refers to the metadata namespace.
  WasmResult requestRouteMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult responseRouteMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult logMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult requestMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult responseMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult requestMetadataValue(StringView name, StringView key, google::protobuf::Value* value_ptr);
  WasmResult responseMetadataValue(StringView name, StringView key, google::protobuf::Value* value_ptr);
  WasmResult requestRouteMetadataStruct(google::protobuf::Struct* struct_ptr);
  WasmResult responseRouteMetadataStruct(google::protobuf::Struct* struct_ptr);
  WasmResult logMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr);
  WasmResult requestMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr);
  WasmResult responseMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr);
  WasmResult logMetadataStruct(google::protobuf::Struct* struct_ptr) { return logMetadataStruct("", struct_ptr); }
  WasmResult requestMetadataStruct(google::protobuf::Struct* struct_ptr) { return requestMetadataStruct("", struct_ptr); }
  WasmResult responseMetadataStruct(google::protobuf::Struct* struct_ptr) { return responseMetadataStruct("", struct_ptr); }
  // Uncached Metadata calls.
  WasmResult getRequestMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult getResponseMetadataValue(StringView key, google::protobuf::Value* value_ptr);
  WasmResult getRequestMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr);
  WasmResult getResponseMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr);

private:
  RootContext* root_{};
};

// Returns nullptr if the Context no longer exists (i.e. the stream has been destroyed).
Context* getContext(uint32_t context_id);

using RootFactory = std::function<std::unique_ptr<RootContext>(uint32_t id, StringView root_id)>;
using ContextFactory = std::function<std::unique_ptr<Context>(uint32_t id, RootContext* root)>;

// Create a factory from a class name.
#define ROOT_FACTORY(_c) [](uint32_t id, StringView root_id) -> std::unique_ptr<RootContext> { \
  return std::make_unique<_c>(id, root_id); \
}
#define CONTEXT_FACTORY(_c) [](uint32_t id, RootContext* root) -> std::unique_ptr<Context> { \
  return std::make_unique<_c>(id, root); \
}

// Register Context factory.
// e.g. static RegisterContextFactory register_MyContext(CONTEXT_FACTORY(MyContext));
struct RegisterContextFactory {
  explicit RegisterContextFactory(ContextFactory context_factory, RootFactory root_factory = nullptr, StringView root_id = "");
};

inline bool ContextBase::isRootCachable(MetadataType type) {
  switch (type) {
    case MetadataType::Node:
      return true;
    default:
      return false;
  }
}

inline bool Context::isImmutable(MetadataType type) {
  switch (type) {
    case MetadataType::Request:
    case MetadataType::Response:
      return false;
    default:
      return true;
  }
}

// Override in subclasses to proactively cache certain types of metadata.
inline bool ContextBase::isProactivelyCachable(MetadataType type) {
  switch (type) {
    case MetadataType::Node:
      return true;
    default:
      return false;
  }
}

// Expressions

#define PROXY_EXPRESSION_GET_STRING(_expression, _string_ptr) do { \
  const char* _value_ptr = nullptr; \
  size_t _value_size = 0; \
  auto _result = static_cast<WasmResult>(proxy_getMetadata(MetadataType::Expression, _expression, sizeof(_expression)-1, &_value_ptr, &_value_size)); \
  if (_result != WasmResult::Ok) { \
    _string_ptr->clear(); \
    return _result; \
  } \
  _string_ptr->assign(_value_ptr, _value_size); \
  return WasmResult::Ok; \
} while(0)

#define PROXY_EXPRESSION_GET(_expression, _data_ptr) do { \
  const char* _value_ptr = nullptr; \
  size_t _value_size = 0; \
  auto _result = static_cast<WasmResult>(proxy_getMetadata(MetadataType::Expression, _expression, sizeof(_expression)-1, &_value_ptr, &_value_size)); \
  if (_result != WasmResult::Ok) { \
    return _result; \
  } \
  if (_value_size != sizeof(*_data_ptr)) { \
    return WasmResult::ResultMismatch; \
  } \
  memcpy(_data_ptr, _value_ptr, sizeof(*_data_ptr)); \
  return WasmResult::Ok; \
} while(0)

inline WasmResult getRequestProtocol(std::string *protocol_ptr) {
  PROXY_EXPRESSION_GET_STRING("request.protocol", protocol_ptr);
}

inline WasmResult getResponseProtocol(std::string *protocol_ptr) {
  PROXY_EXPRESSION_GET_STRING("response.protocol", protocol_ptr);
}

inline WasmResult getRequestDestinationPort(uint32_t* port) {
  PROXY_EXPRESSION_GET("request.destination_port", port);
}

inline WasmResult getResponseDestinationPort(uint32_t* port) {
  PROXY_EXPRESSION_GET("response.destination_port", port);
}

inline WasmResult getRequestResponseCode(uint32_t* response_code) {
  PROXY_EXPRESSION_GET("request.response_code", response_code);
}

inline WasmResult getResponseResponseCode(uint32_t* response_code) {
  PROXY_EXPRESSION_GET("response.response_code", response_code);
}

inline WasmResult getRequestTlsVersion(std::string *tls_version_ptr) {
  PROXY_EXPRESSION_GET_STRING("request.tls_version", tls_version_ptr);
}

inline WasmResult getResponseTlsVersion(std::string *tls_version_ptr) {
  PROXY_EXPRESSION_GET_STRING("response.tls_version", tls_version_ptr);
}

inline WasmResult getRequestPeerCertificatePresented(bool *peer_certificate_presented) {
  PROXY_EXPRESSION_GET("request.peer_certificate_presented", peer_certificate_presented);
}

inline WasmResult getResponsePeerCertificatePresented(bool *peer_certificate_presented) {
  PROXY_EXPRESSION_GET("response.peer_certificate_presented", peer_certificate_presented);
}

inline WasmResult getPluginDirection(PluginDirection *direction_ptr) {
  PROXY_EXPRESSION_GET("plugin.direction", reinterpret_cast<uint32_t*>(direction_ptr));
}

// Generic selector
inline Optional<WasmDataPtr> getSelectorExpression(std::initializer_list<StringView> parts) {
  size_t size = 0;
  for (auto part: parts) {
    size += part.size() + 1; // null terminated string value
  }

  char* buffer = static_cast<char*>(::malloc(size));
  char* b = buffer;

  for (auto part : parts) {
    memcpy(b, part.data(), part.size());
    b += part.size();
    *b++ = 0;
  }

  const char* value_ptr = nullptr;
  size_t value_size = 0;
  auto result = proxy_getSelectorExpression(buffer, size, &value_ptr, &value_size);
  ::free(buffer);
  if (result != WasmResult::Ok) {
    return {};
  }
  return std::make_unique<WasmData>(value_ptr, value_size);
}

// Metadata
inline WasmResult getMetadata(MetadataType type, StringView key, WasmDataPtr *wasm_data) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  auto result = static_cast<WasmResult>(proxy_getMetadata(type, key.data(), key.size(), &value_ptr, &value_size));
  if (result != WasmResult::Ok) {
    wasm_data->reset();
    return result;
  }
  *wasm_data = std::make_unique<WasmData>(value_ptr, value_size);
  return WasmResult::Ok;
}

inline WasmResult getMetadataValue(MetadataType type, StringView key, google::protobuf::Value* result_ptr) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  WasmResult result = static_cast<WasmResult>(proxy_getMetadata(type, key.data(), key.size(), &value_ptr, &value_size));
  if (result != WasmResult::Ok) {
    result_ptr->Clear();
    return result;
  }
  if (!value_size) {
    result_ptr->Clear();
    return WasmResult::Ok;
  }
  if (!result_ptr->ParseFromArray(value_ptr, value_size)) {
    result_ptr->Clear();
    return WasmResult::ParseFailure;
  }
  return WasmResult::Ok;
}

inline WasmResult getMetadataStringValue(MetadataType type, StringView key, std::string* string_ptr) {
  google::protobuf::Value value;
  auto result = getMetadataValue(type, key, &value);
  *string_ptr = value.string_value();
  return result;
}

inline WasmResult setMetadata(MetadataType type, StringView key, StringView value) {
  return static_cast<WasmResult>(proxy_setMetadata(type, key.data(), key.size(), value.data(), value.size()));
}

inline WasmResult setMetadataValue(MetadataType type, StringView key,
                                       const google::protobuf::Value& value) {
  std::string output;
  if (!value.SerializeToString(&output)) {
    return WasmResult::SerializationFailure;
  }
  return static_cast<WasmResult>(proxy_setMetadata(type, key.data(), key.size(), output.data(), output.size()));
}

inline WasmResult setMetadataStringValue(MetadataType type, StringView key, StringView s) {
  google::protobuf::Value value;
  value.set_string_value(s.data(), s.size());
  return setMetadataValue(type, key, value);
}

inline WasmResult getMetadataValuePairs(MetadataType type, WasmDataPtr* pairs_ptr) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  auto result = static_cast<WasmResult>(proxy_getMetadataPairs(type, &value_ptr, &value_size));
  if (result != WasmResult::Ok) {
    pairs_ptr->reset();
    return result;
  }
  *pairs_ptr =  std::make_unique<WasmData>(value_ptr, value_size);
  return WasmResult::Ok;
}

inline WasmResult getMetadataStruct(MetadataType type, StringView name, google::protobuf::Struct* struct_ptr) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  auto result = proxy_getMetadataStruct(type, name.data(), name.size(), &value_ptr, &value_size);
  if (result != WasmResult::Ok || !value_size) {
    struct_ptr->Clear();
    return result;
  }
  google::protobuf::Struct s;
  if (!s.ParseFromArray(value_ptr, value_size)) {
    return WasmResult::ParseFailure;
  }
  *struct_ptr = s;
  return WasmResult::Ok;
}

inline WasmResult setMetadataStruct(MetadataType type, StringView name,
                                        const google::protobuf::Struct& s) {
  std::string output;
  if (!s.SerializeToString(&output)) {
    return WasmResult::SerializationFailure;
  }
  return static_cast<WasmResult>(proxy_setMetadataStruct(type, name.data(), name.size(), output.data(), output.size()));
}

inline WasmResult ContextBase::metadataValue(MetadataType type, StringView key, google::protobuf::Value* value_ptr) {
  if (isRootCachable(type)) {
    if (auto context = asContext()) {
      return context->root()->metadataValue(type, key, value_ptr);
    }
  }
  auto cache_key = std::make_pair(static_cast<int32_t>(type), std::string(key));
  auto it = value_cache_.find(cache_key);
  if (it != value_cache_.end()) {
    *value_ptr = it->second;
    return WasmResult::Ok;
  }
  if (isProactivelyCachable(type)) {
    WasmDataPtr values;
    auto result = getMetadataValuePairs(type, &values);
    if (result != WasmResult::Ok) {
      return result;
    }
    for (auto &p : values->pairs()) {
      google::protobuf::Value value;
      if (value.ParseFromArray(p.second.data(), p.second.size())) {
        auto k = std::make_pair(static_cast<int32_t>(type), std::string(p.first));
        value_cache_[k] = value;
      }
    }
    auto it = value_cache_.find(cache_key);
    if (it != value_cache_.end()) {
      *value_ptr = it->second;
      return WasmResult::Ok;
    }
    return WasmResult::NotFound;
  } else {
    auto result = getMetadataValue(type, key, value_ptr);
    if (result != WasmResult::Ok) {
      value_ptr->Clear();
      return result;
    }
    value_cache_[cache_key] = *value_ptr;
    return WasmResult::Ok;
  }
}

inline WasmResult Context::requestRouteMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return metadataValue(MetadataType::RequestRoute, key, value_ptr);
}

inline WasmResult Context::responseRouteMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return metadataValue(MetadataType::ResponseRoute, key, value_ptr);
}

inline WasmResult Context::logMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return metadataValue(MetadataType::Log, key, value_ptr);
}

inline WasmResult Context::requestMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return metadataValue(MetadataType::Request, key, value_ptr);
}

inline WasmResult Context::responseMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return metadataValue(MetadataType::Response, key, value_ptr);
}

inline WasmResult ContextBase::nodeMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return metadataValue(MetadataType::Node, key, value_ptr);
}

inline WasmResult ContextBase::metadataStruct(MetadataType type, StringView name, google::protobuf::Struct* struct_ptr) {
  if (isRootCachable(type)) {
    if (auto context = asContext()) {
      return context->root()->metadataStruct(type, name, struct_ptr);
    }
  }
  auto cache_key = std::make_pair(static_cast<int32_t>(type),  std::string(name));
  auto it = struct_cache_.find(cache_key);
  if (it != struct_cache_.end()) {
    *struct_ptr = it->second;
    return WasmResult::Ok;
  }
  auto result = getMetadataStruct(type, name, struct_ptr);
  if (result != WasmResult::Ok) {
    struct_ptr->Clear();
    return result;
  }
  struct_cache_[cache_key] = *struct_ptr;
  return WasmResult::Ok;
}

inline WasmResult ContextBase::namedMetadataValue(
    MetadataType type, StringView name, StringView key, google::protobuf::Value* value_ptr) {
  if (isRootCachable(type)) {
    if (auto context = asContext()) {
      return context->root()->namedMetadataValue(type, name, key, value_ptr);
    }
  }
  auto n = std::string(name);
  auto cache_key = std::make_tuple(static_cast<int32_t>(type),  n, std::string(key));
  auto it = name_value_cache_.find(cache_key);
  if (it != name_value_cache_.end()) {
    *value_ptr = it->second;
    return WasmResult::Ok;
  }
  google::protobuf::Struct s;
  auto result = metadataStruct(type, name, &s);
  if (result != WasmResult::Ok) {
    value_ptr->Clear();
    return result;
  }
  for (auto &f : s.fields()) {
    auto k = std::make_tuple(static_cast<int32_t>(type), n, f.first);
    name_value_cache_[k] =  f.second;
  }
  struct_cache_[std::make_pair(static_cast<int32_t>(type), n)] = std::move(s);
  it = name_value_cache_.find(cache_key);
  if (it != name_value_cache_.end()) {
    *value_ptr = it->second;
    return WasmResult::Ok;
  }
  return WasmResult::NotFound;
}

inline WasmResult Context::requestMetadataValue(StringView name, StringView key, google::protobuf::Value* value_ptr) {
  return namedMetadataValue(MetadataType::Request, name, key, value_ptr);
}

inline WasmResult Context::responseMetadataValue(StringView name, StringView key, google::protobuf::Value* value_ptr) {
  return namedMetadataValue(MetadataType::Response, name, key, value_ptr);
}

inline WasmResult Context::requestRouteMetadataStruct(google::protobuf::Struct* struct_ptr) {
  return metadataStruct(MetadataType::RequestRoute, struct_ptr);
}

inline WasmResult Context::responseRouteMetadataStruct(google::protobuf::Struct* struct_ptr) {
  return metadataStruct(MetadataType::ResponseRoute, struct_ptr);
}

inline WasmResult ContextBase::nodeMetadataStruct(google::protobuf::Struct* struct_ptr) {
  return metadataStruct(MetadataType::Node, struct_ptr);
}

inline WasmResult Context::logMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr) {
  return metadataStruct(MetadataType::Log, name, struct_ptr);
}

inline WasmResult Context::requestMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr) {
  return metadataStruct(MetadataType::Request, name, struct_ptr);
}

inline WasmResult Context::responseMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr) {
  return metadataStruct(MetadataType::Response, name, struct_ptr);
}

inline WasmResult Context::getRequestMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return getMetadataValue(MetadataType::Request, key, value_ptr);
}

inline WasmResult Context::getResponseMetadataValue(StringView key, google::protobuf::Value* value_ptr) {
  return getMetadataValue(MetadataType::Response, key, value_ptr);
}

inline WasmResult Context::getRequestMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr) {
  return getMetadataStruct(MetadataType::Request, name, struct_ptr);
}

inline WasmResult Context::getResponseMetadataStruct(StringView name, google::protobuf::Struct* struct_ptr) {
  return getMetadataStruct(MetadataType::Response, name, struct_ptr);
}

// Continue/Respond/Route
inline WasmResult continueRequest() { return proxy_continueRequest(); }
inline WasmResult continueResponse() { return proxy_continueResponse(); }
inline WasmResult sendLocalResponse(uint32_t response_code, StringView response_code_details, StringView body,
    const HeaderStringPairs& additional_response_headers, GrpcStatus grpc_status = GrpcStatus::InvalidCode) {
  const char* ptr = nullptr;
  size_t size = 0;
  exportPairs(additional_response_headers, &ptr, &size);
  return proxy_sendLocalResponse(response_code, response_code_details.data(), response_code_details.size(),
      body.data(), body.size(), ptr, size, static_cast<uint32_t>(grpc_status));
}
inline void clearRouteCache() { proxy_clearRouteCache(); }


// SharedData
inline WasmResult getSharedData(StringView key, WasmDataPtr* value, uint32_t* cas = nullptr) {
  uint32_t dummy_cas;
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  if (!cas)
    cas = &dummy_cas;
  auto result = proxy_getSharedData(key.data(), key.size(), &value_ptr, &value_size, cas);
  if (result != WasmResult::Ok) {
    return result;
  }
  *value = std::make_unique<WasmData>(value_ptr, value_size);
  return WasmResult::Ok;
}

inline WasmResult setSharedData(StringView key, StringView value, uint32_t cas = 0) {
  return proxy_setSharedData(key.data(), key.size(), value.data(), value.size(), cas);
}

inline WasmDataPtr getSharedDataValue(StringView key, uint32_t* cas = nullptr) {
  WasmDataPtr data;
  auto result = getSharedData(key, &data, cas);
  if (result != WasmResult::Ok) {
    logAbort("getSharedData returned WasmError: " + toString(result));
  }
  return data;
}

// SharedQueue
inline WasmResult registerSharedQueue(StringView queue_name, uint32_t* token) {
  return proxy_registerSharedQueue(queue_name.data(), queue_name.size(), token);
}

inline WasmResult resolveSharedQueue(StringView vm_id, StringView queue_name, uint32_t* token) {
  return proxy_resolveSharedQueue(vm_id.data(), vm_id.size(), queue_name.data(), queue_name.size(), token);
}

inline WasmResult enqueueSharedQueue(uint32_t token, StringView data) {
  return proxy_enqueueSharedQueue(token, data.data(), data.size());
}

inline WasmResult dequeueSharedQueue(uint32_t token, WasmDataPtr* data) {
  const char* data_ptr = nullptr;
  size_t data_size = 0;
  auto result = proxy_dequeueSharedQueue(token, &data_ptr, &data_size);
  *data = std::make_unique<WasmData>(data_ptr, data_size);
  return result;
}

// Headers/Trailers
inline WasmResult addHeaderMapValue(HeaderMapType type, StringView key, StringView value) {
  return proxy_addHeaderMapValue(type, key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getHeaderMapValue(HeaderMapType type, StringView key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getHeaderMapValue(type, key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceHeaderMapValue(HeaderMapType type, StringView key, StringView value) {
  proxy_replaceHeaderMapValue(type, key.data(), key.size(), value.data(), value.size());
}

inline void removeHeaderMapValue(HeaderMapType type, StringView key) {
  proxy_removeHeaderMapValue(type, key.data(), key.size());
}

inline WasmDataPtr getHeaderMapPairs(HeaderMapType type) {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getHeaderMapPairs(type, &ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline void setHeaderMapPairs(HeaderMapType type, const HeaderStringPairs &pairs) {
  const char* ptr = nullptr;
  size_t size = 0;
  exportPairs(pairs, &ptr, &size);
  proxy_setHeaderMapPairs(type, ptr, size);
}

inline WasmResult getHeaderMapSize(HeaderMapType type, size_t* size) {
  return proxy_getHeaderMapSize(type, size);
}

inline void addRequestHeader(StringView key, StringView value) {
  addHeaderMapValue(HeaderMapType::RequestHeaders, key, value);
}
inline WasmDataPtr getRequestHeader(StringView key) {
  return getHeaderMapValue(HeaderMapType::RequestHeaders, key);
}
inline void replaceRequestHeader(StringView key, StringView value) {
  replaceHeaderMapValue(HeaderMapType::RequestHeaders, key, value);
}
inline void removeRequestHeader(StringView key) {
  removeHeaderMapValue(HeaderMapType::RequestHeaders, key);
}
inline WasmDataPtr getRequestHeaderPairs() {
  return getHeaderMapPairs(HeaderMapType::RequestHeaders);
}
inline void setRequestHeaderPairs(const HeaderStringPairs &pairs) {
  return setHeaderMapPairs(HeaderMapType::RequestHeaders, pairs);
}
inline WasmResult getRequestHeaderSize(size_t* size) {
  return getHeaderMapSize(HeaderMapType::RequestHeaders, size);
}

inline void addRequestTrailer(StringView key, StringView value) {
  addHeaderMapValue(HeaderMapType::RequestTrailers, key, value);
}
inline WasmDataPtr getRequestTrailer(StringView key) {
  return getHeaderMapValue(HeaderMapType::RequestTrailers, key);
}
inline void replaceRequestTrailer(StringView key, StringView value) {
  replaceHeaderMapValue(HeaderMapType::RequestTrailers, key, value);
}
inline void removeRequestTrailer(StringView key) {
  removeHeaderMapValue(HeaderMapType::RequestTrailers, key);
}
inline WasmDataPtr getRequestTrailerPairs() {
  return getHeaderMapPairs(HeaderMapType::RequestTrailers);
}
inline void setRequestTrailerPairs(const HeaderStringPairs &pairs) {
  return setHeaderMapPairs(HeaderMapType::RequestTrailers, pairs);
}
inline WasmResult getRequestTrailerSize(size_t* size) {
  return getHeaderMapSize(HeaderMapType::RequestTrailers, size);
}

inline void addResponseHeader(StringView key, StringView value) {
  addHeaderMapValue(HeaderMapType::ResponseHeaders, key, value);
}
inline WasmDataPtr getResponseHeader(StringView key) {
  return getHeaderMapValue(HeaderMapType::ResponseHeaders, key);
}
inline void replaceResponseHeader(StringView key, StringView value) {
  replaceHeaderMapValue(HeaderMapType::ResponseHeaders, key, value);
}
inline void removeResponseHeader(StringView key) {
  removeHeaderMapValue(HeaderMapType::ResponseHeaders, key);
}
inline WasmDataPtr getResponseHeaderPairs() {
  return getHeaderMapPairs(HeaderMapType::ResponseHeaders);
}
inline void setResponseHeaderPairs(const HeaderStringPairs &pairs) {
  return setHeaderMapPairs(HeaderMapType::ResponseHeaders, pairs);
}
inline WasmResult getResponseHeaderSize(size_t* size) {
  return getHeaderMapSize(HeaderMapType::ResponseHeaders, size);
}

inline void addResponseTrailer(StringView key, StringView value) {
  addHeaderMapValue(HeaderMapType::ResponseTrailers, key, value);
}
inline WasmDataPtr getResponseTrailer(StringView key) {
  return getHeaderMapValue(HeaderMapType::ResponseTrailers, key);
}
inline void replaceResponseTrailer(StringView key, StringView value) {
  replaceHeaderMapValue(HeaderMapType::ResponseTrailers, key, value);
}
inline void removeResponseTrailer(StringView key) {
  removeHeaderMapValue(HeaderMapType::ResponseTrailers, key);
}
inline WasmDataPtr getResponseTrailerPairs() {
  return getHeaderMapPairs(HeaderMapType::ResponseTrailers);
}
inline void setResponseTrailerPairs(const HeaderStringPairs &pairs) {
  return setHeaderMapPairs(HeaderMapType::ResponseTrailers, pairs);
}
inline WasmResult getResponseTrailerSize(size_t* size) {
  return getHeaderMapSize(HeaderMapType::ResponseTrailers, size);
}

// Body
inline WasmDataPtr getRequestBodyBufferBytes(size_t start, size_t length) {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getRequestBodyBufferBytes(start, length, &ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline WasmDataPtr getResponseBodyBufferBytes(size_t start, size_t length) {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getResponseBodyBufferBytes(start, length, &ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

// HTTP

inline void MakeHeaderStringPairsBuffer(const HeaderStringPairs& headers, void** buffer_ptr,
                                        size_t* size_ptr) {
  if (headers.empty()) {
    *buffer_ptr = nullptr;
    *size_ptr = 0;
    return;
  }
  int size = 4; // number of headers
  for (auto& p : headers) {
    size += 8;                   // size of key, size of value
    size += p.first.size() + 1;  // null terminated key
    size += p.second.size() + 1; // null terminated value
  }
  char* buffer = static_cast<char*>(::malloc(size));
  char* b = buffer;
  *reinterpret_cast<int32_t*>(b) = headers.size();
  b += sizeof(int32_t);
  for (auto& p : headers) {
    *reinterpret_cast<int32_t*>(b) = p.first.size();
    b += sizeof(int32_t);
    *reinterpret_cast<int32_t*>(b) = p.second.size();
    b += sizeof(int32_t);
  }
  for (auto& p : headers) {
    memcpy(b, p.first.data(), p.first.size());
    b += p.first.size();
    *b++ = 0;
    memcpy(b, p.second.data(), p.second.size());
    b += p.second.size();
    *b++ = 0;
  }
  *buffer_ptr = buffer;
  *size_ptr = size;
}

inline uint32_t makeHttpCall(StringView uri, const HeaderStringPairs& request_headers,
                             StringView request_body, const HeaderStringPairs& request_trailers,
                             uint32_t timeout_milliseconds) {
  void *headers_ptr = nullptr, *trailers_ptr = nullptr;
  size_t headers_size = 0, trailers_size = 0;
  MakeHeaderStringPairsBuffer(request_headers, &headers_ptr, &headers_size);
  MakeHeaderStringPairsBuffer(request_trailers, &trailers_ptr, &trailers_size);
  uint32_t result =
      proxy_httpCall(uri.data(), uri.size(), headers_ptr, headers_size, request_body.data(),
                     request_body.size(), trailers_ptr, trailers_size, timeout_milliseconds);
  ::free(headers_ptr);
  ::free(trailers_ptr);
  return result;
}

// Low level metrics interface.

inline WasmResult defineMetric(MetricType type, StringView name, uint32_t* metric_id) {
  return proxy_defineMetric(type, name.data(), name.size(), metric_id);
}

inline WasmResult incrementMetric(uint32_t metric_id, int64_t offset) {
  return proxy_incrementMetric(metric_id, offset);
}

inline WasmResult recordMetric(uint32_t metric_id, uint64_t value) {
  return proxy_recordMetric(metric_id, value);
}

inline WasmResult getMetric(uint32_t metric_id, uint64_t* value) { return proxy_getMetric(metric_id, value); }

// Higher level metrics interface.

struct MetricTag {
  enum class TagType : uint32_t {
    String = 0,
    Int = 1,
    Bool = 2,
  };
  std::string name;
  TagType tagType;
};

struct MetricBase {
  MetricBase(MetricType t, const std::string& n) : type(t), name(n) {}
  MetricBase(MetricType t, const std::string& n, std::vector<MetricTag> ts)
      : type(t), name(n), tags(ts.begin(), ts.end()){}
  MetricBase(MetricType t, const std::string& n, std::vector<MetricTag> ts, std::string  fs, std::string vs)
      : type(t), name(n), tags(ts.begin(), ts.end()), field_separator(fs), value_separator(vs) {}

  MetricType type;
  std::string name;
  std::string prefix;
  std::vector<MetricTag> tags;
  std::unordered_map<std::string, uint32_t> metric_ids;

  std::string field_separator = ".";  // used to separate two fields.
  std::string value_separator = ".";  // used to separate a field from its value.

  std::string prefixWithFields(const std::vector<std::string>& fields);
  uint32_t resolveFullName(const std::string& n);
  uint32_t resolveWithFields(const std::vector<std::string>& fields);
  void partiallyResolveWithFields(const std::vector<std::string>& fields);
  std::string nameFromIdSlow(uint32_t id);
};

struct Metric : public MetricBase {
  Metric(MetricType t, const std::string& n) : MetricBase(t, n) {}
  Metric(MetricType t, const std::string& n, std::vector<MetricTag> ts) : MetricBase(t, n, ts) {}
  Metric(MetricType t, const std::string& n, std::vector<MetricTag> ts, std::string field_separator, std::string value_separator) : MetricBase(t, n, ts, field_separator, value_separator) {}

  template <typename... Fields> void increment(int64_t offset, Fields... tags);
  template <typename... Fields> void record(uint64_t value, Fields... tags);
  template <typename... Fields> uint64_t get(Fields... tags);
  template <typename... Fields> uint32_t resolve(Fields... tags);
  template <typename... Fields> Metric partiallyResolve(Fields... tags);
};

inline std::string MetricBase::prefixWithFields(const std::vector<std::string>& fields) {
  size_t s = prefix.size();
  for (size_t i = 0; i < fields.size(); i++) {
    s += tags[i].name.size() + 1;  // 1 more for "."
  }
  for (auto& f : fields) {
    s += f.size() + 1;  // 1 more for "."
  }
  std::string n;
  n.reserve(s);
  n.append(prefix);
  for (size_t i = 0; i < fields.size(); i++) {
    n.append(tags[i].name);
    n.append(value_separator);
    n.append(fields[i]);
    n.append(field_separator);
  }
  return n;
}

inline uint32_t MetricBase::resolveWithFields(const std::vector<std::string>& fields) {
  if (fields.size() != tags.size()) {
#if WASM_EXCEPTIONS
    throw ProxyException("metric fields.size() != tags.size()");
#else
    logAbort("metric fields.size() != tags.size()");
#endif
  }
  return resolveFullName(prefixWithFields(fields) + name);
}

inline void MetricBase::partiallyResolveWithFields(const std::vector<std::string>& fields) {
  if (fields.size() >= tags.size()) {
#if WASM_EXCEPTIONS
    throw ProxyException("metric fields.size() >= tags.size()");
#else
    logAbort("metric fields.size() >= tags.size()");
#endif
  }
  prefix = prefixWithFields(fields);
  tags.erase(tags.begin(), tags.begin()+(fields.size()));
}

template <typename T> inline std::string toString(T t) { return std::to_string(t); }

template <> inline std::string toString(StringView t) { return std::string(t); }

template <> inline std::string toString(const char* t) { return std::string(t); }

template <> inline std::string toString(std::string t) { return t; }

template <> inline std::string toString(bool t) { return t ? "true" : "false"; }

template <typename T> struct StringToStringView {
  typedef T type;
};

template <> struct StringToStringView<std::string> {
  typedef StringView type;
};

inline uint32_t MetricBase::resolveFullName(const std::string& n) {
  auto it = metric_ids.find(n);
  if (it == metric_ids.end()) {
    uint32_t metric_id;
    CHECK_RESULT(defineMetric(type, n, &metric_id));
    metric_ids[n] = metric_id;
    return metric_id;
  }
  return it->second;
}

inline std::string MetricBase::nameFromIdSlow(uint32_t id) {
  for (auto& p : metric_ids)
    if (p.second == id)
      return p.first;
  return "";
}

template <typename... Fields> inline uint32_t Metric::resolve(Fields... f) {
  std::vector<std::string> fields{toString(f)...};
  return resolveWithFields(fields);
}

template <typename... Fields> Metric Metric::partiallyResolve(Fields... f) {
  std::vector<std::string> fields{toString(f)...};
  Metric partial_metric(*this);
  partial_metric.partiallyResolveWithFields(fields);
  return partial_metric;
}

template <typename... Fields> inline void Metric::increment(int64_t offset, Fields... f) {
  std::vector<std::string> fields{toString(f)...};
  auto metric_id = resolveWithFields(fields);
  incrementMetric(metric_id, offset);
}

template <typename... Fields> inline void Metric::record(uint64_t value, Fields... f) {
  std::vector<std::string> fields{toString(f)...};
  auto metric_id = resolveWithFields(fields);
  recordMetric(metric_id, value);
}

template <typename... Fields> inline uint64_t Metric::get(Fields... f) {
  std::vector<std::string> fields{toString(f)...};
  auto metric_id = resolveWithFields(fields);
  uint64_t value;
  CHECK_RESULT(getMetric(metric_id, &value));
  return value;
}

template <typename T> struct MetricTagDescriptor {
  MetricTagDescriptor(StringView n) : name(n) {}
  MetricTagDescriptor(const char* n) : name(n) {}
  typedef T type;
  StringView name;
};

template <typename T> inline MetricTag toMetricTag(const MetricTagDescriptor<T>&) { return {}; }

template <> inline MetricTag toMetricTag(const MetricTagDescriptor<const char*>& d) {
  return {std::string(d.name), MetricTag::TagType::String};
}

template <> inline MetricTag toMetricTag(const MetricTagDescriptor<std::string>& d) {
  return {std::string(d.name), MetricTag::TagType::String};
}

template <> inline MetricTag toMetricTag(const MetricTagDescriptor<StringView>& d) {
  return {std::string(d.name), MetricTag::TagType::String};
}

template <> inline MetricTag toMetricTag(const MetricTagDescriptor<int>& d) {
  return {std::string(d.name), MetricTag::TagType::Int};
}

template <> inline MetricTag toMetricTag(const MetricTagDescriptor<bool>& d) {
  return {std::string(d.name), MetricTag::TagType::Bool};
}

struct SimpleCounter {
  SimpleCounter(uint32_t id) : metric_id(id) {}

  void increment(int64_t offset) { recordMetric(metric_id, offset); }
  void record(int64_t offset) { increment(offset); }
  uint64_t get() {
    uint64_t value;
    CHECK_RESULT(getMetric(metric_id, &value));
    return value;
  }
  void operator++() { increment(1); }
  void operator++(int) { increment(1); }

  uint32_t metric_id;
};

struct SimpleGauge {
  SimpleGauge(uint32_t id) : metric_id(id) {}

  void record(uint64_t offset) { recordMetric(metric_id, offset); }
  uint64_t get() {
    uint64_t value;
    CHECK_RESULT(getMetric(metric_id, &value));
    return value;
  }

  uint32_t metric_id;
};

struct SimpleHistogram {
  SimpleHistogram(uint32_t id) : metric_id(id) {}

  void record(int64_t offset) { recordMetric(metric_id, offset); }

  uint32_t metric_id;
};

template <typename... Tags> struct Counter : public MetricBase {
  static Counter<Tags...>* New(StringView name, MetricTagDescriptor<Tags>... fieldnames);

  Counter<Tags...>(StringView name, MetricTagDescriptor<Tags>... descriptors) :
    Counter<Tags...>(std::string(name), std::vector<MetricTag>({toMetricTag(descriptors)...})) {}

  SimpleCounter resolve(Tags... f) {
    std::vector<std::string> fields{toString(f)...};
    return SimpleCounter(resolveWithFields(fields));
  }

  template<typename... AdditionalTags>
  Counter<AdditionalTags...>* extendAndResolve(Tags... f, MetricTagDescriptor<AdditionalTags>... fieldnames) {
    std::vector<std::string> fields{toString(f)...};
    auto new_counter = Counter<AdditionalTags...>::New(name, fieldnames...);
    new_counter->prefix = prefixWithFields(fields);
    return new_counter;
  }

  void increment(int64_t offset, Tags... tags) {
    std::vector<std::string> fields{toString(tags)...};
    auto metric_id = resolveWithFields(fields);
    incrementMetric(metric_id, offset);
  }

  void record(int64_t offset, Tags... tags) { increment(offset, tags...); }

  uint64_t get(Tags... tags) {
    std::vector<std::string> fields{toString(tags)...};
    auto metric_id = resolveWithFields(fields);
    uint64_t value;
    CHECK_RESULT(getMetric(metric_id, &value));
    return value;
  }

private:
  Counter(const std::string& name, std::vector<MetricTag> tags)
      : MetricBase(MetricType::Counter, name, tags) {}
};

template <typename... Tags>
inline Counter<Tags...>* Counter<Tags...>::New(StringView name,
                                               MetricTagDescriptor<Tags>... descriptors) {
  return new Counter<Tags...>(std::string(name),
                              std::vector<MetricTag>({toMetricTag(descriptors)...}));
}

template <typename... Tags> struct Gauge : public MetricBase {
  static Gauge<Tags...>* New(StringView name, MetricTagDescriptor<Tags>... fieldnames);

  Gauge<Tags...>(StringView name, MetricTagDescriptor<Tags>... descriptors) :
    Gauge<Tags...>(std::string(name), std::vector<MetricTag>({toMetricTag(descriptors)...})) {}

  SimpleGauge resolve(Tags... f) {
    std::vector<std::string> fields{toString(f)...};
    return SimpleGauge(resolveWithFields(fields));
  }

  template<typename... AdditionalTags>
  Gauge<AdditionalTags...>* extendAndResolve(Tags... f, MetricTagDescriptor<AdditionalTags>... fieldnames) {
    std::vector<std::string> fields{toString(f)...};
    auto new_gauge = Gauge<AdditionalTags...>::New(name, fieldnames...);
    new_gauge->prefix = prefixWithFields(fields);
    return new_gauge;
  }

  void record(int64_t offset, typename StringToStringView<Tags>::type... tags) {
    std::vector<std::string> fields{toString(tags)...};
    auto metric_id = resolveWithFields(fields);
    recordMetric(metric_id, offset);
  }

  uint64_t get(Tags... tags) {
    std::vector<std::string> fields{toString(tags)...};
    auto metric_id = resolveWithFields(fields);
    uint64_t value;
    CHECK_RESULT(getMetric(metric_id, &value));
    return value;
  }

private:
  Gauge(const std::string& name, std::vector<MetricTag> tags)
      : MetricBase(MetricType::Gauge, name, tags) {}
};

template <typename... Tags>
inline Gauge<Tags...>* Gauge<Tags...>::New(StringView name,
                                           MetricTagDescriptor<Tags>... descriptors) {
  return new Gauge<Tags...>(std::string(name),
                            std::vector<MetricTag>({toMetricTag(descriptors)...}));
}

template <typename... Tags> struct Histogram : public MetricBase {
  static Histogram<Tags...>* New(StringView name, MetricTagDescriptor<Tags>... fieldnames);

  Histogram<Tags...>(StringView name, MetricTagDescriptor<Tags>... descriptors) :
    Histogram<Tags...>(std::string(name), std::vector<MetricTag>({toMetricTag(descriptors)...})) {}

  SimpleHistogram resolve(Tags... f) {
    std::vector<std::string> fields{toString(f)...};
    return SimpleHistogram(resolveWithFields(fields));
  }

  template<typename... AdditionalTags>
  Histogram<AdditionalTags...>* extendAndResolve(Tags... f, MetricTagDescriptor<AdditionalTags>... fieldnames) {
    std::vector<std::string> fields{toString(f)...};
    auto new_histogram = Histogram<AdditionalTags...>::New(name, fieldnames...);
    new_histogram->prefix = prefixWithFields(fields);
    return new_histogram;
  }

  void record(int64_t offset, typename StringToStringView<Tags>::type... tags) {
    std::vector<std::string> fields{toString(tags)...};
    auto metric_id = resolveWithFields(fields);
    recordMetric(metric_id, offset);
  }

private:
  Histogram(const std::string& name, std::vector<MetricTag> tags)
      : MetricBase(MetricType::Histogram, name, tags) {}
};

template <typename... Tags>
inline Histogram<Tags...>* Histogram<Tags...>::New(StringView name,
                                                   MetricTagDescriptor<Tags>... descriptors) {
  return new Histogram<Tags...>(std::string(name),
                                std::vector<MetricTag>({toMetricTag(descriptors)...}));
}

inline uint32_t grpcCall(StringView service, StringView service_name, StringView method_name,
    const google::protobuf::MessageLite &request, uint32_t timeout_milliseconds) {
  std::string serialized_request;
  request.SerializeToString(&serialized_request);
  return proxy_grpcCall(service.data(), service.size(), service_name.data(), service_name.size(), method_name.data(), method_name.size(),
      serialized_request.data(), serialized_request.size(), timeout_milliseconds);
                         
}

inline uint32_t grpcStream(StringView service, StringView service_name, StringView method_name) {
  return proxy_grpcStream(service.data(), service.size(), service_name.data(), service_name.size(), method_name.data(), method_name.size());
}

inline WasmResult grpcCancel(uint32_t token) {
  return proxy_grpcCancel(token);
}

inline WasmResult grpcClose(uint32_t token) {
  return proxy_grpcClose(token);
}

inline WasmResult grpcSend(uint32_t token, StringView message, bool end_stream) {
  return proxy_grpcSend(token, message.data(), message.size(), end_stream ? 1 : 0);
}

inline bool RootContext::httpCall(StringView uri, const HeaderStringPairs& request_headers,
    StringView request_body, const HeaderStringPairs& request_trailers,
    uint32_t timeout_milliseconds, HttpCallCallback callback) {
  auto token = makeHttpCall(uri, request_headers, request_body, request_trailers, timeout_milliseconds);
  if (token) {
    http_calls_[token] = std::move(callback);
    return true;
  }
  return false;
}

inline void RootContext::onHttpCallResponse(uint32_t token, std::unique_ptr<WasmData> header_pairs,
    std::unique_ptr<WasmData> body, std::unique_ptr<WasmData> trailer_pairs) {
  auto it = http_calls_.find(token);
  if (it != http_calls_.end()) {
    it->second(std::move(header_pairs), std::move(body), std::move(trailer_pairs));
    http_calls_.erase(token);
  }
}

inline bool RootContext::grpcSimpleCall(StringView service, StringView service_name, StringView method_name,
    const google::protobuf::MessageLite &request, uint32_t timeout_milliseconds, Context::GrpcSimpleCallCallback callback) {
  auto token = grpcCall(service, service_name, method_name, request, timeout_milliseconds);
  if (token) {
    asRoot()->simple_grpc_calls_[token] = std::move(callback);
    return true;
  }
  return false;
}

inline void GrpcCallHandlerBase::cancel() {
  grpcCancel(token_);
  context_->grpc_calls_.erase(token_);
}

inline void GrpcStreamHandlerBase::reset() {
  grpcCancel(token_);
  context_->grpc_streams_.erase(token_);
}

inline void GrpcStreamHandlerBase::close() {
  grpcClose(token_);
  local_close_ = true;
  if (local_close_ && remote_close_) {
    context_->grpc_streams_.erase(token_);
  }
  // NB: else callbacks can still occur: reset() to prevent further callbacks.
}

inline void GrpcStreamHandlerBase::send(StringView message, bool end_of_stream) {
  grpcSend(token_, message, end_of_stream);
  if (end_of_stream) {
    // NB: callbacks can still occur: reset() to prevent further callbacks.
    local_close_ = local_close_ || end_of_stream;
    if (local_close_ && remote_close_) {
      context_->grpc_streams_.erase(token_);
    }
  }
}

inline void RootContext::onGrpcCreateInitialMetadata(uint32_t token) {
  {
    auto it = grpc_calls_.find(token);
    if (it != grpc_calls_.end()) {
      it->second->onCreateInitialMetadata();
      return;
    }
  }
  {
    auto it = grpc_streams_.find(token);
    if (it != grpc_streams_.end()) {
      it->second->onCreateInitialMetadata();
      return;
    }
  }
}

inline void RootContext::onGrpcReceiveInitialMetadata(uint32_t token) {
  {
    auto it = grpc_streams_.find(token);
    if (it != grpc_streams_.end()) {
      it->second->onReceiveInitialMetadata();
      return;
    }
  }
}

inline void RootContext::onGrpcReceiveTrailingMetadata(uint32_t token) {
  {
    auto it = grpc_streams_.find(token);
    if (it != grpc_streams_.end()) {
      it->second->onReceiveTrailingMetadata();
      return;
    }
  }
}

inline void RootContext::onGrpcReceive(uint32_t token, std::unique_ptr<WasmData> message) {
  {
    auto it = simple_grpc_calls_.find(token);
    if (it != simple_grpc_calls_.end()) {
      it->second(GrpcStatus::Ok, std::move(message));
      simple_grpc_calls_.erase(token);
      return;
    }
  }
  {
    auto it = grpc_calls_.find(token);
    if (it != grpc_calls_.end()) {
      it->second->onSuccess(std::move(message));
      grpc_calls_.erase(token);
      return;
    }
  }
  {
    auto it = grpc_streams_.find(token);
    if (it != grpc_streams_.end()) {
      it->second->onReceive(std::move(message));
      grpc_streams_.erase(token);
      return;
    }
  }
}

inline void GrpcStreamHandlerBase::doRemoteClose(GrpcStatus status, std::unique_ptr<WasmData> error_message) {
  auto context = context_;
  auto token = token_;
  this->onRemoteClose(status, std::move(error_message));
  if (context->grpc_streams_.find(token) != context->grpc_streams_.end()) {
    // We have not been deleted, e.g. by reset() in the onRemoteCall() virtual handler.
    remote_close_ = true;
    if (local_close_ && remote_close_) {
      context_->grpc_streams_.erase(token_);
    }
    // else do not erase the token since we can still send in this state.
  }
}

inline void RootContext::onGrpcClose(uint32_t token, GrpcStatus status, std::unique_ptr<WasmData> message) {
  {
    auto it = simple_grpc_calls_.find(token);
    if (it != simple_grpc_calls_.end()) {
      it->second(status, std::move(message));
      simple_grpc_calls_.erase(token);
      return;
    }
  }
  {
    auto it = grpc_calls_.find(token);
    if (it != grpc_calls_.end()) {
      it->second->onFailure(status, std::move(message));
      grpc_calls_.erase(token);
      return;
    }
  }
  {
    auto it = grpc_streams_.find(token);
    if (it != grpc_streams_.end()) {
      it->second->doRemoteClose(status, std::move(message));
      return;
    }
  }
}

inline bool RootContext::grpcCallHandler(StringView service, StringView service_name,
    StringView method_name, const google::protobuf::MessageLite &request, uint32_t timeout_milliseconds,
    std::unique_ptr<GrpcCallHandlerBase> handler) {
  auto token = grpcCall(service, service_name, method_name, request, timeout_milliseconds);
  if (token) {
    handler->token_ = token;
    handler->context_ = this;
    grpc_calls_[token] = std::move(handler);
    return true;
  }
  return false;
}

inline bool RootContext::grpcStreamHandler(StringView service, StringView service_name,
    StringView method_name, std::unique_ptr<GrpcStreamHandlerBase> handler) {
  auto token = grpcStream(service, service_name, method_name);
  if (token) {
    handler->token_ = token;
    handler->context_ = this;
    grpc_streams_[token] = std::move(handler);
    return true;
  }
  return false;
}

inline WasmResult ContextBase::setEffectiveContext() {
  return proxy_setEffectiveContext(id_);
}

inline uint64_t getCurrentTimeNanoseconds() {
  uint64_t t;
  CHECK_RESULT(proxy_getCurrentTimeNanoseconds(&t));
  return t;
}
