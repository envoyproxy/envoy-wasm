#include "extensions/stat_sinks/wasm/config.h"

#include <memory>

#include "envoy/config/metrics/v3/stats.pb.h"
#include "envoy/config/metrics/v3/stats.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/network/resolver_impl.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/stat_sinks/common/statsd/statsd.h"
#include "extensions/stat_sinks/wasm/wasm_stat_sinks_impl.h"
#include "extensions/stat_sinks/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace StatSinks {
namespace Wasm {

Stats::SinkPtr WasmSinkFactory::createStatsSink(const Protobuf::Message& config,
                                                Server::Instance& server) {
  const auto& wasm_config =
      MessageUtil::downcastAndValidate<const envoy::extensions::StatSinks::v3::Wasm&>(
          config, server.messageValidationContext().staticValidationVisitor());

  auto wasm_sink = std::make_unique<WasmStatSink>(nullptr);

  auto plugin = std::make_shared<Common::Wasm::Plugin>(
      wasm_config.config().name(), wasm_config.config().root_id(),
      wasm_config.config().vm_config().vm_id(),
      Common::Wasm::anyToBytes(wasm_config.config().configuration()),
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, context.localInfo(),
      nullptr /* listener_metadata */);

  auto callback = [wasm_sink, &context, plugin](Common::Wasm::WasmHandleSharedPtr base_wasm) {
    auto root_context = base_wasm->wasm()->getOrCreateRootContext(plugin);
    if (!base_wasm->wasm()->configure(root_context, plugin)) {
      return nullptr;
    }
    wasm_sink->setSingleton(base_wasm);
  };

  Common::Wasm::createWasm(
      wasm_config.config().vm_config(), plugin, context.scope().createScope(""),
      context.clusterManager(), context.initManager(), context.dispatcher(), context.random(),
      context.api(), context.lifecycleNotifier(), remote_data_provider_, std::move(callback));

  return wasm_sink;
}

ProtobufTypes::MessagePtr WasmSinkFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::StatSinks::Wasm>();
}

std::string WasmSinkFactory::name() const { return StatsSinkNames::get().Wasm; }

/**
 * Static registration for the wasm access log. @see RegisterFactory.
 */
REGISTER_FACTORY(WasmSinkFactory, Server::Configuration::StatsSinkFactory);

} // namespace Wasm
} // namespace StatSinks
} // namespace Extensions
} // namespace Envoy
