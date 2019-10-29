#include "common/router/config_impl.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "envoy/http/header_map.h"
#include "envoy/runtime/runtime.h"
#include "envoy/type/percent.pb.validate.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/upstream.h"

#include "common/common/assert.h"
#include "common/common/empty_string.h"
#include "common/common/fmt.h"
#include "common/common/hash.h"
#include "common/common/logger.h"
#include "common/common/regex.h"
#include "common/common/utility.h"
#include "common/config/metadata.h"
#include "common/config/utility.h"
#include "common/config/well_known_names.h"
#include "common/http/headers.h"
#include "common/http/utility.h"
#include "common/protobuf/protobuf.h"
#include "common/protobuf/utility.h"
#include "common/router/retry_state_impl.h"

#include "extensions/filters/http/well_known_names.h"

#include "absl/strings/match.h"

namespace Envoy {
namespace Router {
namespace {

InternalRedirectAction
convertInternalRedirectAction(const envoy::api::v2::route::RouteAction& route) {
  switch (route.internal_redirect_action()) {
  case envoy::api::v2::route::RouteAction::PASS_THROUGH_INTERNAL_REDIRECT:
    return InternalRedirectAction::PassThrough;
  case envoy::api::v2::route::RouteAction::HANDLE_INTERNAL_REDIRECT:
    return InternalRedirectAction::Handle;
  default:
    return InternalRedirectAction::PassThrough;
  }
}

} // namespace

std::string SslRedirector::newPath(const Http::HeaderMap& headers) const {
  return Http::Utility::createSslRedirectPath(headers);
}

HedgePolicyImpl::HedgePolicyImpl(const envoy::api::v2::route::HedgePolicy& hedge_policy)
    : initial_requests_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(hedge_policy, initial_requests, 1)),
      additional_request_chance_(hedge_policy.additional_request_chance()),
      hedge_on_per_try_timeout_(hedge_policy.hedge_on_per_try_timeout()) {}

HedgePolicyImpl::HedgePolicyImpl()
    : initial_requests_(1), additional_request_chance_({}), hedge_on_per_try_timeout_(false) {}

RetryPolicyImpl::RetryPolicyImpl(const envoy::api::v2::route::RetryPolicy& retry_policy,
                                 ProtobufMessage::ValidationVisitor& validation_visitor)
    : retriable_headers_(
          Http::HeaderUtility::buildHeaderMatcherVector(retry_policy.retriable_headers())),
      retriable_request_headers_(
          Http::HeaderUtility::buildHeaderMatcherVector(retry_policy.retriable_request_headers())),
      validation_visitor_(&validation_visitor) {
  per_try_timeout_ =
      std::chrono::milliseconds(PROTOBUF_GET_MS_OR_DEFAULT(retry_policy, per_try_timeout, 0));
  num_retries_ = PROTOBUF_GET_WRAPPED_OR_DEFAULT(retry_policy, num_retries, 1);
  retry_on_ = RetryStateImpl::parseRetryOn(retry_policy.retry_on()).first;
  retry_on_ |= RetryStateImpl::parseRetryGrpcOn(retry_policy.retry_on()).first;

  for (const auto& host_predicate : retry_policy.retry_host_predicate()) {
    auto& factory = Envoy::Config::Utility::getAndCheckFactory<Upstream::RetryHostPredicateFactory>(
        host_predicate.name());
    auto config = Envoy::Config::Utility::translateToFactoryConfig(host_predicate,
                                                                   validation_visitor, factory);
    retry_host_predicate_configs_.emplace_back(host_predicate.name(), std::move(config));
  }

  const auto& retry_priority = retry_policy.retry_priority();
  if (!retry_priority.name().empty()) {
    auto& factory = Envoy::Config::Utility::getAndCheckFactory<Upstream::RetryPriorityFactory>(
        retry_priority.name());
    retry_priority_config_ =
        std::make_pair(retry_priority.name(), Envoy::Config::Utility::translateToFactoryConfig(
                                                  retry_priority, validation_visitor, factory));
  }

  auto host_selection_attempts = retry_policy.host_selection_retry_max_attempts();
  if (host_selection_attempts) {
    host_selection_attempts_ = host_selection_attempts;
  }

  for (auto code : retry_policy.retriable_status_codes()) {
    retriable_status_codes_.emplace_back(code);
  }

  if (retry_policy.has_retry_back_off()) {
    base_interval_ = std::chrono::milliseconds(
        PROTOBUF_GET_MS_REQUIRED(retry_policy.retry_back_off(), base_interval));
    if ((*base_interval_).count() < 1) {
      base_interval_ = std::chrono::milliseconds(1);
    }

    max_interval_ = PROTOBUF_GET_OPTIONAL_MS(retry_policy.retry_back_off(), max_interval);
    if (max_interval_) {
      // Apply the same rounding to max interval in case both are set to sub-millisecond values.
      if ((*max_interval_).count() < 1) {
        max_interval_ = std::chrono::milliseconds(1);
      }

      if ((*max_interval_).count() < (*base_interval_).count()) {
        throw EnvoyException(
            "retry_policy.max_interval must greater than or equal to the base_interval");
      }
    }
  }
}

std::vector<Upstream::RetryHostPredicateSharedPtr> RetryPolicyImpl::retryHostPredicates() const {
  std::vector<Upstream::RetryHostPredicateSharedPtr> predicates;

  for (const auto& config : retry_host_predicate_configs_) {
    auto& factory = Envoy::Config::Utility::getAndCheckFactory<Upstream::RetryHostPredicateFactory>(
        config.first);
    predicates.emplace_back(factory.createHostPredicate(*config.second, num_retries_));
  }

  return predicates;
}

Upstream::RetryPrioritySharedPtr RetryPolicyImpl::retryPriority() const {
  if (retry_priority_config_.first.empty()) {
    return nullptr;
  }

  auto& factory = Envoy::Config::Utility::getAndCheckFactory<Upstream::RetryPriorityFactory>(
      retry_priority_config_.first);

  return factory.createRetryPriority(*retry_priority_config_.second, *validation_visitor_,
                                     num_retries_);
}

CorsPolicyImpl::CorsPolicyImpl(const envoy::api::v2::route::CorsPolicy& config,
                               Runtime::Loader& loader)
    : config_(config), loader_(loader), allow_methods_(config.allow_methods()),
      allow_headers_(config.allow_headers()), expose_headers_(config.expose_headers()),
      max_age_(config.max_age()),
      legacy_enabled_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, enabled, true)) {
  for (const auto& origin : config.allow_origin()) {
    envoy::type::matcher::StringMatcher matcher_config;
    matcher_config.set_exact(origin);
    allow_origins_.push_back(std::make_unique<Matchers::StringMatcherImpl>(matcher_config));
  }
  for (const auto& regex : config.allow_origin_regex()) {
    envoy::type::matcher::StringMatcher matcher_config;
    matcher_config.set_regex(regex);
    allow_origins_.push_back(std::make_unique<Matchers::StringMatcherImpl>(matcher_config));
  }
  for (const auto& string_match : config.allow_origin_string_match()) {
    allow_origins_.push_back(std::make_unique<Matchers::StringMatcherImpl>(string_match));
  }
  if (config.has_allow_credentials()) {
    allow_credentials_ = PROTOBUF_GET_WRAPPED_REQUIRED(config, allow_credentials);
  }
}

