#pragma once

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
#include "common/stats/symbol_table_impl.h"

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

#include "api/wasm/cpp/proxy_wasm_result.h"
#include "api/wasm/cpp/proxy_wasm_metadata.h"

class Context;
class Wasm;
class WasmVm;

using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;
using PairsWithStringValues = std::vector<std::pair<absl::string_view, std::string>>;

enum class StreamType : int32_t { Request = 0, Response = 1, MAX = 1 };

// Handlers for functions exported from envoy to wasm.
Word logHandler(void* raw_context, Word level, Word address, Word size);
Word getPropertyHandler(void* raw_context, Word path_ptr, Word path_size, Word value_ptr_ptr,
                        Word value_size_ptr);
Word setPropertyHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr,
                        Word value_size);
Word continueRequestHandler(void* raw_context);
Word continueResponseHandler(void* raw_context);
Word sendLocalResponseHandler(void* raw_context, Word response_code, Word response_code_details_ptr,
                              Word response_code_details_size, Word body_ptr, Word body_size,
                              Word additional_response_header_pairs_ptr,
                              Word additional_response_header_pairs_size, Word grpc_status);
Word clearRouteCacheHandler(void* raw_context);
Word getSharedDataHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr_ptr,
                          Word value_size_ptr, Word cas_ptr);
Word setSharedDataHandler(void* raw_context, Word key_ptr, Word key_size, Word value_ptr,
                          Word value_size, Word cas);
Word registerSharedQueueHandler(void* raw_context, Word queue_name_ptr, Word queue_name_size,
                                Word token_ptr);
Word resolveSharedQueueHandler(void* raw_context, Word vm_id_ptr, Word vm_id_size,
                               Word queue_name_ptr, Word queue_name_size, Word token_ptr);
Word dequeueSharedQueueHandler(void* raw_context, Word token, Word data_ptr_ptr,
                               Word data_size_ptr);
Word enqueueSharedQueueHandler(void* raw_context, Word token, Word data_ptr, Word data_size);
Word getDownstreamDataBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                         Word size_ptr);
Word getUpstreamDataBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                       Word size_ptr);
Word addHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr, Word value_size);
Word getHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                              Word value_ptr_ptr, Word value_size_ptr);
Word replaceHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size,
                                  Word value_ptr, Word value_size);
Word removeHeaderMapValueHandler(void* raw_context, Word type, Word key_ptr, Word key_size);
Word getHeaderMapPairsHandler(void* raw_context, Word type, Word ptr_ptr, Word size_ptr);
Word setHeaderMapPairsHandler(void* raw_context, Word type, Word ptr, Word size);
Word getHeaderMapSizeHandler(void* raw_context, Word type, Word result_ptr);
Word getRequestBodyBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                      Word size_ptr);
Word getResponseBodyBufferBytesHandler(void* raw_context, Word start, Word length, Word ptr_ptr,
                                       Word size_ptr);
Word httpCallHandler(void* raw_context, Word uri_ptr, Word uri_size, Word header_pairs_ptr,
                     Word header_pairs_size, Word body_ptr, Word body_size, Word trailer_pairs_ptr,
                     Word trailer_pairs_size, Word timeout_milliseconds);
Word defineMetricHandler(void* raw_context, Word metric_type, Word name_ptr, Word name_size,
                         Word result_ptr);
Word incrementMetricHandler(void* raw_context, Word metric_id, int64_t offset);
Word recordMetricHandler(void* raw_context, Word metric_id, uint64_t value);
Word getMetricHandler(void* raw_context, Word metric_id, Word result_uint64_ptr);
Word grpcCallHandler(void* raw_context, Word service_ptr, Word service_size, Word service_name_ptr,
                     Word service_name_size, Word method_name_ptr, Word method_name_size,
                     Word request_ptr, Word request_size, Word timeout_milliseconds);
Word grpcStreamHandler(void* raw_context, Word service_ptr, Word service_size,
                       Word service_name_ptr, Word service_name_size, Word method_name_ptr,
                       Word method_name_size);
Word grpcCancelHandler(void* raw_context, Word token);
Word grpcCloseHandler(void* raw_context, Word token);
Word grpcSendHandler(void* raw_context, Word token, Word message_ptr, Word message_size,
                     Word end_stream);

Word setTickPeriodMillisecondsHandler(void* raw_context, Word tick_period_milliseconds);
Word getCurrentTimeNanosecondsHandler(void* raw_context, Word result_uint64_ptr);

