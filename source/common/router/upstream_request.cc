#include "common/router/upstream_request.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "envoy/event/dispatcher.h"
#include "envoy/event/timer.h"
#include "envoy/grpc/status.h"
#include "envoy/http/conn_pool.h"
#include "envoy/runtime/runtime.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/upstream.h"

#include "common/common/assert.h"
#include "common/common/empty_string.h"
#include "common/common/enum_to_int.h"
#include "common/common/scope_tracker.h"
#include "common/common/utility.h"
#include "common/grpc/common.h"
#include "common/http/codes.h"
#include "common/http/header_map_impl.h"
#include "common/http/headers.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"
#include "common/network/application_protocol.h"
#include "common/network/transport_socket_options_impl.h"
#include "common/network/upstream_server_name.h"
#include "common/network/upstream_subject_alt_names.h"
#include "common/router/config_impl.h"
#include "common/router/debug_config.h"
#include "common/router/router.h"
#include "common/stream_info/uint32_accessor_impl.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/common/proxy_protocol/proxy_protocol_header.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Router {

UpstreamRequest::UpstreamRequest(RouterFilterInterface& parent,
                                 std::unique_ptr<GenericConnPool>&& conn_pool)
    : parent_(parent), conn_pool_(std::move(conn_pool)), grpc_rq_success_deferred_(false),
      stream_info_(parent_.callbacks()->dispatcher().timeSource()),
      start_time_(parent_.callbacks()->dispatcher().timeSource().monotonicTime()),
      calling_encode_headers_(false), upstream_canary_(false), decode_complete_(false),
      encode_complete_(false), encode_trailers_(false), retried_(false), awaiting_headers_(true),
      outlier_detection_timeout_recorded_(false),
      create_per_try_timeout_on_request_complete_(false), paused_for_connect_(false),
      record_timeout_budget_(parent_.cluster()->timeoutBudgetStats().has_value()) {
  if (parent_.config().start_child_span_) {
    span_ = parent_.callbacks()->activeSpan().spawnChild(
        parent_.callbacks()->tracingConfig(), "router " + parent.cluster()->name() + " egress",
        parent.timeSource().systemTime());
    if (parent.attemptCount() != 1) {
      // This is a retry request, add this metadata to span.
      span_->setTag(Tracing::Tags::get().RetryCount, std::to_string(parent.attemptCount() - 1));
    }
  }

  stream_info_.healthCheck(parent_.callbacks()->streamInfo().healthCheck());
  if (conn_pool_->protocol().has_value()) {
    stream_info_.protocol(conn_pool_->protocol().value());
  }
}

UpstreamRequest::~UpstreamRequest() {
  if (span_ != nullptr) {
    Tracing::HttpTracerUtility::finalizeUpstreamSpan(*span_, upstream_headers_.get(),
                                                     upstream_trailers_.get(), stream_info_,
                                                     Tracing::EgressConfig::get());
  }

  if (per_try_timeout_ != nullptr) {
    // Allows for testing.
    per_try_timeout_->disableTimer();
  }
  if (max_stream_duration_timer_ != nullptr) {
    max_stream_duration_timer_->disableTimer();
  }
  clearRequestEncoder();

  // If desired, fire the per-try histogram when the UpstreamRequest
  // completes.
  if (record_timeout_budget_) {
    Event::Dispatcher& dispatcher = parent_.callbacks()->dispatcher();
    const MonotonicTime end_time = dispatcher.timeSource().monotonicTime();
    const std::chrono::milliseconds response_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    parent_.cluster()
        ->timeoutBudgetStats()
        ->upstream_rq_timeout_budget_per_try_percent_used_.recordValue(
            FilterUtility::percentageOfTimeout(response_time, parent_.timeout().per_try_timeout_));
  }

  stream_info_.setUpstreamTiming(upstream_timing_);
  stream_info_.onRequestComplete();
  for (const auto& upstream_log : parent_.config().upstream_logs_) {
    upstream_log->log(parent_.downstreamHeaders(), upstream_headers_.get(),
                      upstream_trailers_.get(), stream_info_);
  }

  while (downstream_data_disabled_ != 0) {
    parent_.callbacks()->onDecoderFilterBelowWriteBufferLowWatermark();
    parent_.cluster()->stats().upstream_flow_control_drained_total_.inc();
    --downstream_data_disabled_;
  }
}

