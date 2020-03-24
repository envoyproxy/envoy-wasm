#pragma once

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

extern thread_local Envoy::Extensions::Common::Wasm::Context* current_context_;

namespace Null {
namespace Plugin {
class RootContext;
class Context;
class ContextBase;
} // namespace Plugin

Plugin::RootContext* nullVmGetRoot(absl::string_view root_id);
Plugin::Context* nullVmGetContext(uint32_t context_id);
Plugin::RootContext* nullVmGetRootContext(uint32_t context_id);
Plugin::ContextBase* nullVmGetContextBase(uint32_t context_id);

namespace Plugin {

#define WS(_x) Word(static_cast<uint64_t>(_x))
#define WR(_x) Word(reinterpret_cast<uint64_t>(_x))

inline WasmResult wordToWasmResult(Word w) { return static_cast<WasmResult>(w.u64_); }

inline WasmResult proxy_get_status(uint32_t* code_ptr, const char** ptr, size_t* size) {
  return wordToWasmResult(Exports::get_status(current_context_, WR(code_ptr), WR(ptr), WR(size)));
}

inline WasmResult proxy_get_configuration(const char** ptr, size_t* size) {
  return wordToWasmResult(Exports::get_configuration(current_context_, WR(ptr), WR(size)));
}

// Logging
inline WasmResult proxy_log(LogLevel level, const char* logMessage, size_t messageSize) {
  return wordToWasmResult(
      Exports::log(current_context_, WS(level), WR(logMessage), WS(messageSize)));
}

// Timer
inline WasmResult proxy_set_tick_period_milliseconds(uint64_t millisecond) {
  return wordToWasmResult(
      Exports::set_tick_period_milliseconds(current_context_, Word(millisecond)));
}
inline WasmResult proxy_get_current_time_nanoseconds(uint64_t* result) {
  return wordToWasmResult(Exports::get_current_time_nanoseconds(current_context_, WR(result)));
}

// State accessors
inline WasmResult proxy_get_property(const char* path_ptr, size_t path_size,
                                     const char** value_ptr_ptr, size_t* value_size_ptr) {
  return wordToWasmResult(Exports::get_property(current_context_, WR(path_ptr), WS(path_size),
                                                WR(value_ptr_ptr), WR(value_size_ptr)));
}
inline WasmResult proxy_set_property(const char* key_ptr, size_t key_size, const char* value_ptr,
                                     size_t value_size) {
  return wordToWasmResult(Exports::set_property(current_context_, WR(key_ptr), WS(key_size),
                                                WR(value_ptr), WS(value_size)));
}

// Continue
inline WasmResult proxy_continue_request() {
  return wordToWasmResult(Exports::continue_request(current_context_));
}
inline WasmResult proxy_continue_response() {
  return wordToWasmResult(Exports::continue_response(current_context_));
}
inline WasmResult
proxy_send_local_response(uint32_t response_code, const char* response_code_details_ptr,
                          size_t response_code_details_size, const char* body_ptr, size_t body_size,
                          const char* additional_response_header_pairs_ptr,
                          size_t additional_response_header_pairs_size, uint32_t grpc_status) {
  return wordToWasmResult(Exports::send_local_response(
      current_context_, WS(response_code), WR(response_code_details_ptr),
      WS(response_code_details_size), WR(body_ptr), WS(body_size),
      WR(additional_response_header_pairs_ptr), WS(additional_response_header_pairs_size),
      WS(grpc_status)));
}
inline WasmResult proxy_clear_route_cache() {
  return wordToWasmResult(Exports::clear_route_cache(current_context_));
}

// SharedData
inline WasmResult proxy_get_shared_data(const char* key_ptr, size_t key_size,
                                        const char** value_ptr, size_t* value_size, uint32_t* cas) {
  return wordToWasmResult(Exports::get_shared_data(current_context_, WR(key_ptr), WS(key_size),
                                                   WR(value_ptr), WR(value_size), WR(cas)));
}
//  If cas != 0 and cas != the current cas for 'key' return false, otherwise set the value and
//  return true.
inline WasmResult proxy_set_shared_data(const char* key_ptr, size_t key_size, const char* value_ptr,
                                        size_t value_size, uint64_t cas) {
  return wordToWasmResult(Exports::set_shared_data(current_context_, WR(key_ptr), WS(key_size),
                                                   WR(value_ptr), WS(value_size), WS(cas)));
}

// SharedQueue
// Note: Registering the same queue_name will overwrite the old registration while preseving any
// pending data. Consequently it should typically be followed by a call to
// proxy_dequeue_shared_queue. Returns unique token for the queue.
inline WasmResult proxy_register_shared_queue(const char* queue_name_ptr, size_t queue_name_size,
                                              uint32_t* token) {
  return wordToWasmResult(Exports::register_shared_queue(current_context_, WR(queue_name_ptr),
                                                         WS(queue_name_size), WR(token)));
}
// Returns unique token for the queue.
inline WasmResult proxy_resolve_shared_queue(const char* vm_id_ptr, size_t vm_id_size,
                                             const char* queue_name_ptr, size_t queue_name_size,
                                             uint32_t* token) {
  return wordToWasmResult(Exports::resolve_shared_queue(current_context_, WR(vm_id_ptr),
                                                        WS(vm_id_size), WR(queue_name_ptr),
                                                        WS(queue_name_size), WR(token)));
}
// Returns true on end-of-stream (no more data available).
inline WasmResult proxy_dequeue_shared_queue(uint32_t token, const char** data_ptr,
                                             size_t* data_size) {
  return wordToWasmResult(
      Exports::dequeue_shared_queue(current_context_, WS(token), WR(data_ptr), WR(data_size)));
}
// Returns false if the queue was not found and the data was not enqueued.
inline WasmResult proxy_enqueue_shared_queue(uint32_t token, const char* data_ptr,
                                             size_t data_size) {
  return wordToWasmResult(
      Exports::enqueue_shared_queue(current_context_, WS(token), WR(data_ptr), WS(data_size)));
}

// Buffer
inline WasmResult proxy_get_buffer_bytes(BufferType type, uint64_t start, uint64_t length,
                                         const char** ptr, size_t* size) {
  return wordToWasmResult(Exports::get_buffer_bytes(current_context_, WS(type), WS(start),
                                                    WS(length), WR(ptr), WR(size)));
}

inline WasmResult proxy_get_buffer_status(BufferType type, size_t* length_ptr,
                                          uint32_t* flags_ptr) {
  return wordToWasmResult(
      Exports::get_buffer_status(current_context_, WS(type), WR(length_ptr), WR(flags_ptr)));
}

inline WasmResult proxy_set_buffer_bytes(BufferType type, uint64_t start, uint64_t length,
                                         const char* data_ptr, size_t data_size) {
  return wordToWasmResult(Exports::set_buffer_bytes(current_context_, WS(type), WS(start),
                                                    WS(length), WR(data_ptr), WS(data_size)));
}

// Headers/Trailers/Metadata Maps
inline WasmResult proxy_add_header_map_value(HeaderMapType type, const char* key_ptr,
                                             size_t key_size, const char* value_ptr,
                                             size_t value_size) {
  return wordToWasmResult(Exports::add_header_map_value(
      current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr), WS(value_size)));
}
inline WasmResult proxy_get_header_map_value(HeaderMapType type, const char* key_ptr,
                                             size_t key_size, const char** value_ptr,
                                             size_t* value_size) {
  return wordToWasmResult(Exports::get_header_map_value(
      current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr), WR(value_size)));
}
inline WasmResult proxy_get_header_map_pairs(HeaderMapType type, const char** ptr, size_t* size) {
  return wordToWasmResult(
      Exports::get_header_map_pairs(current_context_, WS(type), WR(ptr), WR(size)));
}
inline WasmResult proxy_set_header_map_pairs(HeaderMapType type, const char* ptr, size_t size) {
  return wordToWasmResult(
      Exports::set_header_map_pairs(current_context_, WS(type), WR(ptr), WS(size)));
}
inline WasmResult proxy_replace_header_map_value(HeaderMapType type, const char* key_ptr,
                                                 size_t key_size, const char* value_ptr,
                                                 size_t value_size) {
  return wordToWasmResult(Exports::replace_header_map_value(
      current_context_, WS(type), WR(key_ptr), WS(key_size), WR(value_ptr), WS(value_size)));
}
inline WasmResult proxy_remove_header_map_value(HeaderMapType type, const char* key_ptr,
                                                size_t key_size) {
  return wordToWasmResult(
      Exports::remove_header_map_value(current_context_, WS(type), WR(key_ptr), WS(key_size)));
}
inline WasmResult proxy_get_header_map_size(HeaderMapType type, size_t* size) {
  return wordToWasmResult(Exports::get_header_map_size(current_context_, WS(type), WR(size)));
}

