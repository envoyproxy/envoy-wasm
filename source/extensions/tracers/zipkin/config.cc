#include "extensions/tracers/zipkin/config.h"

#include "envoy/config/trace/v3alpha/trace.pb.h"
#include "envoy/config/trace/v3alpha/trace.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/common/utility.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/tracers/well_known_names.h"
#include "extensions/tracers/zipkin/zipkin_tracer_impl.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace Zipkin {

ZipkinTracerFactory::ZipkinTracerFactory() : FactoryBase(TracerNames::get().Zipkin) {}

Tracing::HttpTracerPtr ZipkinTracerFactory::createHttpTracerTyped(
    const envoy::config::trace::v3alpha::ZipkinConfig& proto_config, Server::Instance& server) {
  Tracing::DriverPtr zipkin_driver = std::make_unique<Zipkin::Driver>(
      proto_config, server.clusterManager(), server.stats(), server.threadLocal(), server.runtime(),
      server.localInfo(), server.random(), server.timeSource());

  return std::make_unique<Tracing::HttpTracerImpl>(std::move(zipkin_driver), server.localInfo());
}

/**
 * Static registration for the lightstep tracer. @see RegisterFactory.
 */
REGISTER_FACTORY(ZipkinTracerFactory, Server::Configuration::TracerFactory);

} // namespace Zipkin
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
