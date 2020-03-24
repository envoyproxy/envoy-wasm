#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Exports {

// Any currently executing Wasm call context.
#define WASM_CONTEXT(_c)                                                                           \
  (ContextOrEffectiveContext(static_cast<Context*>((void)_c, current_context_)))
// The id of the context which should be used for calls out of the VM in place of current_context_
// above.

namespace {

inline Word wasmResultToWord(WasmResult r) { return Word(static_cast<uint64_t>(r)); }

Context* ContextOrEffectiveContext(Context* context) {
  if (effective_context_id_ == 0) {
    return context;
  }
  auto effective_context = context->wasm()->getContext(effective_context_id_);
  if (effective_context) {
    return effective_context;
  }
  // The effective_context_id_ no longer exists, revert to the true context.
  return context;
}

Pairs toPairs(absl::string_view buffer) {
  Pairs result;
  const char* b = buffer.data();
  if (buffer.size() < sizeof(uint32_t)) {
    return {};
  }
  auto size = *reinterpret_cast<const uint32_t*>(b);
  b += sizeof(uint32_t);
  if (sizeof(uint32_t) + size * 2 * sizeof(uint32_t) > buffer.size()) {
    return {};
  }
  result.resize(size);
  for (uint32_t i = 0; i < size; i++) {
    result[i].first = absl::string_view(nullptr, *reinterpret_cast<const uint32_t*>(b));
    b += sizeof(uint32_t);
    result[i].second = absl::string_view(nullptr, *reinterpret_cast<const uint32_t*>(b));
    b += sizeof(uint32_t);
  }
  for (auto& p : result) {
    p.first = absl::string_view(b, p.first.size());
    b += p.first.size() + 1;
    p.second = absl::string_view(b, p.second.size());
    b += p.second.size() + 1;
  }
  return result;
}

template <typename Pairs> size_t pairsSize(const Pairs& result) {
  size_t size = 4; // number of headers
  for (auto& p : result) {
    size += 8;                   // size of key, size of value
    size += p.first.size() + 1;  // null terminated key
    size += p.second.size() + 1; // null terminated value
  }
  return size;
}

template <typename Pairs> void marshalPairs(const Pairs& result, char* buffer) {
  char* b = buffer;
  *reinterpret_cast<uint32_t*>(b) = result.size();
  b += sizeof(uint32_t);
  for (auto& p : result) {
    *reinterpret_cast<uint32_t*>(b) = p.first.size();
    b += sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(b) = p.second.size();
    b += sizeof(uint32_t);
  }
  for (auto& p : result) {
    memcpy(b, p.first.data(), p.first.size());
    b += p.first.size();
    *b++ = 0;
    memcpy(b, p.second.data(), p.second.size());
    b += p.second.size();
    *b++ = 0;
  }
}

template <typename Pairs>
bool getPairs(Context* context, const Pairs& result, uint64_t ptr_ptr, uint64_t size_ptr) {
  if (result.empty()) {
    return context->wasm()->copyToPointerSize("", ptr_ptr, size_ptr);
  }
  uint64_t size = pairsSize(result);
  uint64_t ptr;
  char* buffer = static_cast<char*>(context->wasm()->allocMemory(size, &ptr));
  marshalPairs(result, buffer);
  if (!context->wasmVm()->setWord(ptr_ptr, Word(ptr))) {
    return false;
  }
  if (!context->wasmVm()->setWord(size_ptr, Word(size))) {
    return false;
  }
  return true;
}

} // namespace

// General ABI.

Word set_property(void* raw_context, Word key_ptr, Word key_size, Word value_ptr, Word value_size) {
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr.u64_, key_size.u64_);
  auto value = context->wasmVm()->getMemory(value_ptr.u64_, value_size.u64_);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(context->setProperty(key.value(), value.value()));
}