ShadowPolicyImpl::ShadowPolicyImpl(const envoy::api::v2::route::RouteAction& config) {
  if (!config.has_request_mirror_policy()) {
    return;
  }

  cluster_ = config.request_mirror_policy().cluster();

  if (config.request_mirror_policy().has_runtime_fraction()) {
    runtime_key_ = config.request_mirror_policy().runtime_fraction().runtime_key();
    default_value_ = config.request_mirror_policy().runtime_fraction().default_value();
  } else {
    runtime_key_ = config.request_mirror_policy().runtime_key();
    default_value_.set_numerator(0);
  }
}

DecoratorImpl::DecoratorImpl(const envoy::api::v2::route::Decorator& decorator)
    : operation_(decorator.operation()) {}

void DecoratorImpl::apply(Tracing::Span& span) const {
  if (!operation_.empty()) {
    span.setOperation(operation_);
  }
}

const std::string& DecoratorImpl::getOperation() const { return operation_; }

RouteTracingImpl::RouteTracingImpl(const envoy::api::v2::route::Tracing& tracing) {
  if (!tracing.has_client_sampling()) {
    client_sampling_.set_numerator(100);
    client_sampling_.set_denominator(envoy::type::FractionalPercent::HUNDRED);
  } else {
    client_sampling_ = tracing.client_sampling();
  }
  if (!tracing.has_random_sampling()) {
    random_sampling_.set_numerator(100);
    random_sampling_.set_denominator(envoy::type::FractionalPercent::HUNDRED);
  } else {
    random_sampling_ = tracing.random_sampling();
  }
  if (!tracing.has_overall_sampling()) {
    overall_sampling_.set_numerator(100);
    overall_sampling_.set_denominator(envoy::type::FractionalPercent::HUNDRED);
  } else {
    overall_sampling_ = tracing.overall_sampling();
  }
}

const envoy::type::FractionalPercent& RouteTracingImpl::getClientSampling() const {
  return client_sampling_;
}

const envoy::type::FractionalPercent& RouteTracingImpl::getRandomSampling() const {
  return random_sampling_;
}

const envoy::type::FractionalPercent& RouteTracingImpl::getOverallSampling() const {
  return overall_sampling_;
}

