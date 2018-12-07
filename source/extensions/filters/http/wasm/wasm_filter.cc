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

void addHeaderHandler(void *raw_context, uint32_t type, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context, Context);
  context->addHeader(static_cast<HeaderType>(type), context->wasm_vm->getMemory(key_ptr, key_size), context->wasm_vm->getMemory(value_ptr, value_size));
}

void getHeaderHandler(void *raw_context, uint32_t type, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto context = WASM_CONTEXT(raw_context, Context);
  auto result = context->getHeader(static_cast<HeaderType>(type), context->wasm_vm->getMemory(key_ptr, key_size));
  context->wasm_vm->copyToPointerSize(result, value_ptr_ptr, value_size_ptr);
}

void replaceHeaderHandler(void *raw_context, uint32_t type, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto context = WASM_CONTEXT(raw_context, Context);
  context->replaceHeader(static_cast<HeaderType>(type), context->wasm_vm->getMemory(key_ptr, key_size),
                         context->wasm_vm->getMemory(value_ptr, value_size));
}

void removeHeaderHandler(void *raw_context, uint32_t type, uint32_t key_ptr, uint32_t key_size) {
  auto context = WASM_CONTEXT(raw_context, Context);
  context->removeHeader(static_cast<HeaderType>(type), context->wasm_vm->getMemory(key_ptr, key_size));
}

void getBodyBufferBytesHandler(void *raw_context, uint32_t start, uint32_t length,  uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context, Context);
  auto result = context->getBodyBufferBytes(start, length);
  context->wasm_vm->copyToPointerSize(result, ptr_ptr, size_ptr);
}

void getHeaderPairsHandler(void *raw_context, uint32_t type, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto context = WASM_CONTEXT(raw_context, Context);
  auto result = context->getHeaderPairs(static_cast<HeaderType>(type));
  if (result.empty()) {
    context->wasm_vm->copyToPointerSize("", ptr_ptr, size_ptr);
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
  context->wasm_vm->copyToPointerSize(absl::string_view(buffer, size), ptr_ptr, size_ptr);
  ::free(buffer);
}

}  // namespace

Context::Context(Wasm* wasm, StreamHandler* stream) :
  Envoy::Extensions::Common::Wasm::Context(wasm->wasm_vm()), wasm_(wasm), stream_(stream) { }

Context::~Context() {}

absl::string_view Context::getSharedData(absl::string_view key) {
  return stream_->filter_.config_->getSharedData(key);
}

void Context::setSharedData(absl::string_view key, absl::string_view value) {
  stream_->filter_.config_->setSharedData(key, value);
}

int Context::requestId() {
  auto& handler = stream_->filter_.request_handler_;
  if (!handler) return 0;
  return handler->context_->id;
}

int Context::responseId() {
  auto& handler = stream_->filter_.response_handler_;
  if (!handler) return 0;
  return handler->context_->id;
}

void Context::addHeader(HeaderType type, absl::string_view key, absl::string_view value) {
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  if (type == HeaderType::Header) {
    stream_->headers_.addCopy(lower_key, std::string(value));
  } else {
    if (!stream_->trailers_) return;
    stream_->trailers_->addCopy(lower_key, std::string(value));
  }
}

absl::string_view Context::getHeader(HeaderType type, absl::string_view key_view) {
  const Http::LowerCaseString lower_key(std::move(std::string(key_view)));
  Http::HeaderEntry* entry = nullptr;
  if (type == HeaderType::Header) {
    entry = stream_->headers_.get(lower_key);
  } else {
    if (!stream_->trailers_) return "";
    entry = stream_->trailers_->get(lower_key);
  }
  if (!entry) return "";
  return entry->value().getStringView();
}

Context::Pairs Context::getHeaderPairs(HeaderType type) {
  Pairs pairs;
  Http::HeaderMap* map = nullptr;
  if (type == HeaderType::Header) {
    map = &stream_->headers_;
  } else {
    if (!stream_->trailers_) return pairs;
    map = stream_->trailers_;
  }
  pairs.reserve(map->size());
  map->iterate(
      [](const Http::HeaderEntry& header, void* context) -> Http::HeaderMap::Iterate {
      (static_cast<Pairs*>(context))->push_back(std::make_pair(header.key().getStringView(), header.value().getStringView()));
      return Http::HeaderMap::Iterate::Continue;
      },
      &pairs);
  return pairs;
}

void Context::removeHeader(HeaderType type, absl::string_view key) {
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  if (type == HeaderType::Header)
    stream_->headers_.remove(lower_key);
  else
    stream_->trailers_->remove(lower_key);
}

