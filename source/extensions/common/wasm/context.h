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

#include "extensions/filters/common/expr/evaluator.h"

#include "eval/public/activation.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

#include "api/wasm/cpp/proxy_wasm_common.h"

class Wasm;
class WasmVm;

using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;
using PairsWithStringValues = std::vector<std::pair<absl::string_view, std::string>>;

using GrpcService = envoy::config::core::v3::GrpcService;

// Plugin contains the information for a filter/service.
struct Plugin {
  Plugin(absl::string_view name, absl::string_view root_id, absl::string_view vm_id,
         envoy::config::core::v3::TrafficDirection direction,
         const LocalInfo::LocalInfo& local_info,
         const envoy::config::core::v3::Metadata* listener_metadata)
      : name_(std::string(name)), root_id_(std::string(root_id)), vm_id_(std::string(vm_id)),
        direction_(direction), local_info_(local_info), listener_metadata_(listener_metadata),
        log_prefix_(makeLogPrefix()) {}

  std::string makeLogPrefix() const;

  const std::string name_;
  const std::string root_id_;
  const std::string vm_id_;
  envoy::config::core::v3::TrafficDirection direction_;
  const LocalInfo::LocalInfo& local_info_;
  const envoy::config::core::v3::Metadata* listener_metadata_;

  std::string log_prefix_;
};
using PluginSharedPtr = std::shared_ptr<Plugin>;

// Opaque context object.
class StorageObject {
public:
  virtual ~StorageObject() = default;
};