// Generic selector
Word get_property(void* raw_context, Word path_ptr, Word path_size, Word value_ptr_ptr,
                  Word value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto path = context->wasmVm()->getMemory(path_ptr.u64_, path_size.u64_);
  if (!path.has_value()) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  std::string value;
  auto result = context->getProperty(path.value(), &value);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->copyToPointerSize(value, value_ptr_ptr.u64_, value_size_ptr.u64_)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word get_configuration(void* raw_context, Word value_ptr_ptr, Word value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto value = context->getConfiguration();
  if (!context->wasm()->copyToPointerSize(value, value_ptr_ptr.u64_, value_size_ptr.u64_)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word get_status(void* raw_context, Word code_ptr, Word value_ptr_ptr, Word value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto status = context->getStatus();
  if (!context->wasm()->setDatatype(code_ptr.u64_, status.first)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  if (!context->wasm()->copyToPointerSize(status.second, value_ptr_ptr.u64_, value_size_ptr.u64_)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

// HTTP

// Continue/Reply/Route
Word continue_request(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->continueRequest();
  return wasmResultToWord(WasmResult::Ok);
}

Word continue_response(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->continueResponse();
  return wasmResultToWord(WasmResult::Ok);
}

Word send_local_response(void* raw_context, Word response_code, Word response_code_details_ptr,
                         Word response_code_details_size, Word body_ptr, Word body_size,
                         Word additional_response_header_pairs_ptr,
                         Word additional_response_header_pairs_size, Word grpc_code) {
  auto context = WASM_CONTEXT(raw_context);
  auto details =
      context->wasmVm()->getMemory(response_code_details_ptr.u64_, response_code_details_size.u64_);
  auto body = context->wasmVm()->getMemory(body_ptr.u64_, body_size.u64_);
  auto additional_response_header_pairs = context->wasmVm()->getMemory(
      additional_response_header_pairs_ptr.u64_, additional_response_header_pairs_size.u64_);
  if (!details || !body || !additional_response_header_pairs) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto additional_headers = toPairs(additional_response_header_pairs.value());
  auto modify_headers = [additional_headers](Http::HeaderMap& headers) {
    for (auto& p : additional_headers) {
      const Http::LowerCaseString lower_key(std::move(std::string(p.first)));
      headers.addCopy(lower_key, std::string(p.second));
    }
  };
  auto grpc_status = static_cast<Grpc::Status::WellKnownGrpcStatus>(grpc_code.u64_);
  auto grpc_status_opt = (grpc_status != Grpc::Status::WellKnownGrpcStatus::InvalidCode)
                             ? absl::optional<Grpc::Status::WellKnownGrpcStatus>(grpc_status)
                             : absl::optional<Grpc::Status::WellKnownGrpcStatus>();
  context->addAfterVmCallAction([context, response_code, body = std::string(body.value()),
                                 modify_headers = std::move(modify_headers), grpc_status_opt,
                                 details = std::string(details.value())] {
    context->sendLocalResponse(static_cast<Envoy::Http::Code>(response_code.u64_), body,
                               modify_headers, grpc_status_opt, details);
  });
  return wasmResultToWord(WasmResult::Ok);
}

Word set_effective_context(void* raw_context, Word context_id) {
  auto context = WASM_CONTEXT(raw_context);
  uint32_t cid = static_cast<uint32_t>(context_id.u64_);
  auto c = context->wasm()->getContext(cid);
  if (!c) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  effective_context_id_ = cid;
  return wasmResultToWord(WasmResult::Ok);
}

Word done(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  return wasmResultToWord(context->wasm()->done(context));
}

Word call_foreign_function(void* raw_context, Word function_name, Word function_name_size,
                           Word arguments, Word arguments_size, Word results, Word results_size) {
  auto context = WASM_CONTEXT(raw_context);
  auto function = context->wasmVm()->getMemory(function_name.u64_, function_name_size.u64_);
  if (!function) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto args_opt = context->wasmVm()->getMemory(arguments.u64_, arguments_size.u64_);
  if (!args_opt) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto f = context->wasm()->getForeignFunction(function.value());
  if (!f) {
    return wasmResultToWord(WasmResult::NotFound);
  }
  auto& wasm = *context->wasm();
  auto& args = args_opt.value();
  uint64_t address = 0;
  void* result = nullptr;
  size_t result_size = 0;
  auto res = f(wasm, args, [&wasm, &address, &result, &result_size](size_t s) -> void* {
    result = wasm.allocMemory(s, &address);
    result_size = s;
    return result;
  });
  if (!context->wasmVm()->setWord(results.u64_, Word(address))) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  if (!context->wasmVm()->setWord(results_size.u64_, Word(result_size))) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(res);
}

Word clear_route_cache(void* raw_context) {
  auto context = WASM_CONTEXT(raw_context);
  context->clearRouteCache();
  return wasmResultToWord(WasmResult::Ok);
}

// SharedData
Word get_shared_data(void* raw_context, Word key_ptr, Word key_size, Word value_ptr_ptr,
                     Word value_size_ptr, Word cas_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr.u64_, key_size.u64_);
  if (!key) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  std::pair<std::string, uint32_t> data;
  WasmResult result = context->getSharedData(key.value(), &data);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->copyToPointerSize(data.first, value_ptr_ptr.u64_, value_size_ptr.u64_)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  if (!context->wasmVm()->setMemory(cas_ptr.u64_, sizeof(uint32_t), &data.second)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word set_shared_data(void* raw_context, Word key_ptr, Word key_size, Word value_ptr,
                     Word value_size, Word cas) {
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr.u64_, key_size.u64_);
  auto value = context->wasmVm()->getMemory(value_ptr.u64_, value_size.u64_);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(context->setSharedData(key.value(), value.value(), cas.u64_));
}

Word register_shared_queue(void* raw_context, Word queue_name_ptr, Word queue_name_size,
                           Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto queue_name = context->wasmVm()->getMemory(queue_name_ptr.u64_, queue_name_size.u64_);
  if (!queue_name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  uint32_t token = context->registerSharedQueue(queue_name.value());
  if (!context->wasm()->setDatatype(token_ptr.u64_, token)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word dequeue_shared_queue(void* raw_context, Word token, Word data_ptr_ptr, Word data_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  std::string data;
  WasmResult result = context->dequeueSharedQueue(token.u32(), &data);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->copyToPointerSize(data, data_ptr_ptr.u64_, data_size_ptr.u64_)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word resolve_shared_queue(void* raw_context, Word vm_id_ptr, Word vm_id_size, Word queue_name_ptr,
                          Word queue_name_size, Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto vm_id = context->wasmVm()->getMemory(vm_id_ptr.u64_, vm_id_size.u64_);
  auto queue_name = context->wasmVm()->getMemory(queue_name_ptr.u64_, queue_name_size.u64_);
  if (!vm_id || !queue_name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  uint32_t token = 0;
  auto result = context->resolveSharedQueue(vm_id.value(), queue_name.value(), &token);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(token_ptr.u64_, token)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word enqueue_shared_queue(void* raw_context, Word token, Word data_ptr, Word data_size) {
  auto context = WASM_CONTEXT(raw_context);
  auto data = context->wasmVm()->getMemory(data_ptr.u64_, data_size.u64_);
  if (!data) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(context->enqueueSharedQueue(token.u32(), data.value()));
}

// Header/Trailer/Metadata Maps
Word add_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size, Word value_ptr,
                          Word value_size) {
  if (type.u64_ > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr.u64_, key_size.u64_);
  auto value = context->wasmVm()->getMemory(value_ptr.u64_, value_size.u64_);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->addHeaderMapValue(static_cast<HeaderMapType>(type.u64_), key.value(), value.value());
  return wasmResultToWord(WasmResult::Ok);
}

Word get_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size,
                          Word value_ptr_ptr, Word value_size_ptr) {
  if (type.u64_ > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr.u64_, key_size.u64_);
  if (!key) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto result = context->getHeaderMapValue(static_cast<HeaderMapType>(type.u64_), key.value());
  context->wasm()->copyToPointerSize(result, value_ptr_ptr.u64_, value_size_ptr.u64_);
  return wasmResultToWord(WasmResult::Ok);
}

Word replace_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr, Word value_size) {
  if (type.u64_ > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr.u64_, key_size.u64_);
  auto value = context->wasmVm()->getMemory(value_ptr.u64_, value_size.u64_);
  if (!key || !value) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->replaceHeaderMapValue(static_cast<HeaderMapType>(type.u64_), key.value(), value.value());
  return wasmResultToWord(WasmResult::Ok);
}

Word remove_header_map_value(void* raw_context, Word type, Word key_ptr, Word key_size) {
  if (type.u64_ > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto key = context->wasmVm()->getMemory(key_ptr.u64_, key_size.u64_);
  if (!key) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->removeHeaderMapValue(static_cast<HeaderMapType>(type.u64_), key.value());
  return wasmResultToWord(WasmResult::Ok);
}

Word get_header_map_pairs(void* raw_context, Word type, Word ptr_ptr, Word size_ptr) {
  if (type.u64_ > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto result = context->getHeaderMapPairs(static_cast<HeaderMapType>(type.u64_));
  if (!getPairs(context, result, ptr_ptr.u64_, size_ptr.u64_)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word set_header_map_pairs(void* raw_context, Word type, Word ptr, Word size) {
  if (type.u64_ > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto data = context->wasmVm()->getMemory(ptr.u64_, size.u64_);
  if (!data) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->setHeaderMapPairs(static_cast<HeaderMapType>(type.u64_), toPairs(data.value()));
  return wasmResultToWord(WasmResult::Ok);
}

Word get_header_map_size(void* raw_context, Word type, Word result_ptr) {
  if (type.u64_ > static_cast<uint64_t>(HeaderMapType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  size_t result = context->getHeaderMapSize(static_cast<HeaderMapType>(type.u64_));
  if (!context->wasmVm()->setWord(result_ptr.u64_, Word(result))) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

// Buffer
Word get_buffer_bytes(void* raw_context, Word type, Word start, Word length, Word ptr_ptr,
                      Word size_ptr) {
  if (type.u64_ > static_cast<uint64_t>(BufferType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto buffer = context->getBuffer(static_cast<BufferType>(type.u64_));
  if (!buffer) {
    return wasmResultToWord(WasmResult::NotFound);
  }
  // NB: check for overflow.
  if (start.u64_ > start.u64_ + length.u64_) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  uint64_t pointer = 0;
  void* p = nullptr;
  uint64_t written = std::min(buffer->length(), length.u64_);
  if (written > 0) {
    p = context->wasm()->allocMemory(written, &pointer);
    if (!p) {
      return wasmResultToWord(WasmResult::InvalidMemoryAccess);
    }
    buffer->copyOut(start.u64_, written, p);
  }
  if (!context->wasmVm()->setWord(ptr_ptr.u64_, Word(pointer))) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  if (!context->wasmVm()->setWord(size_ptr.u64_, Word(written))) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word get_buffer_status(void* raw_context, Word type, Word length_ptr, Word flags_ptr) {
  if (type.u64_ > static_cast<uint64_t>(BufferType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto buffer = context->getBuffer(static_cast<BufferType>(type.u64_));
  if (!buffer) {
    return wasmResultToWord(WasmResult::NotFound);
  }
  auto length = buffer->length();
  uint32_t flags = context->end_of_stream() ? static_cast<uint32_t>(BufferFlags::EndOfStream) : 0;
  if (!context->wasmVm()->setWord(length_ptr.u64_, Word(length))) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  if (!context->wasm()->setDatatype(flags_ptr.u64_, flags)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word set_buffer_bytes(void* raw_context, Word type, Word start, Word length, Word data_ptr,
                      Word data_size) {
  if (type.u64_ > static_cast<uint64_t>(BufferType::MAX)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto buffer = context->getBuffer(static_cast<BufferType>(type.u64_));
  if (!buffer) {
    return wasmResultToWord(WasmResult::NotFound);
  }
  auto data = context->wasmVm()->getMemory(data_ptr.u64_, data_size.u64_);
  if (!data) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  WasmResult result = WasmResult::InternalFailure;
  if (context->setBuffer(static_cast<BufferType>(type.u64_),
                         [start = start.u64_, length = length.u64_, data, &result](auto& buffer) {
                           if (start == 0) {
                             if (length == 0) {
                               buffer.prepend(data.value());
                               result = WasmResult::Ok;
                             } else if (length >= buffer.length()) {
                               buffer.drain(buffer.length());
                               buffer.add(data.value());
                               result = WasmResult::Ok;
                             } else {
                               result = WasmResult::BadArgument;
                             }
                           } else if (start >= buffer.length()) {
                             buffer.add(data.value());
                             result = WasmResult::Ok;
                           } else {
                             result = WasmResult::BadArgument;
                           }
                         }) != WasmResult::Ok) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  return wasmResultToWord(result);
}

Word http_call(void* raw_context, Word uri_ptr, Word uri_size, Word header_pairs_ptr,
               Word header_pairs_size, Word body_ptr, Word body_size, Word trailer_pairs_ptr,
               Word trailer_pairs_size, Word timeout_milliseconds, Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context)->rootContext();
  auto uri = context->wasmVm()->getMemory(uri_ptr.u64_, uri_size.u64_);
  auto body = context->wasmVm()->getMemory(body_ptr.u64_, body_size.u64_);
  auto header_pairs = context->wasmVm()->getMemory(header_pairs_ptr.u64_, header_pairs_size.u64_);
  auto trailer_pairs =
      context->wasmVm()->getMemory(trailer_pairs_ptr.u64_, trailer_pairs_size.u64_);
  if (!uri || !body || !header_pairs || !trailer_pairs) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto headers = toPairs(header_pairs.value());
  auto trailers = toPairs(trailer_pairs.value());
  uint32_t token = 0;
  // NB: try to write the token to verify the memory before starting the async operation.
  if (!context->wasm()->setDatatype(token_ptr.u64_, token)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  auto result = context->httpCall(uri.value(), headers, body.value(), trailers,
                                  timeout_milliseconds.u64_, &token);
  context->wasm()->setDatatype(token_ptr.u64_, token);
  return wasmResultToWord(result);
}

Word define_metric(void* raw_context, Word metric_type, Word name_ptr, Word name_size,
                   Word metric_id_ptr) {
  if (metric_type.u64_ > static_cast<uint64_t>(Context::MetricType::Max)) {
    return wasmResultToWord(WasmResult::BadArgument);
  }
  auto context = WASM_CONTEXT(raw_context);
  auto name = context->wasmVm()->getMemory(name_ptr.u64_, name_size.u64_);
  if (!name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  uint32_t metric_id = 0;
  auto result = context->defineMetric(static_cast<Context::MetricType>(metric_type.u64_),
                                      name.value(), &metric_id);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(metric_id_ptr.u64_, metric_id)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word increment_metric(void* raw_context, Word metric_id, int64_t offset) {
  auto context = WASM_CONTEXT(raw_context);
  return wasmResultToWord(context->incrementMetric(metric_id.u64_, offset));
}

Word record_metric(void* raw_context, Word metric_id, uint64_t value) {
  auto context = WASM_CONTEXT(raw_context);
  return wasmResultToWord(context->recordMetric(metric_id.u64_, value));
}

Word get_metric(void* raw_context, Word metric_id, Word result_uint64_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  uint64_t value = 0;
  auto result = context->getMetric(metric_id.u64_, &value);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(result_uint64_ptr.u64_, value)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word grpc_call(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
               Word service_name_size, Word method_name_ptr, Word method_name_size,
               Word initial_metadata_ptr, Word initial_metadata_size, Word request_ptr,
               Word request_size, Word timeout_milliseconds, Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context)->rootContext();
  auto service = context->wasmVm()->getMemory(service_ptr.u64_, service_size.u64_);
  auto service_name = context->wasmVm()->getMemory(service_name_ptr.u64_, service_name_size.u64_);
  auto method_name = context->wasmVm()->getMemory(method_name_ptr.u64_, method_name_size.u64_);
  auto initial_metadata_pairs =
      context->wasmVm()->getMemory(initial_metadata_ptr.u64_, initial_metadata_size.u64_);
  auto initial_metadata = toPairs(initial_metadata_pairs.value());
  auto request = context->wasmVm()->getMemory(request_ptr.u64_, request_size.u64_);
  if (!service || !service_name || !method_name || !request) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  GrpcService service_proto;
  if (!service_proto.ParseFromArray(service.value().data(), service.value().size())) {
    return wasmResultToWord(WasmResult::ParseFailure);
  }
  uint32_t token = 0;
  auto result = context->grpcCall(service_proto, service_name.value(), method_name.value(),
                                  initial_metadata, request.value(),
                                  std::chrono::milliseconds(timeout_milliseconds.u64_), &token);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(token_ptr.u64_, token)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word grpc_stream(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
                 Word service_name_size, Word method_name_ptr, Word method_name_size,
                 Word initial_metadata_ptr, Word initial_metadata_size, Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context)->rootContext();
  auto service = context->wasmVm()->getMemory(service_ptr.u64_, service_size.u64_);
  auto service_name = context->wasmVm()->getMemory(service_name_ptr.u64_, service_name_size.u64_);
  auto method_name = context->wasmVm()->getMemory(method_name_ptr.u64_, method_name_size.u64_);
  if (!service || !service_name || !method_name) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  GrpcService service_proto;
  if (!service_proto.ParseFromArray(service.value().data(), service.value().size())) {
    return wasmResultToWord(WasmResult::ParseFailure);
  }
  auto initial_metadata_pairs =
      context->wasmVm()->getMemory(initial_metadata_ptr.u64_, initial_metadata_size.u64_);
  auto initial_metadata = toPairs(initial_metadata_pairs.value());
  uint32_t token = 0;
  auto result = context->grpcStream(service_proto, service_name.value(), method_name.value(),
                                    initial_metadata, &token);
  if (result != WasmResult::Ok) {
    return wasmResultToWord(result);
  }
  if (!context->wasm()->setDatatype(token_ptr.u64_, token)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word grpc_cancel(void* raw_context, Word token) {
  auto context = WASM_CONTEXT(raw_context)->rootContext();
  return wasmResultToWord(context->grpcCancel(token.u64_));
}

Word grpc_close(void* raw_context, Word token) {
  auto context = WASM_CONTEXT(raw_context)->rootContext();
  return wasmResultToWord(context->grpcClose(token.u64_));
}

Word grpc_send(void* raw_context, Word token, Word message_ptr, Word message_size,
               Word end_stream) {
  auto context = WASM_CONTEXT(raw_context)->rootContext();
  auto message = context->wasmVm()->getMemory(message_ptr.u64_, message_size.u64_);
  if (!message) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(context->grpcSend(token.u64_, message.value(), end_stream.u64_));
}

// Implementation of writev-like() syscall that redirects stdout/stderr to Envoy logs.
Word writevImpl(void* raw_context, Word fd, Word iovs, Word iovs_len, Word* nwritten_ptr) {
  auto context = WASM_CONTEXT(raw_context);

  // Read syscall args.
  spdlog::level::level_enum log_level;
  switch (fd.u64_) {
  case 1 /* stdout */:
    log_level = spdlog::level::info;
    break;
  case 2 /* stderr */:
    log_level = spdlog::level::err;
    break;
  default:
    return 8; // __WASI_EBADF
  }

  std::string s;
  for (size_t i = 0; i < iovs_len.u64_; i++) {
    auto memslice =
        context->wasmVm()->getMemory(iovs.u64_ + i * 2 * sizeof(uint32_t), 2 * sizeof(uint32_t));
    if (!memslice) {
      return 21; // __WASI_EFAULT
    }
    const uint32_t* iovec = reinterpret_cast<const uint32_t*>(memslice.value().data());
    if (iovec[1] /* buf_len */) {
      memslice = context->wasmVm()->getMemory(iovec[0] /* buf */, iovec[1] /* buf_len */);
      if (!memslice) {
        return 21; // __WASI_EFAULT
      }
      s.append(memslice.value().data(), memslice.value().size());
    }
  }

  size_t written = s.size();
  if (written) {
    // Remove trailing newline from the logs, if any.
    if (s[written - 1] == '\n') {
      s.erase(written - 1);
    }
    context->scriptLog(log_level, s);
  }
  *nwritten_ptr = Word(written);
  return 0; // __WASI_ESUCCESS
}

// __wasi_errno_t __wasi_fd_write(_wasi_fd_t fd, const _wasi_ciovec_t *iov, size_t iovs_len, size_t*
// nwritten);
Word wasi_unstable_fd_write(void* raw_context, Word fd, Word iovs, Word iovs_len,
                            Word nwritten_ptr) {
  auto context = WASM_CONTEXT(raw_context);

  Word nwritten(0);
  auto result = writevImpl(raw_context, fd, iovs, iovs_len, &nwritten);
  if (result.u64_ != 0) { // __WASI_ESUCCESS
    return result;
  }
  if (!context->wasmVm()->setWord(nwritten_ptr.u64_, Word(nwritten))) {
    return 21; // __WASI_EFAULT
  }
  return 0; // __WASI_ESUCCESS
}

// __wasi_errno_t __wasi_fd_read(_wasi_fd_t fd, const __wasi_iovec_t *iovs,
//    size_t iovs_len, __wasi_size_t *nread);
Word wasi_unstable_fd_read(void*, Word, Word, Word, Word) {
  // Don't support reading of any files.
  return 52; // __WASI_ERRNO_ENOSYS
}

// __wasi_errno_t __wasi_fd_seek(__wasi_fd_t fd, __wasi_filedelta_t offset, __wasi_whence_t
// whence,__wasi_filesize_t *newoffset);
Word wasi_unstable_fd_seek(void*, Word, int64_t, Word, Word) {
  throw WasmException("wasi_unstable fd_seek");
}

// __wasi_errno_t __wasi_fd_close(__wasi_fd_t fd);
Word wasi_unstable_fd_close(void*, Word) { throw WasmException("wasi_unstable fd_close"); }

// __wasi_errno_t __wasi_fd_fdstat_get(__wasi_fd_t fd, __wasi_fdstat_t *stat)
Word wasi_unstable_fd_fdstat_get(void* raw_context, Word fd, Word statOut) {
  // We will only support this interface on stdout and stderr
  if (fd.u64_ != 1 && fd.u64_ != 2) {
    return 8; // __WASI_EBADF;
  }

  // The last word points to a 24-byte structure, which we
  // are mostly going to zero out.
  uint64_t wasi_fdstat[3];
  wasi_fdstat[0] = 0;
  wasi_fdstat[1] = 64; // This sets "fs_rights_base" to __WASI_RIGHTS_FD_WRITE
  wasi_fdstat[2] = 0;

  auto context = WASM_CONTEXT(raw_context);
  context->wasmVm()->setMemory(statOut.u64_, 3 * sizeof(uint64_t), &wasi_fdstat);

  return 0; // __WASI_ESUCCESS
}

// __wasi_errno_t __wasi_environ_get(char **environ, char *environ_buf);
Word wasi_unstable_environ_get(void*, Word, Word) {
  return 0; // __WASI_ESUCCESS
}

// __wasi_errno_t __wasi_environ_sizes_get(size_t *environ_count, size_t *environ_buf_size);
Word wasi_unstable_environ_sizes_get(void* raw_context, Word count_ptr, Word buf_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  if (!context->wasmVm()->setWord(count_ptr.u64_, Word(0))) {
    return 21; // __WASI_EFAULT
  }
  if (!context->wasmVm()->setWord(buf_size_ptr.u64_, Word(0))) {
    return 21; // __WASI_EFAULT
  }
  return 0; // __WASI_ESUCCESS
}

// __wasi_errno_t __wasi_args_get(size_t **argv, size_t *argv_buf);
Word wasi_unstable_args_get(void*, Word, Word) {
  return 0; // __WASI_ESUCCESS
}

// __wasi_errno_t __wasi_args_sizes_get(size_t *argc, size_t *argv_buf_size);
Word wasi_unstable_args_sizes_get(void* raw_context, Word argc_ptr, Word argv_buf_size_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  if (!context->wasmVm()->setWord(argc_ptr.u64_, Word(0))) {
    return 21; // __WASI_EFAULT
  }
  if (!context->wasmVm()->setWord(argv_buf_size_ptr.u64_, Word(0))) {
    return 21; // __WASI_EFAULT
  }
  return 0; // __WASI_ESUCCESS
}

// void __wasi_proc_exit(__wasi_exitcode_t rval);
void wasi_unstable_proc_exit(void*, Word) { throw WasmException("wasi_unstable proc_exit"); }

Word pthread_equal(void*, Word left, Word right) { return left.u64_ == right.u64_; }

Word set_tick_period_milliseconds(void* raw_context, Word tick_period_milliseconds) {
  return wasmResultToWord(
      WASM_CONTEXT(raw_context)
          ->setTickPeriod(std::chrono::milliseconds(tick_period_milliseconds.u64_)));
}

Word get_current_time_nanoseconds(void* raw_context, Word result_uint64_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  uint64_t result = context->getCurrentTimeNanoseconds();
  if (!context->wasm()->setDatatype(result_uint64_ptr.u64_, result)) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  return wasmResultToWord(WasmResult::Ok);
}

Word log(void* raw_context, Word level, Word address, Word size) {
  auto context = WASM_CONTEXT(raw_context);
  auto message = context->wasmVm()->getMemory(address.u64_, size.u64_);
  if (!message) {
    return wasmResultToWord(WasmResult::InvalidMemoryAccess);
  }
  context->scriptLog(static_cast<spdlog::level::level_enum>(level.u64_), message.value());
  return wasmResultToWord(WasmResult::Ok);
}

} // namespace Exports
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