void UpstreamRequest::decode100ContinueHeaders(Http::ResponseHeaderMapPtr&& headers) {
  ScopeTrackerScopeState scope(&parent_.callbacks()->scope(), parent_.callbacks()->dispatcher());

  ASSERT(100 == Http::Utility::getResponseStatus(*headers));
  parent_.onUpstream100ContinueHeaders(std::move(headers), *this);
}

void UpstreamRequest::decodeHeaders(Http::ResponseHeaderMapPtr&& headers, bool end_stream) {
  ScopeTrackerScopeState scope(&parent_.callbacks()->scope(), parent_.callbacks()->dispatcher());

  // TODO(rodaine): This is actually measuring after the headers are parsed and not the first
  // byte.
  upstream_timing_.onFirstUpstreamRxByteReceived(parent_.callbacks()->dispatcher().timeSource());
  maybeEndDecode(end_stream);

  awaiting_headers_ = false;
  if (!parent_.config().upstream_logs_.empty()) {
    upstream_headers_ = Http::createHeaderMap<Http::ResponseHeaderMapImpl>(*headers);
  }
  const uint64_t response_code = Http::Utility::getResponseStatus(*headers);
  stream_info_.response_code_ = static_cast<uint32_t>(response_code);

  if (paused_for_connect_ && response_code == 200) {
    encodeBodyAndTrailers();
    paused_for_connect_ = false;
  }

  parent_.onUpstreamHeaders(response_code, std::move(headers), *this, end_stream);
}

void UpstreamRequest::decodeData(Buffer::Instance& data, bool end_stream) {
  ScopeTrackerScopeState scope(&parent_.callbacks()->scope(), parent_.callbacks()->dispatcher());

  maybeEndDecode(end_stream);
  stream_info_.addBytesReceived(data.length());
  parent_.onUpstreamData(data, *this, end_stream);
}

void UpstreamRequest::decodeTrailers(Http::ResponseTrailerMapPtr&& trailers) {
  ScopeTrackerScopeState scope(&parent_.callbacks()->scope(), parent_.callbacks()->dispatcher());

  maybeEndDecode(true);
  if (!parent_.config().upstream_logs_.empty()) {
    upstream_trailers_ = Http::createHeaderMap<Http::ResponseTrailerMapImpl>(*trailers);
  }
  parent_.onUpstreamTrailers(std::move(trailers), *this);
}

void UpstreamRequest::decodeMetadata(Http::MetadataMapPtr&& metadata_map) {
  parent_.onUpstreamMetadata(std::move(metadata_map));
}

void UpstreamRequest::maybeEndDecode(bool end_stream) {
  if (end_stream) {
    upstream_timing_.onLastUpstreamRxByteReceived(parent_.callbacks()->dispatcher().timeSource());
    decode_complete_ = true;
  }
}

void UpstreamRequest::onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr host) {
  stream_info_.onUpstreamHostSelected(host);
  upstream_host_ = host;
  parent_.callbacks()->streamInfo().onUpstreamHostSelected(host);
  parent_.onUpstreamHostSelected(host);
}

void UpstreamRequest::encodeHeaders(bool end_stream) {
  ASSERT(!encode_complete_);
  encode_complete_ = end_stream;

  conn_pool_->newStream(this);
}

