#pragma once

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

extern thread_local Envoy::Extensions::Common::Wasm::Context* current_context_;

namespace Null {
namespace Plugin {
class RootContext;
}

Plugin::RootContext* nullVmGetRoot(absl::string_view root_id);

namespace Plugin {

#define WS(_x) Word(static_cast<uint64_t>(_x))
#define WR(_x) Word(reinterpret_cast<uint64_t>(_x))

inline WasmResult wordToWasmResult(Word w) { return static_cast<WasmResult>(w.u64_); }

// Logging
inline WasmResult proxy_log(LogLevel level, const char* logMessage, size_t messageSize) {
  return wordToWasmResult(logHandler(current_context_, WS(level), WR(logMessage), WS(messageSize)));
}

// Timer
inline WasmResult proxy_setTickPeriodMilliseconds(uint64_t millisecond) {
  return wordToWasmResult(setTickPeriodMillisecondsHandler(current_context_, Word(millisecond)));
}
inline WasmResult proxy_getCurrentTimeNanoseconds(uint64_t* result) {
  return wordToWasmResult(getCurrentTimeNanosecondsHandler(current_context_, WR(result)));
}

// State accessors
inline WasmResult proxy_getProperty(const char* path_ptr, size_t path_size,
                                    const char** value_ptr_ptr, size_t* value_size_ptr) {
  return wordToWasmResult(getPropertyHandler(current_context_, WR(path_ptr), WS(path_size),
                                             WR(value_ptr_ptr), WR(value_size_ptr)));
}
inline WasmResult proxy_setProperty(const char* key_ptr, size_t key_size, const char* value_ptr,
                                    size_t value_size) {
  return wordToWasmResult(setPropertyHandler(current_context_, WR(key_ptr), WS(key_size),
                                             WR(value_ptr), WS(value_size)));
}

// Continue
inline WasmResult proxy_continueRequest() {
  return wordToWasmResult(continueRequestHandler(current_context_));
}
inline WasmResult proxy_continueResponse() {
  return wordToWasmResult(continueResponseHandler(current_context_));
}
inline WasmResult
proxy_sendLocalResponse(uint32_t response_code, const char* response_code_details_ptr,
                        size_t response_code_details_size, const char* body_ptr, size_t body_size,
                        const char* additional_response_header_pairs_ptr,
                        size_t additional_response_header_pairs_size, uint32_t grpc_status) {
  return wordToWasmResult(
      sendLocalResponseHandler(current_context_, WS(response_code), WR(response_code_details_ptr),
                               WS(response_code_details_size), WR(body_ptr), WS(body_size),
                               WR(additional_response_header_pairs_ptr),
                               WS(additional_response_header_pairs_size), WS(grpc_status)));
}
inline WasmResult proxy_clearRouteCache() {
  return wordToWasmResult(clearRouteCacheHandler(current_context_));
}

// SharedData
inline WasmResult proxy_getSharedData(const char* key_ptr, size_t key_size, const char** value_ptr,
                                      size_t* value_size, uint32_t* cas) {
  return wordToWasmResult(getSharedDataHandler(current_context_, WR(key_ptr), WS(key_size),
                                               WR(value_ptr), WR(value_size), WR(cas)));
}
//  If cas != 0 and cas != the current cas for 'key' return false, otherwise set the value and
//  return true.
inline WasmResult proxy_setSharedData(const char* key_ptr, size_t key_size, const char* value_ptr,
                                      size_t value_size, uint64_t cas) {
  return wordToWasmResult(setSharedDataHandler(current_context_, WR(key_ptr), WS(key_size),
                                               WR(value_ptr), WS(value_size), WS(cas)));
}

// SharedQueue
// Note: Registering the same queue_name will overwrite the old registration while preseving any
// pending data. Consequently it should typically be followed by a call to proxy_dequeueSharedQueue.
// Returns unique token for the queue.
inline WasmResult proxy_registerSharedQueue(const char* queue_name_ptr, size_t queue_name_size,
                                            uint32_t* token) {
  return wordToWasmResult(registerSharedQueueHandler(current_context_, WR(queue_name_ptr),
                                                     WS(queue_name_size), WR(token)));
}
// Returns unique token for the queue.
inline WasmResult proxy_resolveSharedQueue(const char* vm_id_ptr, size_t vm_id_size,
                                           const char* queue_name_ptr, size_t queue_name_size,
                                           uint32_t* token) {
  return wordToWasmResult(resolveSharedQueueHandler(current_context_, WR(vm_id_ptr), WS(vm_id_size),
                                                    WR(queue_name_ptr), WS(queue_name_size),
                                                    WR(token)));
}
// Returns true on end-of-stream (no more data available).
inline WasmResult proxy_dequeueSharedQueue(uint32_t token, const char** data_ptr,
                                           size_t* data_size) {
  return wordToWasmResult(
      dequeueSharedQueueHandler(current_context_, WS(token), WR(data_ptr), WR(data_size)));
}
// Returns false if the queue was not found and the data was not enqueued.
inline WasmResult proxy_enqueueSharedQueue(uint32_t token, const char* data_ptr, size_t data_size) {
  return wordToWasmResult(
      enqueueSharedQueueHandler(current_context_, WS(token), WR(data_ptr), WS(data_size)));
}

// Network
inline WasmResult proxy_getDownstreamDataBufferBytes(uint64_t start, uint64_t length,
                                                     const char** ptr, size_t* size) {
  return wordToWasmResult(getDownstreamDataBufferBytesHandler(current_context_, WS(start),
                                                              WS(length), WR(ptr), WR(size)));
}
inline WasmResult proxy_getUpstreamDataBufferBytes(uint64_t start, uint64_t length,
                                                   const char** ptr, size_t* size) {
  return wordToWasmResult(getUpstreamDataBufferBytesHandler(current_context_, WS(start), WS(length),
                                                            WR(ptr), WR(size)));
}

// Headers/Trailers/Metadata Maps
inline WasmResult proxy_addHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size,
                                          const char* value_ptr, size_t value_size) {
  return wordToWasmResult(addHeaderMapValueHandler(current_context_, WS(type), WR(key_ptr),
                                                   WS(key_size), WR(value_ptr), WS(value_size)));
}
inline WasmResult proxy_getHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size,
                                          const char** value_ptr, size_t* value_size) {
  return wordToWasmResult(getHeaderMapValueHandler(current_context_, WS(type), WR(key_ptr),
                                                   WS(key_size), WR(value_ptr), WR(value_size)));
}
inline WasmResult proxy_getHeaderMapPairs(HeaderMapType type, const char** ptr, size_t* size) {
  return wordToWasmResult(getHeaderMapPairsHandler(current_context_, WS(type), WR(ptr), WR(size)));
}
inline WasmResult proxy_setHeaderMapPairs(HeaderMapType type, const char* ptr, size_t size) {
  return wordToWasmResult(setHeaderMapPairsHandler(current_context_, WS(type), WR(ptr), WS(size)));
}
inline WasmResult proxy_replaceHeaderMapValue(HeaderMapType type, const char* key_ptr,
                                              size_t key_size, const char* value_ptr,
                                              size_t value_size) {
  return wordToWasmResult(replaceHeaderMapValueHandler(
      current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr), WS(value_size)));
}
inline WasmResult proxy_removeHeaderMapValue(HeaderMapType type, const char* key_ptr,
                                             size_t key_size) {
  return wordToWasmResult(
      removeHeaderMapValueHandler(current_context_, WS(type), WR(key_ptr), WS(key_size)));
}
inline WasmResult proxy_getHeaderMapSize(HeaderMapType type, size_t* size) {
  return wordToWasmResult(getHeaderMapSizeHandler(current_context_, WS(type), WR(size)));
}

