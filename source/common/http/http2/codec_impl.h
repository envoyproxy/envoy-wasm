#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "envoy/event/deferred_deletable.h"
#include "envoy/http/codec.h"
#include "envoy/network/connection.h"
#include "envoy/stats/scope.h"

#include "common/buffer/buffer_impl.h"
#include "common/buffer/watermark_buffer.h"
#include "common/common/linked_object.h"
#include "common/common/logger.h"
#include "common/http/codec_helper.h"
#include "common/http/header_map_impl.h"
#include "common/http/http2/metadata_decoder.h"
#include "common/http/http2/metadata_encoder.h"
#include "common/http/utility.h"

#include "absl/types/optional.h"
#include "nghttp2/nghttp2.h"

namespace Envoy {
namespace Http {
namespace Http2 {

const std::string ALPN_STRING = "h2";

// This is not the full client magic, but it's the smallest size that should be able to
// differentiate between HTTP/1 and HTTP/2.
const std::string CLIENT_MAGIC_PREFIX = "PRI * HTTP/2";

/**
 * All stats for the HTTP/2 codec. @see stats_macros.h
 */
// clang-format off
#define ALL_HTTP2_CODEC_STATS(COUNTER)                                                             \
  COUNTER(header_overflow)                                                                         \
  COUNTER(headers_cb_no_stream)                                                                    \
  COUNTER(rx_messaging_error)                                                                      \
  COUNTER(rx_reset)                                                                                \
  COUNTER(too_many_header_frames)                                                                  \
  COUNTER(trailers)                                                                                \
  COUNTER(tx_reset)
// clang-format on

/**
 * Wrapper struct for the HTTP/2 codec stats. @see stats_macros.h
 */
struct CodecStats {
  ALL_HTTP2_CODEC_STATS(GENERATE_COUNTER_STRUCT)
};

class Utility {
public:
  /**
   * Deal with https://tools.ietf.org/html/rfc7540#section-8.1.2.5
   * @param key supplies the incoming header key.
   * @param value supplies the incoming header value.
   * @param cookies supplies the header string to fill if this is a cookie header that needs to be
   *                rebuilt.
   */
  static bool reconstituteCrumbledCookies(const HeaderString& key, const HeaderString& value,
                                          HeaderString& cookies);
};

/**
 * Base class for HTTP/2 client and server codecs.
 */
class ConnectionImpl : public virtual Connection, protected Logger::Loggable<Logger::Id::http2> {
public:
  ConnectionImpl(Network::Connection& connection, Stats::Scope& stats,
                 const Http2Settings& http2_settings, const uint32_t max_request_headers_kb)
      : stats_{ALL_HTTP2_CODEC_STATS(POOL_COUNTER_PREFIX(stats, "http2."))},
        connection_(connection), max_request_headers_kb_(max_request_headers_kb),
        per_stream_buffer_limit_(http2_settings.initial_stream_window_size_), dispatching_(false),
        raised_goaway_(false), pending_deferred_reset_(false) {}

  ~ConnectionImpl() override;

  // Http::Connection
  void dispatch(Buffer::Instance& data) override;
  void goAway() override;
  Protocol protocol() override { return Protocol::Http2; }
  void shutdownNotice() override;
  bool wantsToWrite() override { return nghttp2_session_want_write(session_); }
  // Propagate network connection watermark events to each stream on the connection.
  void onUnderlyingConnectionAboveWriteBufferHighWatermark() override {
    for (auto& stream : active_streams_) {
      stream->runHighWatermarkCallbacks();
    }
  }
  void onUnderlyingConnectionBelowWriteBufferLowWatermark() override {
    for (auto& stream : active_streams_) {
      stream->runLowWatermarkCallbacks();
    }
  }

protected:
  /**
   * Wrapper for static nghttp2 callback dispatchers.
   */
  class Http2Callbacks {
  public:
    Http2Callbacks();
    ~Http2Callbacks();

    const nghttp2_session_callbacks* callbacks() { return callbacks_; }

  private:
    nghttp2_session_callbacks* callbacks_;
  };

  /**
   * Wrapper for static nghttp2 session options.
   */
  class Http2Options {
  public:
    Http2Options(const Http2Settings& http2_settings);
    ~Http2Options();

    const nghttp2_option* options() { return options_; }

  protected:
    nghttp2_option* options_;
  };

  class ClientHttp2Options : public Http2Options {
  public:
    ClientHttp2Options(const Http2Settings& http2_settings);
  };

