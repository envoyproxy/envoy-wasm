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

void WasmFactory::createWasm(const envoy::config::wasm::v2::WasmService& config,
                             Server::Configuration::WasmFactoryContext& context,
                             Server::CreateWasmCallback&& cb) {

  auto callback = [&context, &config, cb](const std::string& code) {
    // Create a base WASM to verify that the code loads before setting/cloning the for the
    // individual
    // threads.
    auto plugin = std::make_shared<Common::Wasm::Plugin>(
        config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
        code, envoy::api::v2::core::TrafficDirection::UNSPECIFIED, context.localInfo(),
        nullptr /* listener_metadata */, context.scope(), context.owned_scope());
    auto base_wasm = Common::Wasm::createWasm(config.config().vm_config(), plugin,
                                              context.clusterManager(), context.dispatcher());
    if (config.singleton()) {
      // Return the WASM VM which will be stored as a singleton by the Server.
      auto root_context = base_wasm->start();
      base_wasm->configure(root_context, config.config().configuration());
      return cb(base_wasm);
    }
    // Per-thread WASM VM.
    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    // NB: no need to keep the resulting slot as Wasm is cached on each thread.
    auto configuration = std::make_shared<std::string>(config.config().configuration());
    context.threadLocal().allocateSlot()->set(
        [base_wasm, configuration](Event::Dispatcher& dispatcher) {
          return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(
              Common::Wasm::getOrCreateThreadLocalWasm(*base_wasm, *configuration, dispatcher));
        });
    // Do not return this WASM VM since this is per-thread. Returning it would indicate that this is
    // a singleton.
    cb(nullptr);
  };

  if (config.config().vm_config().code().has_local()) {
    local_data_provider_ = std::make_unique<Config::DataSource::LocalAsyncDataProvider>(
        context.initManager(), config.config().vm_config().code().local(), true, context.api(),
        std::move(callback));
  } else {
    remote_data_provider_ = std::make_unique<Config::DataSource::RemoteAsyncDataProvider>(
        context.clusterManager(), context.initManager(),
        config.config().vm_config().code().remote(), true, std::move(callback));
  }
}

/**
 * Static registration for the wasm factory. @see RegistryFactory.
 */
static Registry::RegisterFactory<WasmFactory, Server::Configuration::WasmFactory> registered_;

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
