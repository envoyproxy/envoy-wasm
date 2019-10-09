#pragma once

#include "envoy/api/api.h"
#include "envoy/api/v2/core/base.pb.h"
#include "envoy/config/bootstrap/v2/bootstrap.pb.h"
#include "envoy/config/filter/network/http_connection_manager/v2/http_connection_manager.pb.h"
#include "envoy/config/grpc_mux.h"
#include "envoy/config/subscription.h"
#include "envoy/json/json_object.h"
#include "envoy/local_info/local_info.h"
#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_matcher.h"
#include "envoy/stats/tag_producer.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/assert.h"
#include "common/common/backoff_strategy.h"
#include "common/common/hash.h"
#include "common/common/hex.h"
#include "common/grpc/common.h"
#include "common/protobuf/protobuf.h"
#include "common/protobuf/utility.h"
#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Config {

/**
 * Constant Api Type Values, used by envoy::api::v2::core::ApiConfigSource.
 */
class ApiTypeValues {
public:
  const std::string UnsupportedRestLegacy{"REST_LEGACY"};
  const std::string Rest{"REST"};
  const std::string Grpc{"GRPC"};
};

/**
 * RateLimitSettings for discovery requests.
 */
struct RateLimitSettings {
  // Default Max Tokens.
  static const uint32_t DefaultMaxTokens = 100;
  // Default Fill Rate.
  static constexpr double DefaultFillRate = 10;

  uint32_t max_tokens_{DefaultMaxTokens};
  double fill_rate_{DefaultFillRate};
  bool enabled_{false};
};

using ApiType = ConstSingleton<ApiTypeValues>;

/**
 * General config API utilities.
 */
class Utility {
public:
  /**
   * Legacy APIs uses JSON and do not have an explicit version.
   * @param input the input to hash.
   * @return std::pair<std::string, uint64_t> the string is the hash converted into
   *         a hex string, pre-pended by a user friendly prefix. The uint64_t is the
   *         raw hash.
   */
  static std::pair<std::string, uint64_t> computeHashedVersion(const std::string& input) {
    uint64_t hash = HashUtil::xxHash64(input);
    return std::make_pair("hash_" + Hex::uint64ToHex(hash), hash);
  }

  /**
   * Extract refresh_delay as a std::chrono::milliseconds from
   * envoy::api::v2::core::ApiConfigSource.
   */
  static std::chrono::milliseconds
  apiConfigSourceRefreshDelay(const envoy::api::v2::core::ApiConfigSource& api_config_source);

  /**
   * Extract request_timeout as a std::chrono::milliseconds from
   * envoy::api::v2::core::ApiConfigSource. If request_timeout isn't set in the config source, a
   * default value of 1s will be returned.
   */
  static std::chrono::milliseconds
  apiConfigSourceRequestTimeout(const envoy::api::v2::core::ApiConfigSource& api_config_source);

  /**
   * Extract initial_fetch_timeout as a std::chrono::milliseconds from
   * envoy::api::v2::core::ConfigSource. If request_timeout isn't set in the config source, a
   * default value of 0s will be returned.
   */
  static std::chrono::milliseconds
  configSourceInitialFetchTimeout(const envoy::api::v2::core::ConfigSource& config_source);

  /**
   * Populate an envoy::api::v2::core::ApiConfigSource.
   * @param cluster supplies the cluster name for the ApiConfigSource.
   * @param refresh_delay_ms supplies the refresh delay for the ApiConfigSource in ms.
   * @param api_type supplies the type of subscription to use for the ApiConfigSource.
   * @param api_config_source a reference to the envoy::api::v2::core::ApiConfigSource object to
   * populate.
   */
  static void translateApiConfigSource(const std::string& cluster, uint32_t refresh_delay_ms,
                                       const std::string& api_type,
                                       envoy::api::v2::core::ApiConfigSource& api_config_source);

  /**
   * Check cluster info for API config sanity. Throws on error.
   * @param error_prefix supplies the prefix to use in error messages.
   * @param cluster_name supplies the cluster name to check.
   * @param cm supplies the cluster manager.
   */
  static void checkCluster(absl::string_view error_prefix, absl::string_view cluster_name,
                           Upstream::ClusterManager& cm);

