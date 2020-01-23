#include "common/router/route_config_update_receiver_impl.h"

#include <string>

#include "envoy/config/route/v3/route.pb.h"
#include "envoy/config/route/v3/route_components.pb.h"
#include "envoy/config/route/v3/route_components.pb.validate.h"
#include "envoy/service/discovery/v3/discovery.pb.h"

#include "common/common/assert.h"
#include "common/common/fmt.h"
#include "common/protobuf/utility.h"
#include "common/router/config_impl.h"

namespace Envoy {
namespace Router {

bool RouteConfigUpdateReceiverImpl::onRdsUpdate(
    const envoy::config::route::v3::RouteConfiguration& rc, const std::string& version_info) {
  const uint64_t new_hash = MessageUtil::hash(rc);
  if (new_hash == last_config_hash_) {
    return false;
  }
  route_config_proto_ = rc;
  last_config_hash_ = new_hash;
  const uint64_t new_vhds_config_hash = rc.has_vhds() ? MessageUtil::hash(rc.vhds()) : 0ul;
  vhds_configuration_changed_ = new_vhds_config_hash != last_vhds_config_hash_;
  last_vhds_config_hash_ = new_vhds_config_hash;
  initializeRdsVhosts(route_config_proto_);
  onUpdateCommon(route_config_proto_, version_info);
  return true;
}

void RouteConfigUpdateReceiverImpl::onUpdateCommon(
    const envoy::config::route::v3::RouteConfiguration& rc, const std::string& version_info) {
  last_config_version_ = version_info;
  last_updated_ = time_source_.systemTime();
  rebuildRouteConfig(rds_virtual_hosts_, vhds_virtual_hosts_, route_config_proto_);
  config_info_.emplace(RouteConfigProvider::ConfigInfo{rc, last_config_version_});
}

bool RouteConfigUpdateReceiverImpl::onVhdsUpdate(
    const Protobuf::RepeatedPtrField<envoy::service::discovery::v3::Resource>& added_resources,
    const Protobuf::RepeatedPtrField<std::string>& removed_resources,
    const std::string& version_info) {
  collectResourceIdsInUpdate(added_resources);
  const bool removed = removeVhosts(vhds_virtual_hosts_, removed_resources);
  const bool updated = updateVhosts(vhds_virtual_hosts_, added_resources);
  onUpdateCommon(route_config_proto_, version_info);
  return removed || updated || !resource_ids_in_last_update_.empty();
}

void RouteConfigUpdateReceiverImpl::collectResourceIdsInUpdate(
    const Protobuf::RepeatedPtrField<envoy::service::discovery::v3::Resource>& added_resources) {
  resource_ids_in_last_update_.clear();
  for (const auto& resource : added_resources) {
    resource_ids_in_last_update_.emplace(resource.name());
    std::copy(resource.aliases().begin(), resource.aliases().end(),
              std::inserter(resource_ids_in_last_update_, resource_ids_in_last_update_.end()));
  }
}

void RouteConfigUpdateReceiverImpl::initializeRdsVhosts(
    const envoy::config::route::v3::RouteConfiguration& route_configuration) {
  rds_virtual_hosts_.clear();
  for (const auto& vhost : route_configuration.virtual_hosts()) {
    rds_virtual_hosts_.emplace(vhost.name(), vhost);
  }
}

bool RouteConfigUpdateReceiverImpl::removeVhosts(
    std::map<std::string, envoy::config::route::v3::VirtualHost>& vhosts,
    const Protobuf::RepeatedPtrField<std::string>& removed_vhost_names) {
  bool vhosts_removed = false;
  for (const auto& vhost_name : removed_vhost_names) {
    auto found = vhosts.find(vhost_name);
    if (found != vhosts.end()) {
      vhosts_removed = true;
      vhosts.erase(vhost_name);
    }
  }
  return vhosts_removed;
}

bool RouteConfigUpdateReceiverImpl::updateVhosts(
    std::map<std::string, envoy::config::route::v3::VirtualHost>& vhosts,
    const Protobuf::RepeatedPtrField<envoy::service::discovery::v3::Resource>& added_resources) {
  bool vhosts_added = false;
  for (const auto& resource : added_resources) {
    // the management server returns empty resources (they contain no virtual hosts in this case)
    // for aliases that it couldn't resolve.
    if (onDemandFetchFailed(resource)) {
      continue;
    }
    envoy::config::route::v3::VirtualHost vhost =
        MessageUtil::anyConvertAndValidate<envoy::config::route::v3::VirtualHost>(
            resource.resource(), validation_visitor_);
    auto found = vhosts.find(vhost.name());
    if (found != vhosts.end()) {
      vhosts.erase(found);
    }
    vhosts.emplace(vhost.name(), vhost);
    vhosts_added = true;
  }
  return vhosts_added;
}

void RouteConfigUpdateReceiverImpl::rebuildRouteConfig(
    const std::map<std::string, envoy::config::route::v3::VirtualHost>& rds_vhosts,
    const std::map<std::string, envoy::config::route::v3::VirtualHost>& vhds_vhosts,
    envoy::config::route::v3::RouteConfiguration& route_config) {
  route_config.clear_virtual_hosts();
  for (const auto& vhost : rds_vhosts) {
    route_config.mutable_virtual_hosts()->Add()->CopyFrom(vhost.second);
  }
  for (const auto& vhost : vhds_vhosts) {
    route_config.mutable_virtual_hosts()->Add()->CopyFrom(vhost.second);
  }
}

bool RouteConfigUpdateReceiverImpl::onDemandFetchFailed(
    const envoy::service::discovery::v3::Resource& resource) const {
  return !resource.has_resource();
}

} // namespace Router
} // namespace Envoy
