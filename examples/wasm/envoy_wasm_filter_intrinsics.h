/*
 * Intrinsic functions available to WASM filter modules.
 */
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "envoy_wasm_intrinsics.h"


//
// Low Level API.
//
enum class FilterHeadersStatus : int { Continue = 0, StopIteration = 1 };
enum class FilterTrailersStatus : int { Continue = 0, StopIteration = 1 };
enum class FilterDataStatus : int { Continue = 0, StopIterationAndBuffer = 1, StopIterationAndWatermark = 2, StopIterationNoBuffer = 3 };

// Calls from Envoy to WASM.

extern "C" int onStart(uint32_t context_id);  // context_id == 0, always returns 0.
extern "C" void onConfigure(uint32_t context_id, uint32_t configuration_ptr, uint32_t configuration_size);  // context_id == 0

extern "C" FilterHeadersStatus onRequestHeaders(uint32_t context_id);
extern "C" FilterDataStatus onRequestBody(uint32_t context_id, uint32_t body_buffer_length, uint32_t end_of_stream);
extern "C" FilterTrailersStatus onRequesstTrailers(uint32_t context_id);

extern "C" FilterHeadersStatus onResponseHeaders(uint32_t context_id);
extern "C" FilterDataStatus onResponseBody(uint32_t context_id, uint32_t body_buffer_length, uint32_t end_of_stream);
extern "C" FilterTrailersStatus onResponsetTrailers(uint32_t context_id);

extern "C" void onDone(uint32_t context_id);
extern "C" void onDestroy(uint32_t context_id);

extern "C" void onHttpCallResponse(uint32_t context_id,
    uint32_t token, uint32_t header_pairs_ptr, uint32_t header_pairs_size,
    uint32_t body_ptr, uint32_t body_size, uint32_t trailer_pairs_ptr,
    uint32_t trailer_pairs_size);

// Calls from WASM to Envoy

// StreamInfo
extern "C" void envoy_getRequestStreamInfoProtocol(const char** value_ptr, size_t* value_size);
extern "C" void envoy_getResponseStreamInfoProtocol(const char** value_ptr, size_t* value_size);

extern "C" void envoy_getRequestMetadata(const char* key_ptr, size_t key_size, const char** value_ptr_ptr, size_t* value_size_ptr);
extern "C" void envoy_setRequestMetadata(const char* key_ptr, size_t key_size, const char* value_ptr, size_t value_size);
extern "C" void envoy_getRequestMetadataPairs(const char** value_ptr, size_t* value_size);
extern "C" void envoy_getResponseMetadata(const char* key_ptr, size_t key_size, const char** value_ptr_ptr, size_t* value_size_ptr);
extern "C" void envoy_setResponseMetadata(const char* key_ptr, size_t key_size, const char* value_ptr, size_t value_size);
extern "C" void envoy_getResponseMetadataPairs(const char** value_ptr, size_t* value_size);

// Continue
extern "C" void envoy_continueRequest();
extern "C" void envoy_continueResponse();

// SharedData
extern "C" void envoy_getSharedData(const char* key_ptr, size_t key_size, const char** value_ptr, size_t* value_size, uint32_t* cas);
//  If cas != 0 and cas != the current cas for 'key' return false, otherwise set the value and return true.
extern "C" bool envoy_setSharedData(const char* key_ptr, size_t key_size, const char* value_ptr, size_t value_size, uint32_t cas);

