#include "envoy/config/filter/http/wasm/v2/wasm.pb.validate.h"

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

TEST(WasmFilterConfigTest, WasmFilterInJson) {
  const std::string json = TestEnvironment::substitute(R"EOF(
  {
    "vm" : "envoy.wasm.vm.wavm",
    "file" : "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers.wasm",
    "configuration" : "",
    "allow_precompiled" : true
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

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
