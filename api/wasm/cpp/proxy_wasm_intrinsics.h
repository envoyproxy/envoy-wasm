/*
 * Intrinsic functions available to WASM modules.
 */
// NOLINT(namespace-envoy)
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef EMSCRIPTEN_PROTOBUF_LITE
#include "proxy_wasm_intrinsics.pb.h"
#else
#include "proxy_wasm_intrinsics_lite.pb.h"
#endif

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE __attribute__((used)) __attribute__((visibility("default")))
#endif

// clang-format off
/*
   API Calls into the VM.

   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(char* configuration, int size);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart();
   extern "C" EMSCRIPTEN_KEEPALIVE int main();  // only called if proxy_onStart() is not available.
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick();
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onCreate(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onRequestHeaders(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onRequestBody(uint32_t context_id,  uint32_t body_buffer_length, uint32_t end_of_stream);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onRequestTrailers(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onRequestMetadata(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onResponseHeaders(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onResponseBody(uint32_t context_id,  uint32_t body_buffer_length, uint32_t end_of_stream);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onResponseTrailers(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onResponseMetadata(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onHttpCallResponse(uint32_t context_id uint32_t token, uint32_t header_pairs_ptr,
     uint32_t header_pairs_size, uint32_t body_ptr, uint32_t body_size, uint32_t trailer_pairs_ptr, uint32_t trailer_pairs_size):
   // The stream has completed.
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDone(uint32_t context_id);
   // onLog occurs after onDone.
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onLog(uint32_t context_id);
   // The Context in the proxy has been destroyed and no further calls will be coming.
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDelete(uint32_t context_id);
*/
// clang-format on

enum class LogLevel : int { trace, debug, info, warn, error, critical };
extern "C" void proxy_log(LogLevel level, const char* logMessage, size_t messageSize);

extern "C" void proxy_setTickPeriodMilliseconds(uint32_t millisecond);

extern "C" uint64_t proxy_getCurrentTimeNanoseconds();

//
// Low Level API.
//
enum class FilterHeadersStatus : int { Continue = 0, StopIteration = 1 };
enum class FilterTrailersStatus : int { Continue = 0, StopIteration = 1 };
enum class FilterDataStatus : int {
  Continue = 0,
  StopIterationAndBuffer = 1,
  StopIterationAndWatermark = 2,
  StopIterationNoBuffer = 3
};
enum class StreamType : int { Request = 0, Response = 1 };
enum class MetadataType : int {
  Request = 0,
  Response = 1,
  RequestRoute = 2,   // Immutable
  ResponseRoute = 3,  // Immutable
  Log = 4             // Immutable
};

// Stream Info

extern "C" void proxy_getProtocol(StreamType type, const char** value_ptr, size_t* value_size);

// Metadata
extern "C" void proxy_getMetadata(MetadataType type, const char* key_ptr, size_t key_size,
                                  const char** value_ptr_ptr, size_t* value_size_ptr);
extern "C" void proxy_setMetadata(MetadataType type, const char* key_ptr, size_t key_size,
                                  const char* value_ptr, size_t value_size);
extern "C" void proxy_getMetadataPairs(MetadataType type, const char** value_ptr,
                                       size_t* value_size);
extern "C" void proxy_getMetadataStruct(MetadataType type, const char* name_ptr, size_t name_size,
                                        const char** value_ptr_ptr, size_t* value_size_ptr);
extern "C" void proxy_setMetadataStruct(MetadataType type, const char* name_ptr, size_t name_size,
                                        const char* value_ptr, size_t value_size);

// Continue
extern "C" void proxy_continueRequest();
extern "C" void proxy_continueResponse();

// SharedData
extern "C" void proxy_getSharedData(const char* key_ptr, size_t key_size, const char** value_ptr,
                                    size_t* value_size, uint32_t* cas);
//  If cas != 0 and cas != the current cas for 'key' return false, otherwise set the value and
//  return true.
extern "C" bool proxy_setSharedData(const char* key_ptr, size_t key_size, const char* value_ptr,
                                    size_t value_size, uint32_t cas);

