#include "extensions/wasm/config.h"

#include "envoy/registry/registry.h"

#include "common/common/empty_string.h"
#include "common/config/datasource.h"
#include "common/protobuf/utility.h"

#include "extensions/common/wasm/wasm.h"

#include <stdio.h>

namespace Envoy {
namespace Extensions {
namespace Wasm {

static const std::string INLINE_STRING = "<inline>";

Server::WasmPtr WasmFactory::createWasm(const envoy::config::wasm::v2::WasmConfig& config,
                                        Server::Configuration::WasmFactoryContext& context) {
  if (config.singleton()) {
    auto wasm = Common::Wasm::createWasm(config.id(), config.vm_config(), context.api());
    wasm->setDispatcher(context.dispatcher());
    return Server::WasmPtr(wasm.release());
  }
  // Per-thread WASM VM.
  auto base_wasm = Common::Wasm::createWasm(config.id(), config.vm_config(), context.api());
  context.threadLocal().allocateSlot()->set([&config, &context, &base_wasm](Event::Dispatcher& dispatcher) {
      // NB: no need to store the result as it is cached on each thread.
      Extensions::Common::Wasm::createThreadLocalWasm(*base_wasm, config.vm_config(), dispatcher, config.configuration(), context.api());
      return nullptr;
      });
  // Do not return this WASM VM since this is per-thread.  Returning it would indicate that this is a singleton.
  return nullptr;
}

/**
 * Static registration for the wasm factory. @see RegistryFactory.
 */
static Registry::RegisterFactory<WasmFactory, Server::Configuration::WasmFactory> registered_;

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