void UpstreamRequest::encodeData(Buffer::Instance& data, bool end_stream) {
  ASSERT(!encode_complete_);
  encode_complete_ = end_stream;

  if (!upstream_ || paused_for_connect_) {
    ENVOY_STREAM_LOG(trace, "buffering {} bytes", *parent_.callbacks(), data.length());
    if (!buffered_request_body_) {
      buffered_request_body_ = std::make_unique<Buffer::WatermarkBuffer>(
          [this]() -> void { this->enableDataFromDownstreamForFlowControl(); },
          [this]() -> void { this->disableDataFromDownstreamForFlowControl(); },
          []() -> void { /* TODO(adisuissa): Handle overflow watermark */ });
      buffered_request_body_->setWatermarks(parent_.callbacks()->decoderBufferLimit());
    }

    buffered_request_body_->move(data);
  } else {
    ASSERT(downstream_metadata_map_vector_.empty());

    ENVOY_STREAM_LOG(trace, "proxying {} bytes", *parent_.callbacks(), data.length());
    stream_info_.addBytesSent(data.length());
    upstream_->encodeData(data, end_stream);
    if (end_stream) {
      upstream_timing_.onLastUpstreamTxByteSent(parent_.callbacks()->dispatcher().timeSource());
    }
  }
}

void UpstreamRequest::encodeTrailers(const Http::RequestTrailerMap& trailers) {
  ASSERT(!encode_complete_);
  encode_complete_ = true;
  encode_trailers_ = true;

  if (!upstream_) {
    ENVOY_STREAM_LOG(trace, "buffering trailers", *parent_.callbacks());
  } else {
    ASSERT(downstream_metadata_map_vector_.empty());

    ENVOY_STREAM_LOG(trace, "proxying trailers", *parent_.callbacks());
    upstream_->encodeTrailers(trailers);
    upstream_timing_.onLastUpstreamTxByteSent(parent_.callbacks()->dispatcher().timeSource());
  }
}

void UpstreamRequest::encodeMetadata(Http::MetadataMapPtr&& metadata_map_ptr) {
  if (!upstream_) {
    ENVOY_STREAM_LOG(trace, "upstream_ not ready. Store metadata_map to encode later: {}",
                     *parent_.callbacks(), *metadata_map_ptr);
    downstream_metadata_map_vector_.emplace_back(std::move(metadata_map_ptr));
  } else {
    ENVOY_STREAM_LOG(trace, "Encode metadata: {}", *parent_.callbacks(), *metadata_map_ptr);
    Http::MetadataMapVector metadata_map_vector;
    metadata_map_vector.emplace_back(std::move(metadata_map_ptr));
    upstream_->encodeMetadata(metadata_map_vector);
  }
}

void UpstreamRequest::onResetStream(Http::StreamResetReason reason,
                                    absl::string_view transport_failure_reason) {
  ScopeTrackerScopeState scope(&parent_.callbacks()->scope(), parent_.callbacks()->dispatcher());

  if (span_ != nullptr) {
    // Add tags about reset.
    span_->setTag(Tracing::Tags::get().Error, Tracing::Tags::get().True);
    span_->setTag(Tracing::Tags::get().ErrorReason, Http::Utility::resetReasonToString(reason));
  }

  clearRequestEncoder();
  awaiting_headers_ = false;
  if (!calling_encode_headers_) {
    stream_info_.setResponseFlag(Filter::streamResetReasonToResponseFlag(reason));
    parent_.onUpstreamReset(reason, transport_failure_reason, *this);
  } else {
    deferred_reset_reason_ = reason;
  }
}

void UpstreamRequest::resetStream() {
  // Don't reset the stream if we're already done with it.
  if (encode_complete_ && decode_complete_) {
    return;
  }

  if (span_ != nullptr) {
    // Add tags about the cancellation.
    span_->setTag(Tracing::Tags::get().Canceled, Tracing::Tags::get().True);
  }

  if (conn_pool_->cancelAnyPendingRequest()) {
    ENVOY_STREAM_LOG(debug, "canceled pool request", *parent_.callbacks());
    ASSERT(!upstream_);
  }

  if (upstream_) {
    ENVOY_STREAM_LOG(debug, "resetting pool request", *parent_.callbacks());
    upstream_->resetStream();
    clearRequestEncoder();
  }
}

