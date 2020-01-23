#pragma once

#include <functional>

#include "envoy/api/api.h"
#include "envoy/config/cluster/v3/cluster.pb.h"
#include "envoy/config/core/v3/config_source.pb.h"
#include "envoy/config/subscription.h"
#include "envoy/event/dispatcher.h"
#include "envoy/local_info/local_info.h"
#include "envoy/service/discovery/v3/discovery.pb.h"
#include "envoy/stats/scope.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/logger.h"

namespace Envoy {
namespace Upstream {

/**
 * CDS API implementation that fetches via Subscription.
 */
class CdsApiImpl : public CdsApi,
                   Config::SubscriptionCallbacks,
                   Logger::Loggable<Logger::Id::upstream> {
public:
  static CdsApiPtr create(const envoy::config::core::v3::ConfigSource& cds_config,
                          ClusterManager& cm, Stats::Scope& scope,
                          ProtobufMessage::ValidationVisitor& validation_visitor);

  // Upstream::CdsApi
  void initialize() override { subscription_->start({}); }
  void setInitializedCb(std::function<void()> callback) override {
    initialize_callback_ = callback;
  }
  const std::string versionInfo() const override { return system_version_info_; }

private:
  // Config::SubscriptionCallbacks
  void onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>& resources,
                      const std::string& version_info) override;
  void onConfigUpdate(
      const Protobuf::RepeatedPtrField<envoy::service::discovery::v3::Resource>& added_resources,
      const Protobuf::RepeatedPtrField<std::string>& removed_resources,
      const std::string& system_version_info) override;
  void onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                            const EnvoyException* e) override;
  std::string resourceName(const ProtobufWkt::Any& resource) override {
    return MessageUtil::anyConvert<envoy::config::cluster::v3::Cluster>(resource).name();
  }
  static std::string loadTypeUrl(envoy::config::core::v3::ApiVersion resource_api_version);
  CdsApiImpl(const envoy::config::core::v3::ConfigSource& cds_config, ClusterManager& cm,
             Stats::Scope& scope, ProtobufMessage::ValidationVisitor& validation_visitor);
  void runInitializeCallbackIfAny();

  ClusterManager& cm_;
  std::unique_ptr<Config::Subscription> subscription_;
  std::string system_version_info_;
  std::function<void()> initialize_callback_;
  Stats::ScopePtr scope_;
  ProtobufMessage::ValidationVisitor& validation_visitor_;
};

} // namespace Upstream
} // namespace Envoy
