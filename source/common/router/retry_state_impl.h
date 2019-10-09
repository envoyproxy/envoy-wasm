#pragma once

#include <cstdint>
#include <string>

#include "envoy/event/timer.h"
#include "envoy/http/codec.h"
#include "envoy/http/header_map.h"
#include "envoy/router/router.h"
#include "envoy/runtime/runtime.h"
#include "envoy/upstream/upstream.h"

#include "common/common/backoff_strategy.h"
#include "common/http/header_utility.h"

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace Envoy {
namespace Router {

/**
 * Wraps retry state for the router.
 */
class RetryStateImpl : public RetryState {
public:
  static RetryStatePtr create(const RetryPolicy& route_policy, Http::HeaderMap& request_headers,
                              const Upstream::ClusterInfo& cluster, Runtime::Loader& runtime,
                              Runtime::RandomGenerator& random, Event::Dispatcher& dispatcher,
                              Upstream::ResourcePriority priority);
  ~RetryStateImpl() override;

  /**
   * Returns the RetryPolicy extracted from the x-envoy-retry-on header.
   * @param config is the value of the header.
   * @return std::pair<uint32_t, bool> the uint32_t is a bitset representing the
   *         valid retry policies in @param config. The bool is TRUE iff all the
   *         policies specified in @param config are valid.
   */
  static std::pair<uint32_t, bool> parseRetryOn(absl::string_view config);

  /**
   * Returns the RetryPolicy extracted from the x-envoy-retry-grpc-on header.
   * @param config is the value of the header.
   * @return std::pair<uint32_t, bool> the uint32_t is a bitset representing the
   *         valid retry policies in @param config. The bool is TRUE iff all the
   *         policies specified in @param config are valid.
   */
  static std::pair<uint32_t, bool> parseRetryGrpcOn(absl::string_view retry_grpc_on_header);

  // Router::RetryState
  bool enabled() override { return retry_on_ != 0; }
  RetryStatus shouldRetryHeaders(const Http::HeaderMap& response_headers,
                                 DoRetryCallback callback) override;
  // Returns true if the retry policy would retry the passed headers. Does not
  // take into account circuit breaking or remaining tries.
  bool wouldRetryFromHeaders(const Http::HeaderMap& response_headers) override;
  RetryStatus shouldRetryReset(const Http::StreamResetReason reset_reason,
                               DoRetryCallback callback) override;
  RetryStatus shouldHedgeRetryPerTryTimeout(DoRetryCallback callback) override;

  void onHostAttempted(Upstream::HostDescriptionConstSharedPtr host) override {
    std::for_each(retry_host_predicates_.begin(), retry_host_predicates_.end(),
                  [&host](auto predicate) { predicate->onHostAttempted(host); });
    if (retry_priority_) {
      retry_priority_->onHostAttempted(host);
    }
  }

  bool shouldSelectAnotherHost(const Upstream::Host& host) override {
    return std::any_of(
        retry_host_predicates_.begin(), retry_host_predicates_.end(),
        [&host](auto predicate) { return predicate->shouldSelectAnotherHost(host); });
  }

  const Upstream::HealthyAndDegradedLoad&
  priorityLoadForRetry(const Upstream::PrioritySet& priority_set,
                       const Upstream::HealthyAndDegradedLoad& original_priority_load) override {
    if (!retry_priority_) {
      return original_priority_load;
    }
    return retry_priority_->determinePriorityLoad(priority_set, original_priority_load);
  }

  uint32_t hostSelectionMaxAttempts() const override { return host_selection_max_attempts_; }

private:
  RetryStateImpl(const RetryPolicy& route_policy, Http::HeaderMap& request_headers,
                 const Upstream::ClusterInfo& cluster, Runtime::Loader& runtime,
                 Runtime::RandomGenerator& random, Event::Dispatcher& dispatcher,
                 Upstream::ResourcePriority priority);

  void enableBackoffTimer();
  void resetRetry();
  bool wouldRetryFromReset(const Http::StreamResetReason reset_reason);
  RetryStatus shouldRetry(bool would_retry, DoRetryCallback callback);

  const Upstream::ClusterInfo& cluster_;
  Runtime::Loader& runtime_;
  Runtime::RandomGenerator& random_;
  Event::Dispatcher& dispatcher_;
  uint32_t retry_on_{};
  uint32_t retries_remaining_{1};
  DoRetryCallback callback_;
  Event::TimerPtr retry_timer_;
  Upstream::ResourcePriority priority_;
  BackOffStrategyPtr backoff_strategy_;
  std::vector<Upstream::RetryHostPredicateSharedPtr> retry_host_predicates_;
  Upstream::RetryPrioritySharedPtr retry_priority_;
  uint32_t host_selection_max_attempts_;
  std::vector<uint32_t> retriable_status_codes_;
  std::vector<Http::HeaderMatcherSharedPtr> retriable_headers_;
  std::vector<Http::HeaderMatcherSharedPtr> retriable_request_headers_;
};

} // namespace Router
} // namespace Envoy
