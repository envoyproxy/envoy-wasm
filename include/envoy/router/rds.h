#pragma once

#include <memory>

#include "envoy/api/v2/rds.pb.h"
#include "envoy/router/router.h"

namespace Envoy {
namespace Router {

/**
 * A provider for constant route configurations.
 */
class RouteConfigProvider {
public:
  struct ConfigInfo {
    // A reference to the currently loaded route configuration. Do not hold this reference beyond
    // the caller of configInfo()'s scope.
    const envoy::api::v2::RouteConfiguration& config_;

    // The discovery version that supplied this route. This will be set to "" in the case of
    // static clusters.
    std::string version_;
  };

  virtual ~RouteConfigProvider() = default;

  /**
   * @return Router::ConfigConstSharedPtr a route configuration for use during a single request. The
   * returned config may be different on a subsequent call, so a new config should be acquired for
   * each request flow.
   */
  virtual Router::ConfigConstSharedPtr config() PURE;

  /**
   * @return the configuration information for the currently loaded route configuration. Note that
   * if the provider has not yet performed an initial configuration load, no information will be
   * returned.
   */
  virtual absl::optional<ConfigInfo> configInfo() const PURE;

  /**
   * @return the last time this RouteConfigProvider was updated. Used for config dumps.
   */
  virtual SystemTime lastUpdated() const PURE;

  /**
   * Callback used to notify RouteConfigProvider about configuration changes.
   */
  virtual void onConfigUpdate() PURE;

  /**
   * Validate if the route configuration can be applied to the context of the route config provider.
   */
  virtual void validateConfig(const envoy::api::v2::RouteConfiguration& config) const PURE;
};

using RouteConfigProviderPtr = std::unique_ptr<RouteConfigProvider>;

} // namespace Router
} // namespace Envoy