// Headers/Trailers
extern "C" void proxy_addRequestHeader(const char* key_ptr, size_t key_size, const char* value_ptr,
                                       size_t value_size);
extern "C" void proxy_getRequestHeader(const char* key_ptr, size_t key_size, const char** value_ptr,
                                       size_t* value_size);
extern "C" void proxy_getRequestHeaderPairs(const char** ptr, size_t* size);
extern "C" void proxy_replaceRequestHeader(const char* key_ptr, size_t key_size,
                                           const char* value_ptr, size_t value_size);
extern "C" void proxy_removeRequestHeader(const char* key_ptr, size_t key_size);

extern "C" void proxy_addRequestTrailer(const char* key_ptr, size_t key_size, const char* value_ptr,
                                        size_t value_size);
extern "C" void proxy_getRequestTrailer(const char* key_ptr, size_t key_size,
                                        const char** value_ptr, size_t* value_size);
extern "C" void proxy_getRequestTrailerPairs(const char** ptr, size_t* size);
extern "C" void proxy_replaceRequestTrailer(const char* key_ptr, size_t key_size,
                                            const char* value_ptr, size_t value_size);
extern "C" void proxy_removeRequestTrailer(const char* key_ptr, size_t key_size);

extern "C" void proxy_addResponseHeader(const char* key_ptr, size_t key_size, const char* value_ptr,
                                        size_t value_size);
extern "C" void proxy_getResponseHeader(const char* key_ptr, size_t key_size,
                                        const char** value_ptr, size_t* value_size);
extern "C" void proxy_getResponseHeaderPairs(const char** ptr, size_t* size);
extern "C" void proxy_replaceResponseHeader(const char* key_ptr, size_t key_size,
                                            const char* value_ptr, size_t value_size);
extern "C" void proxy_removeResponseHeader(const char* key_ptr, size_t key_size);

extern "C" void proxy_addResponseTrailer(const char* key_ptr, size_t key_size,
                                         const char* value_ptr, size_t value_size);
extern "C" void proxy_getResponseTrailer(const char* key_ptr, size_t key_size,
                                         const char** value_ptr, size_t* value_size);
extern "C" void proxy_getResponseTrailerPairs(const char** ptr, size_t* size);
extern "C" void proxy_replaceResponseTrailer(const char* key_ptr, size_t key_size,
                                             const char* value_ptr, size_t value_size);
extern "C" void proxy_removeResponseTrailer(const char* key_ptr, size_t key_size);

// Body
extern "C" void proxy_getRequestBodyBufferBytes(uint32_t start, uint32_t length, const char** ptr,
                                                size_t* size);
extern "C" void proxy_getResponseBodyBufferBytes(uint32_t start, uint32_t length, const char** ptr,
                                                 size_t* size);

// HTTP
// Returns token, used in callback onHttpCallResponse
extern "C" uint32_t proxy_httpCall(const char* uri_ptr, size_t uri_size, void* header_pairs_ptr,
                                   size_t header_pairs_size, const char* body_ptr, size_t body_size,
                                   void* trailer_pairs_ptr, size_t trailer_pairs_size,
                                   uint32_t timeout_milliseconds);

// Metrics

enum class MetricType : uint32_t {
  Counter = 0,
  Gauge = 1,
  Histogram = 2,
};
// Returns a metric_id which can be used to report a metric. On error returns 0.
extern "C" uint32_t proxy_defineMetric(MetricType type, const char* name_ptr, size_t name_size);
extern "C" void proxy_incrementMetric(uint32_t metric_id, int64_t offset);
extern "C" void proxy_recordMetric(uint32_t metric_id, uint64_t value);
extern "C" uint64_t proxy_getMetric(uint32_t metric_id);

//
// High Level C++ API.
//

class ProxyException : std::runtime_error {
public:
  ProxyException(const std::string& message) : std::runtime_error(message) {}
};

