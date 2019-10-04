#include "envoy/config/filter/network/wasm/v2/wasm.pb.validate.h"

#include "common/common/base64.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/network/wasm/config.h"

#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::ReturnRef;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Wasm {

class WasmFilterConfigTest : public testing::TestWithParam<std::string> {
protected:
  WasmFilterConfigTest() : api_(Api::createApiForTest(stats_store_)) {
    ON_CALL(context_, api()).WillByDefault(ReturnRef(*api_));
    ON_CALL(context_, scope()).WillByDefault(ReturnRef(stats_store_));
    ON_CALL(context_, listenerMetadata()).WillByDefault(ReturnRef(listener_metadata_));
  }

  NiceMock<Server::Configuration::MockFactoryContext> context_;
  Stats::IsolatedStoreImpl stats_store_;
  Api::ApiPtr api_;
  envoy::api::v2::core::Metadata listener_metadata_;
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmFilterConfigTest,
                         testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                             "v8", "wavm"
#elif defined(ENVOY_WASM_V8)
                             "v8"
#elif defined(ENVOY_WASM_WAVM)
                             "wavm"
#endif
                             ));

TEST_P(WasmFilterConfigTest, YamlLoadFromFileWASM) {
  const std::string yaml = TestEnvironment::substitute(absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF("
      code: { filename: "{{ test_rundir }}/test/extensions/filters/network/wasm/test_data/logging_cpp.wasm" }
  )EOF"));

  envoy::config::filter::network::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;
  Network::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, context_);
  Network::MockConnection connection;
  EXPECT_CALL(connection, addFilter(_));
  cb(connection);
}

TEST_P(WasmFilterConfigTest, YamlLoadInlineWASM) {
  const std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/network/wasm/test_data/logging_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  const std::string yaml = absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                        GetParam(), R"EOF("
      code: { inline_bytes: ")EOF",
                                        Base64::encode(code.data(), code.size()), R"EOF(" }
  )EOF");

  envoy::config::filter::network::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;
  Network::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, context_);
  Network::MockConnection connection;
  EXPECT_CALL(connection, addFilter(_));
  cb(connection);
}

TEST_P(WasmFilterConfigTest, YamlLoadInlineBadCode) {
  const std::string yaml = absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                        GetParam(), R"EOF("
      code: { inline_string: "bad code" }
  )EOF");

  envoy::config::filter::network::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;
  EXPECT_THROW_WITH_MESSAGE(factory.createFilterFactoryFromProto(proto_config, context_),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to initialize WASM code from <inline>");
}

} // namespace Wasm
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
