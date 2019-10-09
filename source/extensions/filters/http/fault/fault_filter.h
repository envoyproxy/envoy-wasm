#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "envoy/api/v2/route/route.pb.h"
#include "envoy/config/filter/http/fault/v2/fault.pb.h"
#include "envoy/http/filter.h"
#include "envoy/runtime/runtime.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "common/buffer/watermark_buffer.h"
#include "common/common/token_bucket_impl.h"
#include "common/http/header_utility.h"
#include "common/stats/symbol_table_impl.h"

#include "extensions/filters/common/fault/fault_config.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Fault {

/**
 * All stats for the fault filter. @see stats_macros.h
 */
#define ALL_FAULT_FILTER_STATS(COUNTER, GAUGE)                                                     \
  COUNTER(aborts_injected)                                                                         \
  COUNTER(delays_injected)                                                                         \
  COUNTER(faults_overflow)                                                                         \
  COUNTER(response_rl_injected)                                                                    \
  GAUGE(active_faults, Accumulate)

/**
 * Wrapper struct for connection manager stats. @see stats_macros.h
 */
struct FaultFilterStats {
  ALL_FAULT_FILTER_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT)
};

/**
 * Configuration for fault injection.
 */
class FaultSettings : public Router::RouteSpecificFilterConfig {
public:
  FaultSettings(const envoy::config::filter::http::fault::v2::HTTPFault& fault);

  const std::vector<Http::HeaderUtility::HeaderDataPtr>& filterHeaders() const {
    return fault_filter_headers_;
  }
  envoy::type::FractionalPercent abortPercentage() const { return abort_percentage_; }
  uint64_t abortCode() const { return http_status_; }
  const Filters::Common::Fault::FaultDelayConfig* requestDelay() const {
    return request_delay_config_.get();
  }
  const std::string& upstreamCluster() const { return upstream_cluster_; }
  const absl::flat_hash_set<std::string>& downstreamNodes() const { return downstream_nodes_; }
  absl::optional<uint64_t> maxActiveFaults() const { return max_active_faults_; }
  const Filters::Common::Fault::FaultRateLimitConfig* responseRateLimit() const {
    return response_rate_limit_.get();
  }
  const std::string& abortPercentRuntime() const { return abort_percent_runtime_; }
  const std::string& delayPercentRuntime() const { return delay_percent_runtime_; }
  const std::string& abortHttpStatusRuntime() const { return abort_http_status_runtime_; }
  const std::string& delayDurationRuntime() const { return delay_duration_runtime_; }
  const std::string& maxActiveFaultsRuntime() const { return max_active_faults_runtime_; }
  const std::string& responseRateLimitPercentRuntime() const {
    return response_rate_limit_percent_runtime_;
  }

private:
  class RuntimeKeyValues {
  public:
    const std::string DelayPercentKey = "fault.http.delay.fixed_delay_percent";
    const std::string AbortPercentKey = "fault.http.abort.abort_percent";
    const std::string DelayDurationKey = "fault.http.delay.fixed_duration_ms";
    const std::string AbortHttpStatusKey = "fault.http.abort.http_status";
    const std::string MaxActiveFaultsKey = "fault.http.max_active_faults";
    const std::string ResponseRateLimitPercentKey = "fault.http.rate_limit.response_percent";
  };

  using RuntimeKeys = ConstSingleton<RuntimeKeyValues>;

  envoy::type::FractionalPercent abort_percentage_;
  uint64_t http_status_{}; // HTTP or gRPC return codes
  Filters::Common::Fault::FaultDelayConfigPtr request_delay_config_;
  std::string upstream_cluster_; // restrict faults to specific upstream cluster
  const std::vector<Http::HeaderUtility::HeaderDataPtr> fault_filter_headers_;
  absl::flat_hash_set<std::string> downstream_nodes_{}; // Inject failures for specific downstream
  absl::optional<uint64_t> max_active_faults_;
  Filters::Common::Fault::FaultRateLimitConfigPtr response_rate_limit_;
  const std::string delay_percent_runtime_;
  const std::string abort_percent_runtime_;
  const std::string delay_duration_runtime_;
  const std::string abort_http_status_runtime_;
  const std::string max_active_faults_runtime_;
  const std::string response_rate_limit_percent_runtime_;
};

/**
 * Configuration for the fault filter.
 */
class FaultFilterConfig {
public:
  FaultFilterConfig(const envoy::config::filter::http::fault::v2::HTTPFault& fault,
                    Runtime::Loader& runtime, const std::string& stats_prefix, Stats::Scope& scope,
                    TimeSource& time_source);

  Runtime::Loader& runtime() { return runtime_; }
  FaultFilterStats& stats() { return stats_; }
  Stats::Scope& scope() { return scope_; }
  const FaultSettings* settings() { return &settings_; }
  TimeSource& timeSource() { return time_source_; }

  void incDelays(absl::string_view downstream_cluster) {
    incCounter(downstream_cluster, delays_injected_);
  }

  void incAborts(absl::string_view downstream_cluster) {
    incCounter(downstream_cluster, aborts_injected_);
  }

private:
  static FaultFilterStats generateStats(const std::string& prefix, Stats::Scope& scope);
  void incCounter(absl::string_view downstream_cluster, Stats::StatName stat_name);

