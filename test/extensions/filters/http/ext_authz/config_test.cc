#include "envoy/config/filter/http/ext_authz/v2/ext_authz.pb.validate.h"
#include "envoy/stats/scope.h"

#include "extensions/filters/http/ext_authz/config.h"

#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace ExtAuthz {
namespace {

TEST(HttpExtAuthzConfigTest, CorrectProtoGrpc) {
  std::string yaml = R"EOF(
  grpc_service:
    google_grpc:
      target_uri: ext_authz_server
      stat_prefix: google
  failure_mode_allow: false
  )EOF";

  ExtAuthzFilterConfig factory;
  ProtobufTypes::MessagePtr proto_config = factory.createEmptyConfigProto();
  TestUtility::loadFromYaml(yaml, *proto_config);

  testing::StrictMock<Server::Configuration::MockFactoryContext> context;
  EXPECT_CALL(context, messageValidationVisitor()).Times(1);
  EXPECT_CALL(context, localInfo()).Times(1);
  EXPECT_CALL(context, clusterManager()).Times(1);
  EXPECT_CALL(context, runtime()).Times(1);
  EXPECT_CALL(context, scope()).Times(2);
  EXPECT_CALL(context.cluster_manager_.async_client_manager_, factoryForGrpcService(_, _, _))
      .WillOnce(Invoke([](const envoy::api::v2::core::GrpcService&, Stats::Scope&, bool) {
        return std::make_unique<NiceMock<Grpc::MockAsyncClientFactory>>();
      }));
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(*proto_config, "stats", context);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamDecoderFilter(_));
  cb(filter_callback);
}

TEST(HttpExtAuthzConfigTest, CorrectProtoHttp) {
  std::string yaml = R"EOF(
  http_service:
    server_uri:
      uri: "ext_authz:9000"
      cluster: "ext_authz"
      timeout: 0.25s
   
    authorization_request: 
      allowed_headers: 
        patterns: 
        - exact: baz
        - prefix: x-
      headers_to_add:
      - key: foo
        value: bar
      - key: bar
        value: foo
    
    authorization_response: 
      allowed_upstream_headers: 
        patterns: 
        - exact: baz
        - prefix: x-success
      allowed_client_headers: 
        patterns: 
        - exact: baz
        - prefix: x-fail

    path_prefix: /extauth
    
  failure_mode_allow: true
  with_request_body:
    max_request_bytes: 100
  )EOF";

  ExtAuthzFilterConfig factory;
  ProtobufTypes::MessagePtr proto_config = factory.createEmptyConfigProto();
  TestUtility::loadFromYaml(yaml, *proto_config);
  testing::StrictMock<Server::Configuration::MockFactoryContext> context;
  EXPECT_CALL(context, messageValidationVisitor()).Times(1);
  EXPECT_CALL(context, localInfo()).Times(1);
  EXPECT_CALL(context, clusterManager()).Times(1);
  EXPECT_CALL(context, runtime()).Times(1);
  EXPECT_CALL(context, scope()).Times(1);
  EXPECT_CALL(context, timeSource()).Times(1);
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(*proto_config, "stats", context);
  testing::StrictMock<Http::MockFilterChainFactoryCallbacks> filter_callback;
  EXPECT_CALL(filter_callback, addStreamDecoderFilter(_));
  cb(filter_callback);
}

} // namespace
} // namespace ExtAuthz
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
