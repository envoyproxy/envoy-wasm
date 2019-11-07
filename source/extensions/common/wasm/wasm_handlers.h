#pragma once

#include <memory>

#include "extensions/common/wasm/wasm_vm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

// Handlers for functions exported from envoy to wasm.
Word getConfigurationHandler(void* raw_context, Word address, Word size);
Word getStatusHandler(void* raw_context, Word status_code, Word address, Word size);
Word logHandler(void* raw_context, Word level, Word address, Word size);
Word getPropertyHandler(void* raw_context, Word path_ptr, Word path_size, Word value_ptr_ptr,
                        Word value_size_ptr);
Word setPropertyHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr,
                        Word value_size);
Word continueRequestHandler(void* raw_context);
Word continueResponseHandler(void* raw_context);
Word sendLocalResponseHandler(void* raw_context, Word response_code, Word response_code_details_ptr,
                              Word response_code_details_size, Word body_ptr, Word body_size,
                              Word additional_response_header_pairs_ptr,
                              Word additional_response_header_pairs_size, Word grpc_status);
Word clearRouteCacheHandler(void* raw_context);
Word getSharedDataHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr_ptr,
                          Word value_size_ptr, Word cas_ptr);
Word setSharedDataHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr,
                          Word value_size, Word cas);
Word registerSharedQueueHandler(void* raw_context, Word queue_name_ptr, Word queue_name_size,
                                Word token_ptr);
Word resolveSharedQueueHandler(void* raw_context, Word vm_id_ptr, Word vm_id_size,
                               Word queue_name_ptr, Word queue_name_size, Word token_ptr);
Word dequeueSharedQueueHandler(void* raw_context, Word token, Word data_ptr_ptr,
                               Word data_size_ptr);
Word enqueueSharedQueueHandler(void* raw_context, Word token, Word data_ptr, Word data_size);
Word getBufferBytesHandler(void* raw_context, Word type, Word start, Word length, Word ptr_ptr,
                           Word size_ptr);
Word getBufferStatusHandler(void* raw_context, Word type, Word length_ptr, Word flags_ptr);
Word addHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr, Word value_size);
Word getHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr_ptr, Word value_size_ptr);
Word replaceHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                                  Word value_ptr, Word value_size);
Word removeHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size);
Word getHeaderMapPairsHandler(void* raw_context, Word type, Word ptr_ptr, Word size_ptr);
Word setHeaderMapPairsHandler(void* raw_context, Word type, Word ptr, Word size);
Word getHeaderMapSizeHandler(void* raw_context, Word type, Word result_ptr);
Word getRequestBodyBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                      Word size_ptr);
Word getResponseBodyBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                       Word size_ptr);
Word httpCallHandler(void* raw_context, Word uri_ptr, Word uri_size, Word header_pairs_ptr,
                     Word header_pairs_size, Word body_ptr, Word body_size, Word trailer_pairs_ptr,
                     Word trailer_pairs_size, Word timeout_milliseconds, Word token_ptr);
Word defineMetricHandler(void* raw_context, Word metric_type, Word name_ptr, Word name_size,
                         Word result_ptr);
Word incrementMetricHandler(void* raw_context, Word metric_id, int64_t offset);
Word recordMetricHandler(void* raw_context, Word metric_id, uint64_t value);
Word getMetricHandler(void* raw_context, Word metric_id, Word result_uint64_ptr);
Word grpcCallHandler(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
                     Word service_name_size, Word method_name_ptr, Word method_name_size,
                     Word request_ptr, Word request_size, Word timeout_milliseconds,
                     Word token_ptr);
Word grpcStreamHandler(void* raw_context, Word service_ptr, Word service_size,
                       Word service_name_ptr, Word service_name_size, Word method_name_ptr,
                       Word method_name_size, Word token_ptr);
Word grpcCancelHandler(void* raw_context, Word token);
Word grpcCloseHandler(void* raw_context, Word token);
Word grpcSendHandler(void* raw_context, Word token, Word message_ptr, Word message_size,
                     Word end_stream);

Word setTickPeriodMillisecondsHandler(void* raw_context, Word tick_period_milliseconds);
Word getCurrentTimeNanosecondsHandler(void* raw_context, Word result_uint64_ptr);

Word setEffectiveContextHandler(void* raw_context, Word context_id);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
