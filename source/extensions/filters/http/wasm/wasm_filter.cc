#include "extensions/filters/http/wasm/wasm_filter.h"

#include "envoy/http/codes.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/enum_to_int.h"
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {
namespace {

template<typename Pairs>
size_t pairsSize(const Pairs& result) {
  size_t size = 4;  // number of headers
  for (auto& p : result) {
    size += 8;  // size of key, size of value
    size += p.first.size() + 1;  // null terminated key
    size += p.second.size() + 1;  // null terminated value
  }
  return size;
}

template<typename Pairs>
void marshalPairs(const Pairs& result, char *buffer) {
  char *b = buffer;
  *reinterpret_cast<uint32_t*>(b) = result.size(); b += sizeof(uint32_t);
  for (auto& p : result) {
    *reinterpret_cast<uint32_t*>(b) = p.first.size(); b += sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(b) = p.second.size(); b += sizeof(uint32_t);
  }
  for (auto& p : result) {
    memcpy(b, p.first.data(), p.first.size()); b += p.first.size(); *b++ = 0;
    memcpy(b, p.second.data(), p.second.size()); b += p.second.size(); *b++ = 0;
  }
}

Pairs toPairs(absl::string_view buffer) {
  Pairs result;
  const char *b = buffer.data();
  if (buffer.size() < sizeof(uint32_t)) return {};
  auto size = *reinterpret_cast<const uint32_t*>(b);  b += sizeof(uint32_t);
  if (sizeof(uint32_t) + size * 2 * sizeof(uint32_t) > buffer.size()) return {};
  result.resize(size);
  for (uint32_t i = 0; i < size; i++) {
    result[i].first = absl::string_view(nullptr, *reinterpret_cast<const uint32_t*>(b)); b += sizeof(uint32_t);
    result[i].second = absl::string_view(nullptr, *reinterpret_cast<const uint32_t*>(b)); b += sizeof(uint32_t);
  }
  for (auto& p : result) {
    p.first = absl::string_view(b, p.first.size()); b += p.first.size() + 1;
    p.second = absl::string_view(b, p.second.size()); b += p.second.size() + 1;
  }
  return result;
}

template<typename Pairs>
void getPairs(Filter *filter, const Pairs& result, uint32_t ptr_ptr, uint32_t size_ptr) {
  if (result.empty()) {
    filter->wasm_vm->copyToPointerSize("", ptr_ptr, size_ptr);
    return;
  }
  uint32_t size = pairsSize(result);
  uint32_t ptr; 
  char *buffer = static_cast<char*>(filter->wasm_vm->allocMemory(size, &ptr));
  marshalPairs(result, buffer);
  filter->wasm_vm->setMemory(ptr_ptr, sizeof(int32_t), &ptr);
  filter->wasm_vm->setMemory(size_ptr, sizeof(int32_t), &size);
}

void exportPairs(Filter *filter, const Pairs& pairs, uint32_t *ptr_ptr, uint32_t *size_ptr) {
  if (pairs.empty()) {
    *ptr_ptr = 0;
    *size_ptr = 0;
    return;
  }
  uint32_t size = pairsSize(pairs);
  char *buffer = static_cast<char*>(filter->wasm_vm->allocMemory(size, ptr_ptr));
  marshalPairs(pairs, buffer);
  *size_ptr = size;
}

Http::HeaderMapPtr buildHeaderMapFromPairs(const Pairs& pairs) {
  auto map = std::make_unique<Http::HeaderMapImpl>();
  for (auto& p : pairs) {
    // Note: because of the lack of a string_view interface for addCopy and
    // the lack of an interface to add an entry with an empty value and return
    // the entry, there is no efficient way to prevent either a double copy 
    // of the valueor a double lookup of the entry.
    map->addCopy(Http::LowerCaseString(std::string(p.first)), std::string(p.second));
  }
  return map;
}

//
// Handlers
//

// StreamInfo
void getRequestStreamInfoProtocolHandler(void *raw_context, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->wasm_vm->copyToPointerSize(filter->getRequestStreamInfoProtocol(), value_ptr_ptr, value_size_ptr);
}

void getResponseStreamInfoProtocolHandler(void *raw_context, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->wasm_vm->copyToPointerSize(filter->getResponseStreamInfoProtocol(), value_ptr_ptr, value_size_ptr);
}

void getRequestMetadataHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->wasm_vm->copyToPointerSize(filter->getRequestMetadata(filter->wasm_vm->getMemory(key_ptr, key_size)), value_ptr_ptr, value_size_ptr);
}

void setRequestMetadataHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->setRequestMetadata(filter->wasm_vm->getMemory(key_ptr, key_size), filter->wasm_vm->getMemory(value_ptr, value_size));
}

void getRequestMetadataPairsHandler(void *raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  getPairs(filter, filter->getRequestMetadataPairs(), ptr_ptr, size_ptr);
}

void getResponseMetadataHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->wasm_vm->copyToPointerSize(filter->getResponseMetadata(filter->wasm_vm->getMemory(key_ptr, key_size)), value_ptr_ptr, value_size_ptr);
}

void setResponseMetadataHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->setResponseMetadata(filter->wasm_vm->getMemory(key_ptr, key_size), filter->wasm_vm->getMemory(value_ptr, value_size));
}

void getResponseMetadataPairsHandler(void *raw_context, uint32_t ptr_ptr, uint32_t size_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  getPairs(filter, filter->getResponseMetadataPairs(), ptr_ptr, size_ptr);
}

// Continue
void continueRequestHandler(void *raw_context) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->continueRequest();
}

void continueResponseHandler(void *raw_context) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  filter->continueResponse();
}

// SharedData
void getSharedDataHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr, uint32_t cas_ptr) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto key = filter->wasm_vm->getMemory(key_ptr, key_size);
  auto p = filter->getSharedData(key);
  filter->wasm_vm->copyToPointerSize(p.first, value_ptr_ptr, value_size_ptr);
  filter->wasm_vm->setMemory(cas_ptr, sizeof(uint32_t), &p.second);
}

uint32_t setSharedDataHandler(void *raw_context, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size, uint32_t cas) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto key = filter->wasm_vm->getMemory(key_ptr, key_size);
  auto value = filter->wasm_vm->getMemory(value_ptr, value_size);
  auto ok = filter->setSharedData(key, value, cas);
  return static_cast<uint32_t>(ok);
}

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

uint32_t httpCallHandler(void *raw_context, uint32_t uri_ptr, uint32_t uri_size, uint32_t header_pairs_ptr, uint32_t header_pairs_size, uint32_t body_ptr, uint32_t body_size, uint32_t trailer_pairs_ptr, uint32_t trailer_pairs_size, uint32_t timeout_milliseconds) {
  auto filter = WASM_CONTEXT(raw_context, Filter);
  auto uri = filter->wasm_vm->getMemory(uri_ptr, uri_size);
  auto headers = toPairs(filter->wasm_vm->getMemory(header_pairs_ptr, header_pairs_size));
  auto body = filter->wasm_vm->getMemory(body_ptr, body_size);
  auto trailers = toPairs(filter->wasm_vm->getMemory(trailer_pairs_ptr, trailer_pairs_size));
  return filter->httpCall(uri, headers, body, trailers, timeout_milliseconds);
}

uint32_t getTotalMemoryHandler(void *) {
  return 0x7FFFFFFF;
}

}  // namespace

// Shared Data
std::pair<std::string, uint32_t> Filter::getSharedData(absl::string_view key) {
  return config_->getSharedData(key);
}