Word setEffectiveContextHandler(void* raw_context, Word context_id);

inline MetadataType StreamType2MetadataType(StreamType type) {
  return static_cast<MetadataType>(type);
}

struct AsyncClientHandler : public Http::AsyncClient::Callbacks {
  // Http::AsyncClient::Callbacks
  void onSuccess(Envoy::Http::MessagePtr&& response) override;
  void onFailure(Http::AsyncClient::FailureReason reason) override;

  Context* context;
  uint32_t token;
  Http::AsyncClient::Request* request;
};

struct GrpcCallClientHandler : public Grpc::RawAsyncRequestCallbacks {
  // Grpc::AsyncRequestCallbacks
  void onCreateInitialMetadata(Http::HeaderMap& metadata) override;
  void onSuccessRaw(Buffer::InstancePtr&& response, Tracing::Span& span) override;
  void onFailure(Grpc::Status::GrpcStatus status, const std::string& message,
                 Tracing::Span& span) override;

  Context* context;
  uint32_t token;
  Grpc::RawAsyncClientPtr client;
  Grpc::AsyncRequest* request;
};

struct GrpcStreamClientHandler : public Grpc::RawAsyncStreamCallbacks {
  // Grpc::AsyncStreamCallbacks
  void onCreateInitialMetadata(Http::HeaderMap& metadata) override;
  void onReceiveInitialMetadata(Http::HeaderMapPtr&& metadata) override;
  bool onReceiveMessageRaw(Buffer::InstancePtr&& response) override;
  void onReceiveTrailingMetadata(Http::HeaderMapPtr&& metadata) override;
  void onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) override;

  Context* context;
  uint32_t token;
  Grpc::RawAsyncClientPtr client;
  Grpc::RawAsyncStream* stream;
};

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
  Context();                                                              // Testing.
  Context(Wasm* wasm);                                                    // Vm Context.
  Context(Wasm* wasm, absl::string_view root_id, PluginSharedPtr plugin); // Root Context.
  Context(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin);  // Stream context.
  ~Context();

  Wasm* wasm() const { return wasm_; }
  uint32_t id() const { return id_; }
  bool isVmContext() { return id_ == 0; }
  bool isRootContext() { return root_context_id_ == 0; }
  Context* root_context() { return root_context_; }

  absl::string_view root_id() const { return plugin_->root_id_; }
  absl::string_view log_prefix() const { return plugin_->log_prefix_; }

  WasmVm* wasmVm() const;
  Upstream::ClusterManager& clusterManager() const;

  // Retrieves the stream info associated with the request (a.k.a active stream).
  // It selects a value based on the following order: encoder callback, decoder
  // callback, log callback. As long as any one of the callbacks is invoked, the value should be
  // available.
  const StreamInfo::StreamInfo* getConstRequestStreamInfo() const;
  StreamInfo::StreamInfo* getRequestStreamInfo() const;

  //
  // VM level downcalls into the WASM code on Context(id == 0).
  //
  virtual bool validateConfiguration(absl::string_view configuration);
  virtual void onStart(absl::string_view root_id, absl::string_view vm_configuration);
  virtual bool onConfigure(absl::string_view configuration);

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
  virtual void onHttpCallResponse(uint32_t token, const Pairs& response_headers,
                                  absl::string_view response_body, const Pairs& response_trailers);
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

  // Network
  virtual WasmResult getDownstreamDataBufferBytes(uint32_t start, uint32_t length,
                                                  absl::string_view* data);
  virtual WasmResult getUpstreamDataBufferBytes(uint32_t start, uint32_t length,
                                                absl::string_view* data);

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

  // Body Buffer
  virtual absl::string_view getRequestBodyBufferBytes(uint32_t start, uint32_t length);
  virtual absl::string_view getResponseBodyBufferBytes(uint32_t start, uint32_t length);

  // HTTP
  // Returns a token which will be used with the corresponding onHttpCallResponse.
  virtual uint32_t httpCall(absl::string_view cluster, const Pairs& request_headers,
                            absl::string_view request_body, const Pairs& request_trailers,
                            int timeout_millisconds);
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
  virtual uint32_t grpcCall(const envoy::api::v2::core::GrpcService& grpc_service,
                            absl::string_view service_name, absl::string_view method_name,
                            absl::string_view request,
                            const absl::optional<std::chrono::milliseconds>& timeout);
  virtual uint32_t grpcStream(const envoy::api::v2::core::GrpcService& grpc_service,
                              absl::string_view service_name, absl::string_view method_name);
  virtual WasmResult grpcClose(uint32_t token);  // cancel on call, close on stream.
  virtual WasmResult grpcCancel(uint32_t token); // cancel on call, reset on stream.
  virtual WasmResult grpcSend(uint32_t token, absl::string_view message,
                              bool end_stream); // stream only

  // Connection
  virtual bool isSsl();

