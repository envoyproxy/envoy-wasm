#pragma once

#include "envoy/http/filter.h"
#include "envoy/upstream/cluster_manager.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

class Wasm;
class StreamHandler;

// A session within the WASM VM.
class Context : public Envoy::Extensions::Common::Wasm::Context {
public:
  using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;

  Context(Wasm* wasm, StreamHandler* stream);
  ~Context() override;

  //
  // Calls from the WASM code.
  //

  // Headers
  enum class HeaderType { Header = 0, Trailer = 1 };
  virtual void addHeader(HeaderType type, absl::string_view key,
      absl::string_view value);
  virtual absl::string_view getHeader(HeaderType type, absl::string_view key);
  virtual Pairs getHeaderPairs(HeaderType type);
  virtual void removeHeader(HeaderType type, absl::string_view key);
  virtual void replaceHeader(HeaderType type, absl::string_view key,
      absl::string_view value);

  // Decoder
  virtual void continueDecoding();

  // Encoder
  virtual void continueEncoding();

  // StreamInfo
  virtual absl::string_view getSteamInfoProtocol();
  virtual absl::string_view getStreamInfoMetadata(absl::string_view filter,
      absl::string_view key);
  virtual void setStreamInfoMetadata(absl::string_view filter,
      absl::string_view key, absl::string_view value);
  virtual Pairs getStreamInfoPairs(absl::string_view filter);

  // Body Buffer
  enum class BodyMode { FullBody = 0, BodyChunks = 1 };
  virtual void setBodyMode(BodyMode body_mode);
  virtual int bodyBufferLength();
  virtual absl::string_view getBodyBufferBytes(int start, int length);

  // HTTP
  virtual void httpCall(absl::string_view cluster, const Pairs& request_headers,
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
  virtual void onCreate();
  virtual void onStart();
  virtual void onBody();
  virtual void onTrailers();
  virtual void onHttpCallResponse(const Pairs& response_headers,
                                  absl::string_view response_body);
  virtual void raiseWasmError(absl::string_view message);
  virtual void onDestroy();

  // Handlers.
  static void getHeaderHandler(void *raw_context, uint32_t type, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr_ptr, uint32_t value_size_ptr);
  static void replaceHeaderHandler(void *raw_context, uint32_t type, uint32_t key_ptr, uint32_t key_size, uint32_t value_ptr, uint32_t value_size);
 
private:
  Wasm *wasm_;
  StreamHandler *stream_;
};

// Thread-local Wasm VM.
class Wasm : public Logger::Loggable<Logger::Id::wasm> {
public:
  Wasm(absl::string_view vm, ThreadLocal::SlotAllocator&);
  ~Wasm() {}

  bool initialize(const std::string& code, absl::string_view name, bool allow_precompiled);
  void configure(absl::string_view configuration);
  void start();

  void allocContextId(Context *context) {
    context->id = next_context_id_++;
  }

  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)> onCreate() {
     return onCreate_;
  }
  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)> onStart() {
     return onStart_;
  }
  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)> onDestroy() {
     return onDestroy_;
  }

  Envoy::Extensions::Common::Wasm::WasmVm* wasm_vm() { return wasm_vm_.get(); }
  Context* general_context() { return general_context_.get(); }

  // For testing.
  void setGeneralContext(std::unique_ptr<Context> context) {
    general_context_ = std::move(context);
  }

private:
  uint32_t next_context_id_ = 0;
  std::unique_ptr<Envoy::Extensions::Common::Wasm::WasmVm> wasm_vm_;
  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t configuration_ptr, uint32_t configuration_size)> configure_;
  std::unique_ptr<Context> general_context_;  // Context unrelated to nay specific filter or stream.
  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)> onCreate_;
  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)> onStart_;
  std::function<void(Envoy::Extensions::Common::Wasm::Context*, uint32_t context_id)> onDestroy_;
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

class Filter;

class StreamHandler : public Http::AsyncClient::Callbacks {
public:
  enum class State { Running, WaitForBodyChunk, WaitForBody, WaitForTrailers, HttpCall, Responded };

  StreamHandler(Http::HeaderMap& headers, bool end_stream, Filter& filter);

  Http::FilterTrailersStatus start();
  Http::FilterDataStatus onData(Buffer::Instance& data, bool end_stream);
  Http::FilterTrailersStatus onTrailers(Http::HeaderMap& trailers);

  void onReset() {
    if (http_request_) {
      http_request_->cancel();
      http_request_ = nullptr;
    }
  }

private:
  friend class Context;
  // Http::AsyncClient::Callbacks
  void onSuccess(Http::MessagePtr&&) override {}
  void onFailure(Http::AsyncClient::FailureReason) override {}

  std::unique_ptr<Context> context_;
  Http::HeaderMap& headers_;
  bool end_stream_;
  bool headers_continued_{};
  bool buffered_body_{};
  bool saw_body_{};
  Filter& filter_;
  Http::HeaderMap* trailers_{};
  State state_{State::Running};
  Http::AsyncClient::Request* http_request_{};
};

class FilterConfig : Logger::Loggable<Logger::Id::wasm> {
public:
  FilterConfig(absl::string_view vm, const std::string& code, absl::string_view name,
               bool allow_precompiled, absl::string_view configuration,
               ThreadLocal::SlotAllocator& tls, Upstream::ClusterManager& cluster_manager);

  Upstream::ClusterManager& cluster_manager() { return cluster_manager_; }

  Wasm* wasm() { return wasm_.get(); }

private:
  Upstream::ClusterManager& cluster_manager_;
  std::unique_ptr<Wasm> wasm_;
};

typedef std::shared_ptr<FilterConfig> FilterConfigConstSharedPtr;

class Filter : public Http::StreamFilter, Logger::Loggable<Logger::Id::wasm> {
public:
  Filter(FilterConfigConstSharedPtr config) : config_(config), request_handler_(), response_handler_() {}

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
  virtual std::unique_ptr<Context> createContext(StreamHandler* handler) {
    auto context = std::make_unique<Context>(config_->wasm(), handler);
    config_->wasm()->allocContextId(context.get());
    return context;
  } 

private:
  FilterConfigConstSharedPtr config_;
  std::unique_ptr<StreamHandler> request_handler_;
  std::unique_ptr<StreamHandler> response_handler_;
  bool destroyed_{};
};

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
