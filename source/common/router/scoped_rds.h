#pragma once

#include <string>

#include "envoy/api/v2/srds.pb.h"
#include "envoy/common/callback.h"
#include "envoy/config/filter/network/http_connection_manager/v2/http_connection_manager.pb.h"
#include "envoy/config/subscription.h"
#include "envoy/router/route_config_provider_manager.h"
#include "envoy/stats/scope.h"

#include "common/config/config_provider_impl.h"
#include "common/init/manager_impl.h"
#include "common/router/rds_impl.h"
#include "common/router/scoped_config_impl.h"

namespace Envoy {
namespace Router {

// Scoped routing configuration utilities.
namespace ScopedRoutesConfigProviderUtil {

// If enabled in the HttpConnectionManager config, returns a ConfigProvider for scoped routing
// configuration.
Envoy::Config::ConfigProviderPtr
create(const envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager&
           config,
       Server::Configuration::FactoryContext& factory_context, const std::string& stat_prefix,
       Envoy::Config::ConfigProviderManager& scoped_routes_config_provider_manager);

} // namespace ScopedRoutesConfigProviderUtil

class ScopedRoutesConfigProviderManager;

// A ConfigProvider for inline scoped routing configuration.
class InlineScopedRoutesConfigProvider : public Envoy::Config::ImmutableConfigProviderBase {
public:
  InlineScopedRoutesConfigProvider(
      ProtobufTypes::ConstMessagePtrVector&& config_protos, std::string name,
      Server::Configuration::FactoryContext& factory_context,
      ScopedRoutesConfigProviderManager& config_provider_manager,
      envoy::api::v2::core::ConfigSource rds_config_source,
      envoy::config::filter::network::http_connection_manager::v2::ScopedRoutes::ScopeKeyBuilder
          scope_key_builder);

  ~InlineScopedRoutesConfigProvider() override = default;

  const std::string& name() const { return name_; }

  // Envoy::Config::ConfigProvider
  Envoy::Config::ConfigProvider::ConfigProtoVector getConfigProtos() const override {
    Envoy::Config::ConfigProvider::ConfigProtoVector out_protos;
    std::for_each(config_protos_.begin(), config_protos_.end(),
                  [&out_protos](const std::unique_ptr<const Protobuf::Message>& message) {
                    out_protos.push_back(message.get());
                  });
    return out_protos;
  }

