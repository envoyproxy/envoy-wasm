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

Server::WasmSharedPtr WasmFactory::createWasm(const envoy::config::wasm::v2::WasmConfig& config,
                                              Server::Configuration::WasmFactoryContext& context) {
  // Create a base WASM to verify that the code loads before setting/cloning the for the individual
  // threads.
  auto base_wasm =
      Common::Wasm::createWasm(config.id(), config.vm_config(), context.clusterManager(),
                               context.dispatcher(), context.api());
  if (config.singleton()) {
    // Return the WASM VM which will be stored as a singleton by the Server.
    return base_wasm;
  }
  auto configuration = std::make_shared<std::string>(config.configuration());
  // Per-thread WASM VM.
  // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
  // NB: no need to keep the resulting slot as Wasm is cached on each thread.
  context.threadLocal().allocateSlot()->set([base_wasm,
                                             configuration](Event::Dispatcher& dispatcher) {
    return Extensions::Common::Wasm::createThreadLocalWasm(*base_wasm, *configuration, dispatcher);
  });
  // Do not return this WASM VM since this is per-thread. Returning it would indicate that this is a
  // singleton.
  return nullptr;
}

/**
 * Static registration for the wasm factory. @see RegistryFactory.
 */
static Registry::RegisterFactory<WasmFactory, Server::Configuration::WasmFactory> registered_;

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
