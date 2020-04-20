#pragma once

#include <atomic>
#include <map>
#include <memory>

#include "envoy/access_log/access_log.h"
#include "envoy/buffer/buffer.h"
#include "envoy/http/filter.h"
#include "envoy/server/wasm.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/assert.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/wasm_state.h"
#include "extensions/filters/common/expr/evaluator.h"

#include "eval/public/activation.h"
#include "include/proxy-wasm/context.h"
#include "include/proxy-wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

#include "proxy_wasm_common.h"

using proxy_wasm::BufferInterface;
using proxy_wasm::MetricType;
using proxy_wasm::Pairs;
using proxy_wasm::PairsWithStringValues;
using proxy_wasm::PluginBase;
using proxy_wasm::WasmBase;
using proxy_wasm::WasmBufferType;
using proxy_wasm::WasmHandleBase;
using proxy_wasm::WasmHeaderMapType;
using proxy_wasm::WasmResult;

using GrpcService = envoy::config::core::v3::GrpcService;

class Wasm;

using WasmHandleBaseSharedPtr = std::shared_ptr<WasmHandleBase>;

// Opaque context object.
class StorageObject {
public:
  virtual ~StorageObject() = default;
};

class Buffer : public proxy_wasm::BufferInterface {
public:
  Buffer() = default;

  Buffer* set(absl::string_view data) {
    data_ = data;
    const_buffer_instance_ = nullptr;
    buffer_instance_ = nullptr;
    return this;
  }
  Buffer* set(::Envoy::Buffer::Instance* buffer_instance) {
    data_ = "";
    buffer_instance_ = buffer_instance;
    const_buffer_instance_ = buffer_instance;
    return this;
  }

  size_t size() const override;
  WasmResult copyTo(WasmBase* wasm, size_t start, size_t length, uint64_t ptr_ptr,
                    uint64_t size_ptr) const override;
  WasmResult copyFrom(size_t start, size_t length, absl::string_view data) override;

private:
  absl::string_view data_;
  const ::Envoy::Buffer::Instance* const_buffer_instance_{};
  ::Envoy::Buffer::Instance* buffer_instance_{};
};

// Plugin contains the information for a filter/service.
struct Plugin : public PluginBase {
  Plugin(absl::string_view name, absl::string_view root_id, absl::string_view vm_id,
         absl::string_view plugin_configuration,
         envoy::config::core::v3::TrafficDirection direction,
         const LocalInfo::LocalInfo& local_info,
         const envoy::config::core::v3::Metadata* listener_metadata)
      : PluginBase(name, root_id, vm_id, plugin_configuration), direction_(direction),
        local_info_(local_info), listener_metadata_(listener_metadata) {}

  envoy::config::core::v3::TrafficDirection direction_;
  const LocalInfo::LocalInfo& local_info_;
  const envoy::config::core::v3::Metadata* listener_metadata_;
};
using PluginSharedPtr = std::shared_ptr<Plugin>;

