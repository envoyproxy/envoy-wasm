#pragma once

#include "envoy/http/filter.h"
#include "envoy/upstream/cluster_manager.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

using namespace Envoy::Extensions::Common::Wasm;

// A session within the WASM VM.
class Session : Logger::Loggable<Logger::Id::wasm>, Http::AsyncClient::Callbacks {
public:
  using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;

  // Calls coming from the WASM code.
  class FilterCallbacks {
  public:
    virtual ~FilterCallbacks() {}

    // Headers
    enum class HeaderType { Header = 0, Trailer = 1 };
    virtual void addHeader(Session* session, HeaderType type, absl::string_view key,
                           absl::string_view value);
    virtual absl::string_view getHeader(Session* session, HeaderType type, absl::string_view key);
    virtual Pairs getHeaderPairs(Session* session, HeaderType type);
    virtual void removeHeader(Session* session, HeaderType type, absl::string_view key);
    virtual void replaceHeader(Session* session, HeaderType type, absl::string_view key,
                               absl::string_view value);

    // Decoder
    virtual void continueDecoding(Session* session) PURE;

    // Encoder
    virtual void continueEncoding(Session* session) PURE;

    // StreamInfo
    virtual absl::string_view getSteamInfoProtocol(Session* session) PURE;
    virtual absl::string_view getStreamInfoMetadata(Session* session, absl::string_view filter,
                                                    absl::string_view key);
    virtual void setStreamInfoMetadata(Session* session, absl::string_view filter,
                                       absl::string_view key, absl::string_view value);
    virtual Pairs getStreamInfoPairs(Session* session, absl::string_view filter);

    // Body Buffer
    enum class BodyMode { FullBody = 0, BodyChunks = 1 };
    virtual void setBodyMode(Session* session, BodyMode body_mode) PURE;
    virtual int bodyBufferLength(Session* session) PURE;
    virtual absl::string_view getBodyBufferBytes(Session* session, int start, int length) PURE;

    // HTTP
    virtual void httpCall(Session* session, absl::string_view cluster, const Pairs& request_headers,
                          absl::string_view request_body, int timeout_millisconds);
    virtual void httpRespond(Session* session, const Pairs& response_headers,
                             absl::string_view body);

    // Metadata: the values are serialized ProtobufWkt::Struct
    virtual absl::string_view getMetadata(Session* session, absl::string_view key);
    virtual absl::string_view setMetadata(Session* session, absl::string_view key,
                                          absl::string_view serialized_proto_struct);
    virtual Pairs getMetadataPairs(Session* session);

    // Connection
    virtual bool isSsl(Session* session) PURE;
  };
  virtual void setFilterCallbacks(Callbacks& callbacks) PURE;

  // Calls into the WASM code.
  virtual void onStart() PURE;
  virtual void onBody() PURE;
  virtual void onTrailers() PURE;
  virtual void onHttpCallResponse(const Pairs& response_headers,
                                  absl::string_view response_body) PURE;
  virtual void raiseWasmError(absl::string_view message) PURE;
};
using SessionPtr = std::unique_ptr<Session>;

#define wasmL_checkudata(a, b, c) nullptr

#define DECLARE_WASM_FUNCTION_EX(Class, Name, Index)                                               \
  static int static_##Name(wasm_State* state) {                                                    \
    Class* object = static_cast<Class*>(wasmL_checkudata(state, Index, typeid(Class).name()));     \
    return object->Name(state);                                                                    \
  }                                                                                                \
  int Name(wasm_State* state);

#define wasm_upvalueindex(x) x

#define DECLARE_WASM_FUNCTION(Class, Name) DECLARE_WASM_FUNCTION_EX(Class, Name, 1)

#define DECLARE_WASM_CLOSURE(Class, Name)                                                          \
  DECLARE_WASM_FUNCTION_EX(Class, Name, wasm_upvalueindex(1))

template <typename T> class WasmRef {
public:
  void reset() {}
};
class wasm_State;

class BufferWrapper;
class ConnectionWrapper;
class MetadataMapWrapper;

class ThreadLocalState : Logger::Loggable<Logger::Id::wasm> {
public:
  ThreadLocalState(const std::string&, ThreadLocal::SlotAllocator&) {}
  SessionPtr createSession() { return nullptr; }
  int getGlobalRef(uint64_t /*slot*/) { return 0; }
  uint64_t registerGlobal(const std::string& /*global*/) { return 0; }
  uint64_t runtimeBytesUsed() { return 0; }
  void runtimeGC() {}

private:
};