RouteEntryImplBase::RouteEntryImplBase(const VirtualHostImpl& vhost,
                                       const envoy::api::v2::route::Route& route,
                                       Server::Configuration::ServerFactoryContext& factory_context,
                                       ProtobufMessage::ValidationVisitor& validator)
    : case_sensitive_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(route.match(), case_sensitive, true)),
      prefix_rewrite_(route.route().prefix_rewrite()), host_rewrite_(route.route().host_rewrite()),
      vhost_(vhost),
      auto_host_rewrite_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(route.route(), auto_host_rewrite, false)),
      auto_host_rewrite_header_(!route.route().auto_host_rewrite_header().empty()
                                    ? absl::optional<Http::LowerCaseString>(Http::LowerCaseString(
                                          route.route().auto_host_rewrite_header()))
                                    : absl::nullopt),
      cluster_name_(route.route().cluster()), cluster_header_name_(route.route().cluster_header()),
      cluster_not_found_response_code_(ConfigUtility::parseClusterNotFoundResponseCode(
          route.route().cluster_not_found_response_code())),
      timeout_(PROTOBUF_GET_MS_OR_DEFAULT(route.route(), timeout, DEFAULT_ROUTE_TIMEOUT_MS)),
      idle_timeout_(PROTOBUF_GET_OPTIONAL_MS(route.route(), idle_timeout)),
      max_grpc_timeout_(PROTOBUF_GET_OPTIONAL_MS(route.route(), max_grpc_timeout)),
      grpc_timeout_offset_(PROTOBUF_GET_OPTIONAL_MS(route.route(), grpc_timeout_offset)),
      loader_(factory_context.runtime()), runtime_(loadRuntimeData(route.match())),
      scheme_redirect_(route.redirect().scheme_redirect()),
      host_redirect_(route.redirect().host_redirect()),
      port_redirect_(route.redirect().port_redirect()
                         ? ":" + std::to_string(route.redirect().port_redirect())
                         : ""),
      path_redirect_(route.redirect().path_redirect()),
      https_redirect_(route.redirect().https_redirect()),
      prefix_rewrite_redirect_(route.redirect().prefix_rewrite()),
      strip_query_(route.redirect().strip_query()),
      hedge_policy_(buildHedgePolicy(vhost.hedgePolicy(), route.route())),
      retry_policy_(buildRetryPolicy(vhost.retryPolicy(), route.route(), validator)),
      rate_limit_policy_(route.route().rate_limits()), shadow_policy_(route.route()),
      priority_(ConfigUtility::parsePriority(route.route().priority())),
      config_headers_(Http::HeaderUtility::buildHeaderDataVector(route.match().headers())),
      total_cluster_weight_(
          PROTOBUF_GET_WRAPPED_OR_DEFAULT(route.route().weighted_clusters(), total_weight, 100UL)),
      request_headers_parser_(HeaderParser::configure(route.request_headers_to_add(),
                                                      route.request_headers_to_remove())),
      response_headers_parser_(HeaderParser::configure(route.response_headers_to_add(),
                                                       route.response_headers_to_remove())),
      retry_shadow_buffer_limit_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(
          route, per_request_buffer_limit_bytes, vhost.retryShadowBufferLimit())),
      metadata_(route.metadata()), typed_metadata_(route.metadata()),
      match_grpc_(route.match().has_grpc()), opaque_config_(parseOpaqueConfig(route)),
      decorator_(parseDecorator(route)), route_tracing_(parseRouteTracing(route)),
      direct_response_code_(ConfigUtility::parseDirectResponseCode(route)),
      direct_response_body_(ConfigUtility::parseDirectResponseBody(route, factory_context.api())),
      per_filter_configs_(route.typed_per_filter_config(), route.per_filter_config(),
                          factory_context, validator),
      route_name_(route.name()), time_source_(factory_context.dispatcher().timeSource()),
      internal_redirect_action_(convertInternalRedirectAction(route.route())) {
  if (route.route().has_metadata_match()) {
    const auto filter_it = route.route().metadata_match().filter_metadata().find(
        Envoy::Config::MetadataFilters::get().ENVOY_LB);
    if (filter_it != route.route().metadata_match().filter_metadata().end()) {
      metadata_match_criteria_ = std::make_unique<MetadataMatchCriteriaImpl>(filter_it->second);
    }
  }

  // If this is a weighted_cluster, we create N internal route entries
  // (called WeightedClusterEntry), such that each object is a simple
  // single cluster, pointing back to the parent. Metadata criteria
  // from the weighted cluster (if any) are merged with and override
  // the criteria from the route.
  if (route.route().cluster_specifier_case() ==
      envoy::api::v2::route::RouteAction::kWeightedClusters) {
    ASSERT(total_cluster_weight_ > 0);

    uint64_t total_weight = 0UL;
    const std::string& runtime_key_prefix = route.route().weighted_clusters().runtime_key_prefix();

    for (const auto& cluster : route.route().weighted_clusters().clusters()) {
      auto cluster_entry = std::make_unique<WeightedClusterEntry>(
          this, runtime_key_prefix + "." + cluster.name(), factory_context, validator, cluster);
      weighted_clusters_.emplace_back(std::move(cluster_entry));
      total_weight += weighted_clusters_.back()->clusterWeight();
    }

    if (total_weight != total_cluster_weight_) {
      throw EnvoyException(fmt::format("Sum of weights in the weighted_cluster should add up to {}",
                                       total_cluster_weight_));
    }
  }

  for (const auto& query_parameter : route.match().query_parameters()) {
    config_query_parameters_.push_back(
        std::make_unique<ConfigUtility::QueryParameterMatcher>(query_parameter));
  }

  if (!route.route().hash_policy().empty()) {
    hash_policy_ = std::make_unique<Http::HashPolicyImpl>(route.route().hash_policy());
  }

  if (route.match().has_tls_context()) {
    tls_context_match_criteria_ =
        std::make_unique<TlsContextMatchCriteriaImpl>(route.match().tls_context());
  }

  // Only set include_vh_rate_limits_ to true if the rate limit policy for the route is empty
  // or the route set `include_vh_rate_limits` to true.
  include_vh_rate_limits_ =
      (rate_limit_policy_.empty() ||
       PROTOBUF_GET_WRAPPED_OR_DEFAULT(route.route(), include_vh_rate_limits, false));

  if (route.route().has_cors()) {
    cors_policy_ =
        std::make_unique<CorsPolicyImpl>(route.route().cors(), factory_context.runtime());
  }
  for (const auto& upgrade_config : route.route().upgrade_configs()) {
    const bool enabled = upgrade_config.has_enabled() ? upgrade_config.enabled().value() : true;
    const bool success =
        upgrade_map_
            .emplace(std::make_pair(
                Envoy::Http::LowerCaseString(upgrade_config.upgrade_type()).get(), enabled))
            .second;
    if (!success) {
      throw EnvoyException(fmt::format("Duplicate upgrade {}", upgrade_config.upgrade_type()));
    }
  }
}

bool RouteEntryImplBase::evaluateRuntimeMatch(const uint64_t random_value) const {
  return !runtime_ ? true
                   : loader_.snapshot().featureEnabled(runtime_->fractional_runtime_key_,
                                                       runtime_->fractional_runtime_default_,
                                                       random_value);
}

bool RouteEntryImplBase::evaluateTlsContextMatch(const StreamInfo::StreamInfo& stream_info) const {
  bool matches = true;

  if (!tlsContextMatchCriteria()) {
    return matches;
  }

  const TlsContextMatchCriteria& criteria = *tlsContextMatchCriteria();

  if (criteria.presented().has_value()) {
    const bool peer_presented = stream_info.downstreamSslConnection() &&
                                stream_info.downstreamSslConnection()->peerCertificatePresented();
    matches &= criteria.presented().value() == peer_presented;
  }

  return matches;
}

bool RouteEntryImplBase::matchRoute(const Http::HeaderMap& headers,
                                    const StreamInfo::StreamInfo& stream_info,
                                    uint64_t random_value) const {
  bool matches = true;

  matches &= evaluateRuntimeMatch(random_value);
  if (!matches) {
    // No need to waste further cycles calculating a route match.
    return false;
  }

  if (match_grpc_) {
    matches &= Grpc::Common::hasGrpcContentType(headers);
  }

  matches &= Http::HeaderUtility::matchHeaders(headers, config_headers_);
  if (!config_query_parameters_.empty()) {
    Http::Utility::QueryParams query_parameters =
        Http::Utility::parseQueryString(headers.Path()->value().getStringView());
    matches &= ConfigUtility::matchQueryParams(query_parameters, config_query_parameters_);
  }

  matches &= evaluateTlsContextMatch(stream_info);

  return matches;
}

const std::string& RouteEntryImplBase::clusterName() const { return cluster_name_; }

void RouteEntryImplBase::finalizeRequestHeaders(Http::HeaderMap& headers,
                                                const StreamInfo::StreamInfo& stream_info,
                                                bool insert_envoy_original_path) const {
  if (!vhost_.globalRouteConfig().mostSpecificHeaderMutationsWins()) {
    // Append user-specified request headers from most to least specific: route-level headers,
    // virtual host level headers and finally global connection manager level headers.
    request_headers_parser_->evaluateHeaders(headers, stream_info);
    vhost_.requestHeaderParser().evaluateHeaders(headers, stream_info);
    vhost_.globalRouteConfig().requestHeaderParser().evaluateHeaders(headers, stream_info);
  } else {
    // Most specific mutations take precedence.
    vhost_.globalRouteConfig().requestHeaderParser().evaluateHeaders(headers, stream_info);
    vhost_.requestHeaderParser().evaluateHeaders(headers, stream_info);
    request_headers_parser_->evaluateHeaders(headers, stream_info);
  }

  if (!host_rewrite_.empty()) {
    headers.Host()->value(host_rewrite_);
  } else if (auto_host_rewrite_header_) {
    Http::HeaderEntry* header = headers.get(*auto_host_rewrite_header_);
    if (header != nullptr) {
      absl::string_view header_value = header->value().getStringView();
      if (!header_value.empty()) {
        headers.Host()->value(header_value);
      }
    }
  }

  // Handle path rewrite
  if (!getPathRewrite().empty()) {
    rewritePathHeader(headers, insert_envoy_original_path);
  }
}

