#pragma once

#include "envoy/http/filter.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/crypto/utility.h"

#include "extensions/common/crypto/crypto_impl.h"
#include "extensions/filters/common/lua/wrappers.h"
#include "extensions/filters/http/lua/wrappers.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Lua {

namespace {
const ProtobufWkt::Struct& getMetadata(Http::StreamFilterCallbacks* callbacks) {
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
} // namespace

/**
 * Callbacks used by a stream handler to access the filter.
 */
class FilterCallbacks {
public:
  virtual ~FilterCallbacks() = default;

  /**
   * Add data to the connection manager buffer.
   * @param data supplies the data to add.
   */
  virtual void addData(Buffer::Instance& data) PURE;

  /**
   * @return const Buffer::Instance* the currently buffered body.
   */
  virtual const Buffer::Instance* bufferedBody() PURE;

  /**
   * Continue filter iteration if iteration has been paused due to an async call.
   */
  virtual void continueIteration() PURE;

  /**
   * Called when headers have been modified by a script. This can only happen prior to headers
   * being continued.
   */
  virtual void onHeadersModified() PURE;

  /**
   * Perform an immediate response.
   * @param headers supplies the response headers.
   * @param body supplies the optional response body.
   * @param state supplies the active Lua state.
   */
  virtual void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, lua_State* state) PURE;

  /**
   * @return const ProtobufWkt::Struct& the value of metadata inside the lua filter scope of current
   * route entry.
   */
  virtual const ProtobufWkt::Struct& metadata() const PURE;

  /**
   * @return StreamInfo::StreamInfo& the current stream info handle. This handle is mutable to
   * accommodate write API e.g. setDynamicMetadata().
   */
  virtual StreamInfo::StreamInfo& streamInfo() PURE;

  /**
   * @return const Network::Connection* the current network connection handle.
   */
  virtual const Network::Connection* connection() const PURE;
};

class Filter;

/**
 * A wrapper for a currently running request/response. This is the primary handle passed to Lua.
 * The script interacts with Envoy entirely through this handle.
 */
