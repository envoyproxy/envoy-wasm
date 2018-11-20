#include <stdio.h>

#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "server/wasm_config_impl.h"

#include "extensions/wasm/config.h"

#include "test/mocks/event/mocks.h"
#include "test/test_common/environment.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

TEST(WasmFactoryTest, CreateWasmFromWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  EXPECT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromPrecompiledWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  EXPECT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  config.set_allow_precompiled(true);
  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromWAT) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  EXPECT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wat"));
  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