  /**
   * Check cluster/local info for API config sanity. Throws on error.
   * @param error_prefix supplies the prefix to use in error messages.
   * @param cluster_name supplies the cluster name to check.
   * @param cm supplies the cluster manager.
   * @param local_info supplies the local info.
   */
  static void checkClusterAndLocalInfo(absl::string_view error_prefix,
                                       absl::string_view cluster_name, Upstream::ClusterManager& cm,
                                       const LocalInfo::LocalInfo& local_info);

  /**
   * Check local info for API config sanity. Throws on error.
   * @param error_prefix supplies the prefix to use in error messages.
   * @param local_info supplies the local info.
   */
  static void checkLocalInfo(absl::string_view error_prefix,
                             const LocalInfo::LocalInfo& local_info);

  /**
   * Check the existence of a path for a filesystem subscription. Throws on error.
   * @param path the path to validate.
   * @param api reference to the Api object
   */
  static void checkFilesystemSubscriptionBackingPath(const std::string& path, Api::Api& api);

  /**
   * Check the grpc_services and cluster_names for API config sanity. Throws on error.
   * @param api_config_source the config source to validate.
   * @throws EnvoyException when an API config has the wrong number of gRPC
   * services or cluster names, depending on expectations set by its API type.
   */
  static void
  checkApiConfigSourceNames(const envoy::api::v2::core::ApiConfigSource& api_config_source);

  /**
   * Check the validity of a cluster backing an api config source. Throws on error.
   * @param clusters the clusters currently loaded in the cluster manager.
   * @param api_config_source the config source to validate.
   * @throws EnvoyException when an API config doesn't have a statically defined non-EDS cluster.
   */
  static void validateClusterName(const Upstream::ClusterManager::ClusterInfoMap& clusters,
                                  const std::string& cluster_name);

  /**
   * Potentially calls Utility::validateClusterName, if a cluster name can be found.
   * @param clusters the clusters currently loaded in the cluster manager.
   * @param api_config_source the config source to validate.
   * @throws EnvoyException when an API config doesn't have a statically defined non-EDS cluster.
   */
  static void checkApiConfigSourceSubscriptionBackingCluster(
      const Upstream::ClusterManager::ClusterInfoMap& clusters,
      const envoy::api::v2::core::ApiConfigSource& api_config_source);

  /**
   * Convert a v1 RDS JSON config to v2 RDS
   * envoy::config::filter::network::http_connection_manager::v2::Rds.
   * @param json_rds source v1 RDS JSON config.
   * @param rds destination v2 RDS envoy::config::filter::network::http_connection_manager::v2::Rds.
   */
  static void
  translateRdsConfig(const Json::Object& json_rds,
                     envoy::config::filter::network::http_connection_manager::v2::Rds& rds);

  /**
   * Parses RateLimit configuration from envoy::api::v2::core::ApiConfigSource to RateLimitSettings.
   * @param api_config_source ApiConfigSource.
   * @return RateLimitSettings.
   */
  static RateLimitSettings
  parseRateLimitSettings(const envoy::api::v2::core::ApiConfigSource& api_config_source);

  /**
   * Generate a SubscriptionStats object from stats scope.
   * @param scope for stats.
   * @return SubscriptionStats for scope.
   */
  static SubscriptionStats generateStats(Stats::Scope& scope) {
    return {ALL_SUBSCRIPTION_STATS(POOL_COUNTER(scope), POOL_GAUGE(scope))};
  }

  /**
   * Get a Factory from the registry with a particular name (and templated type) with error checking
   * to ensure the name and factory are valid.
   * @param name string identifier for the particular implementation. Note: this is a proto string
   * because it is assumed that this value will be pulled directly from the configuration proto.
   */
  template <class Factory> static Factory& getAndCheckFactory(const std::string& name) {
    if (name.empty()) {
      throw EnvoyException("Provided name for static registration lookup was empty.");
    }

    Factory* factory = Registry::FactoryRegistry<Factory>::getFactory(name);

    if (factory == nullptr) {
      throw EnvoyException(
          fmt::format("Didn't find a registered implementation for name: '{}'", name));
    }

    return *factory;
  }