// Body
inline WasmResult proxy_getRequestBodyBufferBytes(uint64_t start, uint64_t length, const char** ptr,
                                                  size_t* size) {
  return wordToWasmResult(getRequestBodyBufferBytesHandler(current_context_, Word(start),
                                                           Word(length), WR(ptr), WR(size)));
}
inline WasmResult proxy_getResponseBodyBufferBytes(uint64_t start, uint64_t length,
                                                   const char** ptr, size_t* size) {
  return wordToWasmResult(getResponseBodyBufferBytesHandler(current_context_, WS(start), WS(length),
                                                            WR(ptr), WR(size)));
}

// HTTP
// Returns token, used in callback onHttpCallResponse
inline uint64_t proxy_httpCall(const char* uri_ptr, size_t uri_size, void* header_pairs_ptr,
                               size_t header_pairs_size, const char* body_ptr, size_t body_size,
                               void* trailer_pairs_ptr, size_t trailer_pairs_size,
                               uint64_t timeout_milliseconds) {
  return httpCallHandler(current_context_, WR(uri_ptr), WS(uri_size), WR(header_pairs_ptr),
                         WS(header_pairs_size), WR(body_ptr), WS(body_size), WR(trailer_pairs_ptr),
                         WS(trailer_pairs_size), WS(timeout_milliseconds))
      .u64_;
}
// gRPC
// Returns token, used in gRPC callbacks (onGrpc...)
inline uint64_t proxy_grpcCall(const char* service_ptr, size_t service_size,
                               const char* service_name_ptr, size_t service_name_size,
                               const char* method_name_ptr, size_t method_name_size,
                               const char* request_ptr, size_t request_size,
                               uint64_t timeout_milliseconds) {
  return grpcCallHandler(current_context_, WR(service_ptr), WS(service_size), WR(service_name_ptr),
                         WS(service_name_size), WR(method_name_ptr), WS(method_name_size),
                         WR(request_ptr), WS(request_size), WS(timeout_milliseconds))
      .u64_;
}
inline uint64_t proxy_grpcStream(const char* service_ptr, size_t service_size,
                                 const char* service_name_ptr, size_t service_name_size,
                                 const char* method_name_ptr, size_t method_name_size) {
  return grpcStreamHandler(current_context_, WR(service_ptr), WS(service_size),
                           WR(service_name_ptr), WS(service_name_size), WR(method_name_ptr),
                           WS(method_name_size))
      .u64_;
}
inline WasmResult proxy_grpcCancel(uint64_t token) {
  return wordToWasmResult(grpcCancelHandler(current_context_, WS(token)));
}
inline WasmResult proxy_grpcClose(uint64_t token) {
  return wordToWasmResult(grpcCloseHandler(current_context_, WS(token)));
}
inline WasmResult proxy_grpcSend(uint64_t token, const char* message_ptr, size_t message_size,
                                 uint64_t end_stream) {
  return wordToWasmResult(grpcSendHandler(current_context_, WS(token), WR(message_ptr),
                                          WS(message_size), WS(end_stream)));
}

// Metrics
// Returns a metric_id which can be used to report a metric. On error returns 0.
inline WasmResult proxy_defineMetric(MetricType type, const char* name_ptr, size_t name_size,
                                     uint32_t* metric_id) {
  return wordToWasmResult(
      defineMetricHandler(current_context_, WS(type), WR(name_ptr), WS(name_size), WR(metric_id)));
}
inline WasmResult proxy_incrementMetric(uint32_t metric_id, int64_t offset) {
  return wordToWasmResult(incrementMetricHandler(current_context_, WS(metric_id), offset));
}
inline WasmResult proxy_recordMetric(uint32_t metric_id, uint64_t value) {
  return wordToWasmResult(recordMetricHandler(current_context_, WS(metric_id), value));
}
inline WasmResult proxy_getMetric(uint32_t metric_id, uint64_t* value) {
  return wordToWasmResult(getMetricHandler(current_context_, WS(metric_id), WR(value)));
}
inline WasmResult proxy_setEffectiveContext(uint64_t context_id) {
  return wordToWasmResult(setEffectiveContextHandler(current_context_, WS(context_id)));
}

#undef WS
#undef WR

inline RootContext* getRoot(StringView root_id) { return nullVmGetRoot(root_id); }

} // namespace Plugin
} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
