#pragma once

#include <string>

#include "envoy/config/trace/v3alpha/trace.pb.h"
#include "envoy/config/trace/v3alpha/trace.pb.validate.h"

#include "extensions/tracers/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace OpenCensus {

/**
 * Config registration for the OpenCensus tracer. @see TracerFactory.
 */
class OpenCensusTracerFactory
    : public Common::FactoryBase<envoy::config::trace::v3alpha::OpenCensusConfig> {
public:
  OpenCensusTracerFactory();

private:
  // FactoryBase
  Tracing::HttpTracerPtr
  createHttpTracerTyped(const envoy::config::trace::v3alpha::OpenCensusConfig& proto_config,
                        Server::Instance& server) override;
};

} // namespace OpenCensus
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
