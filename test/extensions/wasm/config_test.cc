#include <stdio.h>

#include "envoy/extensions/wasm/v3/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/stats/isolated_store_impl.h"

#include "server/wasm_config_impl.h"

#include "extensions/wasm/config.h"

#include "test/mocks/event/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

class WasmFactoryTest : public testing::TestWithParam<std::string> {};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmFactoryTest,
                         testing::Values("v8"
#if defined(ENVOY_WASM_WAVM)
                                         ,
                                         "wavm"
#endif
                                         ));

TEST_P(WasmFactoryTest, CreateWasmFromWasm) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/wasm/test_data/start_cpp.wasm"));
  config.set_singleton(true);
  NiceMock<Server::MockInstance> server;
  Init::ExpectableWatcherImpl init_watcher;
  Stats::IsolatedStoreImpl stats_store;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  EXPECT_CALL(server, api()).WillRepeatedly(testing::ReturnRef(*api));
  Init::ManagerImpl init_manager{"init_manager"};
  EXPECT_CALL(server, initManager()).WillRepeatedly(testing::ReturnRef(init_manager));
  Server::Configuration::WasmFactoryContextImpl context(server, scope);
  Server::WasmServicePtr wasmptr = nullptr;
  factory->createWasm(config, context,
                      [&wasmptr](Server::WasmServicePtr wasm) { wasmptr = std::move(wasm); });
  EXPECT_CALL(init_watcher, ready());
  server.initManager().initialize(init_watcher);
  EXPECT_NE(wasmptr, nullptr);
  wasmptr.reset();
}

TEST_P(WasmFactoryTest, CreateWasmFromWasmPerThread) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/wasm/test_data/start_cpp.wasm"));
  NiceMock<Server::MockInstance> server;
  Init::ExpectableWatcherImpl init_watcher;
  Stats::IsolatedStoreImpl stats_store;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  EXPECT_CALL(server, api()).WillRepeatedly(testing::ReturnRef(*api));
  Init::ManagerImpl init_manager{"init_manager"};
  EXPECT_CALL(server, initManager()).WillRepeatedly(testing::ReturnRef(init_manager));
  Server::Configuration::WasmFactoryContextImpl context(server, scope);
  Server::WasmServicePtr wasmptr = nullptr;
  factory->createWasm(config, context,
                      [&wasmptr](Server::WasmServicePtr wasm) { wasmptr = std::move(wasm); });
  EXPECT_CALL(init_watcher, ready());
  server.initManager().initialize(init_watcher);
  EXPECT_NE(wasmptr, nullptr);
  wasmptr.reset();
  server.threadLocal().shutdownThread();
}

TEST_P(WasmFactoryTest, MissingImport) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/wasm/test_data/missing_cpp.wasm"));
  config.set_singleton(true);
  NiceMock<Server::MockInstance> server;
  Stats::IsolatedStoreImpl stats_store;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  EXPECT_CALL(server, api()).WillRepeatedly(testing::ReturnRef(*api));
  Server::Configuration::WasmFactoryContextImpl context(server, scope);
  EXPECT_THROW_WITH_REGEX(factory->createWasm(config, context, [](Server::WasmServicePtr) {});
                          , Extensions::Common::Wasm::WasmException,
                          "Failed to load Wasm module due to a missing import: env.missing");
}

TEST_P(WasmFactoryTest, UnspecifiedRuntime) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime("");
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/wasm/test_data/start_cpp.wasm"));
  config.set_singleton(true);
  NiceMock<Server::MockInstance> server;
  Stats::IsolatedStoreImpl stats_store;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  EXPECT_CALL(server, api()).WillRepeatedly(testing::ReturnRef(*api));
  Server::Configuration::WasmFactoryContextImpl context(server, scope);
  EXPECT_THROW_WITH_MESSAGE(factory->createWasm(config, context, [](Server::WasmServicePtr) {}),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to create Wasm VM with unspecified runtime.");
}

TEST_P(WasmFactoryTest, UnknownRuntime) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime("envoy.wasm.runtime.invalid");
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/wasm/test_data/start_cpp.wasm"));
  config.set_singleton(true);
  NiceMock<Server::MockInstance> server;
  Stats::IsolatedStoreImpl stats_store;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  EXPECT_CALL(server, api()).WillRepeatedly(testing::ReturnRef(*api));
  Server::Configuration::WasmFactoryContextImpl context(server, scope);
  EXPECT_THROW_WITH_MESSAGE(factory->createWasm(config, context, [](Server::WasmServicePtr) {}),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to create Wasm VM using envoy.wasm.runtime.invalid runtime. "
                            "Envoy was compiled without support for it.");
}

TEST_P(WasmFactoryTest, StartFailed) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/wasm/test_data/start_cpp.wasm"));
  ProtobufWkt::StringValue plugin_configuration;
  plugin_configuration.set_value("bad");
  config.set_singleton(true);
  config.mutable_config()->mutable_vm_config()->mutable_configuration()->PackFrom(
      plugin_configuration);
  NiceMock<Server::MockInstance> server;
  Init::ExpectableWatcherImpl init_watcher;
  Stats::IsolatedStoreImpl stats_store;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  EXPECT_CALL(server, api()).WillRepeatedly(testing::ReturnRef(*api));
  Init::ManagerImpl init_manager{"init_manager"};
  EXPECT_CALL(server, initManager()).WillRepeatedly(testing::ReturnRef(init_manager));
  Server::Configuration::WasmFactoryContextImpl context(server, scope);
  Server::WasmServicePtr wasmptr = nullptr;
  EXPECT_THROW_WITH_MESSAGE(factory->createWasm(config, context, [](Server::WasmServicePtr) {}),
                            Extensions::Common::Wasm::WasmException, "Failed to start base Wasm");
  EXPECT_CALL(init_watcher, ready());
  server.initManager().initialize(init_watcher);
  EXPECT_EQ(wasmptr, nullptr);
  wasmptr.reset();
}

TEST_P(WasmFactoryTest, ConfigureFailed) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/wasm/test_data/start_cpp.wasm"));
  ProtobufWkt::StringValue plugin_configuration;
  plugin_configuration.set_value("bad");
  config.mutable_config()->mutable_configuration()->PackFrom(plugin_configuration);
  config.set_singleton(true);
  NiceMock<Server::MockInstance> server;
  Init::ExpectableWatcherImpl init_watcher;
  Stats::IsolatedStoreImpl stats_store;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  EXPECT_CALL(server, api()).WillRepeatedly(testing::ReturnRef(*api));
  Init::ManagerImpl init_manager{"init_manager"};
  EXPECT_CALL(server, initManager()).WillRepeatedly(testing::ReturnRef(init_manager));
  Server::Configuration::WasmFactoryContextImpl context(server, scope);
  Server::WasmServicePtr wasmptr = nullptr;
  EXPECT_THROW_WITH_MESSAGE(factory->createWasm(config, context, [](Server::WasmServicePtr) {}),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to configure base Wasm plugin");
  EXPECT_CALL(init_watcher, ready());
  server.initManager().initialize(init_watcher);
  EXPECT_EQ(wasmptr, nullptr);
  wasmptr.reset();
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
