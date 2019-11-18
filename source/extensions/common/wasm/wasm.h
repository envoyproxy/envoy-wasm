#pragma once

#include <atomic>
#include <map>
#include <memory>

#include "envoy/access_log/access_log.h"
#include "envoy/buffer/buffer.h"
#include "envoy/common/exception.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/http/filter.h"
#include "envoy/server/wasm.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats.h"
#include "envoy/thread_local/thread_local.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/assert.h"
#include "common/common/logger.h"
#include "common/common/stack_array.h"
#include "common/config/datasource.h"
#include "common/stats/symbol_table_impl.h"

#include "extensions/common/wasm/wasm_exports.h"
#include "extensions/common/wasm/wasm_vm.h"
#include "extensions/common/wasm/well_known_names.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {

// TODO: move to source/common/stats/symbol_table_impl.h when upstreaming.
namespace Stats {
using StatNameSetSharedPtr = std::shared_ptr<Stats::StatNameSet>;
} // namespace Stats

namespace Extensions {
namespace Common {
namespace Wasm {

#include "api/wasm/cpp/proxy_wasm_common.h"

// clang-format off
#define ALL_WASM_STATS(COUNTER, GAUGE)      \
  COUNTER(created)                             \
  GAUGE(active, NeverImport)                   \
// clang-format on

struct WasmStats {
  ALL_WASM_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT)
};

class Context;
class Wasm;
class WasmVm;

using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;
using PairsWithStringValues = std::vector<std::pair<absl::string_view, std::string>>;

// Plugin contains the information for a filter/service.
struct Plugin {
  Plugin(absl::string_view name, absl::string_view root_id, absl::string_view vm_id,
         envoy::api::v2::core::TrafficDirection direction, const LocalInfo::LocalInfo& local_info,
         const envoy::api::v2::core::Metadata* listener_metadata)
      : name_(std::string(name)), root_id_(std::string(root_id)), vm_id_(std::string(vm_id)),
        direction_(direction), local_info_(local_info), listener_metadata_(listener_metadata),
        log_prefix_(makeLogPrefix()) {}

  std::string makeLogPrefix() const;

  const std::string name_;
  const std::string root_id_;
  const std::string vm_id_;
  envoy::api::v2::core::TrafficDirection direction_;
  const LocalInfo::LocalInfo& local_info_;
  const envoy::api::v2::core::Metadata* listener_metadata_;

