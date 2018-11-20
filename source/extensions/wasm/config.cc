#include "extensions/wasm/config.h"

#include "envoy/registry/registry.h"

#include "common/common/empty_string.h"
#include "common/config/datasource.h"
#include "common/protobuf/utility.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/wasm/wasm.h"

#include <stdio.h>

namespace Envoy {
namespace Extensions {
namespace Wasm {

static const std::string INLINE_STRING = "<inline>";

Server::WasmPtr WasmFactory::createWasm(const envoy::config::wasm::v2::WasmConfig& config,
                                        Server::Configuration::WasmFactoryContext& context) {
  const auto& code = Config::DataSource::read(config.code(), true);
  const auto& path = Config::DataSource::getPath(config.code())
                         .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  if (code.empty()) {
    throw EnvoyException(fmt::format("Failed to load WASM code from {}", path));
  }
  auto vm = std::make_unique<Wasm>(config.vm());
  if (vm) {
    if (!vm->initialize(code, path, config.allow_precompiled()))
      return nullptr;
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