protected:
  friend class Wasm;
  friend struct AsyncClientHandler;
  friend struct GrpcCallClientHandler;
  friend struct GrpcStreamClientHandler;

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
  Context* root_context_{nullptr}; // set in all contexts.
  PluginSharedPtr plugin_;
  bool in_vm_context_created_ = false;
  bool destroyed_ = false;

  uint32_t next_http_call_token_ = 1;
  uint32_t next_grpc_token_ = 1; // Odd tokens are for Calls even for Streams.

  // MB: must be a node-type map as we take persistent references to the entries.
  std::map<uint32_t, AsyncClientHandler> http_request_;
  std::map<uint32_t, GrpcCallClientHandler> grpc_call_request_;
  std::map<uint32_t, GrpcStreamClientHandler> grpc_stream_;
  Envoy::Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Envoy::Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};

  // Network filter state.
  Network::ReadFilterCallbacks* network_read_filter_callbacks_{};
  Network::WriteFilterCallbacks* network_write_filter_callbacks_{};
  Buffer::Instance* network_downstream_data_buffer_{};
  Buffer::Instance* network_upstream_data_buffer_{};

  // HTTP Filter state.
  Http::HeaderMap* request_headers_{};
  Http::HeaderMap* response_headers_{};
  Buffer::Instance* requestBodyBuffer_{};
  Buffer::Instance* responseBodyBuffer_{};
  bool request_end_of_stream_{};
  bool response_end_of_stream_{};
  Http::HeaderMap* request_trailers_{};
  Http::HeaderMap* response_trailers_{};
  Http::MetadataMap* request_metadata_{};
  Http::MetadataMap* response_metadata_{};

  Http::HeaderMap* grpc_create_initial_metadata_{};
  Http::HeaderMapPtr grpc_receive_initial_metadata_{};
  Http::HeaderMapPtr grpc_receive_trailing_metadata_{};

  const StreamInfo::StreamInfo* access_log_stream_info_{};
  const Http::HeaderMap* access_log_request_headers_{};
  const Http::HeaderMap* access_log_response_headers_{};
  const Http::HeaderMap* access_log_request_trailers_{}; // unused
  const Http::HeaderMap* access_log_response_trailers_{};

  ProtobufWkt::Struct temporary_metadata_;
};

// Wasm execution instance. Manages the Envoy side of the Wasm interface.
class Wasm : public Envoy::Server::Wasm,
             public ThreadLocal::ThreadLocalObject,
             public Logger::Loggable<Logger::Id::wasm>,
             public std::enable_shared_from_this<Wasm> {
public:
  Wasm(absl::string_view runtime, absl::string_view vm_id, absl::string_view vm_configuration,
       PluginSharedPtr plugin, Stats::ScopeSharedPtr scope,
       Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher);
  Wasm(const Wasm& other, Event::Dispatcher& dispatcher);
  ~Wasm() {}

  bool initialize(const std::string& code, bool allow_precompiled = false);
  void startVm(Context* root_context);
  bool configure(Context* root_context, absl::string_view configuration);
  Context* start(); // returns the root Context.

  absl::string_view vm_id() const { return vm_id_; }
  const PluginSharedPtr& plugin() const { return plugin_; }
  void setPlugin(const PluginSharedPtr plugin) { plugin_ = plugin; }
  WasmVm* wasmVm() const { return wasm_vm_.get(); }
  Context* vmContext() const { return vm_context_.get(); }
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
  void startForTesting(std::unique_ptr<Context> root_context);

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

  std::string vm_id_; // The effective vm_id (may be a hash).
  std::unique_ptr<WasmVm> wasm_vm_;

  PluginSharedPtr plugin_;
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
  WasmCallWord<3> validateConfiguration_;
  WasmCallVoid<5> onStart_;
  WasmCallWord<3> onConfigure_;
  WasmCallVoid<1> onTick_;

  WasmCallVoid<2> onCreate_;

  WasmCallWord<1> onNewConnection_;
  WasmCallWord<3> onDownstreamData_;
  WasmCallWord<3> onUpstreamData_;
  WasmCallVoid<2> onDownstreamConnectionClose_;
  WasmCallVoid<2> onUpstreamConnectionClose_;

  WasmCallWord<1> onRequestHeaders_;
  WasmCallWord<3> onRequestBody_;
  WasmCallWord<1> onRequestTrailers_;
  WasmCallWord<1> onRequestMetadata_;

  WasmCallWord<1> onResponseHeaders_;
  WasmCallWord<3> onResponseBody_;
  WasmCallWord<1> onResponseTrailers_;
  WasmCallWord<1> onResponseMetadata_;

  WasmCallVoid<8> onHttpCallResponse_;

  WasmCallVoid<4> onGrpcReceive_;
  WasmCallVoid<5> onGrpcClose_;
  WasmCallVoid<2> onGrpcCreateInitialMetadata_;
  WasmCallVoid<2> onGrpcReceiveInitialMetadata_;
  WasmCallVoid<2> onGrpcReceiveTrailingMetadata_;

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

  // Stats/Metrics
  Stats::StatNameSetSharedPtr stat_name_set_;
  uint32_t next_counter_metric_id_ = kMetricTypeCounter;
  uint32_t next_gauge_metric_id_ = kMetricTypeGauge;
  uint32_t next_histogram_metric_id_ = kMetricTypeHistogram;
  absl::flat_hash_map<uint32_t, Stats::Counter*> counters_;
  absl::flat_hash_map<uint32_t, Stats::Gauge*> gauges_;
  absl::flat_hash_map<uint32_t, Stats::Histogram*> histograms_;
};

