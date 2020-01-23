#include "extensions/filters/http/grpc_web/config.h"

#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace GrpcWeb {
namespace {

TEST(GrpcWebFilterConfigTest, GrpcWebFilter) {
  NiceMock<Server::Configuration::MockFactoryContext> context;
  GrpcWebFilterConfig factory;
  envoy::extensions::filters::http::grpc_web::v3::GrpcWeb config;
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(config, "stats", context);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  cb(filter_callback);
}

} // namespace
} // namespace GrpcWeb
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
