#include "extensions/filters/network/dubbo_proxy/router/config.h"

#include "envoy/registry/registry.h"

#include "extensions/filters/network/dubbo_proxy/router/router_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace DubboProxy {
namespace Router {

DubboFilters::FilterFactoryCb RouterFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::config::filter::dubbo::router::v2alpha1::Router&, const std::string&,
    Server::Configuration::FactoryContext& context) {
  return [&context](DubboFilters::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addDecoderFilter(std::make_shared<Router>(context.clusterManager()));
  };
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
REGISTER_FACTORY(RouterFilterConfig, DubboFilters::NamedDubboFilterConfigFactory);

} // namespace Router
} // namespace DubboProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
