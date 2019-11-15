#pragma once

#include <memory>

#include "extensions/common/wasm/wasm_vm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Exports {

// ABI functions exported from envoy to wasm.

Word getConfiguration(void* raw_context, Word address, Word size);
Word getStatus(void* raw_context, Word status_code, Word address, Word size);
Word log(void* raw_context, Word level, Word address, Word size);
Word getProperty(void* raw_context, Word path_ptr, Word path_size, Word value_ptr_ptr,
                 Word value_size_ptr);
Word setProperty(void* raw_context, Word key_ptr, Word key_size, Word value_ptr, Word value_size);
Word continueRequest(void* raw_context);
Word continueResponse(void* raw_context);
Word sendLocalResponse(void* raw_context, Word response_code, Word response_code_details_ptr,
                       Word response_code_details_size, Word body_ptr, Word body_size,
                       Word additional_response_header_pairs_ptr,
                       Word additional_response_header_pairs_size, Word grpc_status);
Word clearRouteCache(void* raw_context);
Word getSharedData(void* raw_context, Word key_ptr, Word key_size, Word value_ptr_ptr,
                   Word value_size_ptr, Word cas_ptr);
Word setSharedData(void* raw_context, Word key_ptr, Word key_size, Word value_ptr, Word value_size,
                   Word cas);
Word registerSharedQueue(void* raw_context, Word queue_name_ptr, Word queue_name_size,
                         Word token_ptr);
Word resolveSharedQueue(void* raw_context, Word vm_id_ptr, Word vm_id_size, Word queue_name_ptr,
                        Word queue_name_size, Word token_ptr);
Word dequeueSharedQueue(void* raw_context, Word token, Word data_ptr_ptr, Word data_size_ptr);
Word enqueueSharedQueue(void* raw_context, Word token, Word data_ptr, Word data_size);
Word getBufferBytes(void* raw_context, Word type, Word start, Word length, Word ptr_ptr,
                    Word size_ptr);
Word getBufferStatus(void* raw_context, Word type, Word length_ptr, Word flags_ptr);
Word addHeaderMapValue(void* raw_context, Word type, Word key_ptr, Word key_size, Word value_ptr,
                       Word value_size);
Word getHeaderMapValue(void* raw_context, Word type, Word key_ptr, Word key_size,
                       Word value_ptr_ptr, Word value_size_ptr);
Word replaceHeaderMapValue(void* raw_context, Word type, Word key_ptr, Word key_size,
                           Word value_ptr, Word value_size);
Word removeHeaderMapValue(void* raw_context, Word type, Word key_ptr, Word key_size);
Word getHeaderMapPairs(void* raw_context, Word type, Word ptr_ptr, Word size_ptr);
Word setHeaderMapPairs(void* raw_context, Word type, Word ptr, Word size);
Word getHeaderMapSize(void* raw_context, Word type, Word result_ptr);
Word getRequestBodyBufferBytes(void* raw_context, Word start, Word length, Word ptr_ptr,
                               Word size_ptr);
Word getResponseBodyBufferBytes(void* raw_context, Word start, Word length, Word ptr_ptr,
                                Word size_ptr);
Word httpCall(void* raw_context, Word uri_ptr, Word uri_size, Word header_pairs_ptr,
              Word header_pairs_size, Word body_ptr, Word body_size, Word trailer_pairs_ptr,
              Word trailer_pairs_size, Word timeout_milliseconds, Word token_ptr);
Word defineMetric(void* raw_context, Word metric_type, Word name_ptr, Word name_size,
                  Word result_ptr);
Word incrementMetric(void* raw_context, Word metric_id, int64_t offset);
Word recordMetric(void* raw_context, Word metric_id, uint64_t value);
Word getMetric(void* raw_context, Word metric_id, Word result_uint64_ptr);
Word grpcCall(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
              Word service_name_size, Word method_name_ptr, Word method_name_size, Word request_ptr,
              Word request_size, Word timeout_milliseconds, Word token_ptr);
Word grpcStream(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
                Word service_name_size, Word method_name_ptr, Word method_name_size,
                Word token_ptr);
Word grpcCancel(void* raw_context, Word token);
Word grpcClose(void* raw_context, Word token);
Word grpcSend(void* raw_context, Word token, Word message_ptr, Word message_size, Word end_stream);

Word setTickPeriodMilliseconds(void* raw_context, Word tick_period_milliseconds);
Word getCurrentTimeNanoseconds(void* raw_context, Word result_uint64_ptr);

Word setEffectiveContext(void* raw_context, Word context_id);
Word done(void* raw_context);

// Runtime environment functions exported from envoy to wasm.

Word wasi_unstable_fd_write(void* raw_context, Word fd, Word iovs, Word iovs_len,
                            Word nwritten_ptr);
Word wasi_unstable_fd_seek(void*, Word, int64_t, Word, Word);
Word wasi_unstable_fd_close(void*, Word);
Word wasi_unstable_environ_get(void*, Word, Word);
Word wasi_unstable_environ_sizes_get(void* raw_context, Word count_ptr, Word buf_size_ptr);
void wasi_unstable_proc_exit(void*, Word);
void wasi_unstable_proc_exit(void*, Word);
Word pthread_equal(void*, Word left, Word right);

} // namespace Exports
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
