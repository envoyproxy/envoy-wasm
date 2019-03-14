#include <stdio.h>

#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/stats/isolated_store_impl.h"

#include "server/wasm_config_impl.h"

#include "extensions/wasm/config.h"

#include "test/mocks/event/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

TEST(WasmFactoryTest, CreateWasmFromWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm("envoy.wasm.vm.wavm");
  config.mutable_vm_config()->mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  config.set_singleton(true);
  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  ThreadLocal::MockInstance tls;
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}
TEST(WasmFactoryTest, CreateWasmFromPrecompiledWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm("envoy.wasm.vm.wavm");
  config.mutable_vm_config()->mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  config.mutable_vm_config()->set_allow_precompiled(true);
  config.set_singleton(true);
  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  ThreadLocal::MockInstance tls;
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromWASMPerThread) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm("envoy.wasm.vm.wavm");
  config.mutable_vm_config()->mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  config.set_id("test_id");
  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  testing::NiceMock<ThreadLocal::MockInstance> tls;
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api);
  auto wasm = factory->createWasm(config, context);
  EXPECT_EQ(wasm, nullptr);
}

TEST(WasmFactoryTest, MissingImport) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm("envoy.wasm.vm.wavm");
  config.mutable_vm_config()->mutable_code()->set_inline_string(
      "(module\n"
      "  (type $0 (func (param i32 i32 i32)))\n"
      "  (type $1 (func))\n"
      "  (import \"env\" \"missing\" (func $missing (param i32 i32 i32)))\n"
      "  (export \"memory\" (memory $2))\n"
      "  (export \"main\" (func $main))\n"
      "  (memory $2 17)\n"
      "  (data $2 (i32.const 1048576) \"Hello, world!\")\n"
      ""
      " (func $main (type $1)\n"
      "   i32.const 1\n"
      "   i32.const 1048576\n"
      "   i32.const 13\n"
      "   call $missing\n"
      "   )\n"
      " )");
  config.set_singleton(true);

  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  ThreadLocal::MockInstance tls;
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api);
  Server::WasmSharedPtr wasm;
  EXPECT_THROW_WITH_MESSAGE(
      wasm = factory->createWasm(config, context), Extensions::Common::Wasm::WasmException,
      "Failed to load WASM module due to a missing import: env.missing func (i32, i32, i32)->()");
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
