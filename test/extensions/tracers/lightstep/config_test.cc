#include "extensions/tracers/lightstep/config.h"

#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Eq;
using testing::NiceMock;
using testing::Return;

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace Lightstep {
namespace {

TEST(LightstepTracerConfigTest, LightstepHttpTracer) {
  NiceMock<Server::MockInstance> server;
  EXPECT_CALL(server.cluster_manager_, get(Eq("fake_cluster")))
      .WillRepeatedly(Return(&server.cluster_manager_.thread_local_cluster_));
  ON_CALL(*server.cluster_manager_.thread_local_cluster_.cluster_.info_, features())
      .WillByDefault(Return(Upstream::ClusterInfo::Features::HTTP2));

  const std::string yaml_string = R"EOF(
  http:
    name: envoy.lightstep
    config:
      collector_cluster: fake_cluster
      access_token_file: fake_file
   )EOF";
  envoy::config::trace::v2::Tracing configuration;
  TestUtility::loadFromYaml(yaml_string, configuration);

  LightstepTracerFactory factory;
  auto message = Config::Utility::translateToFactoryConfig(
      configuration.http(), ProtobufMessage::getStrictValidationVisitor(), factory);
  Tracing::HttpTracerPtr lightstep_tracer = factory.createHttpTracer(*message, server);
  EXPECT_NE(nullptr, lightstep_tracer);
}

} // namespace
} // namespace Lightstep
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
