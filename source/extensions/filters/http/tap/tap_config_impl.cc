#include "extensions/filters/http/tap/tap_config_impl.h"

#include "envoy/data/tap/v2alpha/http.pb.h"

#include "common/common/assert.h"
#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace TapFilter {

namespace TapCommon = Extensions::Common::Tap;

namespace {
Http::HeaderMap::Iterate fillHeaderList(const Http::HeaderEntry& header, void* context) {
  Protobuf::RepeatedPtrField<envoy::api::v2::core::HeaderValue>& header_list =
      *reinterpret_cast<Protobuf::RepeatedPtrField<envoy::api::v2::core::HeaderValue>*>(context);
  auto& new_header = *header_list.Add();
  new_header.set_key(std::string(header.key().getStringView()));
  new_header.set_value(std::string(header.value().getStringView()));
  return Http::HeaderMap::Iterate::Continue;
}
} // namespace

HttpTapConfigImpl::HttpTapConfigImpl(envoy::service::tap::v2alpha::TapConfig&& proto_config,
                                     Common::Tap::Sink* admin_streamer)
    : TapCommon::TapConfigBaseImpl(std::move(proto_config), admin_streamer) {}

HttpPerRequestTapperPtr HttpTapConfigImpl::createPerRequestTapper(uint64_t stream_id) {
  return std::make_unique<HttpPerRequestTapperImpl>(shared_from_this(), stream_id);
}

void HttpPerRequestTapperImpl::streamRequestHeaders() {
  TapCommon::TraceWrapperPtr trace = makeTraceSegment();
  request_headers_->iterate(
      fillHeaderList,
      trace->mutable_http_streamed_trace_segment()->mutable_request_headers()->mutable_headers());
  sink_handle_->submitTrace(std::move(trace));
}

void HttpPerRequestTapperImpl::onRequestHeaders(const Http::HeaderMap& headers) {
  request_headers_ = &headers;
  config_->rootMatcher().onHttpRequestHeaders(headers, statuses_);
  if (config_->streaming() && config_->rootMatcher().matchStatus(statuses_).matches_) {
    ASSERT(!started_streaming_trace_);
    started_streaming_trace_ = true;
    streamRequestHeaders();
  }
}

void HttpPerRequestTapperImpl::streamBufferedRequestBody() {
  if (buffered_streamed_request_body_ != nullptr) {
    sink_handle_->submitTrace(std::move(buffered_streamed_request_body_));
    buffered_streamed_request_body_.reset();
  }
}

void HttpPerRequestTapperImpl::onRequestBody(const Buffer::Instance& data) {
  onBody(data, buffered_streamed_request_body_, config_->maxBufferedRxBytes(),
         &envoy::data::tap::v2alpha::HttpStreamedTraceSegment::mutable_request_body_chunk,
         &envoy::data::tap::v2alpha::HttpBufferedTrace::mutable_request);
}

void HttpPerRequestTapperImpl::streamRequestTrailers() {
  if (request_trailers_ != nullptr) {
    TapCommon::TraceWrapperPtr trace = makeTraceSegment();
    request_trailers_->iterate(fillHeaderList, trace->mutable_http_streamed_trace_segment()
                                                   ->mutable_request_trailers()
                                                   ->mutable_headers());
    sink_handle_->submitTrace(std::move(trace));
  }
}

void HttpPerRequestTapperImpl::onRequestTrailers(const Http::HeaderMap& trailers) {
  request_trailers_ = &trailers;
  config_->rootMatcher().onHttpRequestTrailers(trailers, statuses_);
  if (config_->streaming() && config_->rootMatcher().matchStatus(statuses_).matches_) {
    if (!started_streaming_trace_) {
      started_streaming_trace_ = true;
      // Flush anything that we already buffered.
      streamRequestHeaders();
      streamBufferedRequestBody();
    }

    streamRequestTrailers();
  }
}

void HttpPerRequestTapperImpl::streamResponseHeaders() {
  TapCommon::TraceWrapperPtr trace = makeTraceSegment();
  response_headers_->iterate(
      fillHeaderList,
      trace->mutable_http_streamed_trace_segment()->mutable_response_headers()->mutable_headers());
  sink_handle_->submitTrace(std::move(trace));
}

void HttpPerRequestTapperImpl::onResponseHeaders(const Http::HeaderMap& headers) {
  response_headers_ = &headers;
  config_->rootMatcher().onHttpResponseHeaders(headers, statuses_);
  if (config_->streaming() && config_->rootMatcher().matchStatus(statuses_).matches_) {
    if (!started_streaming_trace_) {
      started_streaming_trace_ = true;
      // Flush anything that we already buffered.
      streamRequestHeaders();
      streamBufferedRequestBody();
      streamRequestTrailers();
    }

    streamResponseHeaders();
  }
}