void RouteEntryImplBase::finalizeResponseHeaders(Http::HeaderMap& headers,
                                                 const StreamInfo::StreamInfo& stream_info) const {
  if (!vhost_.globalRouteConfig().mostSpecificHeaderMutationsWins()) {
    // Append user-specified request headers from most to least specific: route-level headers,
    // virtual host level headers and finally global connection manager level headers.
    response_headers_parser_->evaluateHeaders(headers, stream_info);
    vhost_.responseHeaderParser().evaluateHeaders(headers, stream_info);
    vhost_.globalRouteConfig().responseHeaderParser().evaluateHeaders(headers, stream_info);
  } else {
    // Most specific mutations take precedence.
    vhost_.globalRouteConfig().responseHeaderParser().evaluateHeaders(headers, stream_info);
    vhost_.responseHeaderParser().evaluateHeaders(headers, stream_info);
    response_headers_parser_->evaluateHeaders(headers, stream_info);
  }
}

absl::optional<RouteEntryImplBase::RuntimeData>
RouteEntryImplBase::loadRuntimeData(const envoy::api::v2::route::RouteMatch& route_match) {
  absl::optional<RuntimeData> runtime;
  RuntimeData runtime_data;

  if (route_match.has_runtime_fraction()) {
    runtime_data.fractional_runtime_default_ = route_match.runtime_fraction().default_value();
    runtime_data.fractional_runtime_key_ = route_match.runtime_fraction().runtime_key();
    return runtime_data;
  }

  return runtime;
}

void RouteEntryImplBase::finalizePathHeader(Http::HeaderMap& headers,
                                            absl::string_view matched_path,
                                            bool insert_envoy_original_path) const {
  const auto& rewrite = getPathRewrite();
  if (rewrite.empty()) {
    return;
  }

  std::string path(headers.Path()->value().getStringView());
  if (insert_envoy_original_path) {
    headers.insertEnvoyOriginalPath().value(*headers.Path());
  }
  ASSERT(case_sensitive_ ? absl::StartsWith(path, matched_path)
                         : absl::StartsWithIgnoreCase(path, matched_path));
  headers.Path()->value(path.replace(0, matched_path.size(), rewrite));
}

absl::string_view RouteEntryImplBase::processRequestHost(const Http::HeaderMap& headers,
                                                         absl::string_view new_scheme,
                                                         absl::string_view new_port) const {

  absl::string_view request_host = headers.Host()->value().getStringView();
  size_t host_end;
  if (request_host.empty()) {
    return request_host;
  }
  // Detect if IPv6 URI
  if (request_host[0] == '[') {
    host_end = request_host.rfind("]:");
    if (host_end != absl::string_view::npos) {
      host_end += 1; // advance to :
    }
  } else {
    host_end = request_host.rfind(":");
  }

  if (host_end != absl::string_view::npos) {
    absl::string_view request_port = request_host.substr(host_end);
    absl::string_view request_protocol = headers.ForwardedProto()->value().getStringView();
    bool remove_port = !new_port.empty();

    if (new_scheme != request_protocol) {
      remove_port |= (request_protocol == Http::Headers::get().SchemeValues.Https.c_str()) &&
                     request_port == ":443";
      remove_port |= (request_protocol == Http::Headers::get().SchemeValues.Http.c_str()) &&
                     request_port == ":80";
    }

    if (remove_port) {
      return request_host.substr(0, host_end);
    }
  }

  return request_host;
}

std::string RouteEntryImplBase::newPath(const Http::HeaderMap& headers) const {
  ASSERT(isDirectResponse());

  absl::string_view final_scheme;
  absl::string_view final_host;
  absl::string_view final_port;
  absl::string_view final_path;

  if (!scheme_redirect_.empty()) {
    final_scheme = scheme_redirect_.c_str();
  } else if (https_redirect_) {
    final_scheme = Http::Headers::get().SchemeValues.Https;
  } else {
    ASSERT(headers.ForwardedProto());
    final_scheme = headers.ForwardedProto()->value().getStringView();
  }

  if (!port_redirect_.empty()) {
    final_port = port_redirect_.c_str();
  } else {
    final_port = "";
  }

  if (!host_redirect_.empty()) {
    final_host = host_redirect_.c_str();
  } else {
    ASSERT(headers.Host());
    final_host = processRequestHost(headers, final_scheme, final_port);
  }

  if (!path_redirect_.empty()) {
    final_path = path_redirect_.c_str();
  } else {
    ASSERT(headers.Path());
    final_path = headers.Path()->value().getStringView();
    if (strip_query_) {
      size_t path_end = final_path.find("?");
      if (path_end != absl::string_view::npos) {
        final_path = final_path.substr(0, path_end);
      }
    }
  }

  return fmt::format("{}://{}{}{}", final_scheme, final_host, final_port, final_path);
}

std::multimap<std::string, std::string>
RouteEntryImplBase::parseOpaqueConfig(const envoy::api::v2::route::Route& route) {
  std::multimap<std::string, std::string> ret;
  if (route.has_metadata()) {
    const auto filter_metadata = route.metadata().filter_metadata().find(
        Extensions::HttpFilters::HttpFilterNames::get().Router);
    if (filter_metadata == route.metadata().filter_metadata().end()) {
      return ret;
    }
    for (const auto& it : filter_metadata->second.fields()) {
      if (it.second.kind_case() == ProtobufWkt::Value::kStringValue) {
        ret.emplace(it.first, it.second.string_value());
      }
    }
  }
  return ret;
}

HedgePolicyImpl RouteEntryImplBase::buildHedgePolicy(
    const absl::optional<envoy::api::v2::route::HedgePolicy>& vhost_hedge_policy,
    const envoy::api::v2::route::RouteAction& route_config) const {
  // Route specific policy wins, if available.
  if (route_config.has_hedge_policy()) {
    return HedgePolicyImpl(route_config.hedge_policy());
  }

  // If not, we fall back to the virtual host policy if there is one.
  if (vhost_hedge_policy) {
    return HedgePolicyImpl(vhost_hedge_policy.value());
  }

  // Otherwise, an empty policy will do.
  return HedgePolicyImpl();
}