// These accessors require Wasm.
inline WasmVm* Context::wasmVm() const { return wasm_->wasmVm(); }
inline Upstream::ClusterManager& Context::clusterManager() const { return wasm_->clusterManager(); }

// Create a high level Wasm VM with Envoy API support. Note: 'id' may be empty if this VM will not
// be shared by APIs (e.g. HTTP Filter + AccessLog).
std::shared_ptr<Wasm> createWasm(const envoy::config::wasm::v2::VmConfig& vm_config,
                                 PluginSharedPtr plugin_config, Stats::ScopeSharedPtr scope,
                                 Upstream::ClusterManager& cluster_manager,
                                 Event::Dispatcher& dispatcher, Api::Api& api);

// Create a ThreadLocal VM from an existing VM (e.g. from createWasm() above).
std::shared_ptr<Wasm> createThreadLocalWasm(Wasm& base_wasm, absl::string_view configuration,
                                            Event::Dispatcher& dispatcher);

std::shared_ptr<Wasm> createWasmForTesting(const envoy::config::wasm::v2::VmConfig& vm_config,
                                           PluginSharedPtr plugin, Stats::ScopeSharedPtr scope,
                                           Upstream::ClusterManager& cluster_manager,
                                           Event::Dispatcher& dispatcher, Api::Api& api,
                                           std::unique_ptr<Context> root_context_for_testing);

// Get an existing ThreadLocal VM matching 'vm_id' or nullptr if there isn't one.
std::shared_ptr<Wasm> getThreadLocalWasmPtr(absl::string_view vm_id);
// Get an existing ThreadLocal VM matching 'vm_id' or create one using 'base_wavm' by cloning or by
// using it it as a template.
std::shared_ptr<Wasm> getOrCreateThreadLocalWasm(Wasm& base_wasm, absl::string_view configuration,
                                                 Event::Dispatcher& dispatcher);

uint32_t resolveQueueForTest(absl::string_view vm_id, absl::string_view queue_name);

inline Context::Context() : root_context_(this) {}

inline Context::Context(Wasm* wasm) : wasm_(wasm), root_context_(this), plugin_(wasm->plugin()) {
  wasm_->contexts_[id_] = this;
}

inline Context::Context(Wasm* wasm, absl::string_view root_id, PluginSharedPtr plugin)
    : wasm_(wasm), id_(wasm->allocContextId()), root_context_(this), plugin_(plugin) {
  RELEASE_ASSERT(root_id == plugin->root_id_, "");
  wasm_->contexts_[id_] = this;
}

inline Context::Context(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin)
    : wasm_(wasm), id_(wasm->allocContextId()), root_context_id_(root_context_id), plugin_(plugin) {
  wasm_->contexts_[id_] = this;
  root_context_ = wasm_->contexts_[root_context_id_];
}

inline void* Wasm::allocMemory(uint64_t size, uint64_t* address) {
  Word a = malloc_(vmContext(), size);
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