  std::string getConfigVersion() const override { return ""; }
  ConfigConstSharedPtr getConfig() const override { return config_; }

private:
  const std::string name_;
  ConfigConstSharedPtr config_;
  const std::vector<std::unique_ptr<const Protobuf::Message>> config_protos_;
  const envoy::api::v2::core::ConfigSource rds_config_source_;
};

/**
 * All SRDS stats. @see stats_macros.h
 */
// clang-format off
#define ALL_SCOPED_RDS_STATS(COUNTER)                                                              \
  COUNTER(config_reload)                                                                           \
  COUNTER(update_empty)

// clang-format on

struct ScopedRdsStats {
  ALL_SCOPED_RDS_STATS(GENERATE_COUNTER_STRUCT)
};

// A scoped RDS subscription to be used with the dynamic scoped RDS ConfigProvider.
class ScopedRdsConfigSubscription : public Envoy::Config::DeltaConfigSubscriptionInstance,
                                    Envoy::Config::SubscriptionCallbacks {
public:
  using ScopedRouteConfigurationMap =
      std::map<std::string, envoy::api::v2::ScopedRouteConfiguration>;

  ScopedRdsConfigSubscription(
      const envoy::config::filter::network::http_connection_manager::v2::ScopedRds& scoped_rds,
      const uint64_t manager_identifier, const std::string& name,
      const envoy::config::filter::network::http_connection_manager::v2::ScopedRoutes::
          ScopeKeyBuilder& scope_key_builder,
      Server::Configuration::FactoryContext& factory_context, const std::string& stat_prefix,
      envoy::api::v2::core::ConfigSource rds_config_source,
      RouteConfigProviderManager& route_config_provider_manager,
      ScopedRoutesConfigProviderManager& config_provider_manager);

  ~ScopedRdsConfigSubscription() override = default;

  const std::string& name() const { return name_; }

  const ScopedRouteMap& scopedRouteMap() const { return scoped_route_map_; }

private:
  // A helper class that takes care the life cycle management of a RDS route provider and the
  // update callback handle.
  struct RdsRouteConfigProviderHelper {
    RdsRouteConfigProviderHelper(
        ScopedRdsConfigSubscription& parent, std::string scope_name,
        envoy::config::filter::network::http_connection_manager::v2::Rds& rds,
        Init::Manager& init_manager);
    ~RdsRouteConfigProviderHelper() { rds_update_callback_handle_->remove(); }
    ConfigConstSharedPtr routeConfig() { return route_provider_->config(); }

    ScopedRdsConfigSubscription& parent_;
    std::string scope_name_;
    std::unique_ptr<RdsRouteConfigProviderImpl> route_provider_;
    // This handle_ is owned by the route config provider's RDS subscription, when the helper
    // destructs, the handle is deleted as well.
    Common::CallbackHandle* rds_update_callback_handle_;
  };

  // Adds or updates scopes, create a new RDS provider for each resource, if an exception is thrown
  // during updating, the exception message is collected via the exception messages vector.
  // Returns true if any scope updated, false otherwise.
  bool addOrUpdateScopes(const Protobuf::RepeatedPtrField<envoy::api::v2::Resource>& resources,
                         Init::Manager& init_manager, const std::string& version_info,
                         std::vector<std::string>& exception_msgs);
  // Removes given scopes from the managed set of scopes.
  // Returns true if any scope updated, false otherwise.
  bool removeScopes(const Protobuf::RepeatedPtrField<std::string>& scope_names,
                    const std::string& version_info);

  // Envoy::Config::DeltaConfigSubscriptionInstance
  void start() override { subscription_->start({}); }

  // Envoy::Config::SubscriptionCallbacks

  // NOTE: state-of-the-world form onConfigUpdate(resources, version_info) will throw an
  // EnvoyException on any error and essentially reject an update. While the Delta form
  // onConfigUpdate(added_resources, removed_resources, version_info) by design will partially
  // accept correct RouteConfiguration from management server.
  void onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>& resources,
                      const std::string& version_info) override;
  void onConfigUpdate(const Protobuf::RepeatedPtrField<envoy::api::v2::Resource>& added_resources,
                      const Protobuf::RepeatedPtrField<std::string>& removed_resources,
                      const std::string& version_info) override;
  void onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                            const EnvoyException*) override {
    ASSERT(Envoy::Config::ConfigUpdateFailureReason::ConnectionFailure != reason);
    DeltaConfigSubscriptionInstance::onConfigUpdateFailed();
  }
  std::string resourceName(const ProtobufWkt::Any& resource) override {
    return MessageUtil::anyConvert<envoy::api::v2::ScopedRouteConfiguration>(resource).name();
  }
  // Propagate RDS updates to ScopeConfigImpl in workers.
  void onRdsConfigUpdate(const std::string& scope_name,
                         RdsRouteConfigSubscription& rds_subscription);

  // ScopedRouteInfo by scope name.
  ScopedRouteMap scoped_route_map_;
  // RdsRouteConfigProvider by scope name.
  absl::flat_hash_map<std::string, std::unique_ptr<RdsRouteConfigProviderHelper>>
      route_provider_by_scope_;
  // A map of (hash, scope-name), used to detect the key conflict between scopes.
  absl::flat_hash_map<uint64_t, std::string> scope_name_by_hash_;
  // For creating RDS subscriptions.
  Server::Configuration::FactoryContext& factory_context_;
  const std::string name_;
  std::unique_ptr<Envoy::Config::Subscription> subscription_;
  const envoy::config::filter::network::http_connection_manager::v2::ScopedRoutes::ScopeKeyBuilder
      scope_key_builder_;
  Stats::ScopePtr scope_;
  ScopedRdsStats stats_;
  const envoy::api::v2::core::ConfigSource rds_config_source_;
  ProtobufMessage::ValidationVisitor& validation_visitor_;
  const std::string stat_prefix_;
  RouteConfigProviderManager& route_config_provider_manager_;
};