// Headers/Trailers
extern "C" void envoy_addRequestHeader(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_getRequestHeader(const char* key_ptr, size_t key_size,
    const char** value_ptr, size_t* value_size);
extern "C" void envoy_getRequestHeaderPairs(const char** ptr, size_t* size);
extern "C" void envoy_replaceRequestHeader(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_removeRequestHeader(const char* key_ptr, size_t key_size);

extern "C" void envoy_addRequestTrailer(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_getRequestTrailer(const char* key_ptr, size_t key_size,
    const char** value_ptr, size_t* value_size);
extern "C" void envoy_getRequestTrailerPairs(const char** ptr, size_t* size);
extern "C" void envoy_replaceRequestTrailer(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_removeRequestTrailer(const char* key_ptr, size_t key_size);

extern "C" void envoy_addResponseHeader(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_getResponseHeader(const char* key_ptr, size_t key_size,
    const char** value_ptr, size_t* value_size);
extern "C" void envoy_getResponseHeaderPairs(const char** ptr, size_t* size);
extern "C" void envoy_replaceResponseHeader(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_removeResponseHeader(const char* key_ptr, size_t key_size);

extern "C" void envoy_addResponseTrailer(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_getResponseTrailer(const char* key_ptr, size_t key_size,
    const char** value_ptr, size_t* value_size);
extern "C" void envoy_getResponseTrailerPairs(const char** ptr, size_t* size);
extern "C" void envoy_replaceResponseTrailer(const char* key_ptr, size_t key_size,
    const char* value_ptr, size_t value_size);
extern "C" void envoy_removeResponseTrailer(const char* key_ptr, size_t key_size);

// Body
extern "C" void envoy_getRequestBodyBufferBytes(uint32_t start, uint32_t length, const char** ptr, size_t* size);
extern "C" void envoy_getResponseBodyBufferBytes(uint32_t start, uint32_t length, const char** ptr, size_t* size);

// HTTP
// Returns token, used in callback onHttpCallResponse
extern "C" uint32_t envoy_httpCall(const char* uri_ptr, size_t uri_size, void* header_pairs_ptr, size_t header_pairs_size,
    const char* body_ptr, size_t body_size, void* trailer_pairs_ptr, size_t trailer_pairs_size, uint32_t timeout_milliseconds);

//
// High Level C++ API.
//

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
  if (!data()) return result;
  auto p = data();
  int n = *reinterpret_cast<const int*>(p); p += sizeof(int);
  result.resize(n);
  auto s = p + n * 8;
  for (int i = 0; i < n; i++) {
    int size = *reinterpret_cast<const int*>(p); p += sizeof(int);
    result[i].first = std::string_view(s, size);
    s += size + 1;
    size = *reinterpret_cast<const int*>(p); p += sizeof(int);
    result[i].second = std::string_view(s, size);
    s += size + 1;
  }
  return result;
}


// Calls coming into the WASM filter.  Implement N
class Context {
public:
  explicit Context(uint32_t id) : id_(id) {}
  virtual ~Context() {}

  uint32_t id() { return id_; }

  static std::unique_ptr<Context> New(uint32_t id);  // For subclassing.

  // Called once when the filter loads on context 0.
  virtual void onStart() {}
  // Called once when the filter loads and on configuration changes on context 0.
  virtual void onConfigure(std::unique_ptr<WasmData> configuration) {}

  // Called on individual requests/response streams.
  virtual FilterHeadersStatus onRequestHeaders() { return FilterHeadersStatus::Continue; }
  virtual FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) { return FilterDataStatus::Continue; }
  virtual FilterTrailersStatus onRequestTrailers() { return FilterTrailersStatus::Continue; }
  virtual FilterHeadersStatus onResponseHeaders() { return FilterHeadersStatus::Continue; }
  virtual FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) { return FilterDataStatus::Continue; }
  virtual FilterTrailersStatus onResponseTrailers() { return FilterTrailersStatus::Continue; }
  virtual void onDone() {}
  virtual void onDestroy() {}

  virtual void onHttpCallResponse(uint32_t token, std::unique_ptr<WasmData> header_pairs,
      std::unique_ptr<WasmData> body, std::unique_ptr<WasmData> trailer_pairs) {}

private:
  uint32_t id_;
};

// StreamInfo
inline WasmDataPtr getRequestStreamInfoProtocol() {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getRequestStreamInfoProtocol(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline WasmDataPtr getResponseStreamInfoProtocol() {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getResponseStreamInfoProtocol(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline WasmDataPtr getRequestMetadata(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getRequestMetadata(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void setRequestMetadata(std::string_view key, std::string_view value) {
  envoy_setRequestMetadata(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getRequestMetadataPairs() {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getRequestMetadataPairs(&value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline WasmDataPtr getResponseMetadata(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getResponseMetadata(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void setResponseMetadata(std::string_view key, std::string_view value) {
  envoy_setResponseMetadata(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getResponseMetadataPairs() {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getResponseMetadataPairs(&value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

// Continue
inline void continueRequest() { envoy_continueRequest(); }
inline void continueResponse() { envoy_continueResponse(); }

// Shared
inline WasmDataPtr getSharedData(std::string_view key, uint32_t* cas = nullptr) {
  uint32_t dummy_cas;
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  if (!cas) cas = &dummy_cas;
  envoy_getSharedData(key.data(), key.size(), &value_ptr, &value_size, cas);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline bool setSharedData(std::string_view key, std::string_view value, uint32_t cas = 0) {
  return envoy_setSharedData(key.data(), key.size(), value.data(), value.size(), cas);
}

// Headers/Trailers
inline void addRequestHeader(std::string_view key, std::string_view value) {
  envoy_addRequestHeader(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getRequestHeader(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getRequestHeader(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceRequestHeader(std::string_view key, std::string_view value) {
  envoy_replaceRequestHeader(key.data(), key.size(), value.data(), value.size());
}

inline void removeRequestHeader(std::string_view key) {
  envoy_removeRequestHeader(key.data(), key.size());
}

inline WasmDataPtr getRequestHeaderPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getRequestHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline void addRequestTrailer(std::string_view key, std::string_view value) {
  envoy_addRequestTrailer(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getRequestTrailer(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getRequestTrailer(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceRequestTrailer(std::string_view key, std::string_view value) {
  envoy_replaceRequestTrailer(key.data(), key.size(), value.data(), value.size());
}

inline void removeRequestTrailer(std::string_view key) {
  envoy_removeRequestTrailer(key.data(), key.size());
}

inline WasmDataPtr getRequestTrailerPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getRequestHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline void addResponseHeader(std::string_view key, std::string_view value) {
  envoy_addResponseHeader(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getResponseHeader(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getResponseHeader(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceResponseHeader(std::string_view key, std::string_view value) {
  envoy_replaceResponseHeader(key.data(), key.size(), value.data(), value.size());
}

inline void removeResponseHeader(std::string_view key) {
  envoy_removeResponseHeader(key.data(), key.size());
}

inline WasmDataPtr getResponseHeaderPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getResponseHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline void addResponseTrailer(std::string_view key, std::string_view value) {
  envoy_addResponseTrailer(key.data(), key.size(), value.data(), value.size());
}

inline WasmDataPtr getResponseTrailer(std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getResponseTrailer(key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

inline void replaceResponseTrailer(std::string_view key, std::string_view value) {
  envoy_replaceResponseTrailer(key.data(), key.size(), value.data(), value.size());
}

inline void removeResponseTrailer(std::string_view key) {
  envoy_removeResponseTrailer(key.data(), key.size());
}

inline WasmDataPtr getResponseTrailerPairs() {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getRequestHeaderPairs(&ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

// Body
inline WasmDataPtr getRequestBodyBufferBytes(size_t start, size_t length) {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getRequestBodyBufferBytes(start, length, &ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

inline WasmDataPtr getResponseBodyBufferBytes(size_t start, size_t length) {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getResponseBodyBufferBytes(start, length, &ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

// HTTP

using HeaderStringPairs = std::vector<std::pair<std::string, std::string>>;

inline void MakeHeaderStringPairsBuffer(const HeaderStringPairs& headers, void** buffer_ptr, size_t* size_ptr) {
  if (headers.empty()) {
    *buffer_ptr = nullptr;
    *size_ptr = 0;
    return;
  }
  int size = 4;  // number of headers
  for (auto& p : headers) {
    size += 8;  // size of key, size of value
    size += p.first.size() + 1;  // null terminated key
    size += p.second.size() + 1;  // null terminated value
  }
  char *buffer = static_cast<char*>(::malloc(size));
  char *b = buffer;
  *reinterpret_cast<int32_t*>(b) = headers.size(); b += sizeof(int32_t);
  for (auto& p : headers) {
    *reinterpret_cast<int32_t*>(b) = p.first.size(); b += sizeof(int32_t);
    *reinterpret_cast<int32_t*>(b) = p.second.size(); b += sizeof(int32_t);
  }
  for (auto& p : headers) {
    memcpy(b, p.first.data(), p.first.size()); b += p.first.size(); *b++ = 0;
    memcpy(b, p.second.data(), p.second.size()); b += p.second.size(); *b++ = 0;
  }
  *buffer_ptr = buffer;
  *size_ptr = size;
}

inline uint32_t httpCall(std::string_view uri, const HeaderStringPairs& request_headers, std::string_view request_body, const HeaderStringPairs& request_trailers, uint32_t timeout_milliseconds) {
  void *headers_ptr = nullptr, *trailers_ptr = nullptr;
  size_t headers_size = 0, trailers_size = 0;
  MakeHeaderStringPairsBuffer(request_headers, &headers_ptr, &headers_size);
  MakeHeaderStringPairsBuffer(request_trailers, &trailers_ptr, &trailers_size);
  uint32_t result = envoy_httpCall(uri.data(), uri.size(), headers_ptr, headers_size, request_body.data(), request_body.size(), trailers_ptr, trailers_size, timeout_milliseconds);
  ::free(headers_ptr);
  ::free(trailers_ptr);
  return result;
}
