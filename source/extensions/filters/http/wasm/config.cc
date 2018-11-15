#include "extensions/filters/http/wasm/config.h"

#include "envoy/config/filter/http/wasm/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/config/filter_json.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/wasm/wasm_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

Http::FilterFactoryCb WasmFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::config::filter::http::wasm::v2::Wasm& proto_config, const std::string&,
    Server::Configuration::FactoryContext& context) {
  FilterConfigConstSharedPtr filter_config(
      new FilterConfig{proto_config.vm(), proto_config.file(), proto_config.configuration(),
                       proto_config.allow_precompiled(), context.threadLocal(), context.clusterManager()});
  return [filter_config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addStreamFilter(std::make_shared<Filter>(filter_config));
  };
}

Http::FilterFactoryCb
WasmFilterConfig::createFilterFactory(const Json::Object& json_config,
                                      const std::string& stat_prefix,
                                      Server::Configuration::FactoryContext& context) {
  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  Config::FilterJson::translateWasmFilter(json_config, proto_config);
  return createFilterFactoryFromProtoTyped(proto_config, stat_prefix, context);
}

/**
 * Static registration for the Wasm filter. @see RegisterFactory.
 */
static Registry::RegisterFactory<WasmFilterConfig,
                                 Server::Configuration::NamedHttpFilterConfigFactory>
    register_;

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
