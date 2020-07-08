#pragma once

#include <string>

#include "envoy/server/instance.h"

#include "server/configuration_impl.h"

namespace Envoy {
namespace Extensions {
namespace StatSinks {
namespace Wasm {

/**
 * Config registration for the Wasm statsd sink. @see StatSinkFactory.
 */
class WasmSinkFactory : Logger::Loggable<Logger::Id::config>,
                        public Server::Configuration::StatsSinkFactory {
public:
  // StatsSinkFactory
  Stats::SinkPtr createStatsSink(const Protobuf::Message& config,
                                 Server::Instance& server) override;

  ProtobufTypes::MessagePtr createEmptyConfigProto() override;

  std::string name() const override;
};

} // namespace Wasm
} // namespace StatSinks
} // namespace Extensions
} // namespace Envoy