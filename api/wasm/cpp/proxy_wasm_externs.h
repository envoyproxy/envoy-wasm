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
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t root_context_id, uint32_t root_id_ptr, uint32_t root_id_size);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(uint32_t root_context_id, uint32_t configuration_ptr, uint32_t configuration_size);
   extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick(uint32_t root_context_id);

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
   extern "C" EMSCRIPTEN_KEEPALIVE proxy_onGrpcCreateInitialMetadata(uint32_t context_id, uint32_t token);
   extern "C" EMSCRIPTEN_KEEPALIVE proxy_onGrpcReceiveInitialMetadata(uint32_t context_id, uint32_t token);
   extern "C" EMSCRIPTEN_KEEPALIVE proxy_onGrpcTrailingMetadata(uint32_t context_id, uint32_t token);
   extern "C" EMSCRIPTEN_KEEPALIVE proxy_onGrpcReceive(uint32_t context_id, uint32_t token,
     uint32_t response_ptr, uint32_t response_size);
   extern "C" EMSCRIPTEN_KEEPALIVE proxy_onGrpcClose(uint32_t context_id, uint32_t token,
     uint32_t status_code, uint32_t status_message_ptr, uint32_t status_message_size);
*/
// clang-format on

//
// Low Level API.
//

// Logging
extern "C" void proxy_log(LogLevel level, const char* logMessage, size_t messageSize);

// Timer (must be called from a root context, e.g. onStart, onTick).
extern "C" void proxy_setTickPeriodMilliseconds(uint32_t millisecond);

// Time
extern "C" uint64_t proxy_getCurrentTimeNanoseconds();

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

// Headers/Trailers/Metadata Maps
extern "C" void proxy_addHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size, const char* value_ptr, size_t value_size);
extern "C" void proxy_getHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size, const char** value_ptr, size_t* value_size);
extern "C" void proxy_getHeaderMapPairs(HeaderMapType type, const char** ptr, size_t* size);
extern "C" void proxy_setHeaderMapPairs(HeaderMapType type, const char* ptr, size_t size);
extern "C" void proxy_replaceHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size, const char* value_ptr, size_t value_size);
extern "C" void proxy_removeHeaderMapValue(HeaderMapType type, const char* key_ptr, size_t key_size);

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
// gRPC
// Returns token, used in gRPC callbacks (onGrpc...)
extern "C" uint32_t proxy_grpcCall(const char* service_ptr, size_t service_size, const char* service_name_ptr,
                                   size_t service_name_size, const char* method_name_ptr, size_t method_name_size,
                                   const char* request_ptr, size_t request_size, uint32_t timeout_milliseconds);
extern "C" uint32_t proxy_grpcStream(const char* service_ptr, size_t service_size, const char* service_name_ptr,
                                     size_t service_name_size, const char* method_name_ptr, size_t method_name_size);
extern "C" void proxy_grpcCancel(uint32_t token);
extern "C" void proxy_grpcClose(uint32_t token);
extern "C" void proxy_grpcSend(uint32_t token, const char* message_ptr, size_t message_size, uint32_t end_stream);

// Metrics
// Returns a metric_id which can be used to report a metric. On error returns 0.
extern "C" uint32_t proxy_defineMetric(MetricType type, const char* name_ptr, size_t name_size);
extern "C" void proxy_incrementMetric(uint32_t metric_id, int64_t offset);
extern "C" void proxy_recordMetric(uint32_t metric_id, uint64_t value);
extern "C" uint64_t proxy_getMetric(uint32_t metric_id);

