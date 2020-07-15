#include "envoy/extensions/stat_sinks/wasm/v3/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/protobuf/protobuf.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/stat_sinks/wasm/config.h"
#include "extensions/stat_sinks/wasm/wasm_stat_sink_impl.h"
#include "extensions/stat_sinks/well_known_names.h"

#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace StatSinks {
namespace Wasm {

class TestFactoryContext : public NiceMock<Server::Configuration::MockFactoryContext> {
public:
  TestFactoryContext(Api::Api& api, Stats::Scope& scope) : api_(api), scope_(scope) {}
  Api::Api& api() override { return api_; }
  Stats::Scope& scope() override { return scope_; }
  const envoy::config::core::v3::Metadata& listenerMetadata() const override {
    return listener_metadata_;
  }

private:
  Api::Api& api_;
  Stats::Scope& scope_;
  envoy::config::core::v3::Metadata listener_metadata_;
};

class WasmStatSinkConfigTest : public testing::TestWithParam<std::string> {};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmStatSinkConfigTest,
                         testing::Values("v8"
#if defined(ENVOY_WASM_WAVM)
                                         ,
                                         "wavm"
#endif
                                         ));

TEST_P(WasmStatSinkConfigTest, CreateWasmFromEmpty) {
  auto factory = Registry::FactoryRegistry<Server::Configuration::StatsSinkFactory>::getFactory(
      StatsSinkNames::get().Wasm);
  ASSERT_NE(factory, nullptr);

  ProtobufTypes::MessagePtr message = factory->createEmptyConfigProto();
  ASSERT_NE(nullptr, message);

  NiceMock<Server::Configuration::MockFactoryContext> context;

  AccessLog::InstanceSharedPtr instance;
  EXPECT_THROW_WITH_MESSAGE(instance = factory->createStatsSink(*message, context),
                            Common::Wasm::WasmException, "Unable to create Wasm access log ");
}

TEST_P(WasmStatSinkConfigTest, CreateWasmFromWASM) {
  auto factory = Registry::FactoryRegistry<Server::Configuration::StatsSinkFactory>::getFactory(
      StatsSinkNames::get().Wasm);
  ASSERT_NE(factory, nullptr);

  envoy::extensions::stat_sinks::wasm::v3::Wasm config;
  config.mutable_config()->mutable_vm_config()->set_runtime(
      absl::StrCat("envoy.wasm.runtime.", GetParam()));
  config.mutable_config()->mutable_vm_config()->mutable_code()->mutable_local()->set_filename(
      TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/stats_sinks/wasm/test_data/test_context_cpp.wasm"));

  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  TestFactoryContext context(*api, stats_store);

  AccessLog::InstanceSharedPtr instance = factory->createStatsSink(config, context);
  EXPECT_NE(nullptr, instance);
  EXPECT_NE(nullptr, dynamic_cast<WasmStatSink*>(instance.get()));
}

} // namespace Wasm
} // namespace StatSinks
} // namespace Extensions
} // namespace Envoy
