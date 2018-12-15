#pragma once

#include "absl/container/flat_hash_map.h"

#include "envoy/http/filter.h"
#include "envoy/upstream/cluster_manager.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

class Context;
class Filter;
class Wasm;

class AsyncClientHandler : public Http::AsyncClient::Callbacks {
public:
  AsyncClientHandler(Context *context, uint32_t token) : context_(context), token_(token) {}

  virtual void onSuccess(Envoy::Http::MessagePtr&& /* response */) {}
  virtual void onFailure(Http::AsyncClient::FailureReason /* reason */) {}

  Context* context() { return context_; }
  uint32_t token() { return token_; }

private:
  Context* const context_;
  const uint32_t token_;
};

enum class HeaderCallType : uint32_t {
  RequestHeader = 0,
  RequestTrailer = 1,
  ResponseHeader = 2,
  ResponseTrailer = 3
};

// A session within the WASM VM.
class Context : public Envoy::Extensions::Common::Wasm::Context {
public:
  using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;

  Context(Wasm* wasm, Filter* filter);
  ~Context() override;

  //
  // Calls from the WASM code.
  //

  // Shared Data
  virtual absl::string_view getSharedData(absl::string_view key);
  virtual void setSharedData(absl::string_view key, absl::string_view value);

  // Request Headers
  virtual void addRequestHeader(absl::string_view key, absl::string_view value);
  virtual absl::string_view getRequestHeader(absl::string_view key);
  virtual Pairs getRequestHeaderPairs();
  virtual void removeRequestHeader(absl::string_view key);
  virtual void replaceRequestHeader(absl::string_view key, absl::string_view value);

  // Request Trailers
  virtual void addRequestTrailer(absl::string_view key, absl::string_view value);
  virtual absl::string_view getRequestTrailer(absl::string_view key);
  virtual Pairs getRequestTrailerPairs();
  virtual void removeRequestTrailer(absl::string_view key);
  virtual void replaceRequestTrailer(absl::string_view key, absl::string_view value);

  // Response Headers
  virtual void addResponseHeader(absl::string_view key, absl::string_view value);
  virtual absl::string_view getResponseHeader(absl::string_view key);
  virtual Pairs getResponseHeaderPairs();
  virtual void removeResponseHeader(absl::string_view key);
  virtual void replaceResponseHeader(absl::string_view key, absl::string_view value);

  // Response Trailers
  virtual void addResponseTrailer(absl::string_view key, absl::string_view value);
  virtual absl::string_view getResponseTrailer(absl::string_view key);
  virtual Pairs getResponseTrailerPairs();
  virtual void removeResponseTrailer(absl::string_view key);
  virtual void replaceResponseTrailer(absl::string_view key, absl::string_view value);

  // Body Buffer
  virtual absl::string_view getRequestBodyBufferBytes(uint32_t start, uint32_t length);
  virtual absl::string_view getResponseBodyBufferBytes(uint32_t start, uint32_t length);

  // StreamInfo
  virtual absl::string_view getSteamInfoProtocol();
  virtual absl::string_view getStreamInfoMetadata(absl::string_view filter,
      absl::string_view key);
  virtual void setStreamInfoMetadata(absl::string_view filter,
      absl::string_view key, absl::string_view value);
  virtual Pairs getStreamInfoPairs(absl::string_view filter);

  // HTTP
  // Returns a token which will be used with the corresponding onHttpCallResponse.
  virtual uint32_t httpCall(absl::string_view cluster, const Pairs& request_headers,
      absl::string_view request_body, int timeout_millisconds);
  virtual void httpRespond(const Pairs& response_headers,
      absl::string_view body);

  // Metadata: the values are serialized ProtobufWkt::Struct
  virtual absl::string_view getMetadata(absl::string_view key);
  virtual absl::string_view setMetadata(absl::string_view key,
      absl::string_view serialized_proto_struct);
  virtual Pairs getMetadataPairs();

  // Connection
  virtual bool isSsl();

  //
  // Calls into the WASM code.
  //

  // General Calls on Context(0)

  virtual void onStart();
  virtual void onConfigure(absl::string_view configuration);

  // Stream Calls on Stream specific context (id != 0).

  // Request
  virtual Http::FilterHeadersStatus onRequestHeaders();
  virtual Http::FilterDataStatus onRequestBody(int body_buffer_length, bool end_of_stream);
  virtual Http::FilterTrailersStatus onRequestTrailers();

  // Response
  virtual Http::FilterHeadersStatus onResponseHeaders();
  virtual Http::FilterDataStatus onResponseBody(int body_buffer_length, bool end_of_stream);
  virtual Http::FilterTrailersStatus onResponseTrailers();

  virtual void onHttpCallResponse(uint32_t token, const Pairs& response_headers,
                                  absl::string_view response_body);
  virtual void onDestroy();

private:
  friend class Filter;