using HeaderMapWrapper = int;
using StreamInfoWrapper = int;
using StreamHandleRef = int;

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

class FilterCallbacks {
public:
  virtual ~FilterCallbacks() {}
  virtual void addData(Buffer::Instance& data) PURE;
  virtual const Buffer::Instance* bufferedBody() PURE;
  virtual void continueIteration() PURE;
  virtual void onHeadersModified() PURE;
  virtual void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body,
                       wasm_State* state) PURE;
  virtual const ProtobufWkt::Struct& metadata() const PURE;
  virtual StreamInfo::StreamInfo& streamInfo() PURE;
  virtual const Network::Connection* connection() const PURE;
};

class Filter;

class StreamHandleWrapper : public Http::AsyncClient::Callbacks {
public:
  enum class State { Running, WaitForBodyChunk, WaitForBody, WaitForTrailers, HttpCall, Responded };

  StreamHandleWrapper(Session& session, Http::HeaderMap& headers, bool end_stream, Filter& filter,
                      FilterCallbacks& callbacks);

  Http::FilterHeadersStatus start(int function_ref);
  Http::FilterDataStatus onData(Buffer::Instance& data, bool end_stream);
  Http::FilterTrailersStatus onTrailers(Http::HeaderMap& trailers);

  void onReset() {
    if (http_request_) {
      http_request_->cancel();
      http_request_ = nullptr;
    }
  }

#if 0
  static ExportedFunctions exportedFunctions() {
    return {
      {"headers", static_wasmHeaders},           {"body", static_wasmBody},
      {"bodyChunks", static_wasmBodyChunks},     {"trailers", static_wasmTrailers},
      {"metadata", static_wasmMetadata},         {"logTrace", static_wasmLogTrace},
      {"logDebug", static_wasmLogDebug},         {"logInfo", static_wasmLogInfo},
      {"logWarn", static_wasmLogWarn},           {"logErr", static_wasmLogErr},
      {"logCritical", static_wasmLogCritical},   {"httpCall", static_wasmHttpCall},
      {"respond", static_wasmRespond},           {"streamInfo", static_wasmStreamInfo},
      {"connection", static_wasmConnection}};
  }
#endif

private:
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmHeaders);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmBody);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmBodyChunks);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmTrailers);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmMetadata);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmLogTrace);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmLogDebug);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmLogInfo);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmLogWarn);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmLogErr);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmLogCritical);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmHttpCall);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmRespond);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmStreamInfo);
  DECLARE_WASM_FUNCTION(StreamHandleWrapper, wasmConnection);

  DECLARE_WASM_CLOSURE(StreamHandleWrapper, wasmBodyIterator);

  static Http::HeaderMapPtr buildHeadersFromTable(wasm_State* state, int table_index);

  void onMarkDead() {
    headers_wrapper_.reset();
    body_wrapper_.reset();
    trailers_wrapper_.reset();
    metadata_wrapper_.reset();
    stream_info_wrapper_.reset();
    connection_wrapper_.reset();
  }

  // Http::AsyncClient::Callbacks
  void onSuccess(Http::MessagePtr&&) override;
  void onFailure(Http::AsyncClient::FailureReason) override;

  Session& session_;
  Http::HeaderMap& headers_;
  bool end_stream_;
  bool headers_continued_{};
  bool buffered_body_{};
  bool saw_body_{};
  Filter& filter_;
  FilterCallbacks& callbacks_;
  Http::HeaderMap* trailers_{};
  WasmRef<HeaderMapWrapper> headers_wrapper_;
  WasmRef<BufferWrapper> body_wrapper_;
  WasmRef<HeaderMapWrapper> trailers_wrapper_;
  WasmRef<MetadataMapWrapper> metadata_wrapper_;
  WasmRef<StreamInfoWrapper> stream_info_wrapper_;
  WasmRef<ConnectionWrapper> connection_wrapper_;
  State state_{State::Running};
  std::function<void()> yield_callback_;
  Http::AsyncClient::Request* http_request_{};
};

class FilterConfig : Logger::Loggable<Logger::Id::wasm> {
public:
  FilterConfig(const std::string& wasm_file, ThreadLocal::SlotAllocator& tls,
               Upstream::ClusterManager& cluster_manager);
  SessionPtr createSession() { return wasm_state_.createSession(); }
  int requestFunctionRef() { return wasm_state_.getGlobalRef(request_function_slot_); }
  int responseFunctionRef() { return wasm_state_.getGlobalRef(response_function_slot_); }
  uint64_t runtimeBytesUsed() { return wasm_state_.runtimeBytesUsed(); }
  void runtimeGC() { return wasm_state_.runtimeGC(); }

