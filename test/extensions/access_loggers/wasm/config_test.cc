#include "envoy/config/accesslog/v2/wasm.pb.h"
#include "envoy/registry/registry.h"

#include "common/access_log/access_log_impl.h"
#include "common/protobuf/protobuf.h"

#include "extensions/access_loggers/wasm/config.h"
#include "extensions/access_loggers/wasm/wasm_access_log_impl.h"
#include "extensions/access_loggers/well_known_names.h"
#include "extensions/common/wasm/wasm.h"

#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace Wasm {

class TestFactoryContext : public NiceMock<Server::Configuration::MockFactoryContext> {
public:
  TestFactoryContext(Api::Api& api, Stats::Scope& scope) : api_(api), scope_(scope) {}
  Api::Api& api() override { return api_; }
  Stats::Scope& scope() override { return scope_; }
  const envoy::api::v2::core::Metadata& listenerMetadata() const override {
    return listener_metadata_;
  }

private:
  Api::Api& api_;
  Stats::Scope& scope_;
  envoy::api::v2::core::Metadata listener_metadata_;
};

class WasmAccessLogConfigTest : public testing::TestWithParam<std::string> {};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmAccessLogConfigTest,
                         testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                             "v8", "wavm"
#elif defined(ENVOY_WASM_V8)
                             "v8"
#elif defined(ENVOY_WASM_WAVM)
                             "wavm"
#endif
                             ));

TEST_P(WasmAccessLogConfigTest, CreateWasmFromEmpty) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::AccessLogInstanceFactory>::getFactory(
          AccessLogNames::get().Wasm);
  ASSERT_NE(factory, nullptr);

  ProtobufTypes::MessagePtr message = factory->createEmptyConfigProto();
  ASSERT_NE(nullptr, message);

  AccessLog::FilterPtr filter;
  NiceMock<Server::Configuration::MockFactoryContext> context;

  AccessLog::InstanceSharedPtr instance;
  EXPECT_THROW_WITH_MESSAGE(
      instance = factory->createAccessLogInstance(*message, std::move(filter), context),
      Common::Wasm::WasmVmException, "Failed to create WASM VM with unspecified runtime.");
}

TEST_P(WasmAccessLogConfigTest, CreateWasmFromWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::AccessLogInstanceFactory>::getFactory(
          AccessLogNames::get().Wasm);
  ASSERT_NE(factory, nullptr);

  envoy::config::accesslog::v2::WasmAccessLog config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/access_loggers/wasm/test_data/logging.wasm"));

  AccessLog::FilterPtr filter;
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  TestFactoryContext context(*api, stats_store);

  AccessLog::InstanceSharedPtr instance =
      factory->createAccessLogInstance(config, std::move(filter), context);
  EXPECT_NE(nullptr, instance);
  EXPECT_NE(nullptr, dynamic_cast<WasmAccessLog*>(instance.get()));
}

} // namespace Wasm
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