void Context::replaceHeader(HeaderType type, absl::string_view key, absl::string_view value) {
  const Http::LowerCaseString lower_key(std::move(std::string(key)));
  Http::HeaderEntry* entry = nullptr;
  if (type == HeaderType::Header)
    entry = stream_->headers_.get(lower_key);
  else
    entry = stream_->trailers_->get(lower_key);
  if (entry != nullptr)
    entry->value(value.data(), value.size());
  else if (type == HeaderType::Header)
    stream_->headers_.addCopy(lower_key, std::string(value));
  else
    stream_->trailers_->addCopy(lower_key, std::string(value));
}

// Body Buffer

absl::string_view Context::getBodyBufferBytes(uint32_t start, uint32_t length) {
  if (!stream_->bodyBuffer_) return "";
  if (stream_->bodyBuffer_->length() < static_cast<uint64_t>((start + length))) return "";
  return absl::string_view(static_cast<char*>(stream_->bodyBuffer_->linearize(start + length)) + start, length);
}

// StreamInfo
absl::string_view Context::getSteamInfoProtocol() { return ""; }
absl::string_view Context::getStreamInfoMetadata(absl::string_view filter,
                                                 absl::string_view key) {
   (void) filter;
   (void) key;
   return "";
}
void Context::setStreamInfoMetadata(absl::string_view filter, absl::string_view key,
                                     absl::string_view value) {
  (void) filter;
  (void) key;
  (void) value;
}
Context::Pairs Context::getStreamInfoPairs(absl::string_view filter) {
  (void) filter;
  return {};
}

// HTTP
void Context::httpCall(absl::string_view cluster, const Pairs& request_headers,
                       absl::string_view request_body, int timeout_milliseconds, int token) {
  (void) cluster;
  (void) request_headers;
  (void) request_body;
  (void) timeout_milliseconds;
  (void) token;
}
void Context::httpRespond(const Pairs& response_headers,
                          absl::string_view body) {
  (void) response_headers;
  (void) body;
}

// Metadata: the values are serialized ProtobufWkt::Struct
absl::string_view Context::getMetadata(absl::string_view key) {
  (void) key;
  return "";
}
absl::string_view Context::setMetadata(absl::string_view key,
                                       absl::string_view serialized_proto_struct) {
  (void) key;
  (void) serialized_proto_struct;
  return "";
}
Context::Pairs Context::getMetadataPairs() {
  return {};
}

// Connection
bool Context::isSsl() { return false; }