RetryPolicyImpl RouteEntryImplBase::buildRetryPolicy(
    const absl::optional<envoy::api::v2::route::RetryPolicy>& vhost_retry_policy,
    const envoy::api::v2::route::RouteAction& route_config,
    ProtobufMessage::ValidationVisitor& validation_visitor) const {
  // Route specific policy wins, if available.
  if (route_config.has_retry_policy()) {
    return RetryPolicyImpl(route_config.retry_policy(), validation_visitor);
  }

  // If not, we fallback to the virtual host policy if there is one.
  if (vhost_retry_policy) {
    return RetryPolicyImpl(vhost_retry_policy.value(), validation_visitor);
  }

  // Otherwise, an empty policy will do.
  return RetryPolicyImpl();
}

DecoratorConstPtr RouteEntryImplBase::parseDecorator(const envoy::api::v2::route::Route& route) {
  DecoratorConstPtr ret;
  if (route.has_decorator()) {
    ret = DecoratorConstPtr(new DecoratorImpl(route.decorator()));
  }
  return ret;
}

RouteTracingConstPtr
RouteEntryImplBase::parseRouteTracing(const envoy::api::v2::route::Route& route) {
  RouteTracingConstPtr ret;
  if (route.has_tracing()) {
    ret = RouteTracingConstPtr(new RouteTracingImpl(route.tracing()));
  }
  return ret;
}

const DirectResponseEntry* RouteEntryImplBase::directResponseEntry() const {
  // A route for a request can exclusively be a route entry, a direct response entry,
  // or a redirect entry.
  if (isDirectResponse()) {
    return this;
  } else {
    return nullptr;
  }
}

const RouteEntry* RouteEntryImplBase::routeEntry() const {
  // A route for a request can exclusively be a route entry, a direct response entry,
  // or a redirect entry.
  if (isDirectResponse()) {
    return nullptr;
  } else {
    return this;
  }
}

RouteConstSharedPtr RouteEntryImplBase::clusterEntry(const Http::HeaderMap& headers,
                                                     uint64_t random_value) const {
  // Gets the route object chosen from the list of weighted clusters
  // (if there is one) or returns self.
  if (weighted_clusters_.empty()) {
    if (!cluster_name_.empty() || isDirectResponse()) {
      return shared_from_this();
    } else {
      ASSERT(!cluster_header_name_.get().empty());
      const Http::HeaderEntry* entry = headers.get(cluster_header_name_);
      std::string final_cluster_name;
      if (entry) {
        final_cluster_name = std::string(entry->value().getStringView());
      }

      // NOTE: Though we return a shared_ptr here, the current ownership model assumes that
      //       the route table sticks around. See snapped_route_config_ in
      //       ConnectionManagerImpl::ActiveStream.
      return std::make_shared<DynamicRouteEntry>(this, final_cluster_name);
    }
  }

  return WeightedClusterUtil::pickCluster(weighted_clusters_, total_cluster_weight_, random_value,
                                          true);
}

void RouteEntryImplBase::validateClusters(Upstream::ClusterManager& cm) const {
  if (isDirectResponse()) {
    return;
  }

  // Currently, we verify that the cluster exists in the CM if we have an explicit cluster or
  // weighted cluster rule. We obviously do not verify a cluster_header rule. This means that
  // trying to use all CDS clusters with a static route table will not work. In the upcoming RDS
  // change we will make it so that dynamically loaded route tables do *not* perform CM checks.
  // In the future we might decide to also have a config option that turns off checks for static
  // route tables. This would enable the all CDS with static route table case.
  if (!cluster_name_.empty()) {
    if (!cm.get(cluster_name_)) {
      throw EnvoyException(fmt::format("route: unknown cluster '{}'", cluster_name_));
    }
  } else if (!weighted_clusters_.empty()) {
    for (const WeightedClusterEntrySharedPtr& cluster : weighted_clusters_) {
      if (!cm.get(cluster->clusterName())) {
        throw EnvoyException(
            fmt::format("route: unknown weighted cluster '{}'", cluster->clusterName()));
      }
    }
  }
}

const RouteSpecificFilterConfig*
RouteEntryImplBase::perFilterConfig(const std::string& name) const {
  return per_filter_configs_.get(name);
}

RouteEntryImplBase::WeightedClusterEntry::WeightedClusterEntry(
    const RouteEntryImplBase* parent, const std::string& runtime_key,
    Server::Configuration::ServerFactoryContext& factory_context,
    ProtobufMessage::ValidationVisitor& validator,
    const envoy::api::v2::route::WeightedCluster_ClusterWeight& cluster)
    : DynamicRouteEntry(parent, cluster.name()), runtime_key_(runtime_key),
      loader_(factory_context.runtime()),
      cluster_weight_(PROTOBUF_GET_WRAPPED_REQUIRED(cluster, weight)),
      request_headers_parser_(HeaderParser::configure(cluster.request_headers_to_add(),
                                                      cluster.request_headers_to_remove())),
      response_headers_parser_(HeaderParser::configure(cluster.response_headers_to_add(),
                                                       cluster.response_headers_to_remove())),
      per_filter_configs_(cluster.typed_per_filter_config(), cluster.per_filter_config(),
                          factory_context, validator) {
  if (cluster.has_metadata_match()) {
    const auto filter_it = cluster.metadata_match().filter_metadata().find(
        Envoy::Config::MetadataFilters::get().ENVOY_LB);
    if (filter_it != cluster.metadata_match().filter_metadata().end()) {
      if (parent->metadata_match_criteria_) {
        cluster_metadata_match_criteria_ =
            parent->metadata_match_criteria_->mergeMatchCriteria(filter_it->second);
      } else {
        cluster_metadata_match_criteria_ =
            std::make_unique<MetadataMatchCriteriaImpl>(filter_it->second);
      }
    }
  }
}

const RouteSpecificFilterConfig*
RouteEntryImplBase::WeightedClusterEntry::perFilterConfig(const std::string& name) const {
  const auto cfg = per_filter_configs_.get(name);
  return cfg != nullptr ? cfg : DynamicRouteEntry::perFilterConfig(name);
}

PrefixRouteEntryImpl::PrefixRouteEntryImpl(
    const VirtualHostImpl& vhost, const envoy::api::v2::route::Route& route,
    Server::Configuration::ServerFactoryContext& factory_context,
    ProtobufMessage::ValidationVisitor& validator)
    : RouteEntryImplBase(vhost, route, factory_context, validator),
      prefix_(route.match().prefix()) {}

