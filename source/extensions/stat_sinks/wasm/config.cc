#include "extensions/stat_sinks/wasm/config.h"

#include <memory>

#include "envoy/config/metrics/v3/stats.pb.h"
#include "envoy/config/metrics/v3/stats.pb.validate.h"
#include "envoy/extensions/stat_sinks/wasm/v3/wasm.pb.validate.h"
#include "envoy/registry/registry.h"
#include "envoy/server/factory_context.h"

#include "common/network/resolver_impl.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/stat_sinks/wasm/wasm_stat_sink_impl.h"
#include "extensions/stat_sinks/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace StatSinks {
namespace Wasm {

Stats::SinkPtr WasmSinkFactory::createStatsSink(const Protobuf::Message& proto_config,
                                                Server::Instance& context) {
  const auto& config =
      MessageUtil::downcastAndValidate<const envoy::extensions::stat_sinks::wasm::v3::Wasm&>(
          proto_config, context.messageValidationContext().staticValidationVisitor());

  auto wasm_sink = std::make_unique<WasmStatSink>(config.config().root_id(), nullptr);

  auto plugin = std::make_shared<Common::Wasm::Plugin>(
      config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
      Common::Wasm::anyToBytes(config.config().configuration()), false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, context.localInfo(), nullptr);

  auto callback = [&wasm_sink, &context, plugin](Common::Wasm::WasmHandleSharedPtr base_wasm) {
    if (!base_wasm) {
      if (plugin->fail_open_) {
        ENVOY_LOG(error, "Unable to create Wasm service {}", plugin->name_);
      } else {
        ENVOY_LOG(critical, "Unable to create Wasm service {}", plugin->name_);
      }
      return;
    }
    wasm_sink->setSingleton(
        Common::Wasm::getOrCreateThreadLocalWasm(base_wasm, plugin, context.dispatcher()));
  };

  Common::Wasm::createWasm(config.config().vm_config(), plugin, nullptr, context.clusterManager(),
                           context.initManager(), context.dispatcher(), context.random(),
                           context.api(), context.lifecycleNotifier(), remote_data_provider_,
                           std::move(callback));

  return wasm_sink;
}

ProtobufTypes::MessagePtr WasmSinkFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::stat_sinks::wasm::v3::Wasm>();
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
