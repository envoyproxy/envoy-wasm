#include "envoy/config/bootstrap/v2/bootstrap.pb.h"
#include "envoy/registry/registry.h"

#include "common/config/well_known_names.h"
#include "common/protobuf/utility.h"

#include "extensions/stat_sinks/common/statsd/statsd.h"
#include "extensions/stat_sinks/dog_statsd/config.h"
#include "extensions/stat_sinks/well_known_names.h"

#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::NiceMock;

namespace Envoy {
namespace Extensions {
namespace StatSinks {
namespace DogStatsd {
namespace {

class DogStatsdConfigLoopbackTest : public testing::TestWithParam<Network::Address::IpVersion> {};
INSTANTIATE_TEST_SUITE_P(IpVersions, DogStatsdConfigLoopbackTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(DogStatsdConfigLoopbackTest, ValidUdpIp) {
  const std::string name = StatsSinkNames::get().DogStatsd;

  envoy::config::metrics::v2::DogStatsdSink sink_config;
  envoy::api::v2::core::Address& address = *sink_config.mutable_address();
  envoy::api::v2::core::SocketAddress& socket_address = *address.mutable_socket_address();
  socket_address.set_protocol(envoy::api::v2::core::SocketAddress::UDP);
  auto loopback_flavor = Network::Test::getCanonicalLoopbackAddress(GetParam());
  socket_address.set_address(loopback_flavor->ip()->addressAsString());
  socket_address.set_port_value(8125);

  Server::Configuration::StatsSinkFactory* factory =
      Registry::FactoryRegistry<Server::Configuration::StatsSinkFactory>::getFactory(name);
  ASSERT_NE(factory, nullptr);

  ProtobufTypes::MessagePtr message = factory->createEmptyConfigProto();
  TestUtility::jsonConvert(sink_config, *message);

  NiceMock<Server::MockInstance> server;
  Stats::SinkPtr sink = factory->createStatsSink(*message, server);
  EXPECT_NE(sink, nullptr);
  auto udp_sink = dynamic_cast<Common::Statsd::UdpStatsdSink*>(sink.get());
  EXPECT_NE(udp_sink, nullptr);
  EXPECT_EQ(udp_sink->getUseTagForTest(), true);
  EXPECT_EQ(udp_sink->getPrefix(), Common::Statsd::getDefaultPrefix());
}

// Negative test for protoc-gen-validate constraints for dog_statsd.
TEST(DogStatsdConfigTest, ValidateFail) {
  NiceMock<Server::MockInstance> server;
  EXPECT_THROW(
      DogStatsdSinkFactory().createStatsSink(envoy::config::metrics::v2::DogStatsdSink(), server),
      ProtoValidationException);
}

TEST_P(DogStatsdConfigLoopbackTest, WithCustomPrefix) {
  const std::string name = StatsSinkNames::get().DogStatsd;

  envoy::config::metrics::v2::DogStatsdSink sink_config;
  envoy::api::v2::core::Address& address = *sink_config.mutable_address();
  envoy::api::v2::core::SocketAddress& socket_address = *address.mutable_socket_address();
  socket_address.set_protocol(envoy::api::v2::core::SocketAddress::UDP);
  auto loopback_flavor = Network::Test::getCanonicalLoopbackAddress(GetParam());
  socket_address.set_address(loopback_flavor->ip()->addressAsString());
  socket_address.set_port_value(8125);

  const std::string customPrefix = "prefix.test";
  sink_config.set_prefix(customPrefix);

  Server::Configuration::StatsSinkFactory* factory =
      Registry::FactoryRegistry<Server::Configuration::StatsSinkFactory>::getFactory(name);
  ASSERT_NE(factory, nullptr);

  ProtobufTypes::MessagePtr message = factory->createEmptyConfigProto();
  TestUtility::jsonConvert(sink_config, *message);

  NiceMock<Server::MockInstance> server;
  Stats::SinkPtr sink = factory->createStatsSink(*message, server);
  ASSERT_NE(sink, nullptr);
  auto udp_sink = dynamic_cast<Common::Statsd::UdpStatsdSink*>(sink.get());
  ASSERT_NE(udp_sink, nullptr);
  EXPECT_EQ(udp_sink->getPrefix(), customPrefix);
}

} // namespace
} // namespace DogStatsd
} // namespace StatSinks
} // namespace Extensions
} // namespace Envoy
