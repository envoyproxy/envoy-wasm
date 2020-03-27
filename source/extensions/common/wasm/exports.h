#pragma once

#include <memory>

#include "extensions/common/wasm/wasm_vm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Exports {

// ABI functions exported from envoy to wasm.

Word get_configuration(void* raw_context, Word address, Word size);
Word get_status(void* raw_context, Word status_code, Word address, Word size);
Word log(void* raw_context, Word level, Word address, Word size);
Word get_property(void* raw_context, Word path_ptr, Word path_size, Word value_ptr_ptr,
                  Word value_size_ptr);
Word set_property(void* raw_context, Word key_ptr, Word key_size, Word value_ptr, Word value_size);
Word continue_request(void* raw_context);
Word continue_response(void* raw_context);
Word send_local_response(void* raw_context, Word response_code, Word response_code_details_ptr,
                         Word response_code_details_size, Word body_ptr, Word body_size,
                         Word additional_response_header_pairs_ptr,
                         Word additional_response_header_pairs_size, Word grpc_status);
Word clear_route_cache(void* raw_context);
Word get_shared_data(void* raw_context, Word key_ptr, Word key_size, Word value_ptr_ptr,
                     Word value_size_ptr, Word cas_ptr);
Word set_shared_data(void* raw_context, Word key_ptr, Word key_size, Word value_ptr,
                     Word value_size, Word cas);
Word register_shared_queue(void* raw_context, Word queue_name_ptr, Word queue_name_size,
                           Word token_ptr);
Word resolve_shared_queue(void* raw_context, Word vm_id_ptr, Word vm_id_size, Word queue_name_ptr,
                          Word queue_name_size, Word token_ptr);
Word dequeue_shared_queue(void* raw_context, Word token, Word data_ptr_ptr, Word data_size_ptr);
Word enqueue_shared_queue(void* raw_context, Word token, Word data_ptr, Word data_size);
Word get_buffer_bytes(void* raw_context, Word type, Word start, Word length, Word ptr_ptr,
                      Word size_ptr);
Word get_buffer_status(void* raw_context, Word type, Word length_ptr, Word flags_ptr);
Word set_buffer_bytes(void* raw_context, Word type, Word start, Word length, Word data_ptr,
                      Word data_size);
Word add_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size, Word value_ptr,
                          Word value_size);
Word get_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size,
                          Word value_ptr_ptr, Word value_size_ptr);
Word replace_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr, Word value_size);
Word remove_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size);
Word get_header_map_pairs(void* raw_context, Word type, Word ptr_ptr, Word size_ptr);
Word set_header_map_pairs(void* raw_context, Word type, Word ptr, Word size);
Word get_header_map_size(void* raw_context, Word type, Word result_ptr);
Word getRequestBodyBufferBytes(void* raw_context, Word start, Word length, Word ptr_ptr,
                               Word size_ptr);
Word get_response_body_buffer_bytes(void* raw_context, Word start, Word length, Word ptr_ptr,
                                    Word size_ptr);
Word http_call(void* raw_context, Word uri_ptr, Word uri_size, Word header_pairs_ptr,
               Word header_pairs_size, Word body_ptr, Word body_size, Word trailer_pairs_ptr,
               Word trailer_pairs_size, Word timeout_milliseconds, Word token_ptr);
Word define_metric(void* raw_context, Word metric_type, Word name_ptr, Word name_size,
                   Word result_ptr);
Word increment_metric(void* raw_context, Word metric_id, int64_t offset);
Word record_metric(void* raw_context, Word metric_id, uint64_t value);
Word get_metric(void* raw_context, Word metric_id, Word result_uint64_ptr);
Word grpc_call(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
               Word service_name_size, Word method_name_ptr, Word method_name_size,
               Word initial_metadata_ptr, Word initial_metadatda_size, Word request_ptr,
               Word request_size, Word timeout_milliseconds, Word token_ptr);
Word grpc_stream(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
                 Word service_name_size, Word method_name_ptr, Word method_name_size,
                 Word initial_metadata_ptr, Word initial_metadatda_size, Word token_ptr);
Word grpc_cancel(void* raw_context, Word token);
Word grpc_close(void* raw_context, Word token);
Word grpc_send(void* raw_context, Word token, Word message_ptr, Word message_size, Word end_stream);

Word set_tick_period_milliseconds(void* raw_context, Word tick_period_milliseconds);
Word get_current_time_nanoseconds(void* raw_context, Word result_uint64_ptr);

Word set_effective_context(void* raw_context, Word context_id);
Word done(void* raw_context);
Word call_foreign_function(void* raw_context, Word function_name, Word function_name_size,
                           Word arguments, Word warguments_size, Word results, Word results_size);

// Runtime environment functions exported from envoy to wasm.

Word wasi_unstable_fd_write(void* raw_context, Word fd, Word iovs, Word iovs_len,
                            Word nwritten_ptr);
Word wasi_unstable_fd_read(void*, Word, Word, Word, Word);
Word wasi_unstable_fd_seek(void*, Word, int64_t, Word, Word);
Word wasi_unstable_fd_close(void*, Word);
Word wasi_unstable_fd_fdstat_get(void*, Word fd, Word statOut);
Word wasi_unstable_environ_get(void*, Word, Word);
Word wasi_unstable_environ_sizes_get(void* raw_context, Word count_ptr, Word buf_size_ptr);
Word wasi_unstable_args_get(void* raw_context, Word argc_ptr, Word argv_buf_size_ptr);
Word wasi_unstable_args_sizes_get(void* raw_context, Word argc_ptr, Word argv_buf_size_ptr);
void wasi_unstable_proc_exit(void*, Word);
void wasi_unstable_proc_exit(void*, Word);
Word pthread_equal(void*, Word left, Word right);

} // namespace Exports
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