  /**
   * Base class for client and server side streams.
   */
  struct StreamImpl : public StreamEncoder,
                      public Stream,
                      public LinkedObject<StreamImpl>,
                      public Event::DeferredDeletable,
                      public StreamCallbackHelper {

    StreamImpl(ConnectionImpl& parent, uint32_t buffer_limit);

    StreamImpl* base() { return this; }
    ssize_t onDataSourceRead(uint64_t length, uint32_t* data_flags);
    int onDataSourceSend(const uint8_t* framehd, size_t length);
    void resetStreamWorker(StreamResetReason reason);
    static void buildHeaders(std::vector<nghttp2_nv>& final_headers, const HeaderMap& headers);
    void saveHeader(HeaderString&& name, HeaderString&& value);
    virtual void submitHeaders(const std::vector<nghttp2_nv>& final_headers,
                               nghttp2_data_provider* provider) PURE;
    void submitTrailers(const HeaderMap& trailers);
    void submitMetadata();

    // Http::StreamEncoder
    void encode100ContinueHeaders(const HeaderMap& headers) override;
    void encodeHeaders(const HeaderMap& headers, bool end_stream) override;
    void encodeData(Buffer::Instance& data, bool end_stream) override;
    void encodeTrailers(const HeaderMap& trailers) override;
    Stream& getStream() override { return *this; }
    void encodeMetadata(const MetadataMapVector& metadata_map_vector) override;

    // Http::Stream
    void addCallbacks(StreamCallbacks& callbacks) override { addCallbacks_(callbacks); }
    void removeCallbacks(StreamCallbacks& callbacks) override { removeCallbacks_(callbacks); }
    void resetStream(StreamResetReason reason) override;
    void readDisable(bool disable) override;
    uint32_t bufferLimit() override { return pending_recv_data_.highWatermark(); }

    void setWriteBufferWatermarks(uint32_t low_watermark, uint32_t high_watermark) {
      pending_recv_data_.setWatermarks(low_watermark, high_watermark);
      pending_send_data_.setWatermarks(low_watermark, high_watermark);
    }

    // If the receive buffer encounters watermark callbacks, enable/disable reads on this stream.
    void pendingRecvBufferHighWatermark();
    void pendingRecvBufferLowWatermark();

    // If the send buffer encounters watermark callbacks, propagate this information to the streams.
    // The router and connection manager will propagate them on as appropriate.
    void pendingSendBufferHighWatermark();
    void pendingSendBufferLowWatermark();

    // Does any necessary WebSocket/Upgrade conversion, then passes the headers
    // to the decoder_.
    void decodeHeaders();

    // Get MetadataEncoder for this stream.
    MetadataEncoder& getMetadataEncoder();
    // Get MetadataDecoder for this stream.
    MetadataDecoder& getMetadataDecoder();
    // Callback function for MetadataDecoder.
    void onMetadataDecoded(MetadataMapPtr&& metadata_map_ptr);

    virtual void transformUpgradeFromH1toH2(HeaderMap& headers) PURE;
    virtual void maybeTransformUpgradeFromH2ToH1() PURE;

    bool buffers_overrun() const { return read_disable_count_ > 0; }

    ConnectionImpl& parent_;
    HeaderMapImplPtr headers_;
    StreamDecoder* decoder_{};
    int32_t stream_id_{-1};
    uint32_t unconsumed_bytes_{0};
    uint32_t read_disable_count_{0};
    Buffer::WatermarkBuffer pending_recv_data_{
        [this]() -> void { this->pendingRecvBufferLowWatermark(); },
        [this]() -> void { this->pendingRecvBufferHighWatermark(); }};
    Buffer::WatermarkBuffer pending_send_data_{
        [this]() -> void { this->pendingSendBufferLowWatermark(); },
        [this]() -> void { this->pendingSendBufferHighWatermark(); }};
    HeaderMapPtr pending_trailers_;
    std::unique_ptr<MetadataDecoder> metadata_decoder_;
    std::unique_ptr<MetadataEncoder> metadata_encoder_;
    absl::optional<StreamResetReason> deferred_reset_;
    HeaderString cookies_;
    bool local_end_stream_sent_ : 1;
    bool remote_end_stream_ : 1;
    bool data_deferred_ : 1;
    bool waiting_for_non_informational_headers_ : 1;
    bool pending_receive_buffer_high_watermark_called_ : 1;
    bool pending_send_buffer_high_watermark_called_ : 1;
    bool reset_due_to_messaging_error_ : 1;
  };

  using StreamImplPtr = std::unique_ptr<StreamImpl>;

  /**
   * Client side stream (request).
   */
  struct ClientStreamImpl : public StreamImpl {
    using StreamImpl::StreamImpl;