class StreamHandleWrapper : public Filters::Common::Lua::BaseLuaObject<StreamHandleWrapper>,
                            public Http::AsyncClient::Callbacks {
public:
  /**
   * The state machine for a stream handler. In the current implementation everything the filter
   * does is a discrete state. This may become sub-optimal as we add other things that might
   * cause the filter to block.
   * TODO(mattklein123): Consider whether we should split the state machine into an overall state
   * and a blocking reason type.
   */
  enum class State {
    // Lua code is currently running or the script has finished.
    Running,
    // Lua script is blocked waiting for the next body chunk.
    WaitForBodyChunk,
    // Lua script is blocked waiting for the full body.
    WaitForBody,
    // Lua script is blocked waiting for trailers.
    WaitForTrailers,
    // Lua script is blocked waiting for the result of an HTTP call.
    HttpCall,
    // Lua script has done a direct response.
    Responded
  };

  StreamHandleWrapper(Filters::Common::Lua::Coroutine& coroutine, Http::HeaderMap& headers,
                      bool end_stream, Filter& filter, FilterCallbacks& callbacks);

  Http::FilterHeadersStatus start(int function_ref);
  Http::FilterDataStatus onData(Buffer::Instance& data, bool end_stream);
  Http::FilterTrailersStatus onTrailers(Http::HeaderMap& trailers);

  void onReset() {
    if (http_request_) {
      http_request_->cancel();
      http_request_ = nullptr;
    }
  }

  static ExportedFunctions exportedFunctions() {
    return {{"headers", static_luaHeaders},
            {"body", static_luaBody},
            {"bodyChunks", static_luaBodyChunks},
            {"trailers", static_luaTrailers},
            {"metadata", static_luaMetadata},
            {"logTrace", static_luaLogTrace},
            {"logDebug", static_luaLogDebug},
            {"logInfo", static_luaLogInfo},
            {"logWarn", static_luaLogWarn},
            {"logErr", static_luaLogErr},
            {"logCritical", static_luaLogCritical},
            {"httpCall", static_luaHttpCall},
            {"respond", static_luaRespond},
            {"streamInfo", static_luaStreamInfo},
            {"connection", static_luaConnection},
            {"importPublicKey", static_luaImportPublicKey},
            {"verifySignature", static_luaVerifySignature}};
  }

private:
  /**
   * Perform an HTTP call to an upstream host.
   * @param 1 (string): The name of the upstream cluster to call. This cluster must be configured.
   * @param 2 (table): A table of HTTP headers. :method, :path, and :authority must be defined.
   * @param 3 (string): Body. Can be nil.
   * @param 4 (int): Timeout in milliseconds for the call.
   * @return headers (table), body (string/nil)
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaHttpCall);

  /**
   * Perform an inline response. This call is currently only valid on the request path. Further
   * filter iteration will stop. No further script code will run after this call.
   * @param 1 (table): A table of HTTP headers. :status must be defined.
   * @param 2 (string): Body. Can be nil.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaRespond);

  /**
   * @return a handle to the headers.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaHeaders);

  /**
   * @return a handle to the full body or nil if there is no body. This call will cause the script
   *         to yield until the entire body is received (or if there is no body will return nil
   *         right away).
   *         NOTE: This call causes Envoy to buffer the body. The max buffer size is configured
   *         based on the currently active flow control settings.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaBody);

  /**
   * @return an iterator that allows the script to iterate through all body chunks as they are
   *         received. The iterator will yield between body chunks. Envoy *will not* buffer
   *         the body chunks in this case, but the script can look at them as they go by.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaBodyChunks);

  /**
   * @return a handle to the trailers or nil if there are no trailers. This call will cause the
   *         script to yield if Envoy does not yet know if there are trailers or not.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaTrailers);

  /**
   * @return a handle to the metadata.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaMetadata);

  /**
   * @return a handle to the stream info.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaStreamInfo);

  /**
   * @return a handle to the network connection.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaConnection);

  /**
   * Log a message to the Envoy log.
   * @param 1 (string): The log message.
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaLogTrace);
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaLogDebug);
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaLogInfo);
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaLogWarn);
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaLogErr);
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaLogCritical);

  /**
   * Verify cryptographic signatures.
   * @param 1 (string) hash function(including SHA1, SHA224, SHA256, SHA384, SHA512)
   * @param 2 (void*)  pointer to public key
   * @param 3 (string) signature
   * @param 4 (int)    length of signature
   * @param 5 (string) clear text
   * @param 6 (int)    length of clear text
   * @return (bool, string) If the first element is true, the second element is empty; otherwise,
   * the second element stores the error message
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaVerifySignature);

  /**
   * Import public key.
   * @param 1 (string) keyder string
   * @param 2 (int)    length of keyder string
   * @return pointer to public key
   */
  DECLARE_LUA_FUNCTION(StreamHandleWrapper, luaImportPublicKey);

  /**
   * This is the closure/iterator returned by luaBodyChunks() above.
   */
  DECLARE_LUA_CLOSURE(StreamHandleWrapper, luaBodyIterator);

  static Http::HeaderMapPtr buildHeadersFromTable(lua_State* state, int table_index);

  // Filters::Common::Lua::BaseLuaObject
  void onMarkDead() override {
    // Headers/body/trailers wrappers do not survive any yields. The user can request them
    // again across yields if needed.
    headers_wrapper_.reset();
    body_wrapper_.reset();
    trailers_wrapper_.reset();
    metadata_wrapper_.reset();
    stream_info_wrapper_.reset();
    connection_wrapper_.reset();
    public_key_wrapper_.reset();
  }

  // Http::AsyncClient::Callbacks
  void onSuccess(Http::MessagePtr&&) override;
  void onFailure(Http::AsyncClient::FailureReason) override;

  Filters::Common::Lua::Coroutine& coroutine_;
  Http::HeaderMap& headers_;
  bool end_stream_;
  bool headers_continued_{};
  bool buffered_body_{};
  bool saw_body_{};
  Filter& filter_;
  FilterCallbacks& callbacks_;
  Http::HeaderMap* trailers_{};
  Filters::Common::Lua::LuaDeathRef<HeaderMapWrapper> headers_wrapper_;
  Filters::Common::Lua::LuaDeathRef<Filters::Common::Lua::BufferWrapper> body_wrapper_;
  Filters::Common::Lua::LuaDeathRef<HeaderMapWrapper> trailers_wrapper_;
  Filters::Common::Lua::LuaDeathRef<Filters::Common::Lua::MetadataMapWrapper> metadata_wrapper_;
  Filters::Common::Lua::LuaDeathRef<StreamInfoWrapper> stream_info_wrapper_;
  Filters::Common::Lua::LuaDeathRef<Filters::Common::Lua::ConnectionWrapper> connection_wrapper_;
  Filters::Common::Lua::LuaDeathRef<PublicKeyWrapper> public_key_wrapper_;
  State state_{State::Running};
  std::function<void()> yield_callback_;
  Http::AsyncClient::Request* http_request_{};
};