  const FaultSettings settings_;
  Runtime::Loader& runtime_;
  FaultFilterStats stats_;
  Stats::Scope& scope_;
  TimeSource& time_source_;
  Stats::StatNameSetPtr stat_name_set_;
  const Stats::StatName aborts_injected_;
  const Stats::StatName delays_injected_;
  const Stats::StatName stats_prefix_; // Includes ".fault".
};

using FaultFilterConfigSharedPtr = std::shared_ptr<FaultFilterConfig>;

/**
 * An HTTP stream rate limiter. Split out for ease of testing and potential code reuse elsewhere.
 */
class StreamRateLimiter : Logger::Loggable<Logger::Id::filter> {
public:
  /**
   * @param max_kbps maximum rate in KiB/s.
   * @param max_buffered_data maximum data to buffer before invoking the pause callback.
   * @param pause_data_cb callback invoked when the limiter has buffered too much data.
   * @param resume_data_cb callback invoked when the limiter has gone under the buffer limit.
   * @param write_data_cb callback invoked to write data to the stream.
   * @param continue_cb callback invoked to continue the stream. This is only used to continue
   *                    trailers that have been paused during body flush.
   * @param time_source the time source to run the token bucket with.
   * @param dispatcher the stream's dispatcher to use for creating timers.
   * @param scope the stream's scope
   */
  StreamRateLimiter(uint64_t max_kbps, uint64_t max_buffered_data,
                    std::function<void()> pause_data_cb, std::function<void()> resume_data_cb,
                    std::function<void(Buffer::Instance&, bool)> write_data_cb,
                    std::function<void()> continue_cb, TimeSource& time_source,
                    Event::Dispatcher& dispatcher, const ScopeTrackedObject& scope);

  /**
   * Called by the stream to write data. All data writes happen asynchronously, the stream should
   * be stopped after this call (all data will be drained from incoming_buffer).
   */
  void writeData(Buffer::Instance& incoming_buffer, bool end_stream);

  /**
   * Called if the stream receives trailers.
   */
  Http::FilterTrailersStatus onTrailers();

  /**
   * Like the owning filter, we must handle inline destruction, so we have a destroy() method which
   * kills any callbacks.
   */
  void destroy() { token_timer_.reset(); }
  bool destroyed() { return token_timer_ == nullptr; }

private:
  void onTokenTimer();

  // We currently divide each second into 16 segments for the token bucket. Thus, the rate limit is
  // KiB per second, divided into 16 segments, ~63ms apart. 16 is used because it divides into 1024
  // evenly.
  static constexpr uint64_t SecondDivisor = 16;

  const uint64_t bytes_per_time_slice_;
  const std::function<void(Buffer::Instance&, bool)> write_data_cb_;
  const std::function<void()> continue_cb_;
  const ScopeTrackedObject& scope_;
  TokenBucketImpl token_bucket_;
  Event::TimerPtr token_timer_;
  bool saw_data_{};
  bool saw_end_stream_{};
  bool saw_trailers_{};
  Buffer::WatermarkBuffer buffer_;
};

/**
 * A filter that is capable of faulting an entire request before dispatching it upstream.
 */
class FaultFilter : public Http::StreamFilter, Logger::Loggable<Logger::Id::filter> {
public:
  FaultFilter(FaultFilterConfigSharedPtr config);
  ~FaultFilter() override;

  // Http::StreamFilterBase
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override;
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override {
    decoder_callbacks_ = &callbacks;
  }

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override {
    return Http::FilterHeadersStatus::Continue;
  }
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap&, bool) override {
    return Http::FilterHeadersStatus::Continue;
  }
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap&) override;
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap&) override {
    return Http::FilterMetadataStatus::Continue;
  }
  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override {
    encoder_callbacks_ = &callbacks;
  }

private:
  bool faultOverflow();
  void recordAbortsInjectedStats();
  void recordDelaysInjectedStats();
  void resetTimerState();
  void postDelayInjection();
  void abortWithHTTPStatus();
  bool matchesTargetUpstreamCluster();
  bool matchesDownstreamNodes(const Http::HeaderMap& headers);
  bool isAbortEnabled();
  bool isDelayEnabled();
  absl::optional<std::chrono::milliseconds> delayDuration(const Http::HeaderMap& request_headers);
  uint64_t abortHttpStatus();
  void maybeIncActiveFaults();
  void maybeSetupResponseRateLimit(const Http::HeaderMap& request_headers);

  FaultFilterConfigSharedPtr config_;
  Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};
  Event::TimerPtr delay_timer_;
  std::string downstream_cluster_{};
  const FaultSettings* fault_settings_;
  bool fault_active_{};
  std::unique_ptr<StreamRateLimiter> response_limiter_;
  std::string downstream_cluster_delay_percent_key_{};
  std::string downstream_cluster_abort_percent_key_{};
  std::string downstream_cluster_delay_duration_key_{};
  std::string downstream_cluster_abort_http_status_key_{};
};

} // namespace Fault
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