void PrefixRouteEntryImpl::rewritePathHeader(Http::HeaderMap& headers,
                                             bool insert_envoy_original_path) const {
  finalizePathHeader(headers, prefix_, insert_envoy_original_path);
}

RouteConstSharedPtr PrefixRouteEntryImpl::matches(const Http::HeaderMap& headers,
                                                  const StreamInfo::StreamInfo& stream_info,
                                                  uint64_t random_value) const {
  if (RouteEntryImplBase::matchRoute(headers, stream_info, random_value) &&
      (case_sensitive_
           ? absl::StartsWith(headers.Path()->value().getStringView(), prefix_)
           : absl::StartsWithIgnoreCase(headers.Path()->value().getStringView(), prefix_))) {
    return clusterEntry(headers, random_value);
  }
  return nullptr;
}

PathRouteEntryImpl::PathRouteEntryImpl(const VirtualHostImpl& vhost,
                                       const envoy::api::v2::route::Route& route,
                                       Server::Configuration::ServerFactoryContext& factory_context,
                                       ProtobufMessage::ValidationVisitor& validator)
    : RouteEntryImplBase(vhost, route, factory_context, validator), path_(route.match().path()) {}

void PathRouteEntryImpl::rewritePathHeader(Http::HeaderMap& headers,
                                           bool insert_envoy_original_path) const {
  finalizePathHeader(headers, path_, insert_envoy_original_path);
}

RouteConstSharedPtr PathRouteEntryImpl::matches(const Http::HeaderMap& headers,
                                                const StreamInfo::StreamInfo& stream_info,
                                                uint64_t random_value) const {
  if (RouteEntryImplBase::matchRoute(headers, stream_info, random_value)) {
    const Http::HeaderString& path = headers.Path()->value();
    absl::string_view query_string = Http::Utility::findQueryStringStart(path);
    size_t compare_length = path.size();
    if (query_string.length() > 0) {
      compare_length = compare_length - query_string.length();
    }

    if (compare_length != path_.size()) {
      return nullptr;
    }

    const absl::string_view path_section = path.getStringView().substr(0, compare_length);
    if (case_sensitive_) {
      if (absl::string_view(path_) == path_section) {
        return clusterEntry(headers, random_value);
      }
    } else {
      if (absl::EqualsIgnoreCase(path_, path_section)) {
        return clusterEntry(headers, random_value);
      }
    }
  }

  return nullptr;
}

RegexRouteEntryImpl::RegexRouteEntryImpl(
    const VirtualHostImpl& vhost, const envoy::api::v2::route::Route& route,
    Server::Configuration::ServerFactoryContext& factory_context,
    ProtobufMessage::ValidationVisitor& validator)
    : RouteEntryImplBase(vhost, route, factory_context, validator) {
  if (route.match().path_specifier_case() == envoy::api::v2::route::RouteMatch::kRegex) {
    regex_ = Regex::Utility::parseStdRegexAsCompiledMatcher(route.match().regex());
    regex_str_ = route.match().regex();
  } else {
    ASSERT(route.match().path_specifier_case() == envoy::api::v2::route::RouteMatch::kSafeRegex);
    regex_ = Regex::Utility::parseRegex(route.match().safe_regex());
    regex_str_ = route.match().safe_regex().regex();
  }
}

absl::string_view RegexRouteEntryImpl::pathOnly(const Http::HeaderMap& headers) const {
  const Http::HeaderString& path = headers.Path()->value();
  const absl::string_view query_string = Http::Utility::findQueryStringStart(path);
  const size_t path_string_length = path.size() - query_string.length();
  return path.getStringView().substr(0, path_string_length);
}

void RegexRouteEntryImpl::rewritePathHeader(Http::HeaderMap& headers,
                                            bool insert_envoy_original_path) const {
  // TODO(yuval-k): This ASSERT can happen if the path was changed by a filter without clearing the
  // route cache. We should consider if ASSERT-ing is the desired behavior in this case.
  ASSERT(regex_->match(pathOnly(headers)));
  finalizePathHeader(headers, pathOnly(headers), insert_envoy_original_path);
}

RouteConstSharedPtr RegexRouteEntryImpl::matches(const Http::HeaderMap& headers,
                                                 const StreamInfo::StreamInfo& stream_info,
                                                 uint64_t random_value) const {
  if (RouteEntryImplBase::matchRoute(headers, stream_info, random_value)) {
    if (regex_->match(pathOnly(headers))) {
      return clusterEntry(headers, random_value);
    }
  }
  return nullptr;
}