void UpstreamRequest::setupPerTryTimeout() {
  ASSERT(!per_try_timeout_);
  if (parent_.timeout().per_try_timeout_.count() > 0) {
    per_try_timeout_ =
        parent_.callbacks()->dispatcher().createTimer([this]() -> void { onPerTryTimeout(); });
    per_try_timeout_->enableTimer(parent_.timeout().per_try_timeout_);
  }
}

void UpstreamRequest::onPerTryTimeout() {
  // If we've sent anything downstream, ignore the per try timeout and let the response continue
  // up to the global timeout
  if (!parent_.downstreamResponseStarted()) {
    ENVOY_STREAM_LOG(debug, "upstream per try timeout", *parent_.callbacks());

    stream_info_.setResponseFlag(StreamInfo::ResponseFlag::UpstreamRequestTimeout);
    parent_.onPerTryTimeout(*this);
  } else {
    ENVOY_STREAM_LOG(debug,
                     "ignored upstream per try timeout due to already started downstream response",
                     *parent_.callbacks());
  }
}

void UpstreamRequest::onPoolFailure(ConnectionPool::PoolFailureReason reason,
                                    absl::string_view transport_failure_reason,
                                    Upstream::HostDescriptionConstSharedPtr host) {
  Http::StreamResetReason reset_reason = Http::StreamResetReason::ConnectionFailure;
  switch (reason) {
  case ConnectionPool::PoolFailureReason::Overflow:
    reset_reason = Http::StreamResetReason::Overflow;
    break;
  case ConnectionPool::PoolFailureReason::RemoteConnectionFailure:
    FALLTHRU;
  case ConnectionPool::PoolFailureReason::LocalConnectionFailure:
    reset_reason = Http::StreamResetReason::ConnectionFailure;
    break;
  case ConnectionPool::PoolFailureReason::Timeout:
    reset_reason = Http::StreamResetReason::LocalReset;
  }

  // Mimic an upstream reset.
  onUpstreamHostSelected(host);
  onResetStream(reset_reason, transport_failure_reason);
}

void UpstreamRequest::onPoolReady(
    std::unique_ptr<GenericUpstream>&& upstream, Upstream::HostDescriptionConstSharedPtr host,
    const Network::Address::InstanceConstSharedPtr& upstream_local_address,
    const StreamInfo::StreamInfo& info) {
  // This may be called under an existing ScopeTrackerScopeState but it will unwind correctly.
  ScopeTrackerScopeState scope(&parent_.callbacks()->scope(), parent_.callbacks()->dispatcher());
  ENVOY_STREAM_LOG(debug, "pool ready", *parent_.callbacks());
  upstream_ = std::move(upstream);

  if (parent_.requestVcluster()) {
    // The cluster increases its upstream_rq_total_ counter right before firing this onPoolReady
    // callback. Hence, the upstream request increases the virtual cluster's upstream_rq_total_ stat
    // here.
    parent_.requestVcluster()->stats().upstream_rq_total_.inc();
  }

  host->outlierDetector().putResult(Upstream::Outlier::Result::LocalOriginConnectSuccess);

  onUpstreamHostSelected(host);

  stream_info_.setUpstreamFilterState(std::make_shared<StreamInfo::FilterStateImpl>(
      info.filterState().parent()->parent(), StreamInfo::FilterState::LifeSpan::Request));
  stream_info_.setUpstreamLocalAddress(upstream_local_address);
  parent_.callbacks()->streamInfo().setUpstreamLocalAddress(upstream_local_address);

  stream_info_.setUpstreamSslConnection(info.downstreamSslConnection());
  parent_.callbacks()->streamInfo().setUpstreamSslConnection(info.downstreamSslConnection());

  if (parent_.downstreamEndStream()) {
    setupPerTryTimeout();
  } else {
    create_per_try_timeout_on_request_complete_ = true;
  }

  // Make sure the connection manager will inform the downstream watermark manager when the
  // downstream buffers are overrun. This may result in immediate watermark callbacks referencing
  // the encoder.
  parent_.callbacks()->addDownstreamWatermarkCallbacks(downstream_watermark_manager_);

  calling_encode_headers_ = true;
  auto* headers = parent_.downstreamHeaders();
  if (parent_.routeEntry()->autoHostRewrite() && !host->hostname().empty()) {
    parent_.downstreamHeaders()->setHost(host->hostname());
  }

  if (span_ != nullptr) {
    span_->injectContext(*parent_.downstreamHeaders());
  }

  upstream_timing_.onFirstUpstreamTxByteSent(parent_.callbacks()->dispatcher().timeSource());

  // Make sure that when we are forwarding CONNECT payload we do not do so until
  // the upstream has accepted the CONNECT request.
  if (conn_pool_->protocol().has_value() &&
      headers->getMethodValue() == Http::Headers::get().MethodValues.Connect) {
    paused_for_connect_ = true;
  }

  if (upstream_host_->cluster().commonHttpProtocolOptions().has_max_stream_duration()) {
    const auto max_stream_duration = std::chrono::milliseconds(DurationUtil::durationToMilliseconds(
        upstream_host_->cluster().commonHttpProtocolOptions().max_stream_duration()));
    if (max_stream_duration.count()) {
      max_stream_duration_timer_ = parent_.callbacks()->dispatcher().createTimer(
          [this]() -> void { onStreamMaxDurationReached(); });
      max_stream_duration_timer_->enableTimer(max_stream_duration);
    }
  }

  upstream_->encodeHeaders(*parent_.downstreamHeaders(), shouldSendEndStream());

  calling_encode_headers_ = false;

  if (!paused_for_connect_) {
    encodeBodyAndTrailers();
  }
}

