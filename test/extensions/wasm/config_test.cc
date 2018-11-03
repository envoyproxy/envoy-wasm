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

TEST(WasmFactoryTest, CreateWasm) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  EXPECT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.set_file(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/envoy_wasm_test.wasm"));
  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