  Upstream::ClusterManager& cluster_manager_;

private:
  ThreadLocalState wasm_state_;
  uint64_t request_function_slot_;
  uint64_t response_function_slot_;
};

typedef std::shared_ptr<FilterConfig> FilterConfigConstSharedPtr;

class Filter : public Http::StreamFilter, Logger::Loggable<Logger::Id::wasm> {
public:
  Filter(FilterConfigConstSharedPtr config) : config_(config) {}

  Upstream::ClusterManager& clusterManager() { return config_->cluster_manager_; }
  void scriptError(const WasmException& e);
  virtual void scriptLog(spdlog::level::level_enum level, const char* message);

  // Http::StreamFilterBase
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override {
    return doHeaders(request_stream_wrapper_, request_session_, decoder_callbacks_,
                     config_->requestFunctionRef(), headers, end_stream);
  }
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override {
    return doData(request_stream_wrapper_, data, end_stream);
  }
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override {
    return doTrailers(request_stream_wrapper_, trailers);
  }
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override {
    decoder_callbacks_.callbacks_ = &callbacks;
  }

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override {
    return Http::FilterHeadersStatus::Continue;
  }
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override {
    return doHeaders(response_stream_wrapper_, response_session_, encoder_callbacks_,
                     config_->responseFunctionRef(), headers, end_stream);
  }
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override {
    return doData(response_stream_wrapper_, data, end_stream);
  };
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override {
    return doTrailers(response_stream_wrapper_, trailers);
  };
  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override {
    encoder_callbacks_.callbacks_ = &callbacks;
  };

private:
  struct DecoderCallbacks : public FilterCallbacks {
    DecoderCallbacks(Filter& parent) : parent_(parent) {}

    // FilterCallbacks
    void addData(Buffer::Instance& data) override {
      return callbacks_->addDecodedData(data, false);
    }
    const Buffer::Instance* bufferedBody() override { return callbacks_->decodingBuffer(); }
    void continueIteration() override { return callbacks_->continueDecoding(); }
    void onHeadersModified() override { callbacks_->clearRouteCache(); }
    void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, wasm_State* state) override;

    const ProtobufWkt::Struct& metadata() const override { return getMetadata(callbacks_); }
    StreamInfo::StreamInfo& streamInfo() override { return callbacks_->streamInfo(); }
    const Network::Connection* connection() const override { return callbacks_->connection(); }

    Filter& parent_;
    Http::StreamDecoderFilterCallbacks* callbacks_{};
  };

  struct EncoderCallbacks : public FilterCallbacks {
    EncoderCallbacks(Filter& parent) : parent_(parent) {}

    // FilterCallbacks
    void addData(Buffer::Instance& data) override {
      return callbacks_->addEncodedData(data, false);
    }
    const Buffer::Instance* bufferedBody() override { return callbacks_->encodingBuffer(); }
    void continueIteration() override { return callbacks_->continueEncoding(); }
    void onHeadersModified() override {}
    void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, wasm_State* state) override;

    const ProtobufWkt::Struct& metadata() const override { return getMetadata(callbacks_); }
    StreamInfo::StreamInfo& streamInfo() override { return callbacks_->streamInfo(); }
    const Network::Connection* connection() const override { return callbacks_->connection(); }

    Filter& parent_;
    Http::StreamEncoderFilterCallbacks* callbacks_{};
  };

  typedef WasmRef<StreamHandleWrapper> StreamHandleRef;

  Http::FilterHeadersStatus doHeaders(StreamHandleRef& handle, SessionPtr& session,
                                      FilterCallbacks& callbacks, int function_ref,
                                      Http::HeaderMap& headers, bool end_stream);
  Http::FilterDataStatus doData(StreamHandleRef& handle, Buffer::Instance& data, bool end_stream);
  Http::FilterTrailersStatus doTrailers(StreamHandleRef& handle, Http::HeaderMap& trailers);

  FilterConfigConstSharedPtr config_;
  DecoderCallbacks decoder_callbacks_{*this};
  EncoderCallbacks encoder_callbacks_{*this};
  StreamHandleRef request_stream_wrapper_;
  StreamHandleRef response_stream_wrapper_;
  bool destroyed_{};

  SessionPtr request_session_;
  SessionPtr response_session_;
};

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
