#include "extensions/filters/http/ip_tagging/ip_tagging_filter.h"

#include "common/http/header_map_impl.h"
#include "common/http/headers.h"

#include "absl/strings/str_join.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace IpTagging {

IpTaggingFilterConfig::IpTaggingFilterConfig(
    const envoy::config::filter::http::ip_tagging::v2::IPTagging& config,
    const std::string& stat_prefix, Stats::Scope& scope, Runtime::Loader& runtime)
    : request_type_(requestTypeEnum(config.request_type())), scope_(scope), runtime_(runtime),
      stat_name_set_(scope.symbolTable().makeSet("IpTagging")),
      stats_prefix_(stat_name_set_->add(stat_prefix + "ip_tagging")),
      hit_(stat_name_set_->add("hit")), no_hit_(stat_name_set_->add("no_hit")),
      total_(stat_name_set_->add("total")) {

  // Once loading IP tags from a file system is supported, the restriction on the size
  // of the set should be removed and observability into what tags are loaded needs
  // to be implemented.
  // TODO(ccaraman): Remove size check once file system support is implemented.
  // Work is tracked by issue https://github.com/envoyproxy/envoy/issues/2695.
  if (config.ip_tags().empty()) {
    throw EnvoyException("HTTP IP Tagging Filter requires ip_tags to be specified.");
  }

  std::vector<std::pair<std::string, std::vector<Network::Address::CidrRange>>> tag_data;
  tag_data.reserve(config.ip_tags().size());
  for (const auto& ip_tag : config.ip_tags()) {
    std::vector<Network::Address::CidrRange> cidr_set;
    cidr_set.reserve(ip_tag.ip_list().size());
    for (const envoy::api::v2::core::CidrRange& entry : ip_tag.ip_list()) {

      // Currently, CidrRange::create doesn't guarantee that the CidrRanges are valid.
      Network::Address::CidrRange cidr_entry = Network::Address::CidrRange::create(entry);
      if (cidr_entry.isValid()) {
        cidr_set.emplace_back(std::move(cidr_entry));
      } else {
        throw EnvoyException(
            fmt::format("invalid ip/mask combo '{}/{}' (format is <ip>/<# mask bits>)",
                        entry.address_prefix(), entry.prefix_len().value()));
      }
    }
    tag_data.emplace_back(ip_tag.ip_tag_name(), cidr_set);
  }
  trie_ = std::make_unique<Network::LcTrie::LcTrie<std::string>>(tag_data);
}

void IpTaggingFilterConfig::incCounter(Stats::StatName name, absl::string_view tag) {
  Stats::SymbolTable::StoragePtr storage =
      scope_.symbolTable().join({stats_prefix_, stat_name_set_->getDynamic(tag), name});
  scope_.counterFromStatName(Stats::StatName(storage.get())).inc();
}

IpTaggingFilter::IpTaggingFilter(IpTaggingFilterConfigSharedPtr config) : config_(config) {}

IpTaggingFilter::~IpTaggingFilter() = default;

void IpTaggingFilter::onDestroy() {}

Http::FilterHeadersStatus IpTaggingFilter::decodeHeaders(Http::HeaderMap& headers, bool) {
  const bool is_internal_request = headers.EnvoyInternalRequest() &&
                                   (headers.EnvoyInternalRequest()->value() ==
                                    Http::Headers::get().EnvoyInternalRequestValues.True.c_str());

  if ((is_internal_request && config_->requestType() == FilterRequestType::EXTERNAL) ||
      (!is_internal_request && config_->requestType() == FilterRequestType::INTERNAL) ||
      !config_->runtime().snapshot().featureEnabled("ip_tagging.http_filter_enabled", 100)) {
    return Http::FilterHeadersStatus::Continue;
  }

  std::vector<std::string> tags =
      config_->trie().getData(callbacks_->streamInfo().downstreamRemoteAddress());

  if (!tags.empty()) {
    const std::string tags_join = absl::StrJoin(tags, ",");
    Http::HeaderMapImpl::appendToHeader(headers.insertEnvoyIpTags().value(), tags_join);

    // We must clear the route cache or else we can't match on x-envoy-ip-tags.
    // TODO(rgs): this should either be configurable, because it's expensive, or optimized.
    callbacks_->clearRouteCache();

    // For a large number(ex > 1000) of tags, stats cardinality will be an issue.
    // If there are use cases with a large set of tags, a way to opt into these stats
    // should be exposed and other observability options like logging tags need to be implemented.
    for (const std::string& tag : tags) {
      config_->incHit(tag);
    }
  } else {
    config_->incNoHit();
  }
  config_->incTotal();
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterDataStatus IpTaggingFilter::decodeData(Buffer::Instance&, bool) {
  return Http::FilterDataStatus::Continue;
}

Http::FilterTrailersStatus IpTaggingFilter::decodeTrailers(Http::HeaderMap&) {
  return Http::FilterTrailersStatus::Continue;
}

void IpTaggingFilter::setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
}

} // namespace IpTagging
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