inline void logTrace(const std::string& logMessage) {
  proxy_log(LogLevel::trace, logMessage.c_str(), logMessage.size());
}
inline void logDebug(const std::string& logMessage) {
  proxy_log(LogLevel::debug, logMessage.c_str(), logMessage.size());
}
inline void logInfo(const std::string& logMessage) {
  proxy_log(LogLevel::info, logMessage.c_str(), logMessage.size());
}
inline void logWarn(const std::string& logMessage) {
  proxy_log(LogLevel::warn, logMessage.c_str(), logMessage.size());
}
inline void logError(const std::string& logMessage) {
  proxy_log(LogLevel::error, logMessage.c_str(), logMessage.size());
}
inline void logCritical(const std::string& logMessage) {
  proxy_log(LogLevel::critical, logMessage.c_str(), logMessage.size());
}

// Buffers coming into the WASM filter.
class WasmData {
public:
  WasmData(const char* data, size_t size) : data_(data), size_(size) {}
  ~WasmData() { ::free((void*)data_); }
  const char* data() { return data_; }
  std::string_view view() { return {data_, size_}; }
  std::string toString() { return std::string(view()); }
  std::vector<std::pair<std::string_view, std::string_view>> pairs();

  WasmData& operator=(const WasmData&) = delete;
  WasmData(const WasmData&) = delete;

private:
  const char* data_;
  size_t size_;
};
typedef std::unique_ptr<WasmData> WasmDataPtr;

inline std::vector<std::pair<std::string_view, std::string_view>> WasmData::pairs() {
  std::vector<std::pair<std::string_view, std::string_view>> result;
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
    result[i].first = std::string_view(s, size);
    s += size + 1;
    size = *reinterpret_cast<const int*>(p);
    p += sizeof(int);
    result[i].second = std::string_view(s, size);
    s += size + 1;
  }
  return result;
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

// Context for a stream.  The distinguished context id == 0 is used for non-stream calls.
class Context {
public:
  explicit Context(uint32_t id) : id_(id) {}
  virtual ~Context() {}

  uint32_t id() { return id_; }

  static std::unique_ptr<Context> New(uint32_t id); // For subclassing.

  // Called once when the filter loads and on configuration changes.
  virtual void onConfigure(std::unique_ptr<WasmData> configuration) {}
  // Called once when the filter loads.
  virtual void onStart() {}

  // Called on individual requests/response streams.
  virtual void onCreate() {}
  virtual FilterHeadersStatus onRequestHeaders() { return FilterHeadersStatus::Continue; }
  virtual FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) {
    return FilterDataStatus::Continue;
  }
  virtual FilterTrailersStatus onRequestTrailers() { return FilterTrailersStatus::Continue; }
  virtual FilterHeadersStatus onResponseHeaders() { return FilterHeadersStatus::Continue; }
  virtual FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) {
    return FilterDataStatus::Continue;
  }
  virtual FilterTrailersStatus onResponseTrailers() { return FilterTrailersStatus::Continue; }
  virtual void onDone() {}
  virtual void onLog() {}
  virtual void onDelete() {}
  virtual void onTick() {}

  virtual void onHttpCallResponse(uint32_t token, std::unique_ptr<WasmData> header_pairs,
                                  std::unique_ptr<WasmData> body,
                                  std::unique_ptr<WasmData> trailer_pairs) {}

  // Metadata
  bool isImmutable(MetadataType type);
  virtual bool isProactivelyCachable(MetadataType type);  // Cache all keys on any read.
  // Caching Metadata calls.  Note: "name" refers to the metadata namespace.
  google::protobuf::Value metadataValue(MetadataType type, std::string_view key);
  google::protobuf::Value requestRouteMetadataValue(std::string_view key);
  google::protobuf::Value responseRouteMetadataValue(std::string_view key);
  google::protobuf::Value logMetadataValue(std::string_view key);
  google::protobuf::Value requestMetadataValue(std::string_view key);
  google::protobuf::Value responseMetadataValue(std::string_view key);
  google::protobuf::Value namedMetadataValue(MetadataType type, std::string_view name, std::string_view key);
  google::protobuf::Value requestMetadataValue(std::string_view name, std::string_view key);
  google::protobuf::Value responseMetadataValue(std::string_view name, std::string_view key);
  google::protobuf::Struct metadataStruct(MetadataType type, std::string_view name = "");
  google::protobuf::Struct requestRouteMetadataStruct();
  google::protobuf::Struct responseRouteMetadataStruct();
  google::protobuf::Struct logMetadataStruct(std::string_view name = "");
  google::protobuf::Struct requestMetadataStruct(std::string_view name = "");
  google::protobuf::Struct responseMetadataStruct(std::string_view name = "");
  // Uncached Metadata calls.
  google::protobuf::Value getRequestMetadataValue(std::string_view key);
  google::protobuf::Value getResponseMetadataValue(std::string_view key);
  google::protobuf::Struct getRequestMetadataStruct(std::string_view name);
  google::protobuf::Struct getResponseMetadataStruct(std::string_view name);

