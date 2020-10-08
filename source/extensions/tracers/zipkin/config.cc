#include "extensions/tracers/zipkin/config.h"

#include "envoy/config/trace/v3/zipkin.pb.h"
#include "envoy/config/trace/v3/zipkin.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/common/utility.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/tracers/zipkin/zipkin_tracer_impl.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace Zipkin {

ZipkinTracerFactory::ZipkinTracerFactory() : FactoryBase("envoy.tracers.zipkin") {}

Tracing::HttpTracerSharedPtr ZipkinTracerFactory::createHttpTracerTyped(
    const envoy::config::trace::v3::ZipkinConfig& proto_config,
    Server::Configuration::TracerFactoryContext& context) {
  Tracing::DriverPtr zipkin_driver = std::make_unique<Zipkin::Driver>(
      proto_config, context.serverFactoryContext().clusterManager(),
      context.serverFactoryContext().scope(), context.serverFactoryContext().threadLocal(),
      context.serverFactoryContext().runtime(), context.serverFactoryContext().localInfo(),
      context.serverFactoryContext().api().randomGenerator(),
      context.serverFactoryContext().timeSource());

  return std::make_shared<Tracing::HttpTracerImpl>(std::move(zipkin_driver),
                                                   context.serverFactoryContext().localInfo());
}

/**
 * Static registration for the Zipkin tracer. @see RegisterFactory.
 */
REGISTER_FACTORY(ZipkinTracerFactory, Server::Configuration::TracerFactory){"envoy.zipkin"};

} // namespace Zipkin
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
