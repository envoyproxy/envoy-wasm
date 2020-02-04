#pragma once

#include "envoy/extensions/filters/http/aws_request_signing/v3/aws_request_signing.pb.h"
#include "envoy/http/filter.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "extensions/filters/http/common/aws/signer.h"
#include "extensions/filters/http/common/pass_through_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AwsRequestSigningFilter {

/**
 * All stats for the AWS request signing filter. @see stats_macros.h
 */
// clang-format off
#define ALL_AWS_REQUEST_SIGNING_FILTER_STATS(COUNTER)                                                           \
  COUNTER(signing_added)                                                                        \
  COUNTER(signing_failed)
// clang-format on

/**
 * Wrapper struct filter stats. @see stats_macros.h
 */
struct FilterStats {
  ALL_AWS_REQUEST_SIGNING_FILTER_STATS(GENERATE_COUNTER_STRUCT)
};

/**
 * Abstract filter configuration.
 */
class FilterConfig {
public:
  virtual ~FilterConfig() = default;

  /**
   * @return the config's signer.
   */
  virtual HttpFilters::Common::Aws::Signer& signer() PURE;

  /**
   * @return the filter stats.
   */
  virtual FilterStats& stats() PURE;
};

using FilterConfigSharedPtr = std::shared_ptr<FilterConfig>;

/**
 * Configuration for the AWS request signing filter.
 */
class FilterConfigImpl : public FilterConfig {
public:
  FilterConfigImpl(HttpFilters::Common::Aws::SignerPtr&& signer, const std::string& stats_prefix,
                   Stats::Scope& scope);

  HttpFilters::Common::Aws::Signer& signer() override;
  FilterStats& stats() override;

private:
  HttpFilters::Common::Aws::SignerPtr signer_;
  FilterStats stats_;
};

/**
 * HTTP AWS request signing auth filter.
 */
class Filter : public Http::PassThroughFilter, Logger::Loggable<Logger::Id::filter> {
public:
  Filter(const std::shared_ptr<FilterConfig>& config);

  static FilterStats generateStats(const std::string& prefix, Stats::Scope& scope);

  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override;

private:
  std::shared_ptr<FilterConfig> config_;
};

} // namespace AwsRequestSigningFilter
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
