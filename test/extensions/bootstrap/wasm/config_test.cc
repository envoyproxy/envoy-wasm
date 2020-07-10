#include "envoy/common/exception.h"
#include "envoy/extensions/wasm/v3/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/stats/isolated_store_impl.h"

#include "extensions/bootstrap/wasm/config.h"

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

using Extensions::Bootstrap::Wasm::WasmServicePtr;

class WasmFactoryTest : public testing::TestWithParam<std::string> {
protected:
  void initializeWithConfig(const envoy::extensions::wasm::v3::WasmService& config) {
    auto factory =
        Registry::FactoryRegistry<Server::Configuration::BootstrapExtensionFactory>::getFactory(
            "envoy.bootstrap.wasm");
    ASSERT_NE(factory, nullptr);
    api_ = Api::createApiForTest(stats_store_);
    EXPECT_CALL(context_, api()).WillRepeatedly(testing::ReturnRef(*api_));
    EXPECT_CALL(context_, initManager()).WillRepeatedly(testing::ReturnRef(init_manager_));
    EXPECT_CALL(context_, lifecycleNotifier())
        .WillRepeatedly(testing::ReturnRef(lifecycle_notifier_));
    extension_ = factory->createBootstrapExtension(config, context_);
    EXPECT_CALL(init_watcher_, ready());
    init_manager_.initialize(init_watcher_);
  }

  testing::NiceMock<Server::Configuration::MockServerFactoryContext> context_;
  testing::NiceMock<Server::MockServerLifecycleNotifier> lifecycle_notifier_;
  Init::ExpectableWatcherImpl init_watcher_;
  Stats::IsolatedStoreImpl stats_store_;
  Api::ApiPtr api_;
  Init::ManagerImpl init_manager_{"init_manager"};
  Server::BootstrapExtensionPtr extension_;
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmFactoryTest,
                         testing::Values("v8"
#if defined(ENVOY_WASM_WAVM)
                                         ,
                                         "wavm"
#endif
                                         ));

TEST_P(WasmFactoryTest, CreateWasmFromWasm) {
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/start_cpp.wasm"));
  config.set_singleton(true);

  initializeWithConfig(config);

  EXPECT_NE(extension_, nullptr);
}

TEST_P(WasmFactoryTest, CreateWasmFromWasmPerThread) {
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/start_cpp.wasm"));

  initializeWithConfig(config);

  EXPECT_NE(extension_, nullptr);
  extension_.reset();
  context_.threadLocal().shutdownThread();
}

TEST_P(WasmFactoryTest, MissingImport) {

  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->set_name("test");
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/missing_cpp.wasm"));
  config.set_singleton(true);

  EXPECT_THROW_WITH_MESSAGE(initializeWithConfig(config), Extensions::Common::Wasm::WasmException,
                            "Unable to create Wasm service test");
}

TEST_P(WasmFactoryTest, UnspecifiedRuntime) {
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->mutable_vm_config()->set_runtime("");
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/start_cpp.wasm"));
  config.set_singleton(true);

  EXPECT_THROW_WITH_REGEX(
      initializeWithConfig(config), EnvoyException,
      "Proto constraint validation failed \\(WasmServiceValidationError\\.Config");
}

TEST_P(WasmFactoryTest, UnknownRuntime) {
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->set_name("test");
  config.mutable_config()->mutable_vm_config()->set_runtime("envoy.wasm.runtime.invalid");
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/start_cpp.wasm"));
  config.set_singleton(true);

  EXPECT_THROW_WITH_MESSAGE(initializeWithConfig(config), Extensions::Common::Wasm::WasmException,
                            "Unable to create Wasm service test");
}

TEST_P(WasmFactoryTest, StartFailed) {
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->set_name("test");
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/start_cpp.wasm"));
  ProtobufWkt::StringValue plugin_configuration;
  plugin_configuration.set_value("bad");
  config.set_singleton(true);
  config.mutable_config()->mutable_vm_config()->mutable_configuration()->PackFrom(
      plugin_configuration);
  EXPECT_THROW_WITH_MESSAGE(initializeWithConfig(config), Extensions::Common::Wasm::WasmException,
                            "Unable to create Wasm service test");
}

TEST_P(WasmFactoryTest, ConfigureFailed) {
  envoy::extensions::wasm::v3::WasmService config;
  config.mutable_config()->set_name("test");
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/start_cpp.wasm"));
  ProtobufWkt::StringValue plugin_configuration;
  plugin_configuration.set_value("bad");
  config.mutable_config()->mutable_configuration()->PackFrom(plugin_configuration);
  config.set_singleton(true);
  EXPECT_THROW_WITH_MESSAGE(initializeWithConfig(config), Extensions::Common::Wasm::WasmException,
                            "Unable to create Wasm service test");
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