bool Filter::setSharedData(absl::string_view key, absl::string_view value, uint32_t cas) {
  return config_->setSharedData(key, value, cas);
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

// HTTP
uint32_t Filter::httpCall(absl::string_view cluster, const Pairs& request_headers,
    absl::string_view request_body, const Pairs& request_trailers, int timeout_milliseconds) {
  if (timeout_milliseconds < 0) return 0;
  auto cluster_string = std::string(cluster);
  if (clusterManager().get(cluster_string) == nullptr) return 0;

  Http::MessagePtr message(new Http::RequestMessageImpl(buildHeaderMapFromPairs(request_headers)));

  // Check that we were provided certain headers.
  if (message->headers().Path() == nullptr || message->headers().Method() == nullptr ||
      message->headers().Host() == nullptr) {
    return 0;
  }

  if (!request_body.empty()) {
    message->body().reset(new Buffer::OwnedImpl(request_body.data(), request_body.size()));
    message->headers().insertContentLength().value(request_body.size());
  }

  if (request_trailers.size() > 0) {
    message->trailers(buildHeaderMapFromPairs(request_trailers));
  }

  absl::optional<std::chrono::milliseconds> timeout;
  if (timeout_milliseconds > 0) {
    timeout = std::chrono::milliseconds(timeout_milliseconds);
  }

  auto token = next_async_token_++;
  auto& handler = http_request_[token];
  auto http_request = clusterManager()
                          .httpAsyncClientForCluster(cluster_string)
                          .send(std::move(message), handler,
                                Http::AsyncClient::RequestOptions().setTimeout(timeout));
  if (!http_request) {
    http_request_.erase(token);
    return 0;
  }
  handler.filter = this;
  handler.token = token;
  handler.request = http_request;
  return token;
}

void Filter::httpRespond(const Pairs& response_headers, absl::string_view body, const Pairs& response_trailers) {
  (void) response_headers;
  (void) body;
  (void) response_trailers;
}

// StreamInfo
std::string Filter::getRequestStreamInfoProtocol() {
  if (!decoder_callbacks_) return "";
  return Http::Utility::getProtocolString(decoder_callbacks_->streamInfo().protocol().value());
}

std::string Filter::getResponseStreamInfoProtocol() {
  if (!encoder_callbacks_) return "";
  return Http::Utility::getProtocolString(encoder_callbacks_->streamInfo().protocol().value());
}

// Metadata: the values are serialized ProtobufWkt::Struct
std::string Filter::getRequestMetadata(absl::string_view key) {
  if (!decoder_callbacks_) return "";
  auto& proto_struct = getMetadata(decoder_callbacks_);
  auto it = proto_struct.fields().find(std::string(key));
  if (it ==  proto_struct.fields().end()) return "";
  std::string result;
  it->second.SerializeToString(&result);
  return result;
}

void Filter::setRequestMetadata(absl::string_view key, absl::string_view serialized_proto_struct) {
  if (!decoder_callbacks_) return;
  decoder_callbacks_->streamInfo().setDynamicMetadata(HttpFilterNames::get().Wasm, MessageUtil::keyValueStruct(std::string(key), std::string(serialized_proto_struct)));
}

PairsWithStringValues Filter::getRequestMetadataPairs() {
  PairsWithStringValues result;
  if (!encoder_callbacks_) return {};
  auto& proto_struct = getMetadata(encoder_callbacks_);
  for (auto &p : proto_struct.fields()) {
    std::string value;
    p.second.SerializeToString(&value);
    result.emplace_back(p.first, std::move(value));
  }
  return result;
}

std::string Filter::getResponseMetadata(absl::string_view key) {
  if (!encoder_callbacks_) return "";
  auto& proto_struct = getMetadata(encoder_callbacks_);
  auto it = proto_struct.fields().find(std::string(key));
  if (it ==  proto_struct.fields().end()) return "";
  std::string result;
  it->second.SerializeToString(&result);
  return result;
}

void Filter::setResponseMetadata(absl::string_view key, absl::string_view serialized_proto_struct) {
  if (!encoder_callbacks_) return;
  encoder_callbacks_->streamInfo().setDynamicMetadata(HttpFilterNames::get().Wasm, MessageUtil::keyValueStruct(std::string(key), std::string(serialized_proto_struct)));
}

PairsWithStringValues Filter::getResponseMetadataPairs() {
  PairsWithStringValues result;
  if (!encoder_callbacks_) return {};
  auto& proto_struct = getMetadata(encoder_callbacks_);
  for (auto &p : proto_struct.fields()) {
    std::string value;
    p.second.SerializeToString(&value);
    result.emplace_back(p.first, std::move(value));
  }
  return result;
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
                                 absl::string_view response_body, const Pairs& response_trailers) {
  if (!wasm_->onHttpCallResponse) return;
  uint32_t headers_ptr, headers_size, trailers_ptr, trailers_size;
  exportPairs(this, response_headers, &headers_ptr, &headers_size);
  exportPairs(this, response_trailers, &trailers_ptr, &trailers_size);
  uint32_t body_ptr = wasm_->wasm_vm()->copyString(response_body);
  uint32_t body_size = response_body.size();
  wasm_->onHttpCallResponse(this, id, token, headers_ptr, headers_size, body_ptr, body_size, trailers_ptr, trailers_size);
}

Wasm::Wasm(absl::string_view vm, ThreadLocal::SlotAllocator&) {
  wasm_vm_ = Common::Wasm::createWasmVm(vm);
  if (wasm_vm_) {
    registerCallback(wasm_vm_.get(), "log", &Common::Wasm::Context::wasmLogHandler);
#define _REGISTER(_fn) registerCallback(wasm_vm_.get(), #_fn, &_fn##Handler);
    _REGISTER(getRequestStreamInfoProtocol);
    _REGISTER(getResponseStreamInfoProtocol);

    _REGISTER(getRequestMetadata);
    _REGISTER(setRequestMetadata);
    _REGISTER(getRequestMetadataPairs);
    _REGISTER(getResponseMetadata);
    _REGISTER(setResponseMetadata);
    _REGISTER(getResponseMetadataPairs);

    _REGISTER(continueRequest);
    _REGISTER(continueResponse);

    _REGISTER(getSharedData);
    _REGISTER(setSharedData);

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

    _REGISTER(httpCall);

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
  _GET(onHttpCallResponse);
  _GET(onDone);
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

void Filter::onDone() {
  if (wasm_->onDone) wasm_->onDone(this, id);
}

void Filter::onDestroy() {
  if (destroyed_) return;
  destroyed_ = true;
  if (wasm_->onDestroy) wasm_->onDestroy(this, id);
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

Http::FilterMetadataStatus Filter::encodeMetadata(Http::MetadataMap&) {
  // TODO(PiotrSikora): add onResponseMetadata();
  return Http::FilterMetadataStatus::Continue;
}

void Filter::setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

void Filter::onAsyncClientSuccess(uint32_t token, Envoy::Http::MessagePtr& response) {
  auto body = absl::string_view(static_cast<char*>(response->body()->linearize(response->body()->length())), response->body()->length());
  onHttpCallResponse(token, getHeaderPairs(&response->headers()), body, getHeaderPairs(response->trailers()));
  http_request_.erase(token);
}

void Filter::onAsyncClientFailure(uint32_t token, Http::AsyncClient::FailureReason /* reason */) {
  onHttpCallResponse(token, {}, "", {});
  http_request_.erase(token);
}

void AsyncClientHandler::onSuccess(Envoy::Http::MessagePtr&& response) {
  filter->onAsyncClientSuccess(token, response);
}

void AsyncClientHandler::onFailure(Http::AsyncClient::FailureReason reason) {
  filter->onAsyncClientFailure(token, reason);
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
