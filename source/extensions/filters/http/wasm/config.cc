#include "extensions/filters/http/wasm/config.h"

#include "envoy/config/filter/http/wasm/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/common/empty_string.h"
#include "common/config/datasource.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/wasm/wasm_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

static const std::string INLINE_STRING = "<inline>";

Http::FilterFactoryCb WasmFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::config::filter::http::wasm::v2::Wasm& proto_config, const std::string&,
    Server::Configuration::FactoryContext& context) {
  const auto& code = Config::DataSource::read(proto_config.code(), true, context.api());
  const auto& path = Config::DataSource::getPath(proto_config.code())
                         .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  if (code.empty()) {
    throw EnvoyException(fmt::format("Failed to load WASM code from {}", path));
  }
  FilterConfigConstSharedPtr filter_config(new FilterConfig{
      proto_config.vm(), code, path, proto_config.allow_precompiled(), proto_config.configuration(),
      context.threadLocal(), context.clusterManager()});
  filter_config->initialize(filter_config);
  return [filter_config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
    auto filter = std::make_shared<Filter>(filter_config);
    filter->wasm()->allocContextId(filter.get());
    callbacks.addStreamFilter(std::move(filter));
  };
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