// A context which will be the target of callbacks for a particular session
// e.g. a handler of a stream.
class Context : public proxy_wasm::ContextBase,
                public Logger::Loggable<Logger::Id::wasm>,
                public AccessLog::Instance,
                public Http::StreamFilter,
                public Network::ConnectionCallbacks,
                public Network::Filter,
                public google::api::expr::runtime::BaseActivation,
                public std::enable_shared_from_this<Context> {
public:
  Context();                                                                    // Testing.
  Context(Wasm* wasm);                                                          // Vm Context.
  Context(Wasm* wasm, const PluginSharedPtr& plugin);                           // Root Context.
  Context(Wasm* wasm, uint32_t root_context_id, const PluginSharedPtr& plugin); // Stream context.
  ~Context() override;

  Wasm* wasm() const;
  Plugin* plugin() const;
  Context* rootContext() const;
  Upstream::ClusterManager& clusterManager() const;

  // proxy_wasm::ContextBase
  void error(absl::string_view message) override;

  // Retrieves the stream info associated with the request (a.k.a active stream).
  // It selects a value based on the following order: encoder callback, decoder
  // callback, log callback, network read filter callback, network write filter
  // callback. As long as any one of the callbacks is invoked, the value should be
  // available.
  const StreamInfo::StreamInfo* getConstRequestStreamInfo() const;
  StreamInfo::StreamInfo* getRequestStreamInfo() const;

  // Retrieves the connection object associated with the request (a.k.a active stream).
  // It selects a value based on the following order: encoder callback, decoder
  // callback. As long as any one of the callbacks is invoked, the value should be
  // available.
  const Network::Connection* getConnection() const;

  //
  // VM level downcalls into the WASM code on Context(id == 0).
  //
  virtual bool validateConfiguration(absl::string_view configuration,
                                     const std::shared_ptr<PluginBase>& plugin); // deprecated

  // AccessLog::Instance
  void log(const Http::RequestHeaderMap* request_headers,
           const Http::ResponseHeaderMap* response_headers,
           const Http::ResponseTrailerMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info) override;

  // Network::ConnectionCallbacks
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // Network::ReadFilter
  Network::FilterStatus onNewConnection() override;
  Network::FilterStatus onData(::Envoy::Buffer::Instance& data, bool end_stream) override;
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override;

  // Network::WriteFilter
  Network::FilterStatus onWrite(::Envoy::Buffer::Instance& data, bool end_stream) override;
  void initializeWriteFilterCallbacks(Network::WriteFilterCallbacks& callbacks) override;

  // proxy_wasm::ContextBase
  void onDownstreamConnectionClose(PeerType) override;
  void onUpstreamConnectionClose(PeerType) override;

  // Http::StreamFilterBase. Note: This calls onDone() in WASM.
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers,
                                          bool end_stream) override;
  Http::FilterDataStatus decodeData(::Envoy::Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::RequestTrailerMap& trailers) override;
  Http::FilterMetadataStatus decodeMetadata(Http::MetadataMap& metadata_map) override;
  void setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) override;

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::ResponseHeaderMap&) override;
  Http::FilterHeadersStatus encodeHeaders(Http::ResponseHeaderMap& headers,
                                          bool end_stream) override;
  Http::FilterDataStatus encodeData(::Envoy::Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus encodeTrailers(Http::ResponseTrailerMap& trailers) override;
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap& metadata_map) override;
  void setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) override;

  // VM calls out to host.
  // proxy_wasm::ContextBase

  // General
  WasmResult log(uint64_t level, absl::string_view message) override;
  uint64_t getCurrentTimeNanoseconds() override;
  std::pair<uint32_t, absl::string_view> getStatus() override;

  // State accessors
  WasmResult getProperty(absl::string_view path, std::string* result) override;
  WasmResult setProperty(absl::string_view path, absl::string_view value) override;
  WasmResult declareProperty(absl::string_view path,
                             std::unique_ptr<const WasmStatePrototype> state_prototype);

  // Continue
  void continueRequest() override;
  void continueResponse() override;
  void sendLocalResponse(uint64_t response_code, absl::string_view body_text,
                         Pairs additional_headers, uint64_t grpc_status,
                         absl::string_view details) override;

  // Shared Data
  WasmResult getSharedData(absl::string_view key,
                           std::pair<std::string /* data */, uint32_t /* cas */>* data) override;
  WasmResult setSharedData(absl::string_view key, absl::string_view value, uint32_t cas) override;

  // Shared Queue
  uint32_t registerSharedQueue(absl::string_view queue_name) override;
  WasmResult resolveSharedQueue(absl::string_view vm_id, absl::string_view queue_name,
                                uint32_t* token) override;
  WasmResult dequeueSharedQueue(uint32_t token, std::string* data) override;
  WasmResult enqueueSharedQueue(uint32_t token, absl::string_view value) override;

  // Header/Trailer/Metadata Maps
  void addHeaderMapValue(WasmHeaderMapType type, absl::string_view key,
                         absl::string_view value) override;
  absl::string_view getHeaderMapValue(WasmHeaderMapType type, absl::string_view key) override;
  Pairs getHeaderMapPairs(WasmHeaderMapType type) override;
  void setHeaderMapPairs(WasmHeaderMapType type, const Pairs& pairs) override;

  void removeHeaderMapValue(WasmHeaderMapType type, absl::string_view key) override;
  void replaceHeaderMapValue(WasmHeaderMapType type, absl::string_view key,
                             absl::string_view value) override;

  uint32_t getHeaderMapSize(WasmHeaderMapType type) override;

  // Buffer
  BufferInterface* getBuffer(WasmBufferType type) override;
  bool end_of_stream() override { return end_of_stream_; }

  // HTTP
  WasmResult httpCall(absl::string_view cluster, const Pairs& request_headers,
                      absl::string_view request_body, const Pairs& request_trailers,
                      int timeout_millisconds, uint32_t* token_ptr) override;
  void httpRespond(const Pairs& response_headers, absl::string_view body,
                   const Pairs& response_trailers);

  // Stats/Metrics
  WasmResult defineMetric(MetricType type, absl::string_view name,
                          uint32_t* metric_id_ptr) override;
  WasmResult incrementMetric(uint32_t metric_id, int64_t offset) override;
  WasmResult recordMetric(uint32_t metric_id, uint64_t value) override;
  WasmResult getMetric(uint32_t metric_id, uint64_t* value_ptr) override;

  // gRPC
  WasmResult grpcCall(absl::string_view grpc_service, absl::string_view service_name,
                      absl::string_view method_name, const Pairs& initial_metadata,
                      absl::string_view request,
                      const absl::optional<std::chrono::milliseconds>& timeout,
                      uint32_t* token_ptr) override;
  WasmResult grpcStream(absl::string_view grpc_service, absl::string_view service_name,
                        absl::string_view method_name, const Pairs& initial_metadat,
                        uint32_t* token_ptr) override;

  WasmResult grpcClose(uint32_t token) override;
  WasmResult grpcCancel(uint32_t token) override;
  WasmResult grpcSend(uint32_t token, absl::string_view message, bool end_stream) override;

  // CEL evaluation
  std::vector<const google::api::expr::runtime::CelFunction*>
  FindFunctionOverloads(absl::string_view) const override {
    return {};
  }
  absl::optional<google::api::expr::runtime::CelValue>
  findValue(absl::string_view name, Protobuf::Arena* arena, bool last) const;
  absl::optional<google::api::expr::runtime::CelValue>
  FindValue(absl::string_view name, Protobuf::Arena* arena) const override {
    return findValue(name, arena, false);
  }
  bool IsPathUnknown(absl::string_view) const override { return false; }
  const std::vector<google::api::expr::runtime::CelAttributePattern>&
  unknown_attribute_patterns() const override {
    static const std::vector<google::api::expr::runtime::CelAttributePattern> empty;
    return empty;
  }
  const Protobuf::FieldMask unknown_paths() const override {
    return Protobuf::FieldMask::default_instance();
  }

  // Foreign function state
  virtual void setForeignData(absl::string_view data_name, std::unique_ptr<StorageObject> data) {
    data_storage_[data_name] = std::move(data);
  }
  template <typename T> T* getForeignData(absl::string_view data_name) {
    const auto& it = data_storage_.find(data_name);
    if (it == data_storage_.end()) {
      return nullptr;
    }
    return dynamic_cast<T*>(it->second.get());
  }
  bool hasForeignData(absl::string_view data_name) const {
    return data_storage_.contains(data_name);
  }