// HTTP
// Returns token, used in callback onHttpCallResponse
inline WasmResult proxy_http_call(const char* uri_ptr, size_t uri_size, void* header_pairs_ptr,
                                  size_t header_pairs_size, const char* body_ptr, size_t body_size,
                                  void* trailer_pairs_ptr, size_t trailer_pairs_size,
                                  uint64_t timeout_milliseconds, uint32_t* token_ptr) {
  return wordToWasmResult(
      Exports::http_call(current_context_, WR(uri_ptr), WS(uri_size), WR(header_pairs_ptr),
                         WS(header_pairs_size), WR(body_ptr), WS(body_size), WR(trailer_pairs_ptr),
                         WS(trailer_pairs_size), WS(timeout_milliseconds), WR(token_ptr)));
}
// gRPC
// Returns token, used in gRPC callbacks (onGrpc...)
inline WasmResult proxy_grpc_call(const char* service_ptr, size_t service_size,
                                  const char* service_name_ptr, size_t service_name_size,
                                  const char* method_name_ptr, size_t method_name_size,
                                  void* initial_metadata_ptr, size_t initial_metadata_size,
                                  const char* request_ptr, size_t request_size,
                                  uint64_t timeout_milliseconds, uint32_t* token_ptr) {
  return wordToWasmResult(
      Exports::grpc_call(current_context_, WR(service_ptr), WS(service_size), WR(service_name_ptr),
                         WS(service_name_size), WR(method_name_ptr), WS(method_name_size),
                         WR(initial_metadata_ptr), WS(initial_metadata_size), WR(request_ptr),
                         WS(request_size), WS(timeout_milliseconds), WR(token_ptr)));
}
inline WasmResult proxy_grpc_stream(const char* service_ptr, size_t service_size,
                                    const char* service_name_ptr, size_t service_name_size,
                                    const char* method_name_ptr, size_t method_name_size,
                                    void* initial_metadata_ptr, size_t initial_metadata_size,
                                    uint32_t* token_ptr) {
  return wordToWasmResult(Exports::grpc_stream(
      current_context_, WR(service_ptr), WS(service_size), WR(service_name_ptr),
      WS(service_name_size), WR(method_name_ptr), WS(method_name_size), WR(initial_metadata_ptr),
      WS(initial_metadata_size), WR(token_ptr)));
}
inline WasmResult proxy_grpc_cancel(uint64_t token) {
  return wordToWasmResult(Exports::grpc_cancel(current_context_, WS(token)));
}
inline WasmResult proxy_grpc_close(uint64_t token) {
  return wordToWasmResult(Exports::grpc_close(current_context_, WS(token)));
}
inline WasmResult proxy_grpc_send(uint64_t token, const char* message_ptr, size_t message_size,
                                  uint64_t end_stream) {
  return wordToWasmResult(Exports::grpc_send(current_context_, WS(token), WR(message_ptr),
                                             WS(message_size), WS(end_stream)));
}

