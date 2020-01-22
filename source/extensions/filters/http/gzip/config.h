#pragma once

#include "envoy/extensions/filters/http/gzip/v3alpha/gzip.pb.h"
#include "envoy/extensions/filters/http/gzip/v3alpha/gzip.pb.validate.h"

#include "extensions/filters/http/common/factory_base.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Gzip {

/**
 * Config registration for the gzip filter. @see NamedHttpFilterConfigFactory.
 */
class GzipFilterFactory
    : public Common::FactoryBase<envoy::extensions::filters::http::gzip::v3alpha::Gzip> {
public:
  GzipFilterFactory() : FactoryBase(HttpFilterNames::get().EnvoyGzip) {}

private:
  Http::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::http::gzip::v3alpha::Gzip& config,
      const std::string& stats_prefix, Server::Configuration::FactoryContext& context) override;
};

} // namespace Gzip
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
