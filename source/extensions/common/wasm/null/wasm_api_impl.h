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

// Logging
inline void proxy_log(LogLevel level, const char* logMessage, size_t messageSize) {
  logHandler(current_context_, WS(level), WR(logMessage), WS(messageSize));
}

// Timer
inline void proxy_setTickPeriodMilliseconds(uint64_t millisecond) {
  setTickPeriodMillisecondsHandler(current_context_, Word(millisecond));
}
inline uint64_t proxy_getCurrentTimeNanoseconds() {
  return getCurrentTimeNanosecondsHandler(current_context_);
}

// Stream Info
inline void proxy_getProtocol(StreamType type, const char** value_ptr, size_t* value_size) {
  getProtocolHandler(current_context_, WS(type), WR(value_ptr), WR(value_size));
}

inline uint32_t proxy_getDestinationPort(StreamType type) {
  return getDestinationPortHandler(current_context_, WS(type));
}

inline uint32_t proxy_getResponseCode(StreamType type) {
  return getResponseCodeHandler(current_context_, WS(type));
}

// Metadata
inline uint64_t proxy_getMetadata(MetadataType type, const char* key_ptr, size_t key_size,
                                  const char** value_ptr_ptr, size_t* value_size_ptr) {
  return getMetadataHandler(current_context_, WS(type), WR(key_ptr), WS(key_size),
                            WR(value_ptr_ptr), WR(value_size_ptr));
}
inline uint64_t proxy_setMetadata(MetadataType type, const char* key_ptr, size_t key_size,
                                  const char* value_ptr, size_t value_size) {
  return setMetadataHandler(current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr),
                            WS(value_size));
}
inline uint64_t proxy_getMetadataPairs(MetadataType type, const char** value_ptr,
                                       size_t* value_size) {
  return getMetadataPairsHandler(current_context_, WS(type), WR(value_ptr), WR(value_size));
}
inline uint64_t proxy_getMetadataStruct(MetadataType type, const char* name_ptr, size_t name_size,
                                        const char** value_ptr_ptr, size_t* value_size_ptr) {
  return getMetadataStructHandler(current_context_, WS(type), WR(name_ptr), WS(name_size),
                                  WR(value_ptr_ptr), WR(value_size_ptr));
}
inline uint64_t proxy_setMetadataStruct(MetadataType type, const char* name_ptr, size_t name_size,
                                        const char* value_ptr, size_t value_size) {
  return setMetadataStructHandler(current_context_, WS(type), WR(name_ptr), WS(name_size),
                                  WR(value_ptr), WS(value_size));
}

// Continue
inline void proxy_continueRequest() { continueRequestHandler(current_context_); }
inline void proxy_continueResponse() { continueResponseHandler(current_context_); }
inline void proxy_sendLocalResponse(uint32_t response_code, const char* response_code_details_ptr,
                                    size_t response_code_details_size, const char* body_ptr,
                                    size_t body_size,
                                    const char* additional_response_header_pairs_ptr,
                                    size_t additional_response_header_pairs_size,
                                    uint32_t grpc_status) {
  sendLocalResponseHandler(current_context_, WS(response_code), WR(response_code_details_ptr),
                           WS(response_code_details_size), WR(body_ptr), WS(body_size),
                           WR(additional_response_header_pairs_ptr),
                           WS(additional_response_header_pairs_size), WS(grpc_status));
}
inline void proxy_clearRouteCache() { clearRouteCacheHandler(current_context_); }

// SharedData
inline void proxy_getSharedData(const char* key_ptr, size_t key_size, const char** value_ptr,
                                size_t* value_size, uint32_t* cas) {
  getSharedDataHandler(current_context_, WR(key_ptr), WS(key_size), WR(value_ptr), WR(value_size),
                       WR(cas));
}
//  If cas != 0 and cas != the current cas for 'key' return false, otherwise set the value and
//  return true.
inline bool proxy_setSharedData(const char* key_ptr, size_t key_size, const char* value_ptr,
                                size_t value_size, uint64_t cas) {
  return setSharedDataHandler(current_context_, WR(key_ptr), WS(key_size), WR(value_ptr),
                              WS(value_size), WS(cas));
}

// SharedQueue
// Note: Registering the same queue_name will overwrite the old registration while preseving any
// pending data. Consequently it should typically be followed by a call to proxy_dequeueSharedQueue.
// Returns unique token for the queue.
inline uint32_t proxy_registerSharedQueue(const char* queue_name_ptr, size_t queue_name_size) {
  return registerSharedQueueHandler(current_context_, WR(queue_name_ptr), WS(queue_name_size));
}
// Returns unique token for the queue.
inline uint32_t proxy_resolveSharedQueue(const char* vm_id_ptr, size_t vm_id_size,
                                         const char* queue_name_ptr, size_t queue_name_size) {
  return resolveSharedQueueHandler(current_context_, WR(vm_id_ptr), WS(vm_id_size),
                                   WR(queue_name_ptr), WS(queue_name_size));
}
// Returns true on end-of-stream (no more data available).
inline bool proxy_dequeueSharedQueue(uint32_t token, const char** data_ptr, size_t* data_size) {
  return dequeueSharedQueueHandler(current_context_, WS(token), WR(data_ptr), WR(data_size));
}
// Returns false if the queue was not found and the data was not enqueued.
inline bool proxy_enqueueSharedQueue(uint32_t token, const char* data_ptr, size_t data_size) {
  return enqueueSharedQueueHandler(current_context_, WS(token), WR(data_ptr), WS(data_size));
}