void UpstreamRequest::encodeBodyAndTrailers() {
  // It is possible to get reset in the middle of an encodeHeaders() call. This happens for
  // example in the HTTP/2 codec if the frame cannot be encoded for some reason. This should never
  // happen but it's unclear if we have covered all cases so protect against it and test for it.
  // One specific example of a case where this happens is if we try to encode a total header size
  // that is too big in HTTP/2 (64K currently).
  if (deferred_reset_reason_) {
    onResetStream(deferred_reset_reason_.value(), absl::string_view());
  } else {
    // Encode metadata after headers and before any other frame type.
    if (!downstream_metadata_map_vector_.empty()) {
      ENVOY_STREAM_LOG(debug, "Send metadata onPoolReady. {}", *parent_.callbacks(),
                       downstream_metadata_map_vector_);
      upstream_->encodeMetadata(downstream_metadata_map_vector_);
      downstream_metadata_map_vector_.clear();
      if (shouldSendEndStream()) {
        Buffer::OwnedImpl empty_data("");
        upstream_->encodeData(empty_data, true);
      }
    }

    if (buffered_request_body_) {
      stream_info_.addBytesSent(buffered_request_body_->length());
      upstream_->encodeData(*buffered_request_body_, encode_complete_ && !encode_trailers_);
    }

    if (encode_trailers_) {
      upstream_->encodeTrailers(*parent_.downstreamTrailers());
    }

    if (encode_complete_) {
      upstream_timing_.onLastUpstreamTxByteSent(parent_.callbacks()->dispatcher().timeSource());
    }
  }
}

void UpstreamRequest::onStreamMaxDurationReached() {
  upstream_host_->cluster().stats().upstream_rq_max_duration_reached_.inc();

  // The upstream had closed then try to retry along with retry policy.
  parent_.onStreamMaxDurationReached(*this);
}

void UpstreamRequest::clearRequestEncoder() {
  // Before clearing the encoder, unsubscribe from callbacks.
  if (upstream_) {
    parent_.callbacks()->removeDownstreamWatermarkCallbacks(downstream_watermark_manager_);
  }
  upstream_.reset();
}