    // StreamImpl
    void submitHeaders(const std::vector<nghttp2_nv>& final_headers,
                       nghttp2_data_provider* provider) override;
    void transformUpgradeFromH1toH2(HeaderMap& headers) override {
      upgrade_type_ = std::string(headers.Upgrade()->value().getStringView());
      Http::Utility::transformUpgradeRequestFromH1toH2(headers);
    }
    void maybeTransformUpgradeFromH2ToH1() override {
      if (!upgrade_type_.empty() && headers_->Status()) {
        Http::Utility::transformUpgradeResponseFromH2toH1(*headers_, upgrade_type_);
      }
    }
    std::string upgrade_type_;
  };

  /**
   * Server side stream (response).
   */
  struct ServerStreamImpl : public StreamImpl {
    using StreamImpl::StreamImpl;

    // StreamImpl
    void encodeHeaders(const HeaderMap& headers, bool end_stream) override {
      // The contract is that client codecs must ensure that :status is present.
      ASSERT(headers.Status() != nullptr);
      StreamImpl::encodeHeaders(headers, end_stream);
    }
    void submitHeaders(const std::vector<nghttp2_nv>& final_headers,
                       nghttp2_data_provider* provider) override;
    void transformUpgradeFromH1toH2(HeaderMap& headers) override {
      Http::Utility::transformUpgradeResponseFromH1toH2(headers);
    }
    void maybeTransformUpgradeFromH2ToH1() override {
      if (Http::Utility::isH2UpgradeRequest(*headers_)) {
        Http::Utility::transformUpgradeRequestFromH2toH1(*headers_);
      }
    }
  };

  ConnectionImpl* base() { return this; }
  StreamImpl* getStream(int32_t stream_id);
  int saveHeader(const nghttp2_frame* frame, HeaderString&& name, HeaderString&& value);
  void sendPendingFrames();
  void sendSettings(const Http2Settings& http2_settings, bool disable_push);

  static Http2Callbacks http2_callbacks_;

  std::list<StreamImplPtr> active_streams_;
  nghttp2_session* session_{};
  CodecStats stats_;
  Network::Connection& connection_;
  const uint32_t max_request_headers_kb_;
  uint32_t per_stream_buffer_limit_;
  bool allow_metadata_;

private:
  virtual ConnectionCallbacks& callbacks() PURE;
  virtual int onBeginHeaders(const nghttp2_frame* frame) PURE;
  int onData(int32_t stream_id, const uint8_t* data, size_t len);
  int onFrameReceived(const nghttp2_frame* frame);
  int onFrameSend(const nghttp2_frame* frame);
  virtual int onHeader(const nghttp2_frame* frame, HeaderString&& name, HeaderString&& value) PURE;
  int onInvalidFrame(int32_t stream_id, int error_code);
  ssize_t onSend(const uint8_t* data, size_t length);
  int onStreamClose(int32_t stream_id, uint32_t error_code);
  int onMetadataReceived(int32_t stream_id, const uint8_t* data, size_t len);
  int onMetadataFrameComplete(int32_t stream_id, bool end_metadata);
  ssize_t packMetadata(int32_t stream_id, uint8_t* buf, size_t len);

  bool dispatching_ : 1;
  bool raised_goaway_ : 1;
  bool pending_deferred_reset_ : 1;
};

/**
 * HTTP/2 client connection codec.
 */
class ClientConnectionImpl : public ClientConnection, public ConnectionImpl {
public:
  ClientConnectionImpl(Network::Connection& connection, ConnectionCallbacks& callbacks,
                       Stats::Scope& stats, const Http2Settings& http2_settings,
                       const uint32_t max_request_headers_kb);

  // Http::ClientConnection
  Http::StreamEncoder& newStream(StreamDecoder& response_decoder) override;

private:
  // ConnectionImpl
  ConnectionCallbacks& callbacks() override { return callbacks_; }
  int onBeginHeaders(const nghttp2_frame* frame) override;
  int onHeader(const nghttp2_frame* frame, HeaderString&& name, HeaderString&& value) override;

  Http::ConnectionCallbacks& callbacks_;
};

/**
 * HTTP/2 server connection codec.
 */
class ServerConnectionImpl : public ServerConnection, public ConnectionImpl {
public:
  ServerConnectionImpl(Network::Connection& connection, ServerConnectionCallbacks& callbacks,
                       Stats::Scope& scope, const Http2Settings& http2_settings,
                       const uint32_t max_request_headers_kb);

private:
  // ConnectionImpl
  ConnectionCallbacks& callbacks() override { return callbacks_; }
  int onBeginHeaders(const nghttp2_frame* frame) override;
  int onHeader(const nghttp2_frame* frame, HeaderString&& name, HeaderString&& value) override;

  ServerConnectionCallbacks& callbacks_;
};

} // namespace Http2
} // namespace Http
} // namespace Envoy
