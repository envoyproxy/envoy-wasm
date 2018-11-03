#include "extensions/wasm/config.h"

#include "envoy/registry/registry.h"

#include "common/protobuf/utility.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/wasm/wasm.h"

#include <stdio.h>

namespace Envoy {
namespace Extensions {
namespace Wasm {

Server::WasmPtr WasmFactory::createWasm(
    const envoy::config::wasm::v2::WasmConfig& config,
    Server::Configuration::WasmFactoryContext& context) {
  auto vm = std::make_unique<Wasm>(config.vm());
  if (vm) {
    if (!vm->initialize(config.file(), config.allow_precompiled())) return nullptr;
    if (!config.configuration().empty())
      vm->configure(config.configuration());
    vm->start(context.dispatcher(), std::chrono::milliseconds(config.tick_timeout_milliseconds()));
  }
  return vm;
}

/**
 * Static registration for the wasm factory. @see RegistryFactory.
 */
static Registry::RegisterFactory<WasmFactory, Server::Configuration::WasmFactory> registered_;

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
