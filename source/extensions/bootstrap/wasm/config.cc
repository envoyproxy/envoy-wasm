#include "extensions/bootstrap/wasm/config.h"

#include "envoy/registry/registry.h"
#include "envoy/server/factory_context.h"

#include "common/common/empty_string.h"
#include "common/config/datasource.h"
#include "common/protobuf/utility.h"

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Bootstrap {
namespace Wasm {

static const std::string INLINE_STRING = "<inline>";

void WasmFactory::createWasm(const envoy::extensions::wasm::v3::WasmService& config,
                             Server::Configuration::ServerFactoryContext& context,
                             CreateWasmServiceCallback&& cb) {
  auto plugin = std::make_shared<Common::Wasm::Plugin>(
      config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
      Common::Wasm::anyToBytes(config.config().configuration()),
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, context.localInfo(), nullptr);

  bool singleton = config.singleton();
  auto callback = [&context, singleton, plugin, cb](Common::Wasm::WasmHandleSharedPtr base_wasm) {
    if (singleton) {
      // Return the WASM VM which will be stored as a singleton by the Server.
      auto root_context = base_wasm->wasm()->getOrCreateRootContext(plugin);
      if (!base_wasm->wasm()->configure(root_context, plugin)) {
        return cb(nullptr);
      }
      return cb(std::make_unique<WasmService>(base_wasm));
    }
    // Per-thread WASM VM.
    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    auto tls_slot = context.threadLocal().allocateSlot();
    tls_slot->set([base_wasm, plugin](Event::Dispatcher& dispatcher) {
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(
          Common::Wasm::getOrCreateThreadLocalWasm(base_wasm, plugin, dispatcher));
    });
    cb(std::make_unique<WasmService>(std::move(tls_slot)));
  };

  Common::Wasm::createWasm(config.config().vm_config(), plugin, context.scope().createScope(""),
                           context.clusterManager(), context.initManager(), context.dispatcher(),
                           context.random(), context.api(), context.lifecycleNotifier(),
                           remote_data_provider_, std::move(callback));
}

Server::BootstrapExtensionPtr
WasmFactory::createBootstrapExtension(const Protobuf::Message& config,
                                      Server::Configuration::ServerFactoryContext& context) {
  auto typed_config =
      MessageUtil::downcastAndValidate<const envoy::extensions::wasm::v3::WasmService&>(
          config, context.messageValidationContext().staticValidationVisitor());

  auto wasm_service_extension = std::make_unique<WasmServiceExtension>();
  createWasm(typed_config, context,
             [extension = wasm_service_extension.get()](WasmServicePtr wasm) {
               extension->wasm_service_ = std::move(wasm);
             });
  return wasm_service_extension;
}

// /**
//  * Static registration for the wasm factory. @see RegistryFactory.
//  */
REGISTER_FACTORY(WasmFactory, Server::Configuration::BootstrapExtensionFactory);

} // namespace Wasm
} // namespace Bootstrap
} // namespace Extensions
} // namespace Envoy