  /**
   * Translate a nested config into a proto message provided by the implementation factory.
   * @param enclosing_message proto that contains a field 'config'. Note: the enclosing proto is
   * provided because for statically registered implementations, a custom config is generally
   * optional, which means the conversion must be done conditionally.
   * @param validation_visitor message validation visitor instance.
   * @param factory implementation factory with the method 'createEmptyConfigProto' to produce a
   * proto to be filled with the translated configuration.
   */
  template <class ProtoMessage, class Factory>
  static ProtobufTypes::MessagePtr
  translateToFactoryConfig(const ProtoMessage& enclosing_message,
                           ProtobufMessage::ValidationVisitor& validation_visitor,
                           Factory& factory) {
    ProtobufTypes::MessagePtr config = factory.createEmptyConfigProto();

    // Fail in an obvious way if a plugin does not return a proto.
    RELEASE_ASSERT(config != nullptr, "");

    translateOpaqueConfig(enclosing_message.typed_config(), enclosing_message.config(),
                          validation_visitor, *config);

    return config;
  }

  /**
   * Create TagProducer instance. Check all tag names for conflicts to avoid
   * unexpected tag name overwriting.
   * @param bootstrap bootstrap proto.
   * @throws EnvoyException when the conflict of tag names is found.
   */
  static Stats::TagProducerPtr
  createTagProducer(const envoy::config::bootstrap::v2::Bootstrap& bootstrap);

  /**
   * Create StatsMatcher instance.
   */
  static Stats::StatsMatcherPtr
  createStatsMatcher(const envoy::config::bootstrap::v2::Bootstrap& bootstrap);

  /**
   * Obtain gRPC async client factory from a envoy::api::v2::core::ApiConfigSource.
   * @param async_client_manager gRPC async client manager.
   * @param api_config_source envoy::api::v2::core::ApiConfigSource. Must have config type GRPC.
   * @return Grpc::AsyncClientFactoryPtr gRPC async client factory.
   */
  static Grpc::AsyncClientFactoryPtr
  factoryForGrpcApiConfigSource(Grpc::AsyncClientManager& async_client_manager,
                                const envoy::api::v2::core::ApiConfigSource& api_config_source,
                                Stats::Scope& scope);

  /**
   * Translate a set of cluster's hosts into a load assignment configuration.
   * @param hosts cluster's list of hosts.
   * @return envoy::api::v2::ClusterLoadAssignment a load assignment configuration.
   */
  static envoy::api::v2::ClusterLoadAssignment
  translateClusterHosts(const Protobuf::RepeatedPtrField<envoy::api::v2::core::Address>& hosts);

  /**
   * Translate opaque config from google.protobuf.Any or google.protobuf.Struct to defined proto
   * message.
   * @param typed_config opaque config packed in google.protobuf.Any
   * @param config the deprecated google.protobuf.Struct config, empty struct if doesn't exist.
   * @param validation_visitor message validation visitor instance.
   * @param out_proto the proto message instantiated by extensions
   */
  static void translateOpaqueConfig(const ProtobufWkt::Any& typed_config,
                                    const ProtobufWkt::Struct& config,
                                    ProtobufMessage::ValidationVisitor& validation_visitor,
                                    Protobuf::Message& out_proto);

  /**
   * Return whether v1-style JSON filter config loading is allowed via 'deprecated_v1: true'.
   */
  static bool allowDeprecatedV1Config(Runtime::Loader& runtime, const Json::Object& config);

  /**
   * Verify any any filter designed to be terminal is configured to be terminal, and vice versa.
   * @param name the name of the filter.
   * @param name the type of filter.
   * @param is_terminal_filter true if the filter is designed to be terminal.
   * @param last_filter_in_current_config true if the filter is last in the configuration.
   * @throws EnvoyException if there is a mismatch between design and configuration.
   */
  static void validateTerminalFilters(const std::string& name, const char* filter_type,
                                      bool is_terminal_filter, bool last_filter_in_current_config) {
    if (is_terminal_filter && !last_filter_in_current_config) {
      throw EnvoyException(
          fmt::format("Error: {} must be the terminal {} filter.", name, filter_type));
    } else if (!is_terminal_filter && last_filter_in_current_config) {
      throw EnvoyException(
          fmt::format("Error: non-terminal filter {} is the last filter in a {} filter chain.",
                      name, filter_type));
    }
  }

  /**
   * Prepares the DNS failure refresh backoff strategy given the cluster configuration.
   * @param cluster the cluster configuration.
   * @param dns_refresh_rate_ms the default DNS refresh rate.
   * @param random the random generator.
   * @return BackOffStrategyPtr for scheduling refreshes.
   */
  static BackOffStrategyPtr prepareDnsRefreshStrategy(const envoy::api::v2::Cluster& cluster,
                                                      uint64_t dns_refresh_rate_ms,
                                                      Runtime::RandomGenerator& random);
};

} // namespace Config
} // namespace Envoy
