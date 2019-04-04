/*
 * Intrinsic functions available to WASM modules.
 */
// NOLINT(namespace-envoy)
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE __attribute__((used)) __attribute__((visibility("default")))
#endif

/*
   API Calls into the VM.

   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(char* configuration, int size);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart();
   extern "C" EMSCRIPTEN_KEEPALIVE int main();  // only called if proxy_onStart() is not available.
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick();
   extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onCreate(uint32_t context_id);
   extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onRequestHeaders(uint32_t context_id);
   extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onRequestBody(uint32_t context_id,  uint32_t
   body_buffer_length, uint32_t end_of_stream size); extern "C" ENSCRIPTEN_KEEPALIVE void
   proxy_onRequestTrailers(uint32_t context_id); extern "C" ENSCRIPTEN_KEEPALIVE void
   proxy_onRequestMetadata(uint32_t context_id); extern "C" ENSCRIPTEN_KEEPALIVE void
   proxy_onResponseHeaders(uint32_t context_id); extern "C" ENSCRIPTEN_KEEPALIVE void
   proxy_onResponseBody(uint32_t context_id,  uint32_t body_buffer_length, uint32_t end_of_stream
   size); extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onResponseTrailers(uint32_t context_id); extern
   "C" ENSCRIPTEN_KEEPALIVE void proxy_onResponseMetadata(uint32_t context_id); extern "C"
   ENSCRIPTEN_KEEPALIVE void proxy_onHttpCallResponse(uint32_t context_id uint32_t token, uint32_t
   header_pairs_ptr, uint32_t header_pairs_size, uint32_t body_ptr, uint32_t body_size, uint32_t
   trailer_pairs_ptr, uint32_t trailer_pairs_size):
   // The stream has completed.
   extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onDone(uint32_t context_id);
   // onLog occurs after onDone.
   extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onLog(uint32_t context_id);
   // The Context in the proxy has been destroyed and no further calls will be coming.
   extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onDelete(uint32_t context_id);
*/

enum class LogLevel : int { trace, debug, info, warn, error, critical };
extern "C" void proxy_log(LogLevel level, const char* logMessage, size_t messageSize);

extern "C" void proxy_setTickPeriodMilliseconds(uint32_t millisecond);

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

// StreamInfo
extern "C" void proxy_getRequestStreamInfoProtocol(const char** value_ptr, size_t* value_size);
extern "C" void proxy_getResponseStreamInfoProtocol(const char** value_ptr, size_t* value_size);

extern "C" void proxy_getRequestMetadata(const char* key_ptr, size_t key_size,
                                         const char** value_ptr_ptr, size_t* value_size_ptr);
extern "C" void proxy_setRequestMetadata(const char* key_ptr, size_t key_size,
                                         const char* value_ptr, size_t value_size);
extern "C" void proxy_getRequestMetadataPairs(const char** value_ptr, size_t* value_size);
extern "C" void proxy_getResponseMetadata(const char* key_ptr, size_t key_size,
                                          const char** value_ptr_ptr, size_t* value_size_ptr);
extern "C" void proxy_setResponseMetadata(const char* key_ptr, size_t key_size,
                                          const char* value_ptr, size_t value_size);
extern "C" void proxy_getResponseMetadataPairs(const char** value_ptr, size_t* value_size);

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

//
// High Level C++ API.
//

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

// Calls coming into the WASM filter. Implement N
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

private:
  uint32_t id_;
};

// StreamInfo
inline WasmDataPtr getRequestStreamInfoProtocol() {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getRequestStreamInfoProtocol(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline WasmDataPtr getResponseStreamInfoProtocol() {
  const char* ptr = nullptr;
  size_t size = 0;
  proxy_getResponseStreamInfoProtocol(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline WasmDataPtr getRequestMetadata(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getRequestMetadata(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void setRequestMetadata(std::string_view key, std::string_view value) {
  proxy_setRequestMetadata(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getRequestMetadataPairs() {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getRequestMetadataPairs(&value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline WasmDataPtr getResponseMetadata(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getResponseMetadata(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void setResponseMetadata(std::string_view key, std::string_view value) {
  proxy_setResponseMetadata(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getResponseMetadataPairs() {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  proxy_getResponseMetadataPairs(&value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
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
