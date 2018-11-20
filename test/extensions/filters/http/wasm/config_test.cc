#include "envoy/config/filter/http/wasm/v2/wasm.pb.validate.h"

#include "common/common/base64.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/wasm/config.h"

#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

TEST(WasmFilterConfigTest, JsonLoadFromFileWASM) {
  const std::string json = TestEnvironment::substitute(R"EOF(
  {
    "vm": "envoy.wasm.vm.wavm",
    "code": {
      "filename": "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers.wasm"
    },
    "allow_precompiled": true
  }
  )EOF");

  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  MessageUtil::loadFromJson(json, proto_config);
  NiceMock<Server::Configuration::MockFactoryContext> context;
  WasmFilterConfig factory;
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  cb(filter_callback);
}

TEST(WasmFilterConfigTest, YamlLoadFromFileWASM) {
  const std::string yaml = TestEnvironment::substitute(R"EOF(
    vm: "envoy.wasm.vm.wavm"
    code: { filename: "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers.wasm" }
  )EOF");

  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  MessageUtil::loadFromYaml(yaml, proto_config);
  NiceMock<Server::Configuration::MockFactoryContext> context;
  WasmFilterConfig factory;
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  cb(filter_callback);
}

TEST(WasmFilterConfigTest, YamlLoadInlineWASM) {
  const std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers.wasm"));
  EXPECT_FALSE(code.empty());
  const std::string yaml = absl::StrCat("vm: \"envoy.wasm.vm.wavm\"\n", "code: { inline_bytes: \"",
                                        Base64::encode(code.data(), code.size()), "\" }\n");

  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  MessageUtil::loadFromYaml(yaml, proto_config);
  NiceMock<Server::Configuration::MockFactoryContext> context;
  WasmFilterConfig factory;
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  cb(filter_callback);
}

TEST(WasmFilterConfigTest, YamlLoadInlineBadCode) {
  const std::string yaml = R"EOF(
    vm: "envoy.wasm.vm.wavm"
    code: { inline_string: "bad code" }
  )EOF";

  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  MessageUtil::loadFromYaml(yaml, proto_config);
  NiceMock<Server::Configuration::MockFactoryContext> context;
  WasmFilterConfig factory;
  EXPECT_THROW_WITH_MESSAGE(factory.createFilterFactoryFromProto(proto_config, "stats", context),
                            Extensions::Common::Wasm::WasmException,
                            "unable to initialize WASM vm");
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
