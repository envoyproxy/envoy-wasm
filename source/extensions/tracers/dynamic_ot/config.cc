#include "extensions/tracers/dynamic_ot/config.h"

#include "envoy/config/trace/v3alpha/trace.pb.h"
#include "envoy/config/trace/v3alpha/trace.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/common/utility.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/tracers/dynamic_ot/dynamic_opentracing_driver_impl.h"
#include "extensions/tracers/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace DynamicOt {

DynamicOpenTracingTracerFactory::DynamicOpenTracingTracerFactory()
    : FactoryBase(TracerNames::get().DynamicOt) {}

Tracing::HttpTracerPtr DynamicOpenTracingTracerFactory::createHttpTracerTyped(
    const envoy::config::trace::v3alpha::DynamicOtConfig& proto_config, Server::Instance& server) {
  const std::string& library = proto_config.library();
  const std::string config = MessageUtil::getJsonStringFromMessage(proto_config.config());
  Tracing::DriverPtr dynamic_driver =
      std::make_unique<DynamicOpenTracingDriver>(server.stats(), library, config);
  return std::make_unique<Tracing::HttpTracerImpl>(std::move(dynamic_driver), server.localInfo());
}

/**
 * Static registration for the dynamic opentracing tracer. @see RegisterFactory.
 */
REGISTER_FACTORY(DynamicOpenTracingTracerFactory, Server::Configuration::TracerFactory);

} // namespace DynamicOt
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