// A context which will be the target of callbacks for a particular session
// e.g. a handler of a stream.
class Context : public Logger::Loggable<Logger::Id::wasm>,
                public AccessLog::Instance,
                public Http::StreamFilter,
                public Network::ConnectionCallbacks,
                public Network::Filter,
                public google::api::expr::runtime::BaseActivation,
                public std::enable_shared_from_this<Context> {
public:
  Context();                                                             // Testing.
  Context(Wasm* wasm);                                                   // Vm Context.
  Context(Wasm* wasm, PluginSharedPtr plugin);                           // Root Context.
  Context(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin); // Stream context.
  ~Context();

  Wasm* wasm() const { return wasm_; }
  uint32_t id() const { return id_; }
  bool isVmContext() { return id_ == 0; }
  bool isRootContext() { return root_context_id_ == 0; }
  Context* root_context() { return root_context_; }

  absl::string_view root_id() const { return plugin_ ? plugin_->root_id_ : root_id_; }
  absl::string_view log_prefix() const { return plugin_ ? plugin_->log_prefix_ : root_log_prefix_; }

  WasmVm* wasmVm() const;
  Upstream::ClusterManager& clusterManager() const;

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
  virtual bool validateConfiguration(absl::string_view configuration, PluginSharedPtr plugin);
  virtual bool onStart(absl::string_view vm_configuration, PluginSharedPtr plugin);
  virtual bool onConfigure(absl::string_view plugin_configuration, PluginSharedPtr plugin);

  //
  // Stream downcalls on Context(id > 0).
  //
  // General stream downcall on a new stream.
  virtual void onCreate(uint32_t root_context_id);
  // Network
  virtual Network::FilterStatus onNetworkNewConnection();
  virtual Network::FilterStatus onDownstreamData(int data_length, bool end_of_stream);
  virtual Network::FilterStatus onUpstreamData(int data_length, bool end_of_stream);
  enum class PeerType : uint32_t {
    Unknown = 0,
    Local = 1,
    Remote = 2,
  };
  virtual void onDownstreamConnectionClose(PeerType);
  virtual void onUpstreamConnectionClose(PeerType);
  // HTTP Filter Stream Request Downcalls.
  virtual Http::FilterHeadersStatus onRequestHeaders();
  virtual Http::FilterDataStatus onRequestBody(int body_buffer_length, bool end_of_stream);
  virtual Http::FilterTrailersStatus onRequestTrailers();
  virtual Http::FilterMetadataStatus onRequestMetadata();
  // HTTP Filter Stream Response Downcalls.
  virtual Http::FilterHeadersStatus onResponseHeaders();
  virtual Http::FilterDataStatus onResponseBody(int body_buffer_length, bool end_of_stream);
  virtual Http::FilterTrailersStatus onResponseTrailers();
  virtual Http::FilterMetadataStatus onResponseMetadata();
  // Async Response Downcalls on any Context.
  virtual void onHttpCallResponse(uint32_t token, uint32_t headers, uint32_t body_size,
                                  uint32_t trailers);
  virtual void onQueueReady(uint32_t token);
  // General stream downcall when the stream/vm has ended.
  virtual bool onDone();
  // General stream downcall for logging. Occurs after onDone().
  virtual void onLog();
  // General stream downcall when no further stream calls will occur.
  virtual void onDelete();

  //
  // General Callbacks.
  //
  virtual void scriptLog(spdlog::level::level_enum level, absl::string_view message);
  virtual WasmResult setTickPeriod(std::chrono::milliseconds tick_period);
  virtual uint64_t getCurrentTimeNanoseconds();
  virtual absl::string_view getConfiguration();
  virtual std::pair<uint32_t, absl::string_view> getStatus();

  //
  // AccessLog::Instance
  //
  void log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
           const Http::HeaderMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info) override;

  //
  // Network::ConnectionCallbacks
  //
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  //
  // Network::ReadFilter
  //
  Network::FilterStatus onNewConnection() override;
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override;

  //
  // Network::WriteFilter
  //
  Network::FilterStatus onWrite(Buffer::Instance& data, bool end_stream) override;
  void initializeWriteFilterCallbacks(Network::WriteFilterCallbacks& callbacks) override;

  //
  // Http::StreamFilterBase
  //
  // Note: This calls onDone() in WASM.
  void onDestroy() override;

  //
  // Http::StreamDecoderFilter
  //
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override;
  Http::FilterMetadataStatus decodeMetadata(Http::MetadataMap& metadata_map) override;
  void setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) override;

  //
  // Http::StreamEncoderFilter
  //
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override;
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override;
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap& metadata_map) override;
  void setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) override;

  //
  // HTTP Filter Callbacks
  //

  // State accessors
  virtual WasmResult getProperty(absl::string_view path, std::string* result);
  virtual WasmResult setProperty(absl::string_view key, absl::string_view serialized_value);

  // Continue
  virtual void continueRequest() {
    if (decoder_callbacks_)
      decoder_callbacks_->continueDecoding();
  }
  virtual void continueResponse() {
    if (encoder_callbacks_)
      encoder_callbacks_->continueEncoding();
  }
  virtual void sendLocalResponse(Http::Code response_code, absl::string_view body_text,
                                 std::function<void(Http::HeaderMap& headers)> modify_headers,
                                 const absl::optional<Grpc::Status::GrpcStatus> grpc_status,
                                 absl::string_view details) {
    if (decoder_callbacks_)
      decoder_callbacks_->sendLocalReply(response_code, body_text, modify_headers, grpc_status,
                                         details);
  }
  virtual void clearRouteCache() {
    if (decoder_callbacks_)
      decoder_callbacks_->clearRouteCache();
  }

  // Shared Data
  virtual WasmResult getSharedData(absl::string_view key,
                                   std::pair<std::string, uint32_t /* cas */>* data);
  virtual WasmResult setSharedData(absl::string_view key, absl::string_view value, uint32_t cas);

  // Shared Queue
  virtual uint32_t registerSharedQueue(absl::string_view queue_name);
  virtual WasmResult resolveSharedQueue(absl::string_view vm_id, absl::string_view queue_name,
                                        uint32_t* token);
  virtual WasmResult dequeueSharedQueue(uint32_t token, std::string* data);
  virtual WasmResult enqueueSharedQueue(uint32_t token, absl::string_view value);

  // Header/Trailer/Metadata Maps
  virtual void addHeaderMapValue(HeaderMapType type, absl::string_view key,
                                 absl::string_view value);
  virtual absl::string_view getHeaderMapValue(HeaderMapType type, absl::string_view key);
  virtual Pairs getHeaderMapPairs(HeaderMapType type);
  virtual void setHeaderMapPairs(HeaderMapType type, const Pairs& pairs);

  virtual void removeHeaderMapValue(HeaderMapType type, absl::string_view key);
  virtual void replaceHeaderMapValue(HeaderMapType type, absl::string_view key,
                                     absl::string_view value);

  virtual uint32_t getHeaderMapSize(HeaderMapType type);

  // Buffer
  virtual Buffer::Instance* getBuffer(BufferType type);
  bool end_of_stream() { return end_of_stream_; }

  // HTTP
  // Returns a token which will be used with the corresponding onHttpCallResponse.
  virtual WasmResult httpCall(absl::string_view cluster, const Pairs& request_headers,
                              absl::string_view request_body, const Pairs& request_trailers,
                              int timeout_millisconds, uint32_t* token_ptr);
  virtual void httpRespond(const Pairs& response_headers, absl::string_view body,
                           const Pairs& response_trailers);

  // Stats/Metrics
  enum class MetricType : uint32_t {
    Counter = 0,
    Gauge = 1,
    Histogram = 2,
    Max = 2,
  };
  virtual WasmResult defineMetric(MetricType type, absl::string_view name, uint32_t* metric_id_ptr);
  virtual WasmResult incrementMetric(uint32_t metric_id, int64_t offset);
  virtual WasmResult recordMetric(uint32_t metric_id, uint64_t value);
  virtual WasmResult getMetric(uint32_t metric_id, uint64_t* value_ptr);

  // gRPC
  // Returns a token which will be used with the corresponding onGrpc and grpc calls.
  virtual WasmResult grpcCall(const GrpcService& grpc_service, absl::string_view service_name,
                              absl::string_view method_name, absl::string_view request,
                              const absl::optional<std::chrono::milliseconds>& timeout,
                              uint32_t* token_ptr);
  virtual WasmResult grpcStream(const GrpcService& grpc_service, absl::string_view service_name,
                                absl::string_view method_name, uint32_t* token_ptr);
  virtual WasmResult grpcClose(uint32_t token);  // cancel on call, close on stream.
  virtual WasmResult grpcCancel(uint32_t token); // cancel on call, reset on stream.
  virtual WasmResult grpcSend(uint32_t token, absl::string_view message,
                              bool end_stream); // stream only

  // CEL evaluation
  virtual std::vector<const google::api::expr::runtime::CelFunction*>
  FindFunctionOverloads(absl::string_view) const override {
    return {};
  }
  virtual absl::optional<google::api::expr::runtime::CelValue>
  FindValue(absl::string_view name, Protobuf::Arena* arena) const override;
  virtual bool IsPathUnknown(absl::string_view) const override { return false; }

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

  // Connection
  virtual bool isSsl();

