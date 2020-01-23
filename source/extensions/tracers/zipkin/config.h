#pragma once

#include "envoy/config/trace/v3/trace.pb.h"
#include "envoy/config/trace/v3/trace.pb.validate.h"

#include "extensions/tracers/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace Zipkin {

/**
 * Config registration for the zipkin tracer. @see TracerFactory.
 */
class ZipkinTracerFactory : public Common::FactoryBase<envoy::config::trace::v3::ZipkinConfig> {
public:
  ZipkinTracerFactory();

private:
  // FactoryBase
  Tracing::HttpTracerPtr
  createHttpTracerTyped(const envoy::config::trace::v3::ZipkinConfig& proto_config,
                        Server::Instance& server) override;
};

} // namespace Zipkin
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