// Metrics
// Returns a metric_id which can be used to report a metric. On error returns 0.
inline WasmResult proxy_define_metric(MetricType type, const char* name_ptr, size_t name_size,
                                      uint32_t* metric_id) {
  return wordToWasmResult(Exports::define_metric(current_context_, WS(type), WR(name_ptr),
                                                 WS(name_size), WR(metric_id)));
}
inline WasmResult proxy_increment_metric(uint32_t metric_id, int64_t offset) {
  return wordToWasmResult(Exports::increment_metric(current_context_, WS(metric_id), offset));
}
inline WasmResult proxy_record_metric(uint32_t metric_id, uint64_t value) {
  return wordToWasmResult(Exports::record_metric(current_context_, WS(metric_id), value));
}
inline WasmResult proxy_get_metric(uint32_t metric_id, uint64_t* value) {
  return wordToWasmResult(Exports::get_metric(current_context_, WS(metric_id), WR(value)));
}

// System
inline WasmResult proxy_set_effective_context(uint64_t context_id) {
  return wordToWasmResult(Exports::set_effective_context(current_context_, WS(context_id)));
}
inline WasmResult proxy_done() { return wordToWasmResult(Exports::done(current_context_)); }

inline WasmResult proxy_call_foreign_function(const char* function_name, size_t function_name_size,
                                              const char* arguments, size_t arguments_size,
                                              char** results, size_t* results_size) {
  return wordToWasmResult(Exports::call_foreign_function(
      current_context_, WR(function_name), WS(function_name_size), WR(arguments),
      WS(arguments_size), WR(results), WR(results_size)));
}

#undef WS
#undef WR

inline RootContext* getRoot(StringView root_id) { return nullVmGetRoot(root_id); }
inline Plugin::Context* getContext(uint32_t context_id) { return nullVmGetContext(context_id); }
inline Plugin::RootContext* getRootContext(uint32_t context_id) {
  return nullVmGetRootContext(context_id);
}
inline Plugin::ContextBase* getContextBase(uint32_t context_id) {
  return nullVmGetContextBase(context_id);
}

} // namespace Plugin
} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
