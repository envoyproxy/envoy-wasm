#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "envoy/config/core/v3/config_source.pb.h"
#include "envoy/config/route/v3/route_components.pb.h"
#include "envoy/config/subscription.h"
#include "envoy/http/codes.h"
#include "envoy/local_info/local_info.h"
#include "envoy/router/rds.h"
#include "envoy/router/route_config_update_receiver.h"
#include "envoy/server/filter_config.h"
#include "envoy/service/discovery/v3/discovery.pb.h"
#include "envoy/singleton/instance.h"
#include "envoy/stats/scope.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/logger.h"
#include "common/init/target_impl.h"
#include "common/protobuf/utility.h"

namespace Envoy {
namespace Router {

#define ALL_VHDS_STATS(COUNTER)                                                                    \
  COUNTER(config_reload)                                                                           \
  COUNTER(update_empty)

struct VhdsStats {
  ALL_VHDS_STATS(GENERATE_COUNTER_STRUCT)
};

class VhdsSubscription : Envoy::Config::SubscriptionCallbacks,
                         Logger::Loggable<Logger::Id::router> {
public:
  VhdsSubscription(RouteConfigUpdatePtr& config_update_info,
                   Server::Configuration::ServerFactoryContext& factory_context,
                   const std::string& stat_prefix,
                   std::unordered_set<RouteConfigProvider*>& route_config_providers,
                   const envoy::config::core::v3::ApiVersion resource_api_version =
                       envoy::config::core::v3::ApiVersion::AUTO);
  ~VhdsSubscription() override { init_target_.ready(); }

  void registerInitTargetWithInitManager(Init::Manager& m) { m.add(init_target_); }
  void updateOnDemand(const std::string& with_route_config_name_prefix);
  static std::string domainNameToAlias(const std::string& route_config_name,
                                       const std::string& domain) {
    return route_config_name + "/" + domain;
  }
  static std::string aliasToDomainName(const std::string& alias) {
    const auto pos = alias.find_last_of("/");
    return pos == std::string::npos ? alias : alias.substr(pos + 1);
  }

private:
  // Config::SubscriptionCallbacks
  void onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>&,
                      const std::string&) override {
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
  void onConfigUpdate(const Protobuf::RepeatedPtrField<envoy::service::discovery::v3::Resource>&,
                      const Protobuf::RepeatedPtrField<std::string>&, const std::string&) override;
  void onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                            const EnvoyException* e) override;
  std::string resourceName(const ProtobufWkt::Any& resource) override {
    return MessageUtil::anyConvert<envoy::config::route::v3::VirtualHost>(resource).name();
  }
  static std::string loadTypeUrl(envoy::config::core::v3::ApiVersion resource_api_version);

  RouteConfigUpdatePtr& config_update_info_;
  Stats::ScopePtr scope_;
  VhdsStats stats_;
  std::unique_ptr<Envoy::Config::Subscription> subscription_;
  Init::TargetImpl init_target_;
  std::unordered_set<RouteConfigProvider*>& route_config_providers_;
};

using VhdsSubscriptionPtr = std::unique_ptr<VhdsSubscription>;

} // namespace Router
} // namespace Envoy
