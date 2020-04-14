#include "extensions/wasm/config.h"

#include <stdio.h>

#include "envoy/registry/registry.h"

#include "common/common/empty_string.h"
#include "common/config/datasource.h"
#include "common/protobuf/utility.h"

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

static const std::string INLINE_STRING = "<inline>";

void WasmFactory::createWasm(const envoy::config::wasm::v3::WasmService& config,
                             Server::Configuration::WasmFactoryContext& context,
                             Server::CreateWasmServiceCallback&& cb) {
  auto plugin = std::make_shared<Common::Wasm::Plugin>(
      config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, context.localInfo(), nullptr);

  auto configuration = std::make_shared<std::string>(config.config().configuration());
  bool singleton = config.singleton();
  auto callback = [&context, configuration, singleton, plugin,
                   cb](Common::Wasm::WasmHandleSharedPtr base_wasm) {
    if (singleton) {
      // Return the WASM VM which will be stored as a singleton by the Server.
      auto root_context = base_wasm->wasm()->getOrCreateRootContext(plugin);
      if (!base_wasm->wasm()->configure(root_context, plugin, *configuration)) {
        cb(nullptr);
      }
      return cb(std::make_unique<Server::WasmService>(base_wasm));
    }
    // Per-thread WASM VM.
    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    auto tls_slot = context.threadLocal().allocateSlot();
    tls_slot->set([base_wasm, plugin, configuration](Event::Dispatcher& dispatcher) {
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(
          Common::Wasm::getOrCreateThreadLocalWasm(base_wasm, plugin, *configuration, dispatcher));
    });
    cb(std::make_unique<Server::WasmService>(std::move(tls_slot)));
  };

  Common::Wasm::createWasm(config.config().vm_config(), plugin, context.scope(),
                           context.clusterManager(), context.initManager(), context.dispatcher(),
                           context.random(), context.api(), remote_data_provider_,
                           std::move(callback));
}

/**
 * Static registration for the wasm factory. @see RegistryFactory.
 */
REGISTER_FACTORY(WasmFactory, Server::Configuration::WasmFactory);

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