private:
  uint32_t id_;
  std::unordered_map<std::pair<MetadataType, std::string>, google::protobuf::Value, PairHash> value_cache_;
  std::unordered_map<std::tuple<MetadataType, std::string, std::string>, google::protobuf::Value, Tuple3Hash> name_value_cache_;
  std::unordered_map<std::pair<MetadataType, std::string>, google::protobuf::Struct, PairHash> struct_cache_;
};

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
inline bool Context::isProactivelyCachable(MetadataType type) {
  return false;
}

// StreamInfo
inline WasmDataPtr getProtocol(StreamType type) {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getProtocol(type, &ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

// Metadata
inline WasmDataPtr getMetadata(MetadataType type, std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getMetadata(type, key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline google::protobuf::Value getMetadataValue(MetadataType type, std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getMetadata(type, key.data(), key.size(), &value_ptr, &value_size);
  if (!value_size) {
    return {};
  }
  google::protobuf::Value value;
  if (!value.ParseFromArray(value_ptr, value_size)) {
    return {};
  }
  return value;
}

inline std::string getMetadataStringValue(MetadataType type, std::string_view key) {
  return getMetadataValue(type, key).string_value();
}

inline void setMetadata(MetadataType type, std::string_view key, std::string_view value) {
  proxy_setMetadata(type, key.data(), key.size(), value.data(), value.size());
}

inline void setMetadataValue(MetadataType type, std::string_view key,
                             const google::protobuf::Value& value) {
  std::string output;
  if (!value.SerializeToString(&output)) {
    return;
  }
  proxy_setMetadata(type, key.data(), key.size(), output.data(), output.size());
}

inline void setMetadataStringValue(MetadataType type, std::string_view key, std::string_view s) {
  google::protobuf::Value value;
  value.set_string_value(s.data(), s.size());
  setMetadataValue(type, key, value);
}

inline WasmDataPtr getMetadataValuePairs(MetadataType type) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getMetadataPairs(type, &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline google::protobuf::Struct getMetadataStruct(MetadataType type, std::string_view name) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getMetadataStruct(type, name.data(), name.size(), &value_ptr, &value_size);
  if (!value_size) {
    return {};
  }
  google::protobuf::Struct s;
  if (!s.ParseFromArray(value_ptr, value_size)) {
    return {};
  }
  return s;
}

inline void setMetadataStruct(MetadataType type, std::string_view name,
                             const google::protobuf::Struct& s) {
  std::string output;
  if (!s.SerializeToString(&output)) {
    return;
  }
  proxy_setMetadataStruct(type, name.data(), name.size(), output.data(), output.size());
}

inline google::protobuf::Value Context::metadataValue(MetadataType type, std::string_view key) {
  auto cache_key = std::make_pair(type, std::string(key));
  auto it = value_cache_.find(cache_key);
  if (it != value_cache_.end()) {
    return it->second;
  }
  if (isProactivelyCachable(type)) {
    auto values = getMetadataValuePairs(type);
    for (auto &p : values->pairs()) {
      google::protobuf::Value value;
      if (value.ParseFromArray(p.second.data(), p.second.size())) {
        auto k = std::make_pair(type, p.first);
        value_cache_[cache_key] = value;
      }
    }
    auto it = value_cache_.find(cache_key);
    if (it != value_cache_.end()) {
      return it->second;
    }
    return {};
  } else {
    auto value = getMetadataValue(type, key);
    value_cache_[cache_key] = value;
    return value;
  }
}

inline google::protobuf::Value Context::requestRouteMetadataValue(std::string_view key) {
  return metadataValue(MetadataType::RequestRoute, key);
}

inline google::protobuf::Value Context::responseRouteMetadataValue(std::string_view key) {
  return metadataValue(MetadataType::ResponseRoute, key);
}

inline google::protobuf::Value Context::logMetadataValue(std::string_view key) {
  return metadataValue(MetadataType::Log, key);
}

inline google::protobuf::Value Context::requestMetadataValue(std::string_view key) {
  return metadataValue(MetadataType::Request, key);
}

inline google::protobuf::Value Context::responseMetadataValue(std::string_view key) {
  return metadataValue(MetadataType::Response, key);
}

inline google::protobuf::Value Context::namedMetadataValue(MetadataType type, std::string_view name, std::string_view key) {
  auto n = std::string(name);
  auto cache_key = std::make_tuple(type,  n, std::string(key));
  auto it = name_value_cache_.find(cache_key);
  if (it != name_value_cache_.end()) {
    return it->second;
  }
  auto s = metadataStruct(type, name);
  for (auto &f : s.fields()) {
    auto k = std::make_tuple(type, n, f.first);
    name_value_cache_[k] =  f.second;
  }
  struct_cache_[std::make_pair(type, n)] = std::move(s);
  it = name_value_cache_.find(cache_key);
  if (it != name_value_cache_.end()) {
    return it->second;
  }
  return {};
}

inline google::protobuf::Value Context::requestMetadataValue(std::string_view name, std::string_view key) {
  return namedMetadataValue(MetadataType::Request, name, key);
}

inline google::protobuf::Value Context::responseMetadataValue(std::string_view name, std::string_view key) {
  return namedMetadataValue(MetadataType::Response, name, key);
}

inline google::protobuf::Struct Context::metadataStruct(MetadataType type, std::string_view name) {
  auto cache_key = std::make_pair(type,  std::string(name));
  auto it = struct_cache_.find(cache_key);
  if (it != struct_cache_.end()) {
    return it->second;
  }
  auto s = getMetadataStruct(MetadataType::Request, name);
  struct_cache_[cache_key] = s;
  return s;
}

inline google::protobuf::Struct Context::requestRouteMetadataStruct() {
  return metadataStruct(MetadataType::RequestRoute);
}

inline google::protobuf::Struct Context::responseRouteMetadataStruct() {
  return metadataStruct(MetadataType::ResponseRoute);
}

inline google::protobuf::Struct Context::logMetadataStruct(std::string_view name) {
  return metadataStruct(MetadataType::Log, name);
}

inline google::protobuf::Struct Context::requestMetadataStruct(std::string_view name) {
  return metadataStruct(MetadataType::Request, name);
}

inline google::protobuf::Struct Context::responseMetadataStruct(std::string_view name) {
  return metadataStruct(MetadataType::Response, name);
}

inline google::protobuf::Value Context::getRequestMetadataValue(std::string_view key) {
  return getMetadataValue(MetadataType::Request, key);
}

inline google::protobuf::Value Context::getResponseMetadataValue(std::string_view key) {
  return getMetadataValue(MetadataType::Response, key);
}

inline google::protobuf::Struct Context::getRequestMetadataStruct(std::string_view name) {
  return getMetadataStruct(MetadataType::Request, name);
}

inline google::protobuf::Struct Context::getResponseMetadataStruct(std::string_view name) {
  return getMetadataStruct(MetadataType::Response, name);
}

// Continue
inline void continueRequest() { proxy_continueRequest(); }
inline void continueResponse() { proxy_continueResponse(); }

// Shared
inline WasmDataPtr getSharedData(std::string_view key, uint32_t* cas = nullptr) {
  uint32_t dummy_cas;
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  if (!cas)
    cas = &dummy_cas;
  proxy_getSharedData(key.data(), key.size(), &value_ptr, &value_size, cas);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline bool setSharedData(std::string_view key, std::string_view value, uint32_t cas = 0) {
  return proxy_setSharedData(key.data(), key.size(), value.data(), value.size(), cas);
}

// Headers/Trailers
inline void addRequestHeader(std::string_view key, std::string_view value) {
  proxy_addRequestHeader(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getRequestHeader(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getRequestHeader(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceRequestHeader(std::string_view key, std::string_view value) {
  proxy_replaceRequestHeader(key.data(), key.size(), value.data(), value.size());
}

inline void removeRequestHeader(std::string_view key) {
  proxy_removeRequestHeader(key.data(), key.size());
}

inline WasmDataPtr getRequestHeaderPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getRequestHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline void addRequestTrailer(std::string_view key, std::string_view value) {
  proxy_addRequestTrailer(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getRequestTrailer(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getRequestTrailer(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceRequestTrailer(std::string_view key, std::string_view value) {
  proxy_replaceRequestTrailer(key.data(), key.size(), value.data(), value.size());
}

inline void removeRequestTrailer(std::string_view key) {
  proxy_removeRequestTrailer(key.data(), key.size());
}

inline WasmDataPtr getRequestTrailerPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getRequestHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline void addResponseHeader(std::string_view key, std::string_view value) {
  proxy_addResponseHeader(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getResponseHeader(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getResponseHeader(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceResponseHeader(std::string_view key, std::string_view value) {
  proxy_replaceResponseHeader(key.data(), key.size(), value.data(), value.size());
}

inline void removeResponseHeader(std::string_view key) {
  proxy_removeResponseHeader(key.data(), key.size());
}

inline WasmDataPtr getResponseHeaderPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getResponseHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline void addResponseTrailer(std::string_view key, std::string_view value) {
  proxy_addResponseTrailer(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getResponseTrailer(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getResponseTrailer(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceResponseTrailer(std::string_view key, std::string_view value) {
  proxy_replaceResponseTrailer(key.data(), key.size(), value.data(), value.size());
}

inline void removeResponseTrailer(std::string_view key) {
  proxy_removeResponseTrailer(key.data(), key.size());
}

inline WasmDataPtr getResponseTrailerPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getRequestHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
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

using HeaderStringPairs = std::vector<std::pair<std::string, std::string>>;

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

inline uint32_t httpCall(std::string_view uri, const HeaderStringPairs& request_headers,
                         std::string_view request_body, const HeaderStringPairs& request_trailers,
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

inline uint32_t defineMetric(MetricType type, std::string_view name) {
  return proxy_defineMetric(type, name.data(), name.size());
}

inline void incrementMetric(uint32_t metric_id, int64_t offset) {
  proxy_incrementMetric(metric_id, offset);
}

inline void recordMetric(uint32_t metric_id, uint64_t value) {
  proxy_recordMetric(metric_id, value);
}

inline uint64_t getMetric(uint32_t metric_id) { return proxy_getMetric(metric_id); }

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
      : type(t), name(n), tags(ts.begin(), ts.end()) {}

  MetricType type;
  std::string name;
  std::vector<MetricTag> tags;
  std::unordered_map<std::string, uint32_t> metric_ids;

  uint32_t resolveFullName(const std::string& n);
  uint32_t resolveWithFields(const std::vector<const std::string>& fields);
  std::string nameFromIdSlow(uint32_t id);
};

struct Metric : public MetricBase {
  Metric(MetricType t, const std::string& n) : MetricBase(t, n) {}
  Metric(MetricType t, const std::string& n, std::vector<MetricTag> ts) : MetricBase(t, n, ts) {}

  template <typename... Fields> void increment(int64_t offset, Fields... tags);
  template <typename... Fields> void record(uint64_t value, Fields... tags);
  template <typename... Fields> uint64_t get(Fields... tags);
  template <typename... Fields> uint32_t resolve(Fields... tags);
};

inline uint32_t MetricBase::resolveWithFields(const std::vector<const std::string>& fields) {
  if (fields.size() != tags.size()) {
    throw ProxyException("metric fields.size() != tags.size()");
  }
  size_t s = 0;
  for (auto& t : tags) {
    s += t.name.size() + 1; // 1 more for "."
  }
  for (auto& f : fields) {
    s += f.size() + 1; // 1 more for "."
  }
  s += name.size() + 2; // "." and "\0";
  std::string n;
  n.reserve(s);
  for (int i = 0; i < tags.size(); i++) {
    n.append(tags[i].name);
    n.append(".");
    n.append(fields[i]);
    n.append(".");
  }
  n.append(name);
  return resolveFullName(n);
}

template <typename T> inline std::string ToString(T t) { return std::to_string(t); }

template <> inline std::string ToString(const char* t) { return std::string(t); }

template <> inline std::string ToString(std::string t) { return std::move(t); }

template <> inline std::string ToString(bool t) { return t ? "true" : "false"; }

inline uint32_t MetricBase::resolveFullName(const std::string& n) {
  auto it = metric_ids.find(n);
  if (it == metric_ids.end()) {
    auto metric_id = defineMetric(type, n);
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

template <typename... Fields> uint32_t Metric::resolve(Fields... f) {
  std::vector<const std::string> fields{ToString(f)...};
  return resolveWithFields(fields);
}

template <typename... Fields> void Metric::increment(int64_t offset, Fields... f) {
  std::vector<const std::string> fields{ToString(f)...};
  auto metric_id = resolveWithFields(fields);
  incrementMetric(metric_id, offset);
}

template <typename... Fields> void Metric::record(uint64_t value, Fields... f) {
  std::vector<const std::string> fields{ToString(f)...};
  auto metric_id = resolveWithFields(fields);
  recordMetric(metric_id, value);
}

template <typename... Fields> uint64_t Metric::get(Fields... f) {
  std::vector<const std::string> fields{ToString(f)...};
  auto metric_id = resolveWithFields(fields);
  return getMetric(metric_id);
}

template <typename T> struct MetricTagDescriptor {
  MetricTagDescriptor(std::string_view n) : name(n) {}
  MetricTagDescriptor(const char* n) : name(n) {}
  typedef T type;
  std::string_view name;
};

template <typename T> inline MetricTag ToMetricTag(const MetricTagDescriptor<T>& d) { return {}; }

template <> inline MetricTag ToMetricTag(const MetricTagDescriptor<const char*>& d) {
  return {std::string(d.name), MetricTag::TagType::String};
}

template <> inline MetricTag ToMetricTag(const MetricTagDescriptor<std::string>& d) {
  return {std::string(d.name), MetricTag::TagType::String};
}

template <> inline MetricTag ToMetricTag(const MetricTagDescriptor<std::string_view>& d) {
  return {std::string(d.name), MetricTag::TagType::String};
}

template <> inline MetricTag ToMetricTag(const MetricTagDescriptor<int>& d) {
  return {std::string(d.name), MetricTag::TagType::Int};
}

template <> inline MetricTag ToMetricTag(const MetricTagDescriptor<bool>& d) {
  return {std::string(d.name), MetricTag::TagType::Bool};
}

struct SimpleCounter {
  SimpleCounter(uint32_t id) : metric_id(id) {}

  void increment(int64_t offset) { recordMetric(metric_id, offset); }
  void record(int64_t offset) { increment(offset); }
  uint64_t get() { return getMetric(metric_id); }
  void operator++() { increment(1); }
  void operator++(int) { increment(1); }

  uint32_t metric_id;
};

struct SimpleGauge {
  SimpleGauge(uint32_t id) : metric_id(id) {}

  void record(uint64_t offset) { recordMetric(metric_id, offset); }
  uint64_t get() { return getMetric(metric_id); }

  uint32_t metric_id;
};

struct SimpleHistogram {
  SimpleHistogram(uint32_t id) : metric_id(id) {}

  void record(int64_t offset) { recordMetric(metric_id, offset); }

  uint32_t metric_id;
};

template <typename... Tags> struct Counter : public MetricBase {
  static Counter<Tags...>* New(std::string_view name, MetricTagDescriptor<Tags>... fieldnames);

  SimpleCounter resolve(Tags... f) {
    std::vector<const std::string> fields{ToString(f)...};
    return SimpleCounter(resolveWithFields(fields));
  }

  void increment(int64_t offset, Tags... tags) {
    std::vector<const std::string> fields{ToString(tags)...};
    auto metric_id = resolveWithFields(fields);
    incrementMetric(metric_id, offset);
  }

  void record(int64_t offset, Tags... tags) { increment(offset, tags...); }

  uint64_t get(Tags... tags) {
    std::vector<const std::string> fields{ToString(tags)...};
    auto metric_id = resolveWithFields(fields);
    return getMetric(metric_id);
  }

private:
  Counter(const std::string& name, std::vector<MetricTag> tags)
      : MetricBase(MetricType::Counter, name, tags) {}
};

template <typename... Tags>
inline Counter<Tags...>* Counter<Tags...>::New(std::string_view name,
                                               MetricTagDescriptor<Tags>... descriptors) {
  return new Counter<Tags...>(std::string(name),
                              std::vector<MetricTag>({ToMetricTag(descriptors)...}));
}

template <typename... Tags> struct Gauge : public MetricBase {
  static Gauge<Tags...>* New(std::string_view name, MetricTagDescriptor<Tags>... fieldnames);

  SimpleGauge resolve(Tags... f) {
    std::vector<const std::string> fields{ToString(f)...};
    return SimpleGauge(resolveWithFields(fields));
  }

  void record(int64_t offset, Tags... tags) {
    std::vector<const std::string> fields{ToString(tags)...};
    auto metric_id = resolveWithFields(fields);
    recordMetric(metric_id, offset);
  }

  uint64_t get(Tags... tags) {
    std::vector<const std::string> fields{ToString(tags)...};
    auto metric_id = resolveWithFields(fields);
    return getMetric(metric_id);
  }

private:
  Gauge(const std::string& name, std::vector<MetricTag> tags)
      : MetricBase(MetricType::Gauge, name, tags) {}
};

template <typename... Tags>
inline Gauge<Tags...>* Gauge<Tags...>::New(std::string_view name,
                                           MetricTagDescriptor<Tags>... descriptors) {
  return new Gauge<Tags...>(std::string(name),
                            std::vector<MetricTag>({ToMetricTag(descriptors)...}));
}

template <typename... Tags> struct Histogram : public MetricBase {
  static Histogram<Tags...>* New(std::string_view name, MetricTagDescriptor<Tags>... fieldnames);

  SimpleHistogram resolve(Tags... f) {
    std::vector<const std::string> fields{ToString(f)...};
    return SimpleHistogram(resolveWithFields(fields));
  }

  void record(int64_t offset, Tags... tags) {
    std::vector<const std::string> fields{ToString(tags)...};
    auto metric_id = resolveWithFields(fields);
    recordMetric(metric_id, offset);
  }

private:
  Histogram(const std::string& name, std::vector<MetricTag> tags)
      : MetricBase(MetricType::Histogram, name, tags) {}
};

template <typename... Tags>
inline Histogram<Tags...>* Histogram<Tags...>::New(std::string_view name,
                                                   MetricTagDescriptor<Tags>... descriptors) {
  return new Histogram<Tags...>(std::string(name),
                                std::vector<MetricTag>({ToMetricTag(descriptors)...}));
}