// Headers/Trailers/Metadata Maps
inline void proxy_addHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size,
                                    const char* value_ptr, size_t value_size) {
  addHeaderMapValueHandler(current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr),
                           WS(value_size));
}
inline void proxy_getHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size,
                                    const char** value_ptr, size_t* value_size) {
  getHeaderMapValueHandler(current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr),
                           WR(value_size));
}
inline void proxy_getHeaderMapPairs(HeaderMapType type, const char** ptr, size_t* size) {
  getHeaderMapPairsHandler(current_context_, WS(type), WR(ptr), WR(size));
}
inline void proxy_setHeaderMapPairs(HeaderMapType type, const char* ptr, size_t size) {
  setHeaderMapPairsHandler(current_context_, WS(type), WR(ptr), WS(size));
}
inline void proxy_replaceHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size,
                                        const char* value_ptr, size_t value_size) {
  replaceHeaderMapValueHandler(current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr),
                               WS(value_size));
}
inline void proxy_removeHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size) {
  removeHeaderMapValueHandler(current_context_, WS(type), WR(key_ptr), WS(key_size));
}
inline uint32_t proxy_getHeaderMapSize(HeaderMapType type) {
  return getHeaderMapSizeHandler(current_context_, WS(type));
}

// Body
inline void proxy_getRequestBodyBufferBytes(uint64_t start, uint64_t length, const char** ptr,
                                            size_t* size) {
  getRequestBodyBufferBytesHandler(current_context_, Word(start), Word(length), WR(ptr), WR(size));
}
inline void proxy_getResponseBodyBufferBytes(uint64_t start, uint64_t length, const char** ptr,
                                             size_t* size) {
  getResponseBodyBufferBytesHandler(current_context_, WS(start), WS(length), WR(ptr), WR(size));
}

// HTTP
// Returns token, used in callback onHttpCallResponse
inline uint64_t proxy_httpCall(const char* uri_ptr, size_t uri_size, void* header_pairs_ptr,
                               size_t header_pairs_size, const char* body_ptr, size_t body_size,
                               void* trailer_pairs_ptr, size_t trailer_pairs_size,
                               uint64_t timeout_milliseconds) {
  return httpCallHandler(current_context_, WR(uri_ptr), WS(uri_size), WR(header_pairs_ptr),
                         WS(header_pairs_size), WR(body_ptr), WS(body_size), WR(trailer_pairs_ptr),
                         WS(trailer_pairs_size), WS(timeout_milliseconds));
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
                         WR(request_ptr), WS(request_size), WS(timeout_milliseconds));
}
inline uint64_t proxy_grpcStream(const char* service_ptr, size_t service_size,
                                 const char* service_name_ptr, size_t service_name_size,
                                 const char* method_name_ptr, size_t method_name_size) {
  return grpcStreamHandler(current_context_, WR(service_ptr), WS(service_size),
                           WR(service_name_ptr), WS(service_name_size), WR(method_name_ptr),
                           WS(method_name_size));
}
inline void proxy_grpcCancel(uint64_t token) { grpcCancelHandler(current_context_, WS(token)); }
inline void proxy_grpcClose(uint64_t token) { grpcCloseHandler(current_context_, WS(token)); }
inline void proxy_grpcSend(uint64_t token, const char* message_ptr, size_t message_size,
                           uint64_t end_stream) {
  grpcSendHandler(current_context_, WS(token), WR(message_ptr), WS(message_size), WS(end_stream));
}

// Metrics
// Returns a metric_id which can be used to report a metric. On error returns 0.
inline uint32_t proxy_defineMetric(MetricType type, const char* name_ptr, size_t name_size) {
  return defineMetricHandler(current_context_, WS(type), WR(name_ptr), WS(name_size));
}
inline void proxy_incrementMetric(uint32_t metric_id, int64_t offset) {
  incrementMetricHandler(current_context_, WS(metric_id), WS(offset));
}
inline void proxy_recordMetric(uint32_t metric_id, uint64_t value) {
  recordMetricHandler(current_context_, WS(metric_id), WS(value));
}
inline uint64_t proxy_getMetric(uint32_t metric_id) {
  return getMetricHandler(current_context_, WS(metric_id));
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