  Wasm* const wasm_;
  Filter* const filter_;
  std::unique_ptr<Context> context_;
  Http::HeaderMap* request_headers_ = nullptr;
  Http::HeaderMap* response_headers_ = nullptr;
  Buffer::Instance* requestBodyBuffer_{};
  Buffer::Instance* responseBodyBuffer_{};
  bool request_end_of_stream_{};
  bool response_end_of_stream_{};
  Http::HeaderMap* request_trailers_{};
  Http::HeaderMap* response_trailers_{};
  absl::flat_hash_map<int, std::pair<AsyncClientHandler, Http::AsyncClient::Request*>> http_request_;
};

using WasmCall0Void = std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)>;
using WasmCall0Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)>;
using WasmCall2Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id, uint32_t, uint32_t)>;
using WasmCall3Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id, uint32_t, uint32_t, uint32_t)>;
using WasmCall5Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>;

// Thread-local Wasm VM.
class Wasm : public ThreadLocal::ThreadLocalObject, public Logger::Loggable<Logger::Id::wasm> {
public:
  Wasm(absl::string_view vm, ThreadLocal::SlotAllocator&);
  Wasm(const Wasm& other);
  ~Wasm() {}

  bool initialize(const std::string& code, absl::string_view name, bool allow_precompiled);
  void start();

  void allocContextId(Context *context) {
    context->id = next_context_id_++;
  }

  Envoy::Extensions::Common::Wasm::WasmVm* wasm_vm() const { return wasm_vm_.get(); }
  Context* general_context() const { return general_context_.get(); }

  // For testing.
  void setGeneralContext(std::unique_ptr<Context> context) {
    general_context_ = std::move(context);
  }

  WasmCall0Int onStart;
  WasmCall2Int onConfigure;

  WasmCall0Int onRequestHeaders;
  WasmCall2Int onRequestBody;
  WasmCall0Int onRequestTrailers;

  WasmCall0Int onResponseHeaders;
  WasmCall2Int onResponseBody;
  WasmCall0Int onResponseTrailers;

  WasmCall5Int onHttpCallResponse;

  WasmCall0Int onLogs;
  WasmCall0Void onDestroy;

private:
  void getFunctions();

  uint32_t next_context_id_ = 0;
  std::unique_ptr<Envoy::Extensions::Common::Wasm::WasmVm> wasm_vm_;
  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t configuration_ptr, uint32_t configuration_size)> configure_;
  std::unique_ptr<Context> general_context_;  // Context unrelated to nay specific filter or stream.
};

inline const ProtobufWkt::Struct& getMetadata(Http::StreamFilterCallbacks* callbacks) {
  if (callbacks->route() == nullptr || callbacks->route()->routeEntry() == nullptr) {
    return ProtobufWkt::Struct::default_instance();
  }
  const auto& metadata = callbacks->route()->routeEntry()->metadata();
  const auto& filter_it = metadata.filter_metadata().find(HttpFilterNames::get().Lua);
  if (filter_it == metadata.filter_metadata().end()) {
    return ProtobufWkt::Struct::default_instance();
  }
  return filter_it->second;
}

class FilterConfig : Logger::Loggable<Logger::Id::wasm> {
public:
  FilterConfig(absl::string_view vm, const std::string& code, absl::string_view name,
               bool allow_precompiled, absl::string_view configuration,
               ThreadLocal::SlotAllocator& tls, Upstream::ClusterManager& cluster_manager);

  Upstream::ClusterManager& cluster_manager() { return cluster_manager_; }
  const std::string& configuration() { return configuration_; }
  Wasm* base_wasm() { return wasm_.get(); }

  std::string getSharedData(absl::string_view key) {
    absl::ReaderMutexLock l(&mutex_);
    auto it = shared_data_.find(std::string(key));
    if (it != shared_data_.end()) {
      return it->second;
    }
    return "";
  }

  void setSharedData(absl::string_view key, absl::string_view value) {
    absl::WriterMutexLock l(&mutex_);
    shared_data_.insert_or_assign(std::string(key), std::string(value));
  }

  Wasm* getThreadWasm() { return &tls_slot_->getTyped<Wasm>(); }

private:
  ThreadLocal::SlotPtr tls_slot_;
  Upstream::ClusterManager& cluster_manager_;
  std::unique_ptr<Wasm> wasm_;
  const std::string configuration_;

  absl::Mutex mutex_;
  absl::flat_hash_map<std::string, std::string> shared_data_;
};

typedef std::shared_ptr<FilterConfig> FilterConfigConstSharedPtr;

class Filter : public Http::StreamFilter, Logger::Loggable<Logger::Id::wasm> {
public:
  Filter(FilterConfigConstSharedPtr config);

  Upstream::ClusterManager& clusterManager() { return config_->cluster_manager(); }

  // Http::StreamFilterBase
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override;
  void setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) override;

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override;
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override;
  void setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) override;

  // Override for testing.
  virtual std::unique_ptr<Context> createContext() {
    auto context = std::make_unique<Context>(wasm_, this);
    wasm_->allocContextId(context.get());
    return context;
  } 

  Context* context() { return context_.get(); }

protected:
  friend class Context;
  FilterConfigConstSharedPtr config_;
  Wasm* wasm_;
  std::unique_ptr<Context> context_;
  Envoy::Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Envoy::Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};
  bool destroyed_ = false;
};

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