using ScopedRdsConfigSubscriptionSharedPtr = std::shared_ptr<ScopedRdsConfigSubscription>;

// A ConfigProvider for scoped RDS that dynamically fetches scoped routing configuration via a
// subscription.
class ScopedRdsConfigProvider : public Envoy::Config::MutableConfigProviderCommonBase {
public:
  ScopedRdsConfigProvider(ScopedRdsConfigSubscriptionSharedPtr&& subscription);

  ScopedRdsConfigSubscription& subscription() {
    return *static_cast<ScopedRdsConfigSubscription*>(subscription_.get());
  }
};

// A ConfigProviderManager for scoped routing configuration that creates static/inline and dynamic
// (xds) config providers.
class ScopedRoutesConfigProviderManager : public Envoy::Config::ConfigProviderManagerImplBase {
public:
  ScopedRoutesConfigProviderManager(
      Server::Admin& admin, Router::RouteConfigProviderManager& route_config_provider_manager)
      : Envoy::Config::ConfigProviderManagerImplBase(admin, "route_scopes"),
        route_config_provider_manager_(route_config_provider_manager) {}

  ~ScopedRoutesConfigProviderManager() override = default;

  // Envoy::Config::ConfigProviderManagerImplBase
  ProtobufTypes::MessagePtr dumpConfigs() const override;

  // Envoy::Config::ConfigProviderManager
  Envoy::Config::ConfigProviderPtr
  createXdsConfigProvider(const Protobuf::Message& config_source_proto,
                          Server::Configuration::FactoryContext& factory_context,
                          const std::string& stat_prefix,
                          const Envoy::Config::ConfigProviderManager::OptionalArg& optarg) override;
  Envoy::Config::ConfigProviderPtr
  createStaticConfigProvider(const Protobuf::Message&, Server::Configuration::FactoryContext&,
                             const Envoy::Config::ConfigProviderManager::OptionalArg&) override {
    ASSERT(false,
           "SRDS supports delta updates and requires the use of the createStaticConfigProvider() "
           "overload that accepts a config proto set as an argument.");
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
  Envoy::Config::ConfigProviderPtr createStaticConfigProvider(
      std::vector<std::unique_ptr<const Protobuf::Message>>&& config_protos,
      Server::Configuration::FactoryContext& factory_context,
      const Envoy::Config::ConfigProviderManager::OptionalArg& optarg) override;

  RouteConfigProviderManager& route_config_provider_manager() {
    return route_config_provider_manager_;
  }

private:
  RouteConfigProviderManager& route_config_provider_manager_;
};

// The optional argument passed to the ConfigProviderManager::create*() functions.
class ScopedRoutesConfigProviderManagerOptArg
    : public Envoy::Config::ConfigProviderManager::OptionalArg {
public:
  ScopedRoutesConfigProviderManagerOptArg(
      std::string scoped_routes_name, const envoy::api::v2::core::ConfigSource& rds_config_source,
      const envoy::config::filter::network::http_connection_manager::v2::ScopedRoutes::
          ScopeKeyBuilder& scope_key_builder)
      : scoped_routes_name_(std::move(scoped_routes_name)), rds_config_source_(rds_config_source),
        scope_key_builder_(scope_key_builder) {}

  const std::string scoped_routes_name_;
  const envoy::api::v2::core::ConfigSource& rds_config_source_;
  const envoy::config::filter::network::http_connection_manager::v2::ScopedRoutes::ScopeKeyBuilder&
      scope_key_builder_;
};

} // namespace Router
} // namespace Envoy
