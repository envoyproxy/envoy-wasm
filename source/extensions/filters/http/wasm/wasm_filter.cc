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

}  // namespace

Context::Context(Wasm* wasm, StreamHandler* stream) :
  Envoy::Extensions::Common::Wasm::Context(wasm->wasm_vm()), wasm_(wasm), stream_(stream) { }

Context::~Context() {}

void Context::addHeader(HeaderType type, absl::string_view key,
    absl::string_view value) {
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
  Http::HeaderEntry* entry;
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
      nullptr);
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
  Http::HeaderEntry* entry;
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

// Decoder
void Context::continueDecoding() {}

// Encoder
void Context::continueEncoding() {}

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

// Body Buffer
void Context::setBodyMode(BodyMode body_mode) {
  (void) body_mode;
}
int Context::bodyBufferLength() {
  return 0;
}
absl::string_view Context::getBodyBufferBytes(int start, int length) {
  (void) start;
  (void) length;
  return "";
}

// HTTP
void Context::httpCall(absl::string_view cluster, const Pairs& request_headers,
                       absl::string_view request_body, int timeout_milliseconds) {
  (void) cluster;
  (void) request_headers;
  (void) request_body;
  (void) timeout_milliseconds;
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
void Context::onStart() { wasm_->onStart()(this, id); }
void Context::onBody() {}
void Context::onTrailers() {}
void Context::onHttpCallResponse(const Pairs& response_headers,
                                 absl::string_view response_body) {
  (void) response_headers;
  (void) response_body;
}
void Context::raiseWasmError(absl::string_view message) {
  (void) message;
}
void Context::onDestroy() { wasm_->onDestroy()(this, id); }

Wasm::Wasm(absl::string_view vm, ThreadLocal::SlotAllocator&) {
  wasm_vm_ = Common::Wasm::createWasmVm(vm);
  if (wasm_vm_) {
    registerCallback(wasm_vm_.get(), "log", &Common::Wasm::Context::wasmLogHandler);
    registerCallback(wasm_vm_.get(), "replaceHeader", &replaceHeaderHandler);
    registerCallback(wasm_vm_.get(), "getHeader", &getHeaderHandler);
    registerCallback(wasm_vm_.get(), "removeHeader", &removeHeaderHandler);
  }
}

bool Wasm::initialize(const std::string& code, absl::string_view name, bool allow_precompiled) {
  if (!wasm_vm_) return false;
  auto ok = wasm_vm_->initialize(code, name, allow_precompiled);
  if (!ok) return false;
  getFunction(wasm_vm_.get(), "_configure", &configure_);
  getFunction(wasm_vm_.get(), "_onStart", &onStart_);
  getFunction(wasm_vm_.get(), "_onDestroy", &onDestroy_);
  if (!onStart_ || !onDestroy_) return false;
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
    : cluster_manager_(cluster_manager), wasm_(new Wasm(vm, tls)) {
  if (wasm_) {
    if (!wasm_->initialize(code, name, allow_precompiled)) {
       ENVOY_LOG(error, "unable to initialize WASM vm");
       throw Common::Wasm::WasmException("unable to initialize WASM vm");
    }
    if (!configuration.empty())
      wasm_->configure(configuration);
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
  request_handler_->context()->onStart();
  return Http::FilterHeadersStatus::Continue;
}
Http::FilterDataStatus Filter::decodeData(Buffer::Instance& data, bool end_stream) {
  (void) data;
  (void) end_stream;
  return Http::FilterDataStatus::StopIterationNoBuffer;
}
Http::FilterTrailersStatus Filter::decodeTrailers(Http::HeaderMap& trailers) {
  (void) trailers;
  return Http::FilterTrailersStatus::Continue;
}
void Filter::setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) {
  (void) callbacks;
}

Http::FilterHeadersStatus Filter::encode100ContinueHeaders(Http::HeaderMap&) {
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterHeadersStatus Filter::encodeHeaders(Http::HeaderMap& headers, bool end_stream) {
  (void) headers;
  (void) end_stream;
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterDataStatus Filter::encodeData(Buffer::Instance& data, bool end_stream) {
  (void) data;
  (void) end_stream;
  return Http::FilterDataStatus::StopIterationNoBuffer;
}

Http::FilterTrailersStatus Filter::encodeTrailers(Http::HeaderMap& trailers) {
  (void) trailers;
  return Http::FilterTrailersStatus::Continue;
}

void Filter::setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) {
  (void) callbacks;
}

StreamHandler::StreamHandler(Http::HeaderMap& headers, bool end_stream, Filter& filter)
    : headers_(headers), end_stream_(end_stream), filter_(filter),
    context_(filter_.createContext(this)) {}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