void HttpPerRequestTapperImpl::streamBufferedResponseBody() {
  if (buffered_streamed_response_body_ != nullptr) {
    sink_handle_->submitTrace(std::move(buffered_streamed_response_body_));
    buffered_streamed_response_body_.reset();
  }
}

void HttpPerRequestTapperImpl::onResponseBody(const Buffer::Instance& data) {
  onBody(data, buffered_streamed_response_body_, config_->maxBufferedTxBytes(),
         &envoy::data::tap::v2alpha::HttpStreamedTraceSegment::mutable_response_body_chunk,
         &envoy::data::tap::v2alpha::HttpBufferedTrace::mutable_response);
}

void HttpPerRequestTapperImpl::onResponseTrailers(const Http::HeaderMap& trailers) {
  response_trailers_ = &trailers;
  config_->rootMatcher().onHttpResponseTrailers(trailers, statuses_);
  if (config_->streaming() && config_->rootMatcher().matchStatus(statuses_).matches_) {
    if (!started_streaming_trace_) {
      started_streaming_trace_ = true;
      // Flush anything that we already buffered.
      streamRequestHeaders();
      streamBufferedRequestBody();
      streamRequestTrailers();
      streamResponseHeaders();
      streamBufferedResponseBody();
    }

    TapCommon::TraceWrapperPtr trace = makeTraceSegment();
    trailers.iterate(fillHeaderList, trace->mutable_http_streamed_trace_segment()
                                         ->mutable_response_trailers()
                                         ->mutable_headers());
    sink_handle_->submitTrace(std::move(trace));
  }
}

bool HttpPerRequestTapperImpl::onDestroyLog() {
  if (config_->streaming() || !config_->rootMatcher().matchStatus(statuses_).matches_) {
    return config_->rootMatcher().matchStatus(statuses_).matches_;
  }

  makeBufferedFullTraceIfNeeded();
  auto& http_trace = *buffered_full_trace_->mutable_http_buffered_trace();
  if (request_headers_ != nullptr) {
    request_headers_->iterate(fillHeaderList, http_trace.mutable_request()->mutable_headers());
  }
  if (request_trailers_ != nullptr) {
    request_trailers_->iterate(fillHeaderList, http_trace.mutable_request()->mutable_trailers());
  }
  if (response_headers_ != nullptr) {
    response_headers_->iterate(fillHeaderList, http_trace.mutable_response()->mutable_headers());
  }
  if (response_trailers_ != nullptr) {
    response_trailers_->iterate(fillHeaderList, http_trace.mutable_response()->mutable_trailers());
  }

  ENVOY_LOG(debug, "submitting buffered trace sink");
  // move is safe as onDestroyLog is the last method called.
  sink_handle_->submitTrace(std::move(buffered_full_trace_));
  return true;
}

void HttpPerRequestTapperImpl::onBody(
    const Buffer::Instance& data, Extensions::Common::Tap::TraceWrapperPtr& buffered_streamed_body,
    uint32_t max_buffered_bytes, MutableBodyChunk mutable_body_chunk,
    MutableMessage mutable_message) {
  // TODO(mattklein123): Body matching.
  if (config_->streaming()) {
    const auto match_status = config_->rootMatcher().matchStatus(statuses_);
    // Without body matching, we must have already started tracing or have not yet matched.
    ASSERT(started_streaming_trace_ || !match_status.matches_);

    if (started_streaming_trace_) {
      // If we have already started streaming, flush a body segment now.
      TapCommon::TraceWrapperPtr trace = makeTraceSegment();
      TapCommon::Utility::addBufferToProtoBytes(
          *(trace->mutable_http_streamed_trace_segment()->*mutable_body_chunk)(),
          max_buffered_bytes, data, 0, data.length());
      sink_handle_->submitTrace(std::move(trace));
    } else if (match_status.might_change_status_) {
      // If we might still match, start buffering the body up to our limit.
      if (buffered_streamed_body == nullptr) {
        buffered_streamed_body = makeTraceSegment();
      }
      auto& body =
          *(buffered_streamed_body->mutable_http_streamed_trace_segment()->*mutable_body_chunk)();
      ASSERT(body.as_bytes().size() <= max_buffered_bytes);
      TapCommon::Utility::addBufferToProtoBytes(body, max_buffered_bytes - body.as_bytes().size(),
                                                data, 0, data.length());
    }
  } else {
    // If we are not streaming, buffer the body up to our limit.
    makeBufferedFullTraceIfNeeded();
    auto& body =
        *(buffered_full_trace_->mutable_http_buffered_trace()->*mutable_message)()->mutable_body();
    ASSERT(body.as_bytes().size() <= max_buffered_bytes);
    TapCommon::Utility::addBufferToProtoBytes(body, max_buffered_bytes - body.as_bytes().size(),
                                              data, 0, data.length());
  }
}

} // namespace TapFilter
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
