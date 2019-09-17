#include "envoy/config/filter/thrift/rate_limit/v2alpha1/rate_limit.pb.validate.h"

#include "extensions/filters/network/thrift_proxy/filters/ratelimit/config.h"

#include "test/extensions/filters/network/thrift_proxy/mocks.h"
#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;

namespace Envoy {
namespace Extensions {
namespace ThriftFilters {
namespace RateLimitFilter {
namespace {

envoy::config::filter::thrift::rate_limit::v2alpha1::RateLimit
parseRateLimitFromV2Yaml(const std::string& yaml) {
  envoy::config::filter::thrift::rate_limit::v2alpha1::RateLimit rate_limit;
  TestUtility::loadFromYaml(yaml, rate_limit);
  return rate_limit;
}

} // namespace

TEST(RateLimitFilterConfigTest, ValidateFail) {
  NiceMock<Server::Configuration::MockFactoryContext> context;
  EXPECT_THROW(
      RateLimitFilterConfig().createFilterFactoryFromProto(
          envoy::config::filter::thrift::rate_limit::v2alpha1::RateLimit(), "stats", context),
      ProtoValidationException);
}

TEST(RateLimitFilterConfigTest, RateLimitFilterCorrectProto) {
  const std::string yaml_string = R"EOF(
domain: "test"
timeout: "1.337s"
rate_limit_service:
  grpc_service:
    envoy_grpc:
      cluster_name: ratelimit_cluster
  )EOF";

  auto proto_config = parseRateLimitFromV2Yaml(yaml_string);

  NiceMock<Server::Configuration::MockFactoryContext> context;

  EXPECT_CALL(context.cluster_manager_.async_client_manager_, factoryForGrpcService(_, _, _))
      .WillOnce(Invoke([](const envoy::api::v2::core::GrpcService&, Stats::Scope&, bool) {
        return std::make_unique<NiceMock<Grpc::MockAsyncClientFactory>>();
      }));

  RateLimitFilterConfig factory;
  auto cb = factory.createFilterFactoryFromProto(proto_config, "stats", context);
  NetworkFilters::ThriftProxy::ThriftFilters::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addDecoderFilter(_));
  cb(filter_callback);
}

} // namespace RateLimitFilter
} // namespace ThriftFilters
} // namespace Extensions
} // namespace Envoy