//
// Calls into the WASM code.
//
Http::FilterHeadersStatus Context::onStart() { 
  if (wasm_->onStart()(this, id) == 0) {
    return Http::FilterHeadersStatus::Continue;
  }
  return  Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus Context::onBody(int body_buffer_length, bool end_of_stream) {
  switch (wasm_->onBody()(this, id, static_cast<uint32_t>(body_buffer_length),
        static_cast<uint32_t>(end_of_stream))) {
    case 0: return Http::FilterDataStatus::Continue;
    case 1: return Http::FilterDataStatus::StopIterationAndBuffer;
    case 2: return Http::FilterDataStatus::StopIterationAndWatermark;
    default: return Http::FilterDataStatus::StopIterationNoBuffer;
  }
}

Http::FilterTrailersStatus Context::onTrailers() {
  if (wasm_->onTrailers()(this, id) == 0) {
    return Http::FilterTrailersStatus::Continue;
  }
  return  Http::FilterTrailersStatus::StopIteration;
}

void Context::onHttpCallResponse(int token, const Pairs& response_headers,
                                 absl::string_view response_body) {
  (void) token;
  (void) response_headers;
  (void) response_body;
}

void Context::onDestroy() { wasm_->onDestroy()(this, id); }

Wasm::Wasm(absl::string_view vm, ThreadLocal::SlotAllocator&) {
  wasm_vm_ = Common::Wasm::createWasmVm(vm);
  if (wasm_vm_) {
    registerCallback(wasm_vm_.get(), "log", &Common::Wasm::Context::wasmLogHandler);
    registerCallback(wasm_vm_.get(), "addHeader", &addHeaderHandler);
    registerCallback(wasm_vm_.get(), "replaceHeader", &replaceHeaderHandler);
    registerCallback(wasm_vm_.get(), "getHeader", &getHeaderHandler);
    registerCallback(wasm_vm_.get(), "removeHeader", &removeHeaderHandler);
    registerCallback(wasm_vm_.get(), "getBodyBufferBytes", &getBodyBufferBytesHandler);
    registerCallback(wasm_vm_.get(), "getHeaderPairs", &getHeaderPairsHandler);
  }
}

bool Wasm::initialize(const std::string& code, absl::string_view name, bool allow_precompiled) {
  if (!wasm_vm_) return false;
  auto ok = wasm_vm_->initialize(code, name, allow_precompiled);
  if (!ok) return false;
  getFunction(wasm_vm_.get(), "_configure", &configure_);
  getFunction(wasm_vm_.get(), "_onStart", &onStart_);
  getFunction(wasm_vm_.get(), "_onBody", &onBody_);
  getFunction(wasm_vm_.get(), "_onTrailers", &onTrailers_);
  getFunction(wasm_vm_.get(), "_onDestroy", &onDestroy_);
  if (!onStart_ || !onBody_ || !onTrailers_ || !onDestroy_) return false;
  general_context_ = std::make_unique<Context>(this, nullptr); 
  allocContextId(general_context_.get());
  return true;
}

void Wasm::configure(absl::string_view configuration) {
  if (configure_) {
    auto address = wasm_vm_->copyString(configuration);
    configure_(general_context_.get(), address, configuration.size());
  }
}

void Wasm::start() {
  wasm_vm_->start(general_context_.get());
}

FilterConfig::FilterConfig(absl::string_view vm, const std::string& code, absl::string_view name,
                           bool allow_precompiled, absl::string_view configuration,
                           ThreadLocal::SlotAllocator& tls,
                           Upstream::ClusterManager& cluster_manager)
    : cluster_manager_(cluster_manager), wasm_(new Wasm(vm, tls)),
code_(code), name_(name), allow_precompiled_(allow_precompiled), configuration_(configuration) {
}

void FilterConfig::start() {
  if (started_) return;
  started_ = true;
  if (wasm_) {
    if (!wasm_->initialize(code_, name_, allow_precompiled_)) {
      ENVOY_LOG(error, "unable to initialize WASM vm");
      throw Common::Wasm::WasmException("unable to initialize WASM vm");
    }
    if (!configuration_.empty())
      wasm_->configure(configuration_);
    wasm_->start();
  }
}

void Filter::onDestroy() {
  if (destroyed_) return;
  destroyed_ = true;
  if (request_handler_) {
    request_handler_->onReset();
  }
  if (response_handler_) {
    response_handler_->onReset();
  }
}

Http::FilterHeadersStatus Filter::decodeHeaders(Http::HeaderMap& headers, bool end_stream) {
  request_handler_ = std::make_unique<StreamHandler>(headers, end_stream, *this);
  return request_handler_->context_->onStart();
}

Http::FilterDataStatus Filter::decodeData(Buffer::Instance& data, bool end_stream) {
  request_handler_->bodyBuffer_ = &data;
  auto result = request_handler_->context_->onBody(data.length(), end_stream);
  request_handler_->bodyBuffer_ = nullptr;
  return result;
}

Http::FilterTrailersStatus Filter::decodeTrailers(Http::HeaderMap& trailers) {
  request_handler_->trailers_ = &trailers;
  auto result = request_handler_->context_->onTrailers();
  request_handler_->trailers_ = nullptr;
  return result;
}

void Filter::setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) {
  decoder_callbacks_ = &callbacks;
}

Http::FilterHeadersStatus Filter::encode100ContinueHeaders(Http::HeaderMap&) {
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterHeadersStatus Filter::encodeHeaders(Http::HeaderMap& headers, bool end_stream) {
  response_handler_ = std::make_unique<StreamHandler>(headers, end_stream, *this);
  return response_handler_->context_->onStart();
}

Http::FilterDataStatus Filter::encodeData(Buffer::Instance& data, bool end_stream) {
  response_handler_->bodyBuffer_ = &data;
  auto result = response_handler_->context_->onBody(data.length(), end_stream);
  response_handler_->bodyBuffer_ = nullptr;
  return result;
}

Http::FilterTrailersStatus Filter::encodeTrailers(Http::HeaderMap& trailers) {
  response_handler_->trailers_ = &trailers;
  auto result = response_handler_->context_->onTrailers();
  response_handler_->trailers_ = nullptr;
  return result;
}

void Filter::setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

StreamHandler::StreamHandler(Http::HeaderMap& headers, bool end_of_stream, Filter& filter)
    : headers_(headers), filter_(filter), context_(filter_.createContext(this)), end_of_stream_(end_of_stream) {}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