VirtualHostImpl::VirtualHostImpl(const envoy::api::v2::route::VirtualHost& virtual_host,
                                 const ConfigImpl& global_route_config,
                                 Server::Configuration::ServerFactoryContext& factory_context,
                                 ProtobufMessage::ValidationVisitor& validator,
                                 bool validate_clusters)
    : stat_name_pool_(factory_context.scope().symbolTable()),
      stat_name_(stat_name_pool_.add(virtual_host.name())),
      rate_limit_policy_(virtual_host.rate_limits()), global_route_config_(global_route_config),
      request_headers_parser_(HeaderParser::configure(virtual_host.request_headers_to_add(),
                                                      virtual_host.request_headers_to_remove())),
      response_headers_parser_(HeaderParser::configure(virtual_host.response_headers_to_add(),
                                                       virtual_host.response_headers_to_remove())),
      per_filter_configs_(virtual_host.typed_per_filter_config(), virtual_host.per_filter_config(),
                          factory_context, validator),
      retry_shadow_buffer_limit_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(
          virtual_host, per_request_buffer_limit_bytes, std::numeric_limits<uint32_t>::max())),
      include_attempt_count_(virtual_host.include_request_attempt_count()),
      virtual_cluster_catch_all_(stat_name_pool_) {

  switch (virtual_host.require_tls()) {
  case envoy::api::v2::route::VirtualHost::NONE:
    ssl_requirements_ = SslRequirements::NONE;
    break;
  case envoy::api::v2::route::VirtualHost::EXTERNAL_ONLY:
    ssl_requirements_ = SslRequirements::EXTERNAL_ONLY;
    break;
  case envoy::api::v2::route::VirtualHost::ALL:
    ssl_requirements_ = SslRequirements::ALL;
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  // Retry and Hedge policies must be set before routes, since they may use them.
  if (virtual_host.has_retry_policy()) {
    retry_policy_ = virtual_host.retry_policy();
  }
  if (virtual_host.has_hedge_policy()) {
    hedge_policy_ = virtual_host.hedge_policy();
  }

  for (const auto& route : virtual_host.routes()) {
    switch (route.match().path_specifier_case()) {
    case envoy::api::v2::route::RouteMatch::kPrefix: {
      routes_.emplace_back(new PrefixRouteEntryImpl(*this, route, factory_context, validator));
      break;
    }
    case envoy::api::v2::route::RouteMatch::kPath: {
      routes_.emplace_back(new PathRouteEntryImpl(*this, route, factory_context, validator));
      break;
    }
    case envoy::api::v2::route::RouteMatch::kRegex:
    case envoy::api::v2::route::RouteMatch::kSafeRegex: {
      routes_.emplace_back(new RegexRouteEntryImpl(*this, route, factory_context, validator));
      break;
    }
    case envoy::api::v2::route::RouteMatch::PATH_SPECIFIER_NOT_SET:
      NOT_REACHED_GCOVR_EXCL_LINE;
    }

    if (validate_clusters) {
      routes_.back()->validateClusters(factory_context.clusterManager());
      if (!routes_.back()->shadowPolicy().cluster().empty()) {
        if (!factory_context.clusterManager().get(routes_.back()->shadowPolicy().cluster())) {
          throw EnvoyException(fmt::format("route: unknown shadow cluster '{}'",
                                           routes_.back()->shadowPolicy().cluster()));
        }
      }
    }
  }

  for (const auto& virtual_cluster : virtual_host.virtual_clusters()) {
    virtual_clusters_.push_back(VirtualClusterEntry(virtual_cluster, stat_name_pool_));
  }

  if (virtual_host.has_cors()) {
    cors_policy_ = std::make_unique<CorsPolicyImpl>(virtual_host.cors(), factory_context.runtime());
  }
}

VirtualHostImpl::VirtualClusterEntry::VirtualClusterEntry(
    const envoy::api::v2::route::VirtualCluster& virtual_cluster, Stats::StatNamePool& pool)
    : stat_name_(pool.add(virtual_cluster.name())) {
  if (virtual_cluster.pattern().empty() == virtual_cluster.headers().empty()) {
    throw EnvoyException("virtual clusters must define either 'pattern' or 'headers'");
  }

  if (!virtual_cluster.pattern().empty()) {
    envoy::api::v2::route::HeaderMatcher matcher_config;
    matcher_config.set_name(Http::Headers::get().Path.get());
    matcher_config.set_regex_match(virtual_cluster.pattern());
    headers_.push_back(std::make_unique<Http::HeaderUtility::HeaderData>(matcher_config));
  } else {
    ASSERT(!virtual_cluster.headers().empty());
    headers_ = Http::HeaderUtility::buildHeaderDataVector(virtual_cluster.headers());
  }

  if (virtual_cluster.method() != envoy::api::v2::core::RequestMethod::METHOD_UNSPECIFIED) {
    envoy::api::v2::route::HeaderMatcher matcher_config;
    matcher_config.set_name(Http::Headers::get().Method.get());
    matcher_config.set_exact_match(
        envoy::api::v2::core::RequestMethod_Name(virtual_cluster.method()));
    headers_.push_back(std::make_unique<Http::HeaderUtility::HeaderData>(matcher_config));
  }
}

const Config& VirtualHostImpl::routeConfig() const { return global_route_config_; }

const RouteSpecificFilterConfig* VirtualHostImpl::perFilterConfig(const std::string& name) const {
  return per_filter_configs_.get(name);
}

const VirtualHostImpl* RouteMatcher::findWildcardVirtualHost(
    const std::string& host, const RouteMatcher::WildcardVirtualHosts& wildcard_virtual_hosts,
    RouteMatcher::SubstringFunction substring_function) const {
  // We do a longest wildcard match against the host that's passed in
  // (e.g. foo-bar.baz.com should match *-bar.baz.com before matching *.baz.com for suffix
  // wildcards). This is done by scanning the length => wildcards map looking for every wildcard
  // whose size is < length.
  for (const auto& iter : wildcard_virtual_hosts) {
    const uint32_t wildcard_length = iter.first;
    const auto& wildcard_map = iter.second;
    // >= because *.foo.com shouldn't match .foo.com.
    if (wildcard_length >= host.size()) {
      continue;
    }
    const auto& match = wildcard_map.find(substring_function(host, wildcard_length));
    if (match != wildcard_map.end()) {
      return match->second.get();
    }
  }
  return nullptr;
}

RouteMatcher::RouteMatcher(const envoy::api::v2::RouteConfiguration& route_config,
                           const ConfigImpl& global_route_config,
                           Server::Configuration::ServerFactoryContext& factory_context,
                           ProtobufMessage::ValidationVisitor& validator, bool validate_clusters) {
  for (const auto& virtual_host_config : route_config.virtual_hosts()) {
    VirtualHostSharedPtr virtual_host(new VirtualHostImpl(
        virtual_host_config, global_route_config, factory_context, validator, validate_clusters));
    for (const std::string& domain_name : virtual_host_config.domains()) {
      const std::string domain = Http::LowerCaseString(domain_name).get();
      bool duplicate_found = false;
      if ("*" == domain) {
        if (default_virtual_host_) {
          throw EnvoyException(fmt::format("Only a single wildcard domain is permitted"));
        }
        default_virtual_host_ = virtual_host;
      } else if (!domain.empty() && '*' == domain[0]) {
        duplicate_found = !wildcard_virtual_host_suffixes_[domain.size() - 1]
                               .emplace(domain.substr(1), virtual_host)
                               .second;
      } else if (!domain.empty() && '*' == domain[domain.size() - 1]) {
        duplicate_found = !wildcard_virtual_host_prefixes_[domain.size() - 1]
                               .emplace(domain.substr(0, domain.size() - 1), virtual_host)
                               .second;
      } else {
        duplicate_found = !virtual_hosts_.emplace(domain, virtual_host).second;
      }
      if (duplicate_found) {
        throw EnvoyException(fmt::format(
            "Only unique values for domains are permitted. Duplicate entry of domain {}", domain));
      }
    }
  }
}

RouteConstSharedPtr VirtualHostImpl::getRouteFromEntries(const Http::HeaderMap& headers,
                                                         const StreamInfo::StreamInfo& stream_info,
                                                         uint64_t random_value) const {
  // No x-forwarded-proto header. This normally only happens when ActiveStream::decodeHeaders
  // bails early (as it rejects a request), so there is no routing is going to happen anyway.
  const auto* forwarded_proto_header = headers.ForwardedProto();
  if (forwarded_proto_header == nullptr) {
    return nullptr;
  }

  // First check for ssl redirect.
  if (ssl_requirements_ == SslRequirements::ALL && forwarded_proto_header->value() != "https") {
    return SSL_REDIRECT_ROUTE;
  } else if (ssl_requirements_ == SslRequirements::EXTERNAL_ONLY &&
             forwarded_proto_header->value() != "https" &&
             !Http::HeaderUtility::isEnvoyInternalRequest(headers)) {
    return SSL_REDIRECT_ROUTE;
  }

  // Check for a route that matches the request.
  for (const RouteEntryImplBaseConstSharedPtr& route : routes_) {
    RouteConstSharedPtr route_entry = route->matches(headers, stream_info, random_value);
    if (nullptr != route_entry) {
      return route_entry;
    }
  }

  return nullptr;
}

const VirtualHostImpl* RouteMatcher::findVirtualHost(const Http::HeaderMap& headers) const {
  // Fast path the case where we only have a default virtual host.
  if (virtual_hosts_.empty() && wildcard_virtual_host_suffixes_.empty() &&
      wildcard_virtual_host_prefixes_.empty()) {
    return default_virtual_host_.get();
  }

  // TODO (@rshriram) Match Origin header in WebSocket
  // request with VHost, using wildcard match
  const std::string host =
      Http::LowerCaseString(std::string(headers.Host()->value().getStringView())).get();
  const auto& iter = virtual_hosts_.find(host);
  if (iter != virtual_hosts_.end()) {
    return iter->second.get();
  }
  if (!wildcard_virtual_host_suffixes_.empty()) {
    const VirtualHostImpl* vhost = findWildcardVirtualHost(
        host, wildcard_virtual_host_suffixes_,
        [](const std::string& h, int l) -> std::string { return h.substr(h.size() - l); });
    if (vhost != nullptr) {
      return vhost;
    }
  }
  if (!wildcard_virtual_host_prefixes_.empty()) {
    const VirtualHostImpl* vhost = findWildcardVirtualHost(
        host, wildcard_virtual_host_prefixes_,
        [](const std::string& h, int l) -> std::string { return h.substr(0, l); });
    if (vhost != nullptr) {
      return vhost;
    }
  }
  return default_virtual_host_.get();
}

RouteConstSharedPtr RouteMatcher::route(const Http::HeaderMap& headers,
                                        const StreamInfo::StreamInfo& stream_info,
                                        uint64_t random_value) const {
  const VirtualHostImpl* virtual_host = findVirtualHost(headers);
  if (virtual_host) {
    return virtual_host->getRouteFromEntries(headers, stream_info, random_value);
  } else {
    return nullptr;
  }
}

const SslRedirector SslRedirectRoute::SSL_REDIRECTOR;
const std::shared_ptr<const SslRedirectRoute> VirtualHostImpl::SSL_REDIRECT_ROUTE{
    new SslRedirectRoute()};

const VirtualCluster*
VirtualHostImpl::virtualClusterFromEntries(const Http::HeaderMap& headers) const {
  for (const VirtualClusterEntry& entry : virtual_clusters_) {
    if (Http::HeaderUtility::matchHeaders(headers, entry.headers_)) {
      return &entry;
    }
  }

  if (!virtual_clusters_.empty()) {
    return &virtual_cluster_catch_all_;
  }

  return nullptr;
}

ConfigImpl::ConfigImpl(const envoy::api::v2::RouteConfiguration& config,
                       Server::Configuration::ServerFactoryContext& factory_context,
                       ProtobufMessage::ValidationVisitor& validator,
                       bool validate_clusters_default)
    : name_(config.name()), symbol_table_(factory_context.scope().symbolTable()),
      uses_vhds_(config.has_vhds()),
      most_specific_header_mutations_wins_(config.most_specific_header_mutations_wins()) {
  route_matcher_ = std::make_unique<RouteMatcher>(
      config, *this, factory_context, validator,
      PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, validate_clusters, validate_clusters_default));

  for (const std::string& header : config.internal_only_headers()) {
    internal_only_headers_.push_back(Http::LowerCaseString(header));
  }

  request_headers_parser_ =
      HeaderParser::configure(config.request_headers_to_add(), config.request_headers_to_remove());
  response_headers_parser_ = HeaderParser::configure(config.response_headers_to_add(),
                                                     config.response_headers_to_remove());
}

