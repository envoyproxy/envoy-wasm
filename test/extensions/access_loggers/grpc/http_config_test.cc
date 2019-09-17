#include "envoy/registry/registry.h"
#include "envoy/server/access_log_config.h"
#include "envoy/stats/scope.h"

#include "extensions/access_loggers/grpc/http_grpc_access_log_impl.h"
#include "extensions/access_loggers/well_known_names.h"

#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace HttpGrpc {
namespace {

class HttpGrpcAccessLogConfigTest : public testing::Test {
public:
  void SetUp() override {
    factory_ =
        Registry::FactoryRegistry<Server::Configuration::AccessLogInstanceFactory>::getFactory(
            AccessLogNames::get().HttpGrpc);
    ASSERT_NE(nullptr, factory_);

    message_ = factory_->createEmptyConfigProto();
    ASSERT_NE(nullptr, message_);

    EXPECT_CALL(context_.cluster_manager_.async_client_manager_, factoryForGrpcService(_, _, _))
        .WillOnce(Invoke([](const envoy::api::v2::core::GrpcService&, Stats::Scope&, bool) {
          return std::make_unique<NiceMock<Grpc::MockAsyncClientFactory>>();
        }));

    auto* common_config = http_grpc_access_log_.mutable_common_config();
    common_config->set_log_name("foo");
    common_config->mutable_grpc_service()->mutable_envoy_grpc()->set_cluster_name("bar");
    TestUtility::jsonConvert(http_grpc_access_log_, *message_);
  }

  AccessLog::FilterPtr filter_;
  NiceMock<Server::Configuration::MockFactoryContext> context_;
  envoy::config::accesslog::v2::HttpGrpcAccessLogConfig http_grpc_access_log_;
  ProtobufTypes::MessagePtr message_;
  Server::Configuration::AccessLogInstanceFactory* factory_{};
};

// Normal OK configuration.
TEST_F(HttpGrpcAccessLogConfigTest, Ok) {
  AccessLog::InstanceSharedPtr instance =
      factory_->createAccessLogInstance(*message_, std::move(filter_), context_);
  EXPECT_NE(nullptr, instance);
  EXPECT_NE(nullptr, dynamic_cast<HttpGrpcAccessLog*>(instance.get()));
}

} // namespace
} // namespace HttpGrpc
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
