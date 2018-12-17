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

class Filter;
class FilterConfig;
class Wasm;

using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;

using WasmCall0Void = std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)>;
using WasmCall0Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)>;
using WasmCall2Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id, uint32_t, uint32_t)>;
using WasmCall3Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id, uint32_t, uint32_t, uint32_t)>;
using WasmCall4Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id, uint32_t, uint32_t, uint32_t, uint32_t)>;
using WasmCall5Int = std::function<uint32_t(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>;

typedef std::shared_ptr<FilterConfig> FilterConfigConstSharedPtr;

class FilterConfig : Logger::Loggable<Logger::Id::wasm> {
public:
  FilterConfig(absl::string_view vm, const std::string& code, absl::string_view name,
               bool allow_precompiled, absl::string_view configuration,
               ThreadLocal::SlotAllocator& tls, Upstream::ClusterManager& cluster_manager);

  Upstream::ClusterManager& cluster_manager() { return cluster_manager_; }
  void initialize(FilterConfigConstSharedPtr filter_config);
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

// Thread-local Wasm VM.
class Wasm : public ThreadLocal::ThreadLocalObject, public Logger::Loggable<Logger::Id::wasm> {
public:
  Wasm(absl::string_view vm, ThreadLocal::SlotAllocator&);
  Wasm(const Wasm& other);
  ~Wasm() {}

  bool initialize(const std::string& code, absl::string_view name, bool allow_precompiled);
  void start();

  void createGeneralContext(FilterConfigConstSharedPtr filter_config);
  void allocContextId(Filter *context);

  Envoy::Extensions::Common::Wasm::WasmVm* wasm_vm() const { return wasm_vm_.get(); }
  Filter* general_context() const { return general_context_.get(); }

  // For testing.
  void setGeneralContext(std::unique_ptr<Filter> context) {
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
  std::unique_ptr<Filter> general_context_;  // Context unrelated to any specific stream.
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

class AsyncClientHandler : public Http::AsyncClient::Callbacks {
public:
  AsyncClientHandler(Filter *filter, uint32_t token) : filter_(filter), token_(token) {}

  virtual void onSuccess(Envoy::Http::MessagePtr&& /* response */) {}
  virtual void onFailure(Http::AsyncClient::FailureReason /* reason */) {}

  Filter* context() { return filter_; }
  uint32_t token() { return token_; }

private:
  Filter* const filter_;
  const uint32_t token_;
};

class Filter : public Envoy::Extensions::Common::Wasm::Context, public Http::StreamFilter {
public:
  explicit Filter(FilterConfigConstSharedPtr config);
  Filter(FilterConfigConstSharedPtr config, Wasm *wasm);

  Upstream::ClusterManager& clusterManager() { return config_->cluster_manager(); }
  Wasm *wasm() { return wasm_; }

  // Http::StreamFilterBase
  // Note: This calls onDestroy() in the VM.
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

  // General Calls on Filter(id == 0)

  virtual void onStart();
  virtual void onConfigure(absl::string_view configuration);

  // Stream Calls on Filter(id > 0)

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

  virtual void onLogs();

protected:
  FilterConfigConstSharedPtr config_;
  Wasm* wasm_;
  uint32_t id_;
  Http::HeaderMap* request_headers_ = nullptr;
  Http::HeaderMap* response_headers_ = nullptr;
  Buffer::Instance* requestBodyBuffer_{};
  Buffer::Instance* responseBodyBuffer_{};
  bool request_end_of_stream_{};
  bool response_end_of_stream_{};
  Http::HeaderMap* request_trailers_{};
  Http::HeaderMap* response_trailers_{};
  absl::flat_hash_map<int, std::pair<AsyncClientHandler, Http::AsyncClient::Request*>> http_request_;
  Envoy::Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Envoy::Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};
  bool destroyed_ = false;
};

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
