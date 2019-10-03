/*
 * Intrinsic functions available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#include "stddef.h"

// clang-format off
/*
   API Calls into the VM.


   // Non-stream calls.
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t root_context_id, uint32_t root_id_ptr, uint32_t root_id_size,
     uint32_t vm_configuration_ptr, uint32_t vm_configuration_size);
   extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_validateConfiguration(uint32_t root_context_id, uint32_t configuration_ptr, uint32_t configuration_size);
   extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_onConfigure(uint32_t root_context_id, uint32_t configuration_ptr, uint32_t configuration_size);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick(uint32_t root_context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onQueueReady(uint32_t root_context_id, uint32_t token);

   // Stream calls.
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onCreate(uint32_t context_id, root_context_id);
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
   extern "C" ENSCRIPTEN_KEEPALIVE void proxy_onDelete(uint32_t context_id);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcCreateInitialMetadata(uint32_t context_id, uint32_t token);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceiveInitialMetadata(uint32_t context_id, uint32_t token);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcTrailingMetadata(uint32_t context_id, uint32_t token);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceive(uint32_t context_id, uint32_t token,
     uint32_t response_ptr, uint32_t response_size);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcClose(uint32_t context_id, uint32_t token,
     uint32_t status_code, uint32_t status_message_ptr, uint32_t status_message_size);
*/
// clang-format on

//
// Low Level API.
//

// Logging
extern "C" WasmResult proxy_log(LogLevel level, const char* logMessage, size_t messageSize);

// Timer (must be called from a root context, e.g. onStart, onTick).
extern "C" WasmResult proxy_setTickPeriodMilliseconds(uint32_t millisecond);

// Time
extern "C" WasmResult proxy_getCurrentTimeNanoseconds(uint64_t* nanoseconds);

// State accessors
extern "C" WasmResult proxy_getProperty(const char* path_ptr, size_t path_size,
                                        const char** value_ptr_ptr, size_t* value_size_ptr);
extern "C" WasmResult proxy_setProperty(const char* path_ptr, size_t path_size,
                                        const char* value_ptr, size_t value_size);

// Continue/Reply/Route
extern "C" WasmResult proxy_continueRequest();
extern "C" WasmResult proxy_continueResponse();
extern "C" WasmResult
proxy_sendLocalResponse(uint32_t response_code, const char* response_code_details_ptr,
                        size_t response_code_details_size, const char* body_ptr, size_t body_size,
                        const char* additional_response_header_pairs_ptr,
                        size_t additional_response_header_pairs_size, uint32_t grpc_status);
extern "C" WasmResult proxy_clearRouteCache();

// SharedData
// Returns: Ok, NotFound
extern "C" WasmResult proxy_getSharedData(const char* key_ptr, size_t key_size,
                                          const char** value_ptr, size_t* value_size,
                                          uint32_t* cas);
//  If cas != 0 and cas != the current cas for 'key' return false, otherwise set the value and
//  return true.
// Returns: Ok, CasMismatch
extern "C" WasmResult proxy_setSharedData(const char* key_ptr, size_t key_size,
                                          const char* value_ptr, size_t value_size, uint32_t cas);

// SharedQueue
// Note: Registering the same queue_name will overwrite the old registration while preseving any
// pending data. Consequently it should typically be followed by a call to proxy_dequeueSharedQueue.
// Returns: Ok
extern "C" WasmResult proxy_registerSharedQueue(const char* queue_name_ptr, size_t queue_name_size,
                                                uint32_t* token);
// Returns: Ok, NotFound
extern "C" WasmResult proxy_resolveSharedQueue(const char* vm_id, size_t vm_id_size,
                                               const char* queue_name_ptr, size_t queue_name_size,
                                               uint32_t* token);
// Returns Ok, Empty, NotFound (token not registered).
extern "C" WasmResult proxy_dequeueSharedQueue(uint32_t token, const char** data_ptr,
                                               size_t* data_size);
// Returns false if the queue was not found and the data was not enqueued.
extern "C" WasmResult proxy_enqueueSharedQueue(uint32_t token, const char* data_ptr,
                                               size_t data_size);

// Network
extern "C" WasmResult proxy_getDownstreamDataBufferBytes(uint32_t start, uint32_t length,
                                                         const char** ptr, size_t* size);
extern "C" WasmResult proxy_getUpstreamDataBufferBytes(uint32_t start, uint32_t length,
                                                       const char** ptr, size_t* size);

// Headers/Trailers/Metadata Maps
extern "C" WasmResult proxy_addHeaderMapValue(HeaderMapType type, const char* key_ptr,
                                              size_t key_size, const char* value_ptr,
                                              size_t value_size);
extern "C" WasmResult proxy_getHeaderMapValue(HeaderMapType type, const char* key_ptr,
                                              size_t key_size, const char** value_ptr,
                                              size_t* value_size);
extern "C" WasmResult proxy_getHeaderMapPairs(HeaderMapType type, const char** ptr, size_t* size);
extern "C" WasmResult proxy_setHeaderMapPairs(HeaderMapType type, const char* ptr, size_t size);
extern "C" WasmResult proxy_replaceHeaderMapValue(HeaderMapType type, const char* key_ptr,
                                                  size_t key_size, const char* value_ptr,
                                                  size_t value_size);
extern "C" WasmResult proxy_removeHeaderMapValue(HeaderMapType type, const char* key_ptr,
                                                 size_t key_size);
extern "C" WasmResult proxy_getHeaderMapSize(HeaderMapType type, size_t* size);

// Body
extern "C" WasmResult proxy_getRequestBodyBufferBytes(uint32_t start, uint32_t length,
                                                      const char** ptr, size_t* size);
extern "C" WasmResult proxy_getResponseBodyBufferBytes(uint32_t start, uint32_t length,
                                                       const char** ptr, size_t* size);

// HTTP
// Returns token, used in callback onHttpCallResponse
extern "C" uint32_t proxy_httpCall(const char* uri_ptr, size_t uri_size, void* header_pairs_ptr,
                                   size_t header_pairs_size, const char* body_ptr, size_t body_size,
                                   void* trailer_pairs_ptr, size_t trailer_pairs_size,
                                   uint32_t timeout_milliseconds);
// gRPC
// Returns token, used in gRPC callbacks (onGrpc...)
extern "C" uint32_t proxy_grpcCall(const char* service_ptr, size_t service_size,
                                   const char* service_name_ptr, size_t service_name_size,
                                   const char* method_name_ptr, size_t method_name_size,
                                   const char* request_ptr, size_t request_size,
                                   uint32_t timeout_milliseconds);
extern "C" uint32_t proxy_grpcStream(const char* service_ptr, size_t service_size,
                                     const char* service_name_ptr, size_t service_name_size,
                                     const char* method_name_ptr, size_t method_name_size);
extern "C" WasmResult proxy_grpcCancel(uint32_t token);
extern "C" WasmResult proxy_grpcClose(uint32_t token);
extern "C" WasmResult proxy_grpcSend(uint32_t token, const char* message_ptr, size_t message_size,
                                     uint32_t end_stream);

// Metrics
extern "C" WasmResult proxy_defineMetric(MetricType type, const char* name_ptr, size_t name_size,
                                         uint32_t* metric_id);
extern "C" WasmResult proxy_incrementMetric(uint32_t metric_id, int64_t offset);
extern "C" WasmResult proxy_recordMetric(uint32_t metric_id, uint64_t value);
extern "C" WasmResult proxy_getMetric(uint32_t metric_id, uint64_t* result);

// System
extern "C" WasmResult proxy_setEffectiveContext(uint32_t effective_context_id);
