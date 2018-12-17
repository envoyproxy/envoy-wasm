#include "extensions/filters/http/wasm/wasm_filter.h"

#include "envoy/http/codes.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/enum_to_int.h"
#include "common/http/message_impl.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {
namespace {

void getPairs(Filter *filter, const Pairs& result, uint32_t ptr_ptr, uint32_t size_ptr) {
  if (result.empty()) {
    filter->wasm_vm->copyToPointerSize("", ptr_ptr, size_ptr);
    return;
  }
  int size = 4;  // number of headers
  for (auto& p : result) {
    size += 8;  // size of key, size of value
    size += p.first.size() + 1;  // null terminated key
    size += p.second.size() + 1;  // null terminated value
  }
  char *buffer = static_cast<char*>(::malloc(size));
  char *b = buffer;
  *reinterpret_cast<int32_t*>(b) = result.size(); b += sizeof(int32_t);
  for (auto& p : result) {
    *reinterpret_cast<int32_t*>(b) = p.first.size(); b += sizeof(int32_t);
    *reinterpret_cast<int32_t*>(b) = p.second.size(); b += sizeof(int32_t);
  }
  for (auto& p : result) {
    memcpy(b, p.first.data(), p.first.size()); b += p.first.size(); *b++ = 0;
    memcpy(b, p.second.data(), p.second.size()); b += p.second.size(); *b++ = 0;
  }
  filter->wasm_vm->copyToPointerSize(absl::string_view(buffer, size), ptr_ptr, size_ptr);
  ::free(buffer);
}

//
// Handlers
//

// Request Headers Handlers
void addRequestHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->addRequestHeader(filter->wasm_vm->getMemory(key_ptr, key_size), filter->wasm_vm->getMemory(value_ptr, value_size));
}

void getRequestHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getRequestHeader(filter->wasm_vm->getMemory(key_ptr, key_size));
  filter->wasm_vm->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceRequestHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->replaceRequestHeader(filter->wasm_vm->getMemory(key_ptr, key_size),
                         filter->wasm_vm->getMemory(value_ptr, value_size));
}

void removeRequestHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->removeRequestHeader(filter->wasm_vm->getMemory(key_ptr, key_size));
}

void getRequestHeaderPairsHandler(void *raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getRequestHeaderPairs();
  getPairs(filter, result, ptr_ptr, size_ptr);
}

// Request Trailers Handlers
void addRequestTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->addRequestTrailer(filter->wasm_vm->getMemory(key_ptr, key_size), filter->wasm_vm->getMemory(value_ptr, value_size));
}

void getRequestTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getRequestTrailer(filter->wasm_vm->getMemory(key_ptr, key_size));
  filter->wasm_vm->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceRequestTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->replaceRequestTrailer(filter->wasm_vm->getMemory(key_ptr, key_size),
                         filter->wasm_vm->getMemory(value_ptr, value_size));
}

void removeRequestTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->removeRequestTrailer(filter->wasm_vm->getMemory(key_ptr, key_size));
}

void getRequestTrailerPairsHandler(void *raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getRequestTrailerPairs();
  getPairs(filter, result, ptr_ptr, size_ptr);
}

// Response Headers Handlers
void addResponseHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->addResponseHeader(filter->wasm_vm->getMemory(key_ptr, key_size), filter->wasm_vm->getMemory(value_ptr, value_size));
}

void getResponseHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getResponseHeader(filter->wasm_vm->getMemory(key_ptr, key_size));
  filter->wasm_vm->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceResponseHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->replaceResponseHeader(filter->wasm_vm->getMemory(key_ptr, key_size),
                         filter->wasm_vm->getMemory(value_ptr, value_size));
}

void removeResponseHeaderHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->removeResponseHeader(filter->wasm_vm->getMemory(key_ptr, key_size));
}

void getResponseHeaderPairsHandler(void *raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getResponseHeaderPairs();
  getPairs(filter, result, ptr_ptr, size_ptr);
}

// Response Trailers Handlers
void addResponseTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->addResponseTrailer(filter->wasm_vm->getMemory(key_ptr, key_size), filter->wasm_vm->getMemory(value_ptr, value_size));
}

void getResponseTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getResponseTrailer(filter->wasm_vm->getMemory(key_ptr, key_size));
  filter->wasm_vm->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceResponseTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->replaceResponseTrailer(filter->wasm_vm->getMemory(key_ptr, key_size),
                         filter->wasm_vm->getMemory(value_ptr, value_size));
}

void removeResponseTrailerHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->removeResponseTrailer(filter->wasm_vm->getMemory(key_ptr, key_size));
}

void getResponseTrailerPairsHandler(void *raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getResponseTrailerPairs();
  getPairs(filter, result, ptr_ptr, size_ptr);
}

// Body Buffer
void getRequestBodyBufferBytesHandler(void *raw_context, uint32_t start, uint32_t length,  uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getRequestBodyBufferBytes(start, length);
  filter->wasm_vm->copyToPointerSize(result, ptr_ptr, size_ptr);
}

void getResponseBodyBufferBytesHandler(void *raw_context, uint32_t start, uint32_t length,  uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto result = filter->getResponseBodyBufferBytes(start, length);
  filter->wasm_vm->copyToPointerSize(result, ptr_ptr, size_ptr);
}

uint32_t getTotalMemoryHandler(void *) {
  return 0x7FFFFFFF;
}

}  // namespace

// Shared Data
absl::string_view Filter::getSharedData(absl::string_view key) {
  return config_->getSharedData(key);
}

void Filter::setSharedData(absl::string_view key, absl::string_view value) {
  config_->setSharedData(key, value);
}

// Headers/Trailer Helper Functions
static void addHeader(Http::HeaderMap *map, absl::string_view key, absl::string_view value) {
  if (!map) return;
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  map->addCopy(lower_key, std::string(value));
}

static absl::string_view getHeader(Http::HeaderMap *map, absl::string_view key_view) {
  if (!map) return "";
  const Http::LowerCaseString lower_key(std::move(std::string(key_view)));
  auto entry = map->get(lower_key);
  if (!entry) return "";
  return entry->value().getStringView();
}

static Pairs getHeaderPairs(Http::HeaderMap *map) {
  if (!map) return {};
  Pairs pairs;
  pairs.reserve(map->size());
  map->iterate(
      [](const Http::HeaderEntry& header, void* pairs) -> Http::HeaderMap::Iterate {
      (static_cast<Pairs*>(pairs))->push_back(std::make_pair(header.key().getStringView(), header.value().getStringView()));
      return Http::HeaderMap::Iterate::Continue;
      },
      &pairs);
  return pairs;
}

static void removeHeader(Http::HeaderMap *map, absl::string_view key) {
  if (!map) return;
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  map->remove(lower_key);
}

static void replaceHeader(Http::HeaderMap *map, absl::string_view key, absl::string_view value) {
  if (!map) return;
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  auto entry = map->get(lower_key);
  if (entry != nullptr)
    entry->value(value.data(), value.size());
  else
    map->addCopy(lower_key, std::string(value));
}

// Request Headers
void Filter::addRequestHeader(absl::string_view key, absl::string_view value) {
  addHeader(request_headers_, key, value);
}

absl::string_view Filter::getRequestHeader(absl::string_view key) {
  return getHeader(request_headers_, key);
}

Pairs Filter::getRequestHeaderPairs() {
  return getHeaderPairs(request_headers_);
}

void Filter::removeRequestHeader(absl::string_view key) {
  removeHeader(request_headers_, key);
}

void Filter::replaceRequestHeader(absl::string_view key, absl::string_view value) {
  replaceHeader(request_headers_, key, value);
}

// Request Trailers
void Filter::addRequestTrailer(absl::string_view key, absl::string_view value) {
  addHeader(request_trailers_, key, value);
}

absl::string_view Filter::getRequestTrailer(absl::string_view key) {
  return getHeader(request_trailers_, key);
}

Pairs Filter::getRequestTrailerPairs() {
  return getHeaderPairs(request_trailers_);
}

void Filter::removeRequestTrailer(absl::string_view key) {
  removeHeader(request_trailers_, key);
}

void Filter::replaceRequestTrailer(absl::string_view key, absl::string_view value) {
  replaceHeader(request_trailers_, key, value);
}

// Response Headers
void Filter::addResponseHeader(absl::string_view key, absl::string_view value) {
  addHeader(response_headers_, key, value);
}

