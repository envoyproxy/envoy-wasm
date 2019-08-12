#pragma once

#include <chrono>
#include <memory>

#include "envoy/event/dispatcher.h"
#include "envoy/http/message.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Http {

/**
 * Supports sending an HTTP request message and receiving a response asynchronously.
 */
class AsyncClient {
public:
  /**
   * Async Client failure reasons.
   */
  enum class FailureReason {
    // The stream has been reset.
    Reset
  };

  /**
   * Notifies caller of async HTTP request status.
   */
  class Callbacks {
  public:
    virtual ~Callbacks() = default;

    /**
     * Called when the async HTTP request succeeds.
     * @param response the HTTP response
     */
    virtual void onSuccess(MessagePtr&& response) PURE;

    /**
     * Called when the async HTTP request fails.
     */
    virtual void onFailure(FailureReason reason) PURE;
  };

  /**
   * Notifies caller of async HTTP stream status.
   * Note the HTTP stream is full-duplex, even if the local to remote stream has been ended
   * by Stream.sendHeaders/sendData with end_stream=true or sendTrailers,
   * StreamCallbacks can continue to receive events until the remote to local stream is closed,
   * and vice versa.
   */
  class StreamCallbacks {
  public:
    virtual ~StreamCallbacks() = default;

    /**
     * Called when all headers get received on the async HTTP stream.
     * @param headers the headers received
     * @param end_stream whether the response is header only
     */
    virtual void onHeaders(HeaderMapPtr&& headers, bool end_stream) PURE;

    /**
     * Called when a data frame get received on the async HTTP stream.
     * This can be invoked multiple times if the data get streamed.
     * @param data the data received
     * @param end_stream whether the data is the last data frame
     */
    virtual void onData(Buffer::Instance& data, bool end_stream) PURE;

    /**
     * Called when all trailers get received on the async HTTP stream.
     * @param trailers the trailers received.
     */
    virtual void onTrailers(HeaderMapPtr&& trailers) PURE;

    /**
     * Called when both the local and remote have gracefully closed the stream.
     * Useful for asymmetric cases where end_stream may not be bidirectionally observable.
     * Note this is NOT called on stream reset.
     */
    virtual void onComplete() PURE;

    /**
     * Called when the async HTTP stream is reset.
     */
    virtual void onReset() PURE;
  };

  /**
   * An in-flight HTTP request.
   */
  class Request {
  public:
    virtual ~Request() = default;

    /**
     * Signals that the request should be cancelled.
     */
    virtual void cancel() PURE;
  };

  /**
   * An in-flight HTTP stream.
   */
  class Stream {
  public:
    virtual ~Stream() = default;

    /***
     * Send headers to the stream. This method cannot be invoked more than once and
     * need to be called before sendData.
     * @param headers supplies the headers to send.
     * @param end_stream supplies whether this is a header only request.
     */
    virtual void sendHeaders(HeaderMap& headers, bool end_stream) PURE;

    /***
     * Send data to the stream. This method can be invoked multiple times if it get streamed.
     * To end the stream without data, call this method with empty buffer.
     * @param data supplies the data to send.
     * @param end_stream supplies whether this is the last data.
     */
    virtual void sendData(Buffer::Instance& data, bool end_stream) PURE;

    /***
     * Send trailers. This method cannot be invoked more than once, and implicitly ends the stream.
     * @param trailers supplies the trailers to send.
     */
    virtual void sendTrailers(HeaderMap& trailers) PURE;

    /***
     * Reset the stream.
     */
    virtual void reset() PURE;
  };

  virtual ~AsyncClient() = default;

  /**
   * A structure to hold the options for AsyncStream object.
   */
  struct StreamOptions {
    StreamOptions& setTimeout(const absl::optional<std::chrono::milliseconds>& v) {
      timeout = v;
      return *this;
    }
    StreamOptions& setTimeout(const std::chrono::milliseconds& v) {
      timeout = v;
      return *this;
    }
    StreamOptions& setBufferBodyForRetry(bool v) {
      buffer_body_for_retry = v;
      return *this;
    }
    StreamOptions& setSendXff(bool v) {
      send_xff = v;
      return *this;
    }

    // For gmock test
    bool operator==(const StreamOptions& src) const {
      return timeout == src.timeout && buffer_body_for_retry == src.buffer_body_for_retry &&
             send_xff == src.send_xff;
    }

    // The timeout supplies the stream timeout, measured since when the frame with
    // end_stream flag is sent until when the first frame is received.
    absl::optional<std::chrono::milliseconds> timeout;

    // The buffer_body_for_retry specifies whether the streamed body will be buffered so that
    // it can be retried. In general, this should be set to false for a true stream. However,
    // streaming is also used in certain cases such as gRPC unary calls, where retry can
    // still be useful.
    bool buffer_body_for_retry{false};

    // If true, x-forwarded-for header will be added.
    bool send_xff{true};
  };

  /**
   * A structure to hold the options for AsyncRequest object.
   */
  struct RequestOptions : public StreamOptions {
    RequestOptions& setTimeout(const absl::optional<std::chrono::milliseconds>& v) {
      StreamOptions::setTimeout(v);
      return *this;
    }
    RequestOptions& setTimeout(const std::chrono::milliseconds& v) {
      StreamOptions::setTimeout(v);
      return *this;
    }
    RequestOptions& setBufferBodyForRetry(bool v) {
      StreamOptions::setBufferBodyForRetry(v);
      return *this;
    }
    RequestOptions& setSendXff(bool v) {
      StreamOptions::setSendXff(v);
      return *this;
    }

    // For gmock test
    bool operator==(const RequestOptions& src) const { return StreamOptions::operator==(src); }
  };

  /**
   * Send an HTTP request asynchronously
   * @param request the request to send.
   * @param callbacks the callbacks to be notified of request status.
   * @param options the data struct to control the request sending.
   * @return a request handle or nullptr if no request could be created. NOTE: In this case
   *         onFailure() has already been called inline. The client owns the request and the
   *         handle should just be used to cancel.
   */

  virtual Request* send(MessagePtr&& request, Callbacks& callbacks,
                        const RequestOptions& options) PURE;

  /**
   * Start an HTTP stream asynchronously.
   * @param callbacks the callbacks to be notified of stream status.
   * @param options the data struct to control the stream.
   * @return a stream handle or nullptr if no stream could be started. NOTE: In this case
   *         onResetStream() has already been called inline. The client owns the stream and
   *         the handle can be used to send more messages or close the stream.
   */
  virtual Stream* start(StreamCallbacks& callbacks, const StreamOptions& options) PURE;

  /**
   * @return Event::Dispatcher& the dispatcher backing this client.
   */
  virtual Event::Dispatcher& dispatcher() PURE;
};

using AsyncClientPtr = std::unique_ptr<AsyncClient>;

} // namespace Http
} // namespace Envoy