namespace {

RouteSpecificFilterConfigConstSharedPtr
createRouteSpecificFilterConfig(const std::string& name, const ProtobufWkt::Any& typed_config,
                                const ProtobufWkt::Struct& config,
                                Server::Configuration::ServerFactoryContext& factory_context,
                                ProtobufMessage::ValidationVisitor& validator) {
  auto& factory = Envoy::Config::Utility::getAndCheckFactory<
      Server::Configuration::NamedHttpFilterConfigFactory>(name);
  ProtobufTypes::MessagePtr proto_config = factory.createEmptyRouteConfigProto();
  Envoy::Config::Utility::translateOpaqueConfig(typed_config, config, validator, *proto_config);
  return factory.createRouteSpecificFilterConfig(*proto_config, factory_context, validator);
}

} // namespace

PerFilterConfigs::PerFilterConfigs(
    const Protobuf::Map<std::string, ProtobufWkt::Any>& typed_configs,
    const Protobuf::Map<std::string, ProtobufWkt::Struct>& configs,
    Server::Configuration::ServerFactoryContext& factory_context,
    ProtobufMessage::ValidationVisitor& validator) {
  if (!typed_configs.empty() && !configs.empty()) {
    throw EnvoyException("Only one of typed_configs or configs can be specified");
  }

  for (const auto& it : typed_configs) {
    auto object = createRouteSpecificFilterConfig(
        it.first, it.second, ProtobufWkt::Struct::default_instance(), factory_context, validator);
    if (object != nullptr) {
      configs_[it.first] = std::move(object);
    }
  }

  for (const auto& it : configs) {
    auto object = createRouteSpecificFilterConfig(it.first, ProtobufWkt::Any::default_instance(),
                                                  it.second, factory_context, validator);
    if (object != nullptr) {
      configs_[it.first] = std::move(object);
    }
  }
}

const RouteSpecificFilterConfig* PerFilterConfigs::get(const std::string& name) const {
  auto it = configs_.find(name);
  return it == configs_.end() ? nullptr : it->second.get();
}

} // namespace Router
} // namespace Envoy