  std::string log_prefix_;
};
using PluginSharedPtr = std::shared_ptr<Plugin>;

// A context which will be the target of callbacks for a particular session
// e.g. a handler of a stream.
class Context : public Logger::Loggable<Logger::Id::wasm>,
                public AccessLog::Instance,
                public Http::StreamFilter,
                public Network::ConnectionCallbacks,
                public Network::Filter,
                public std::enable_shared_from_this<Context> {
public:
  Context();                                                                // Testing.
  Context(Wasm* wasm);                                                      // Vm Context.
  Context(Wasm* wasm, PluginSharedPtr plugin);                              // Root Context.
  Context(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin);    // Stream context.
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
  // General stream downcall when the stream has ended.
  virtual void onDone();
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
  virtual WasmResult grpcCall(const envoy::api::v2::core::GrpcService& grpc_service,
                              absl::string_view service_name, absl::string_view method_name,
                              absl::string_view request,
                              const absl::optional<std::chrono::milliseconds>& timeout,
                              uint32_t* token_ptr);
  virtual WasmResult grpcStream(const envoy::api::v2::core::GrpcService& grpc_service,
                                absl::string_view service_name, absl::string_view method_name,
                                uint32_t* token_ptr);
  virtual WasmResult grpcClose(uint32_t token);  // cancel on call, close on stream.
  virtual WasmResult grpcCancel(uint32_t token); // cancel on call, reset on stream.
  virtual WasmResult grpcSend(uint32_t token, absl::string_view message,
                              bool end_stream); // stream only

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
  uint32_t root_context_id_{0};    // 0 for roots and the general context.
  std::string root_id_;            // set only in root context.
  std::string root_log_prefix_;    // set only in root context.
  const LocalInfo::LocalInfo* root_local_info_{nullptr};  // set only for root_context.
  Context* root_context_{nullptr}; // set in all contexts.
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
};

// Wasm execution instance. Manages the Envoy side of the Wasm interface.
class Wasm : public Envoy::Server::Wasm,
             public ThreadLocal::ThreadLocalObject,
             public Logger::Loggable<Logger::Id::wasm>,
             public std::enable_shared_from_this<Wasm> {
public:
  Wasm(absl::string_view runtime, absl::string_view vm_id, absl::string_view vm_configuration,
       Stats::ScopeSharedPtr scope, Upstream::ClusterManager& cluster_manager,
       Event::Dispatcher& dispatcher);
  Wasm(const Wasm& other, Event::Dispatcher& dispatcher);
  ~Wasm();

  bool initialize(const std::string& code, bool allow_precompiled = false);
  void startVm(Context* root_context);
  bool configure(Context* root_context, PluginSharedPtr plugin, absl::string_view configuration);
  Context* start(PluginSharedPtr plugin); // returns the root Context.

  absl::string_view vm_id() const { return vm_id_; }
  absl::string_view vm_id_with_hash() const { return vm_id_with_hash_; }
  WasmVm* wasm_vm() const { return wasm_vm_.get(); }
  Context* vm_context() const { return vm_context_.get(); }
  Stats::StatNameSetSharedPtr stat_name_set() const { return stat_name_set_; }
  Context* getRootContext(absl::string_view root_id) { return root_contexts_[root_id].get(); }
  Context* getContext(uint32_t id) {
    auto it = contexts_.find(id);
    if (it != contexts_.end())
      return it->second;
    return nullptr;
  }
  Upstream::ClusterManager& clusterManager() const { return cluster_manager_; }
  void setTickPeriod(uint32_t root_context_id, std::chrono::milliseconds tick_period);
  void tickHandler(uint32_t root_context_id);
  void queueReady(uint32_t root_context_id, uint32_t token);

  uint32_t allocContextId();

  const std::string& code() const { return code_; }
  const std::string& vm_configuration() const { return vm_configuration_; }
  bool allow_precompiled() const { return allow_precompiled_; }
  void setInitialConfiguration(const std::string& vm_configuration) {
    vm_configuration_ = vm_configuration;
  }

  //
  // AccessLog::Instance
  //
  void log(absl::string_view root_id, const Http::HeaderMap* request_headers,
           const Http::HeaderMap* response_headers, const Http::HeaderMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info);

  // Support functions.
  void* allocMemory(uint64_t size, uint64_t* address);
  // Allocate a null-terminated string in the VM and return the pointer to use as a call arguments.
  uint64_t copyString(absl::string_view s);
  uint64_t copyBuffer(const Buffer::Instance& buffer);
  // Copy the data in 's' into the VM along with the pointer-size pair. Returns true on success.
  bool copyToPointerSize(absl::string_view s, uint64_t ptr_ptr, uint64_t size_ptr);
  bool copyToPointerSize(const Buffer::Instance& buffer, uint64_t start, uint64_t length,
                         uint64_t ptr_ptr, uint64_t size_ptr);
  template <typename T> bool setDatatype(uint64_t ptr, const T& t);

  // For testing.
  void setContext(Context* context) { contexts_[context->id()] = context; }
  void startForTesting(std::unique_ptr<Context> root_context, PluginSharedPtr plugin);

  bool getEmscriptenVersion(uint32_t* emscripten_metadata_major_version,
                            uint32_t* emscripten_metadata_minor_version,
                            uint32_t* emscripten_abi_major_version,
                            uint32_t* emscripten_abi_minor_version) {
    if (!is_emscripten_) {
      return false;
    }
    *emscripten_metadata_major_version = emscripten_metadata_major_version_;
    *emscripten_metadata_minor_version = emscripten_metadata_minor_version_;
    *emscripten_abi_major_version = emscripten_abi_major_version_;
    *emscripten_abi_minor_version = emscripten_abi_minor_version_;
    return true;
  }

private:
  friend class Context;
  // These are the same as the values of the Context::MetricType enum, here separately for
  // convenience.
  static const uint32_t kMetricTypeCounter = 0x0;
  static const uint32_t kMetricTypeGauge = 0x1;
  static const uint32_t kMetricTypeHistogram = 0x2;
  static const uint32_t kMetricTypeMask = 0x3;
  static const uint32_t kMetricIdIncrement = 0x4;
  static void StaticAsserts() {
    static_assert(static_cast<uint32_t>(Context::MetricType::Counter) == kMetricTypeCounter, "");
    static_assert(static_cast<uint32_t>(Context::MetricType::Gauge) == kMetricTypeGauge, "");
    static_assert(static_cast<uint32_t>(Context::MetricType::Histogram) == kMetricTypeHistogram,
                  "");
  }

  bool isCounterMetricId(uint32_t metric_id) {
    return (metric_id & kMetricTypeMask) == kMetricTypeCounter;
  }
  bool isGaugeMetricId(uint32_t metric_id) {
    return (metric_id & kMetricTypeMask) == kMetricTypeGauge;
  }
  bool isHistogramMetricId(uint32_t metric_id) {
    return (metric_id & kMetricTypeMask) == kMetricTypeHistogram;
  }
  uint32_t nextCounterMetricId() { return next_counter_metric_id_ += kMetricIdIncrement; }
  uint32_t nextGaugeMetricId() { return next_gauge_metric_id_ += kMetricIdIncrement; }
  uint32_t nextHistogramMetricId() { return next_histogram_metric_id_ += kMetricIdIncrement; }

  void registerCallbacks();    // Register functions called out from WASM.
  void establishEnvironment(); // Language specific environments.
  void getFunctions();         // Get functions call into WASM.

  std::string vm_id_;           // User-provided vm_id.
  std::string vm_id_with_hash_; // vm_id + hash of code.
  std::unique_ptr<WasmVm> wasm_vm_;
  Stats::ScopeSharedPtr scope_;

  Upstream::ClusterManager& cluster_manager_;
  Event::Dispatcher& dispatcher_;

  uint32_t next_context_id_ = 1;        // 0 is reserved for the VM context.
  std::shared_ptr<Context> vm_context_; // Context unrelated to any specific root or stream
                                        // (e.g. for global constructors).
  absl::flat_hash_map<std::string, std::unique_ptr<Context>> root_contexts_;
  absl::flat_hash_map<uint32_t, Context*> contexts_;                    // Contains all contexts.
  std::unordered_map<uint32_t, std::chrono::milliseconds> tick_period_; // per root_id.
  std::unordered_map<uint32_t, Event::TimerPtr> timer_;                 // per root_id.

  TimeSource& time_source_;

  WasmCallVoid<0> _start_; /* Emscripten v1.39.0+ */
  WasmCallVoid<0> __wasm_call_ctors_;

  WasmCallWord<1> malloc_;
  WasmCallVoid<1> free_;

  // Calls into the VM.
  WasmCallWord<2> validateConfiguration_;
  WasmCallWord<2> onStart_;
  WasmCallWord<2> onConfigure_;
  WasmCallVoid<1> onTick_;

  WasmCallVoid<2> onCreate_;

  WasmCallWord<1> onNewConnection_;
  WasmCallWord<3> onDownstreamData_;
  WasmCallWord<3> onUpstreamData_;
  WasmCallVoid<2> onDownstreamConnectionClose_;
  WasmCallVoid<2> onUpstreamConnectionClose_;

  WasmCallWord<2> onRequestHeaders_;
  WasmCallWord<3> onRequestBody_;
  WasmCallWord<2> onRequestTrailers_;
  WasmCallWord<2> onRequestMetadata_;

  WasmCallWord<2> onResponseHeaders_;
  WasmCallWord<3> onResponseBody_;
  WasmCallWord<2> onResponseTrailers_;
  WasmCallWord<2> onResponseMetadata_;

  WasmCallVoid<5> onHttpCallResponse_;

  WasmCallVoid<3> onGrpcReceive_;
  WasmCallVoid<3> onGrpcClose_;
  WasmCallVoid<3> onGrpcCreateInitialMetadata_;
  WasmCallVoid<3> onGrpcReceiveInitialMetadata_;
  WasmCallVoid<3> onGrpcReceiveTrailingMetadata_;

  WasmCallVoid<2> onQueueReady_;

  WasmCallVoid<1> onDone_;
  WasmCallVoid<1> onLog_;
  WasmCallVoid<1> onDelete_;

  // Used by the base_wasm to enable non-clonable thread local Wasm(s) to be constructed.
  std::string code_;
  std::string vm_configuration_;
  bool allow_precompiled_ = false;

  bool is_emscripten_ = false;
  uint32_t emscripten_metadata_major_version_ = 0;
  uint32_t emscripten_metadata_minor_version_ = 0;
  uint32_t emscripten_abi_major_version_ = 0;
  uint32_t emscripten_abi_minor_version_ = 0;
  uint32_t emscripten_standalone_wasm_ = 0;

  // Host Stats/Metrics
  WasmStats wasm_stats_;

  // Plulgin Stats/Metrics
  Stats::StatNameSetSharedPtr stat_name_set_;
  uint32_t next_counter_metric_id_ = kMetricTypeCounter;
  uint32_t next_gauge_metric_id_ = kMetricTypeGauge;
  uint32_t next_histogram_metric_id_ = kMetricTypeHistogram;
  absl::flat_hash_map<uint32_t, Stats::Counter*> counters_;
  absl::flat_hash_map<uint32_t, Stats::Gauge*> gauges_;
  absl::flat_hash_map<uint32_t, Stats::Histogram*> histograms_;
};

// These accessors require Wasm.
inline WasmVm* Context::wasmVm() const { return wasm_->wasm_vm(); }
inline Upstream::ClusterManager& Context::clusterManager() const { return wasm_->clusterManager(); }

using CreateWasmCallback = std::function<void(std::shared_ptr<Wasm>)>;

// Create a high level Wasm VM with Envoy API support. Note: 'id' may be empty if this VM will not
// be shared by APIs (e.g. HTTP Filter + AccessLog).
void createWasm(const envoy::config::wasm::v2::VmConfig& vm_config, PluginSharedPtr plugin_config,
                Stats::ScopeSharedPtr scope, Upstream::ClusterManager& cluster_manager,
                Init::Manager& init_manager, Event::Dispatcher& dispatcher, Api::Api& api,
                Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                CreateWasmCallback&& cb);

// Create a ThreadLocal VM from an existing VM (e.g. from createWasm() above).
std::shared_ptr<Wasm> createThreadLocalWasm(Wasm& base_wasm, PluginSharedPtr plugin,
                                            absl::string_view configuration,
                                            Event::Dispatcher& dispatcher);

void createWasmForTesting(const envoy::config::wasm::v2::VmConfig& vm_config,
                          PluginSharedPtr plugin, Stats::ScopeSharedPtr scope,
                          Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                          Event::Dispatcher& dispatcher, Api::Api& api,
                          std::unique_ptr<Context> root_context_for_testing,
                          Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                          CreateWasmCallback&& cb);

// Get an existing ThreadLocal VM matching 'vm_id' or nullptr if there isn't one.
std::shared_ptr<Wasm> getThreadLocalWasmPtr(absl::string_view vm_id);
// Get an existing ThreadLocal VM matching 'vm_id' or create one using 'base_wavm' by cloning or by
// using it it as a template.
std::shared_ptr<Wasm> getOrCreateThreadLocalWasm(Wasm& base_wasm, PluginSharedPtr plugin,
                                                 absl::string_view configuration,
                                                 Event::Dispatcher& dispatcher);

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name);

inline Context::Context() : root_context_(this) {}

inline Context::Context(Wasm* wasm) : wasm_(wasm), root_context_(this) {
  wasm_->contexts_[id_] = this;
}

inline Context::Context(Wasm* wasm, PluginSharedPtr plugin) {
  initializeRoot(wasm, plugin);
}

inline void Context::initializeRoot(Wasm* wasm, PluginSharedPtr plugin) {
  wasm_ = wasm;
  id_ = wasm->allocContextId();
  root_id_ = plugin->root_id_,
  root_log_prefix_ = makeRootLogPrefix(plugin->vm_id_);
  root_local_info_ = &plugin->local_info_;
  root_context_ = this;
  wasm_->contexts_[id_] = this;
}

inline Context::Context(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin)
    : wasm_(wasm), id_(wasm->allocContextId()), root_context_id_(root_context_id), plugin_(plugin) {
  wasm_->contexts_[id_] = this;
  root_context_ = wasm_->contexts_[root_context_id_];
}

inline void* Wasm::allocMemory(uint64_t size, uint64_t* address) {
  Word a = malloc_(vm_context(), size);
  if (!a.u64_) {
    throw WasmException("malloc_ returns nullptr (OOM)");
  }
  auto memory = wasm_vm_->getMemory(a.u64_, size);
  if (!memory) {
    throw WasmException("malloc_ returned illegal address");
  }
  *address = a.u64_;
  return const_cast<void*>(reinterpret_cast<const void*>(memory.value().data()));
}

inline uint64_t Wasm::copyString(absl::string_view s) {
  if (s.empty()) {
    return 0; // nullptr
  }
  uint64_t pointer;
  uint8_t* m = static_cast<uint8_t*>(allocMemory((s.size() + 1), &pointer));
  memcpy(m, s.data(), s.size());
  m[s.size()] = 0;
  return pointer;
}

inline uint64_t Wasm::copyBuffer(const Buffer::Instance& buffer) {
  uint64_t pointer;
  auto length = buffer.length();
  if (length <= 0) {
    return 0;
  }
  Buffer::RawSlice oneRawSlice;
  // NB: we need to pass in >= 1 in order to get the real "n" (see Buffer::Instance for details).
  int nSlices = buffer.getRawSlices(&oneRawSlice, 1);
  if (nSlices <= 0) {
    return 0;
  }
  uint8_t* m = static_cast<uint8_t*>(allocMemory(length, &pointer));
  if (nSlices == 1) {
    memcpy(m, oneRawSlice.mem_, oneRawSlice.len_);
    return pointer;
  }
  STACK_ARRAY(manyRawSlices, Buffer::RawSlice, nSlices);
  buffer.getRawSlices(manyRawSlices.begin(), nSlices);
  auto p = m;
  for (int i = 0; i < nSlices; i++) {
    memcpy(p, manyRawSlices[i].mem_, manyRawSlices[i].len_);
    p += manyRawSlices[i].len_;
  }
  return pointer;
}

inline bool Wasm::copyToPointerSize(absl::string_view s, uint64_t ptr_ptr, uint64_t size_ptr) {
  uint64_t pointer = 0;
  uint64_t size = s.size();
  void* p = nullptr;
  if (size > 0) {
    p = allocMemory(size, &pointer);
    if (!p) {
      return false;
    }
    memcpy(p, s.data(), size);
  }
  if (!wasm_vm_->setWord(ptr_ptr, Word(pointer))) {
    return false;
  }
  if (!wasm_vm_->setWord(size_ptr, Word(size))) {
    return false;
  }
  return true;
}

inline bool Wasm::copyToPointerSize(const Buffer::Instance& buffer, uint64_t start, uint64_t length,
                                    uint64_t ptr_ptr, uint64_t size_ptr) {
  uint64_t size = buffer.length();
  if (size < start + length) {
    return false;
  }
  auto nslices = buffer.getRawSlices(nullptr, 0);
  auto slices = std::make_unique<Buffer::RawSlice[]>(nslices + 10 /* pad for evbuffer overrun */);
  auto actual_slices = buffer.getRawSlices(&slices[0], nslices);
  uint64_t pointer = 0;
  char* p = static_cast<char*>(allocMemory(length, &pointer));
  auto s = start;
  auto l = length;
  if (!p) {
    return false;
  }
  for (uint64_t i = 0; i < actual_slices; i++) {
    if (slices[i].len_ <= s) {
      s -= slices[i].len_;
      continue;
    }
    auto ll = l;
    if (ll > s + slices[i].len_)
      ll = s + slices[i].len_;
    memcpy(p, static_cast<char*>(slices[i].mem_) + s, ll);
    l -= ll;
    if (l <= 0) {
      break;
    }
    s = 0;
    p += ll;
  }
  if (!wasm_vm_->setWord(ptr_ptr, Word(pointer))) {
    return false;
  }
  if (!wasm_vm_->setWord(size_ptr, Word(length))) {
    return false;
  }
  return true;
}

template <typename T> inline bool Wasm::setDatatype(uint64_t ptr, const T& t) {
  return wasm_vm_->setMemory(ptr, sizeof(T), &t);
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