void UpstreamRequest::DownstreamWatermarkManager::onAboveWriteBufferHighWatermark() {
  ASSERT(parent_.upstream_);

  // There are two states we should get this callback in: 1) the watermark was
  // hit due to writes from a different filter instance over a shared
  // downstream connection, or 2) the watermark was hit due to THIS filter
  // instance writing back the "winning" upstream request. In either case we
  // can disable reads from upstream.
  ASSERT(!parent_.parent_.finalUpstreamRequest() ||
         &parent_ == parent_.parent_.finalUpstreamRequest());
  // The downstream connection is overrun. Pause reads from upstream.
  // If there are multiple calls to readDisable either the codec (H2) or the underlying
  // Network::Connection (H1) will handle reference counting.
  parent_.parent_.cluster()->stats().upstream_flow_control_paused_reading_total_.inc();
  parent_.upstream_->readDisable(true);
}

void UpstreamRequest::DownstreamWatermarkManager::onBelowWriteBufferLowWatermark() {
  ASSERT(parent_.upstream_);

  // One source of connection blockage has buffer available. Pass this on to the stream, which
  // will resume reads if this was the last remaining high watermark.
  parent_.parent_.cluster()->stats().upstream_flow_control_resumed_reading_total_.inc();
  parent_.upstream_->readDisable(false);
}

void UpstreamRequest::disableDataFromDownstreamForFlowControl() {
  // If there is only one upstream request, we can be assured that
  // disabling reads will not slow down other upstream requests. If we've
  // already seen the full downstream request (downstream_end_stream_) then
  // disabling reads is a noop.
  // This assert condition must be true because
  // parent_.upstreamRequests().size() can only be greater than 1 in the
  // case of a per-try-timeout with hedge_on_per_try_timeout enabled, and
  // the per try timeout timer is started only after downstream_end_stream_
  // is true.
  ASSERT(parent_.upstreamRequests().size() == 1 || parent_.downstreamEndStream());
  parent_.cluster()->stats().upstream_flow_control_backed_up_total_.inc();
  parent_.callbacks()->onDecoderFilterAboveWriteBufferHighWatermark();
  ++downstream_data_disabled_;
}

void UpstreamRequest::enableDataFromDownstreamForFlowControl() {
  // If there is only one upstream request, we can be assured that
  // disabling reads will not overflow any write buffers in other upstream
  // requests. If we've already seen the full downstream request
  // (downstream_end_stream_) then enabling reads is a noop.
  // This assert condition must be true because
  // parent_.upstreamRequests().size() can only be greater than 1 in the
  // case of a per-try-timeout with hedge_on_per_try_timeout enabled, and
  // the per try timeout timer is started only after downstream_end_stream_
  // is true.
  ASSERT(parent_.upstreamRequests().size() == 1 || parent_.downstreamEndStream());
  parent_.cluster()->stats().upstream_flow_control_drained_total_.inc();
  parent_.callbacks()->onDecoderFilterBelowWriteBufferLowWatermark();
  ASSERT(downstream_data_disabled_ != 0);
  if (downstream_data_disabled_ > 0) {
    --downstream_data_disabled_;
  }
}

void HttpConnPool::newStream(GenericConnectionPoolCallbacks* callbacks) {
  callbacks_ = callbacks;
  // It's possible for a reset to happen inline within the newStream() call. In this case, we
  // might get deleted inline as well. Only write the returned handle out if it is not nullptr to
  // deal with this case.
  Http::ConnectionPool::Cancellable* handle =
      conn_pool_->newStream(*callbacks->upstreamRequest(), *this);
  if (handle) {
    conn_pool_stream_handle_ = handle;
  }
}

void TcpConnPool::onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
                              Upstream::HostDescriptionConstSharedPtr host) {
  upstream_handle_ = nullptr;
  Network::Connection& latched_conn = conn_data->connection();
  auto upstream =
      std::make_unique<TcpUpstream>(callbacks_->upstreamRequest(), std::move(conn_data));
  callbacks_->onPoolReady(std::move(upstream), host, latched_conn.localAddress(),
                          latched_conn.streamInfo());
}

bool HttpConnPool::cancelAnyPendingRequest() {
  if (conn_pool_stream_handle_) {
    conn_pool_stream_handle_->cancel();
    conn_pool_stream_handle_ = nullptr;
    return true;
  }
  return false;
}