protected:
  friend class Wasm;

  void addAfterVmCallAction(std::function<void()> f);
  void initializeRoot(WasmBase* wasm, std::shared_ptr<PluginBase> plugin) override;
  void onCloseTCP();

  virtual absl::string_view getConfiguration();
  virtual WasmResult setTickPeriod(std::chrono::milliseconds tick_period);
  virtual void clearRouteCache() {
    if (decoder_callbacks_) {
      decoder_callbacks_->clearRouteCache();
    }
  }

  struct AsyncClientHandler : public Http::AsyncClient::Callbacks {
    // Http::AsyncClient::Callbacks
    void onSuccess(const Http::AsyncClient::Request&,
                   Envoy::Http::ResponseMessagePtr&& response) override {
      context_->onHttpCallSuccess(token_, std::move(response));
    }
    void onFailure(const Http::AsyncClient::Request&,
                   Http::AsyncClient::FailureReason reason) override {
      context_->onHttpCallFailure(token_, reason);
    }

    Context* context_;
    uint32_t token_;
    Http::AsyncClient::Request* request_;
  };

  struct GrpcCallClientHandler : public Grpc::RawAsyncRequestCallbacks {
    // Grpc::AsyncRequestCallbacks
    void onCreateInitialMetadata(Http::RequestHeaderMap& initial_metadata) override {
      context_->onGrpcCreateInitialMetadata(token_, initial_metadata);
    }
    void onSuccessRaw(::Envoy::Buffer::InstancePtr&& response, Tracing::Span& /* span */) override {
      context_->onGrpcReceiveWrapper(token_, std::move(response));
    }
    void onFailure(Grpc::Status::GrpcStatus status, const std::string& message,
                   Tracing::Span& /* span */) override {
      context_->onGrpcCloseWrapper(token_, status, message);
    }

    Context* context_;
    uint32_t token_;
    Grpc::RawAsyncClientPtr client_;
    Grpc::AsyncRequest* request_;
  };

  struct GrpcStreamClientHandler : public Grpc::RawAsyncStreamCallbacks {
    // Grpc::AsyncStreamCallbacks
    void onCreateInitialMetadata(Http::RequestHeaderMap&) override {}
    void onReceiveInitialMetadata(Http::ResponseHeaderMapPtr&& metadata) override {
      context_->onGrpcReceiveInitialMetadataWrapper(token_, std::move(metadata));
    }
    bool onReceiveMessageRaw(::Envoy::Buffer::InstancePtr&& response) override {
      context_->onGrpcReceiveWrapper(token_, std::move(response));
      return true;
    }
    void onReceiveTrailingMetadata(Http::ResponseTrailerMapPtr&& metadata) override {
      context_->onGrpcReceiveTrailingMetadataWrapper(token_, std::move(metadata));
    }
    void onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) override {
      context_->onGrpcCloseWrapper(token_, status, message);
    }

    Context* context_;
    uint32_t token_;
    Grpc::RawAsyncClientPtr client_;
    Grpc::RawAsyncStream* stream_;
  };

  void onHttpCallSuccess(uint32_t token, Envoy::Http::ResponseMessagePtr&& response);
  void onHttpCallFailure(uint32_t token, Http::AsyncClient::FailureReason reason);

  void onGrpcCreateInitialMetadata(uint32_t token, Http::RequestHeaderMap& metadata);
  void onGrpcReceiveInitialMetadataWrapper(uint32_t token, Http::HeaderMapPtr&& metadata);
  void onGrpcReceiveWrapper(uint32_t token, ::Envoy::Buffer::InstancePtr response);
  void onGrpcReceiveTrailingMetadataWrapper(uint32_t token, Http::HeaderMapPtr&& metadata);
  void onGrpcCloseWrapper(uint32_t token, const Grpc::Status::GrpcStatus& status,
                          const absl::string_view message);

  bool IsGrpcStreamToken(uint32_t token) { return (token & 1) == 0; }
  bool IsGrpcCallToken(uint32_t token) { return (token & 1) == 1; }

  Http::HeaderMap* getMap(WasmHeaderMapType type);
  const Http::HeaderMap* getConstMap(WasmHeaderMapType type);

  const LocalInfo::LocalInfo* root_local_info_{nullptr}; // set only for root_context.

  uint32_t next_http_call_token_ = 1;
  uint32_t next_grpc_token_ = 1; // Odd tokens are for Calls even for Streams.

  // Network callbacks.
  Network::ReadFilterCallbacks* network_read_filter_callbacks_{};
  Network::WriteFilterCallbacks* network_write_filter_callbacks_{};

  // HTTP callbacks.
  Envoy::Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Envoy::Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};

  // General state. Only available (non-nullptr) during the calls requiring it (e.g. onConfigure()).
  absl::string_view configuration_;
  uint32_t status_code_{0};
  absl::string_view status_message_;

  // Network filter state.
  ::Envoy::Buffer::Instance* network_downstream_data_buffer_{};
  ::Envoy::Buffer::Instance* network_upstream_data_buffer_{};

  // HTTP filter state.
  Http::RequestHeaderMap* request_headers_{};
  Http::ResponseHeaderMap* response_headers_{};
  ::Envoy::Buffer::Instance* request_body_buffer_{};
  ::Envoy::Buffer::Instance* response_body_buffer_{};
  Http::RequestTrailerMap* request_trailers_{};
  Http::ResponseTrailerMap* response_trailers_{};
  Http::MetadataMap* request_metadata_{};
  Http::MetadataMap* response_metadata_{};

  // Only available during onHttpCallResponse.
  Envoy::Http::ResponseMessagePtr* http_call_response_{};

  Http::HeaderMapPtr grpc_receive_initial_metadata_{};
  Http::HeaderMapPtr grpc_receive_trailing_metadata_{};

  // Only available (non-nullptr) during onGrpcReceive.
  ::Envoy::Buffer::InstancePtr grpc_receive_buffer_;

  // Only available (non-nullptr) during grpcCall and grpcStream.
  Http::RequestHeaderMapPtr grpc_initial_metadata_;

  // Access log state.
  const StreamInfo::StreamInfo* access_log_stream_info_{};
  const Http::RequestHeaderMap* access_log_request_headers_{};
  const Http::ResponseHeaderMap* access_log_response_headers_{};
  const Http::RequestTrailerMap* access_log_request_trailers_{}; // unused
  const Http::ResponseTrailerMap* access_log_response_trailers_{};

  // Temporary state.
  ProtobufWkt::Struct temporary_metadata_;
  bool end_of_stream_;
  bool buffering_request_body_ = false;
  bool buffering_response_body_ = false;
  Buffer buffer_;

  // MB: must be a node-type map as we take persistent references to the entries.
  std::map<uint32_t, AsyncClientHandler> http_request_;
  std::map<uint32_t, GrpcCallClientHandler> grpc_call_request_;
  std::map<uint32_t, GrpcStreamClientHandler> grpc_stream_;

  // Opaque state.
  absl::flat_hash_map<std::string, std::unique_ptr<StorageObject>> data_storage_;

  // TCP State.
  bool upstream_closed_ = false;
  bool downstream_closed_ = false;
  bool tcp_connection_closed_ = false;

  // Filter state prototype declaration.
  absl::flat_hash_map<std::string, std::unique_ptr<const WasmStatePrototype>> state_prototypes_;
};
using ContextSharedPtr = std::shared_ptr<Context>;

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name);

WasmResult serializeValue(Filters::Common::Expr::CelValue value, std::string* result);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