absl::string_view Filter::getResponseHeader(absl::string_view key) {
  return getHeader(response_headers_, key);
}

Pairs Filter::getResponseHeaderPairs() {
  return getHeaderPairs(response_headers_);
}

void Filter::removeResponseHeader(absl::string_view key) {
  removeHeader(response_headers_, key);
}

void Filter::replaceResponseHeader(absl::string_view key, absl::string_view value) {
  replaceHeader(response_headers_, key, value);
}

// Response Trailers
void Filter::addResponseTrailer(absl::string_view key, absl::string_view value) {
  addHeader(response_trailers_, key, value);
}

absl::string_view Filter::getResponseTrailer(absl::string_view key) {
  return getHeader(response_trailers_, key);
}

Pairs Filter::getResponseTrailerPairs() {
  return getHeaderPairs(response_trailers_);
}

void Filter::removeResponseTrailer(absl::string_view key) {
  removeHeader(response_trailers_, key);
}

void Filter::replaceResponseTrailer(absl::string_view key, absl::string_view value) {
  replaceHeader(response_trailers_, key, value);
}


// Body Buffer

absl::string_view Filter::getRequestBodyBufferBytes(uint32_t start, uint32_t length) {
  if (!requestBodyBuffer_) return "";
  if (requestBodyBuffer_->length() < static_cast<uint64_t>((start + length))) return "";
  return absl::string_view(static_cast<char*>(requestBodyBuffer_->linearize(start + length)) + start, length);
}

absl::string_view Filter::getResponseBodyBufferBytes(uint32_t start, uint32_t length) {
  if (!responseBodyBuffer_) return "";
  if (responseBodyBuffer_->length() < static_cast<uint64_t>((start + length))) return "";
  return absl::string_view(static_cast<char*>(responseBodyBuffer_->linearize(start + length)) + start, length);
}

// StreamInfo
absl::string_view Filter::getSteamInfoProtocol() { return ""; }
absl::string_view Filter::getStreamInfoMetadata(absl::string_view filter,
                                                 absl::string_view key) {
   (void) filter;
   (void) key;
   return "";
}
void Filter::setStreamInfoMetadata(absl::string_view filter, absl::string_view key,
                                     absl::string_view value) {
  (void) filter;
  (void) key;
  (void) value;
}
Pairs Filter::getStreamInfoPairs(absl::string_view filter) {
  (void) filter;
  return {};
}

// HTTP
uint32_t Filter::httpCall(absl::string_view cluster, const Pairs& request_headers,
                       absl::string_view request_body, int timeout_milliseconds) {
  (void) cluster;
  (void) request_headers;
  (void) request_body;
  (void) timeout_milliseconds;
  return 0;
}
void Filter::httpRespond(const Pairs& response_headers,
                          absl::string_view body) {
  (void) response_headers;
  (void) body;
}

// Metadata: the values are serialized ProtobufWkt::Struct
absl::string_view Filter::getMetadata(absl::string_view key) {
  (void) key;
  return "";
}
absl::string_view Filter::setMetadata(absl::string_view key,
                                       absl::string_view serialized_proto_struct) {
  (void) key;
  (void) serialized_proto_struct;
  return "";
}
Pairs Filter::getMetadataPairs() {
  return {};
}

// Connection
bool Filter::isSsl() { return false; }

//
// Calls into the WASM code.
//
void Filter::onStart() { 
  if (!wasm_->onStart) return;
  wasm_->onStart(this, id);
}

void Filter::onConfigure(absl::string_view configuration) { 
  if (!wasm_->onConfigure) return;
  if (configuration.empty()) return;
  auto address = wasm_->wasm_vm()->copyString(configuration);
  wasm_->onConfigure(this, id, address, static_cast<uint32_t>(configuration.size()));

}