protected:
  friend class Wasm;

  struct AsyncClientHandler : public Http::AsyncClient::Callbacks {
    // Http::AsyncClient::Callbacks
    void onSuccess(Envoy::Http::MessagePtr&& response) override {
      context_->onHttpCallSuccess(token_, response);
    }
    void onFailure(Http::AsyncClient::FailureReason reason) override {
      context_->onHttpCallFailure(token_, reason);
    }

    Context* context_;
    uint32_t token_;
    Http::AsyncClient::Request* request_;
  };

  struct GrpcCallClientHandler : public Grpc::RawAsyncRequestCallbacks {
    // Grpc::AsyncRequestCallbacks
    void onCreateInitialMetadata(Http::HeaderMap& metadata) override {
      context_->onGrpcCreateInitialMetadata(token_, metadata);
    }
    void onSuccessRaw(Buffer::InstancePtr&& response, Tracing::Span& /* span */) override {
      context_->onGrpcReceive(token_, std::move(response));
    }
    void onFailure(Grpc::Status::GrpcStatus status, const std::string& message,
                   Tracing::Span& /* span */) override {
      context_->onGrpcClose(token_, status, message);
    }

    Context* context_;
    uint32_t token_;
    Grpc::RawAsyncClientPtr client_;
    Grpc::AsyncRequest* request_;
  };

  struct GrpcStreamClientHandler : public Grpc::RawAsyncStreamCallbacks {
    // Grpc::AsyncStreamCallbacks
    void onCreateInitialMetadata(Http::HeaderMap& metadata) override {
      context_->onGrpcCreateInitialMetadata(token_, metadata);
    }
    void onReceiveInitialMetadata(Http::HeaderMapPtr&& metadata) override {
      context_->onGrpcReceiveInitialMetadata(token_, std::move(metadata));
    }
    bool onReceiveMessageRaw(Buffer::InstancePtr&& response) override {
      context_->onGrpcReceive(token_, std::move(response));
      return true;
    }
    void onReceiveTrailingMetadata(Http::HeaderMapPtr&& metadata) override {
      context_->onGrpcReceiveTrailingMetadata(token_, std::move(metadata));
    }
    void onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) override {
      context_->onGrpcClose(token_, status, message);
    }

    Context* context_;
    uint32_t token_;
    Grpc::RawAsyncClientPtr client_;
    Grpc::RawAsyncStream* stream_;
  };

  void initializeRoot(Wasm* wasm, PluginSharedPtr plugin);
  std::string makeRootLogPrefix(absl::string_view vm_id) const;

  void onHttpCallSuccess(uint32_t token, Envoy::Http::MessagePtr& response);
  void onHttpCallFailure(uint32_t token, Http::AsyncClient::FailureReason reason);

  virtual void onGrpcCreateInitialMetadata(uint32_t token,
                                           Http::HeaderMap& metadata); // For both Call and Stream.
  virtual void onGrpcReceive(uint32_t token,
                             Buffer::InstancePtr response); // Call (implies OK close) and Stream.
  virtual void onGrpcClose(uint32_t token, const Grpc::Status::GrpcStatus& status,
                           const absl::string_view message); // Call (not OK) and Stream.

  virtual void onGrpcReceiveInitialMetadata(uint32_t token,
                                            Http::HeaderMapPtr&& metadata); // Stream only.
  virtual void onGrpcReceiveTrailingMetadata(uint32_t token,
                                             Http::HeaderMapPtr&& metadata); // Stream only.

  bool IsGrpcStreamToken(uint32_t token) { return (token & 1) == 0; }
  bool IsGrpcCallToken(uint32_t token) { return (token & 1) == 1; }

  Http::HeaderMap* getMap(HeaderMapType type);
  const Http::HeaderMap* getConstMap(HeaderMapType type);

  Wasm* wasm_{nullptr};
  uint32_t id_{0};
  uint32_t root_context_id_{0};                          // 0 for roots and the general context.
  Context* root_context_{nullptr};                       // set in all contexts.
  std::string root_id_;                                  // set only in root context.
  std::string root_log_prefix_;                          // set only in root context.
  const LocalInfo::LocalInfo* root_local_info_{nullptr}; // set only for root_context.
  PluginSharedPtr plugin_;
  bool in_vm_context_created_ = false;
  bool destroyed_ = false;

  uint32_t next_http_call_token_ = 1;
  uint32_t next_grpc_token_ = 1; // Odd tokens are for Calls even for Streams.

  // Network callbacks.
  Network::ReadFilterCallbacks* network_read_filter_callbacks_{};
  Network::WriteFilterCallbacks* network_write_filter_callbacks_{};

  // HTTP callbacks.
  Envoy::Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Envoy::Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};

  // General state.
  // NB: this are only available (non-nullptr) during the calls requiring it (e.g. onConfigure()).
  absl::string_view configuration_;
  uint32_t status_code_{0};
  absl::string_view status_message_;

  // Network filter state.
  Buffer::Instance* network_downstream_data_buffer_{};
  Buffer::Instance* network_upstream_data_buffer_{};

  // HTTP filter state.
  // NB: this are only available (non-nullptr) during the calls corresponding to when the data is
  // live. For example, request_headers_ is available during the onRequestHeaders() call.
  Http::HeaderMap* request_headers_{};
  Http::HeaderMap* response_headers_{};
  Buffer::Instance* request_body_buffer_{};
  Buffer::Instance* response_body_buffer_{};
  Http::HeaderMap* request_trailers_{};
  Http::HeaderMap* response_trailers_{};
  Http::MetadataMap* request_metadata_{};
  Http::MetadataMap* response_metadata_{};

  // Only available during onHttpCallResponse.
  Envoy::Http::MessagePtr* http_call_response_{};

  Http::HeaderMap* grpc_create_initial_metadata_{};
  Http::HeaderMapPtr grpc_receive_initial_metadata_{};
  Http::HeaderMapPtr grpc_receive_trailing_metadata_{};

  // NB: this are only available (non-nullptr) during onGrpcReceive.
  Buffer::InstancePtr grpc_receive_buffer_;

  const StreamInfo::StreamInfo* access_log_stream_info_{};
  const Http::HeaderMap* access_log_request_headers_{};
  const Http::HeaderMap* access_log_response_headers_{};
  const Http::HeaderMap* access_log_request_trailers_{}; // unused
  const Http::HeaderMap* access_log_response_trailers_{};

  // Temporary state.
  ProtobufWkt::Struct temporary_metadata_;
  bool end_of_stream_;

  // MB: must be a node-type map as we take persistent references to the entries.
  std::map<uint32_t, AsyncClientHandler> http_request_;
  std::map<uint32_t, GrpcCallClientHandler> grpc_call_request_;
  std::map<uint32_t, GrpcStreamClientHandler> grpc_stream_;

  // Opaque state
  absl::flat_hash_map<std::string, std::unique_ptr<StorageObject>> data_storage_;
};

using ContextSharedPtr = std::shared_ptr<Context>;

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name);

WasmResult serializeValue(Filters::Common::Expr::CelValue value, std::string* result);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
