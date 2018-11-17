#include "envoy/config/filter/http/wasm/v2/wasm.pb.validate.h"

#include "extensions/filters/http/wasm/config.h"

#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

TEST(WasmFilterConfigTest, WasmFilterInJson) {
  std::string json_string = R"EOF(
  {
    "vm" : "envoy.wasm.vm.wavm",
    "file" : "test.wasm",
    "configuration" : "",
    "allow_precompiled" : true
  }
  )EOF";

  Json::ObjectSharedPtr json_config = Json::Factory::loadFromString(json_string);
  NiceMock<Server::Configuration::MockFactoryContext> context;
  WasmFilterConfig factory;
  Http::FilterFactoryCb cb = factory.createFilterFactory(*json_config, "stats", context);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  cb(filter_callback);
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