/**
 * Global configuration for the filter.
 */
class FilterConfig : Logger::Loggable<Logger::Id::lua> {
public:
  FilterConfig(const std::string& lua_code, ThreadLocal::SlotAllocator& tls,
               Upstream::ClusterManager& cluster_manager);
  Filters::Common::Lua::CoroutinePtr createCoroutine() { return lua_state_.createCoroutine(); }
  int requestFunctionRef() { return lua_state_.getGlobalRef(request_function_slot_); }
  int responseFunctionRef() { return lua_state_.getGlobalRef(response_function_slot_); }
  uint64_t runtimeBytesUsed() { return lua_state_.runtimeBytesUsed(); }
  void runtimeGC() { return lua_state_.runtimeGC(); }

  Upstream::ClusterManager& cluster_manager_;

private:
  Filters::Common::Lua::ThreadLocalState lua_state_;
  uint64_t request_function_slot_;
  uint64_t response_function_slot_;
};

using FilterConfigConstSharedPtr = std::shared_ptr<FilterConfig>;

// TODO(mattklein123): Filter stats.

/**
 * The HTTP Lua filter. Allows scripts to run in both the request an response flow.
 */
class Filter : public Http::StreamFilter, Logger::Loggable<Logger::Id::lua> {
public:
  Filter(FilterConfigConstSharedPtr config) : config_(config) {}

  Upstream::ClusterManager& clusterManager() { return config_->cluster_manager_; }
  void scriptError(const Filters::Common::Lua::LuaException& e);
  virtual void scriptLog(spdlog::level::level_enum level, const char* message);

  // Http::StreamFilterBase
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override {
    return doHeaders(request_stream_wrapper_, request_coroutine_, decoder_callbacks_,
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
    return doHeaders(response_stream_wrapper_, response_coroutine_, encoder_callbacks_,
                     config_->responseFunctionRef(), headers, end_stream);
  }
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override {
    return doData(response_stream_wrapper_, data, end_stream);
  };
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override {
    return doTrailers(response_stream_wrapper_, trailers);
  };
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap&) override {
    return Http::FilterMetadataStatus::Continue;
  }
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
    void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, lua_State* state) override;

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
    void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, lua_State* state) override;

    const ProtobufWkt::Struct& metadata() const override { return getMetadata(callbacks_); }
    StreamInfo::StreamInfo& streamInfo() override { return callbacks_->streamInfo(); }
    const Network::Connection* connection() const override { return callbacks_->connection(); }

    Filter& parent_;
    Http::StreamEncoderFilterCallbacks* callbacks_{};
  };

  using StreamHandleRef = Filters::Common::Lua::LuaDeathRef<StreamHandleWrapper>;

  Http::FilterHeadersStatus doHeaders(StreamHandleRef& handle,
                                      Filters::Common::Lua::CoroutinePtr& coroutine,
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

  // These coroutines used to be owned by the stream handles. After investigating #3570, it
  // became clear that there is a circular memory reference when a coroutine yields. Basically,
  // the coroutine holds a reference to the stream wrapper. I'm not completely sure why this is,
  // but I think it is because the yield happens via a stream handle method, so the runtime must
  // hold a reference so that it can return out of the yield through the object. So now we hold
  // the coroutine references at the same level as the stream handles so that when the filter is
  // destroyed the circular reference is broken and both objects are cleaned up.
  //
  // Note that the above explanation probably means that we don't need to hold a reference to the
  // coroutine at all and it would be taken care of automatically via a runtime internal reference
  // when a yield happens. However, given that I don't fully understand the runtime internals, this
  // seems like a safer fix for now.
  Filters::Common::Lua::CoroutinePtr request_coroutine_;
  Filters::Common::Lua::CoroutinePtr response_coroutine_;
};

} // namespace Lua
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