absl::optional<Http::Protocol> HttpConnPool::protocol() const { return conn_pool_->protocol(); }

void HttpConnPool::onPoolFailure(ConnectionPool::PoolFailureReason reason,
                                 absl::string_view transport_failure_reason,
                                 Upstream::HostDescriptionConstSharedPtr host) {
  callbacks_->onPoolFailure(reason, transport_failure_reason, host);
}

void HttpConnPool::onPoolReady(Http::RequestEncoder& request_encoder,
                               Upstream::HostDescriptionConstSharedPtr host,
                               const StreamInfo::StreamInfo& info) {
  conn_pool_stream_handle_ = nullptr;
  auto upstream = std::make_unique<HttpUpstream>(*callbacks_->upstreamRequest(), &request_encoder);
  callbacks_->onPoolReady(std::move(upstream), host,
                          request_encoder.getStream().connectionLocalAddress(), info);
}

TcpUpstream::TcpUpstream(UpstreamRequest* upstream_request,
                         Tcp::ConnectionPool::ConnectionDataPtr&& upstream)
    : upstream_request_(upstream_request), upstream_conn_data_(std::move(upstream)) {
  upstream_conn_data_->connection().enableHalfClose(true);
  upstream_conn_data_->addUpstreamCallbacks(*this);
}

void TcpUpstream::encodeData(Buffer::Instance& data, bool end_stream) {
  upstream_conn_data_->connection().write(data, end_stream);
}

void TcpUpstream::encodeHeaders(const Http::RequestHeaderMap&, bool end_stream) {
  // Headers should only happen once, so use this opportunity to add the proxy
  // proto header, if configured.
  ASSERT(upstream_request_->parent().routeEntry()->connectConfig().has_value());
  Buffer::OwnedImpl data;
  auto& connect_config = upstream_request_->parent().routeEntry()->connectConfig().value();
  if (connect_config.has_proxy_protocol_config()) {
    const Network::Connection& connection = *upstream_request_->parent().callbacks()->connection();
    Extensions::Common::ProxyProtocol::generateProxyProtoHeader(
        connect_config.proxy_protocol_config(), connection, data);
  }

  if (data.length() != 0 || end_stream) {
    upstream_conn_data_->connection().write(data, end_stream);
  }

  // TcpUpstream::encodeHeaders is called after the UpstreamRequest is fully initialized. Also use
  // this time to synthesize the 200 response headers downstream to complete the CONNECT handshake.
  Http::ResponseHeaderMapPtr headers{
      Http::createHeaderMap<Http::ResponseHeaderMapImpl>({{Http::Headers::get().Status, "200"}})};
  upstream_request_->decodeHeaders(std::move(headers), false);
}

void TcpUpstream::encodeTrailers(const Http::RequestTrailerMap&) {
  Buffer::OwnedImpl data;
  upstream_conn_data_->connection().write(data, true);
}

void TcpUpstream::readDisable(bool disable) {
  if (upstream_conn_data_->connection().state() != Network::Connection::State::Open) {
    return;
  }
  upstream_conn_data_->connection().readDisable(disable);
}

void TcpUpstream::resetStream() {
  upstream_request_ = nullptr;
  upstream_conn_data_->connection().close(Network::ConnectionCloseType::NoFlush);
}

void TcpUpstream::onUpstreamData(Buffer::Instance& data, bool end_stream) {
  upstream_request_->decodeData(data, end_stream);
}

void TcpUpstream::onEvent(Network::ConnectionEvent event) {
  if (event != Network::ConnectionEvent::Connected && upstream_request_) {
    upstream_request_->onResetStream(Http::StreamResetReason::ConnectionTermination, "");
  }
}

void TcpUpstream::onAboveWriteBufferHighWatermark() {
  if (upstream_request_) {
    upstream_request_->disableDataFromDownstreamForFlowControl();
  }
}

void TcpUpstream::onBelowWriteBufferLowWatermark() {
  if (upstream_request_) {
    upstream_request_->enableDataFromDownstreamForFlowControl();
  }
}

} // namespace Router
} // namespace Envoy
