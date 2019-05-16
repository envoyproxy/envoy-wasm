#include <stdio.h>

#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/stats/isolated_store_impl.h"

#include "server/wasm_config_impl.h"

#include "extensions/wasm/config.h"

#include "test/mocks/event/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

class WasmFactoryTest : public TestBaseWithParam<std::string> {};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmFactoryTest, testing::Values("wavm", "v8"));

TEST_P(WasmFactoryTest, CreateWasmFromWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm(absl::StrCat("envoy.wasm.vm.", GetParam()));
  config.mutable_vm_config()->mutable_code()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/logging_cpp.wasm"));
  config.set_singleton(true);
  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  ThreadLocal::MockInstance tls;
  Stats::IsolatedStoreImpl stats_store;
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api,
                                                        scope, local_info);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}
TEST_P(WasmFactoryTest, CreateWasmFromPrecompiledWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm(absl::StrCat("envoy.wasm.vm.", GetParam()));
  config.mutable_vm_config()->mutable_code()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/logging_cpp.wasm"));
  config.mutable_vm_config()->set_allow_precompiled(true);
  config.set_singleton(true);
  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  ThreadLocal::MockInstance tls;
  Stats::IsolatedStoreImpl stats_store;
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api,
                                                        scope, local_info);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST_P(WasmFactoryTest, CreateWasmFromWASMPerThread) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm(absl::StrCat("envoy.wasm.vm.", GetParam()));
  config.mutable_vm_config()->mutable_code()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/logging_cpp.wasm"));
  config.set_id("test_id");
  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  testing::NiceMock<ThreadLocal::MockInstance> tls;
  Stats::IsolatedStoreImpl stats_store;
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api,
                                                        scope, local_info);
  auto wasm = factory->createWasm(config, context);
  EXPECT_EQ(wasm, nullptr);
}

TEST_P(WasmFactoryTest, MissingImport) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.mutable_vm_config()->set_vm(absl::StrCat("envoy.wasm.vm.", GetParam()));
  config.mutable_vm_config()->mutable_code()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/missing_cpp.wasm"));
  config.set_singleton(true);
  Upstream::MockClusterManager cluster_manager;
  Event::MockDispatcher dispatcher;
  ThreadLocal::MockInstance tls;
  Stats::IsolatedStoreImpl stats_store;
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Server::Configuration::WasmFactoryContextImpl context(cluster_manager, dispatcher, tls, *api,
                                                        scope, local_info);
  Server::WasmSharedPtr wasm;
  EXPECT_THROW_WITH_REGEX(wasm = factory->createWasm(config, context),
                          Extensions::Common::Wasm::WasmException,
                          "Failed to load WASM module due to a missing import: env._missing.*");
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