Http::FilterHeadersStatus Filter::onRequestHeaders() { 
  if (!wasm_->onRequestHeaders) return Http::FilterHeadersStatus::Continue;
  if (wasm_->onRequestHeaders(this, id) == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return  Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Filter::onRequestBody(int body_buffer_length, bool end_of_stream) {
  if (!wasm_->onRequestBody) return  Http::FilterDataStatus::Continue;
  switch (wasm_->onRequestBody(this, id, static_cast<uint32_t>(body_buffer_length),
        static_cast<uint32_t>(end_of_stream))) {
    case 0: return Http::FilterDataStatus::Continue;
    case 1: return Http::FilterDataStatus::StopIterationAndBuffer;
    case 2: return Http::FilterDataStatus::StopIterationAndWatermark;
    default: return Http::FilterDataStatus::StopIterationNoBuffer;
  }
}

Http::FilterTrailersStatus Filter::onRequestTrailers() { 
  if (!wasm_->onRequestTrailers) return Http::FilterTrailersStatus::Continue;
  if (wasm_->onRequestTrailers(this, id) == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return  Http::FilterTrailersStatus::StopIteration;
}

Http::FilterHeadersStatus Filter::onResponseHeaders() { 
  if (!wasm_->onResponseHeaders) return Http::FilterHeadersStatus::Continue;
  if (wasm_->onResponseHeaders(this, id) == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return  Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Filter::onResponseBody(int body_buffer_length, bool end_of_stream) {
  if (!wasm_->onResponseBody) return  Http::FilterDataStatus::Continue;
  switch (wasm_->onResponseBody(this, id, static_cast<uint32_t>(body_buffer_length),
        static_cast<uint32_t>(end_of_stream))) {
    case 0: return Http::FilterDataStatus::Continue;
    case 1: return Http::FilterDataStatus::StopIterationAndBuffer;
    case 2: return Http::FilterDataStatus::StopIterationAndWatermark;
    default: return Http::FilterDataStatus::StopIterationNoBuffer;
  }
}

Http::FilterTrailersStatus Filter::onResponseTrailers() { 
  if (!wasm_->onResponseTrailers) return Http::FilterTrailersStatus::Continue;
  if (wasm_->onResponseTrailers(this, id) == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return  Http::FilterTrailersStatus::StopIteration;
}

void Filter::onHttpCallResponse(uint32_t token, const Pairs& response_headers,
                                 absl::string_view response_body) {
  (void) token;
  (void) response_headers;
  (void) response_body;
}

void Filter::onLogs() {
  if (!wasm_->onLogs) return;
  wasm_->onLogs(this, id);
}

Wasm::Wasm(absl::string_view vm, ThreadLocal::SlotAllocator&) {
  wasm_vm_ = Common::Wasm::createWasmVm(vm);
  if (wasm_vm_) {
    registerCallback(wasm_vm_.get(), "log", &Common::Wasm::Context::wasmLogHandler);
#define _REGISTER(_fn) registerCallback(wasm_vm_.get(), #_fn, &_fn##Handler);
    _REGISTER(getRequestHeader);
    _REGISTER(addRequestHeader);
    _REGISTER(replaceRequestHeader);
    _REGISTER(removeRequestHeader);
    _REGISTER(getRequestHeaderPairs);

    _REGISTER(getRequestTrailer);
    _REGISTER(addRequestTrailer);
    _REGISTER(replaceRequestTrailer);
    _REGISTER(removeRequestTrailer);
    _REGISTER(getRequestTrailerPairs);

    _REGISTER(getResponseHeader);
    _REGISTER(addResponseHeader);
    _REGISTER(replaceResponseHeader);
    _REGISTER(removeResponseHeader);
    _REGISTER(getResponseHeaderPairs);

    _REGISTER(getResponseTrailer);
    _REGISTER(addResponseTrailer);
    _REGISTER(replaceResponseTrailer);
    _REGISTER(removeResponseTrailer);
    _REGISTER(getResponseTrailerPairs);

    _REGISTER(getRequestBodyBufferBytes);
    _REGISTER(getResponseBodyBufferBytes);

    _REGISTER(getTotalMemory);
#undef _REGISTER
  }
}

void Wasm::getFunctions() {
#define _GET(_fn) getFunction(wasm_vm_.get(), "_" #_fn, &_fn);
  _GET(onStart);
  _GET(onConfigure);
  _GET(onRequestHeaders);
  _GET(onRequestBody);
  _GET(onRequestTrailers);
  _GET(onResponseHeaders);
  _GET(onResponseBody);
  _GET(onResponseTrailers);
  _GET(onLogs);
  _GET(onDestroy);
#undef _GET
}

Wasm::Wasm(const Wasm& wasm) {
  wasm_vm_ = wasm.wasm_vm()->Clone();
  getFunctions();
}

bool Wasm::initialize(const std::string& code, absl::string_view name, bool allow_precompiled) {
  if (!wasm_vm_) return false;
  auto ok = wasm_vm_->initialize(code, name, allow_precompiled);
  if (!ok) return false;
  getFunctions();
  return true;
}

void Wasm::createGeneralContext(FilterConfigConstSharedPtr filter_config) {
  general_context_ = std::make_unique<Filter>(filter_config, this);
  allocContextId(general_context_.get());
}

void Wasm::allocContextId(Filter *filter) {
  filter->id = next_context_id_++;
}

void Wasm::start() {
  wasm_vm_->start(general_context_.get());
}

FilterConfig::FilterConfig(absl::string_view vm, const std::string& code, absl::string_view name,
                           bool allow_precompiled, absl::string_view configuration,
                           ThreadLocal::SlotAllocator& tls,
                           Upstream::ClusterManager& cluster_manager)
    : tls_slot_(tls.allocateSlot()), cluster_manager_(cluster_manager), wasm_(new Wasm(vm, tls)), configuration_(configuration) {
  if (wasm_) {
    if (!wasm_->initialize(code, name, allow_precompiled)) {
      ENVOY_LOG(error, "unable to initialize WASM vm");
      throw Common::Wasm::WasmException("unable to initialize WASM vm");
    }
  }
}

void FilterConfig::initialize(FilterConfigConstSharedPtr this_ptr) {
  if (!wasm_) return;
  auto wasm = wasm_.get();
  auto& configuration = configuration_;
  tls_slot_->set([wasm, configuration, this_ptr](Event::Dispatcher&) {
      auto thread_wasm = std::make_unique<Wasm>(*wasm);
      thread_wasm->createGeneralContext(this_ptr);
      thread_wasm->start();  // Generic WASM Start.
      thread_wasm->general_context()->onStart();  // Filter Start.
      thread_wasm->general_context()->onConfigure(configuration);  // Initial configuartion.
      return ThreadLocal::ThreadLocalObjectSharedPtr{thread_wasm.release()};
      });
}

Filter::Filter(FilterConfigConstSharedPtr config) : Extensions::Common::Wasm::Context(config->getThreadWasm()->wasm_vm()), config_(config), wasm_(config->getThreadWasm()) {}
Filter::Filter(FilterConfigConstSharedPtr config, Wasm *wasm) : Extensions::Common::Wasm::Context(wasm->wasm_vm()), config_(config), wasm_(wasm) {}

void Filter::onDestroy() {
  if (destroyed_) return;
  destroyed_ = true;
  wasm_->onDestroy(this, id);
}

Http::FilterHeadersStatus Filter::decodeHeaders(Http::HeaderMap& headers, bool end_stream) {
  request_headers_ = &headers;
  request_end_of_stream_ = end_stream;
  return onRequestHeaders();
}

Http::FilterDataStatus Filter::decodeData(Buffer::Instance& data, bool end_stream) {
  requestBodyBuffer_ = &data;
  auto result = onRequestBody(data.length(), end_stream);
  requestBodyBuffer_ = nullptr;
  return result;
}

Http::FilterTrailersStatus Filter::decodeTrailers(Http::HeaderMap& trailers) {
  request_trailers_ = &trailers;
  return onRequestTrailers();
}

void Filter::setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) {
  decoder_callbacks_ = &callbacks;
}

Http::FilterHeadersStatus Filter::encode100ContinueHeaders(Http::HeaderMap&) {
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterHeadersStatus Filter::encodeHeaders(Http::HeaderMap& headers, bool end_stream) {
  response_headers_ = &headers;
  response_end_of_stream_ = end_stream;
  return onResponseHeaders();
}

Http::FilterDataStatus Filter::encodeData(Buffer::Instance& data, bool end_stream) {
  responseBodyBuffer_ = &data;
  auto result = onResponseBody(data.length(), end_stream);
  responseBodyBuffer_ = nullptr;
  return result;
}

Http::FilterTrailersStatus Filter::encodeTrailers(Http::HeaderMap& trailers) {
  response_trailers_ = &trailers;
  return onResponseTrailers();
}

void Filter::setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}
} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
