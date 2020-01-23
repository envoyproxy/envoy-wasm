#include "test/integration/integration.h"
#include "test/mocks/http/stream_encoder.h"
#include "test/server/utility.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::ReturnRef;

namespace Envoy {
namespace {

class ApiListenerIntegrationTest : public BaseIntegrationTest,
                                   public testing::TestWithParam<Network::Address::IpVersion> {
public:
  ApiListenerIntegrationTest() : BaseIntegrationTest(GetParam(), bootstrap_config()) {
    use_lds_ = false;
    autonomous_upstream_ = true;
  }

  void SetUp() override {
    config_helper_.addConfigModifier([](envoy::config::bootstrap::v3::Bootstrap& bootstrap) {
      // currently ApiListener does not trigger this wait
      // https://github.com/envoyproxy/envoy/blob/0b92c58d08d28ba7ef0ed5aaf44f90f0fccc5dce/test/integration/integration.cc#L454
      // Thus, the ApiListener has to be added in addition to the already existing listener in the
      // config.
      bootstrap.mutable_static_resources()->add_listeners()->MergeFrom(
          Server::parseListenerFromV2Yaml(api_listener_config()));
    });
    BaseIntegrationTest::initialize();
  }

  void TearDown() override {
    test_server_.reset();
    fake_upstreams_.clear();
  }

  static std::string bootstrap_config() {
    // At least one empty filter chain needs to be specified.
    return ConfigHelper::BASE_CONFIG + R"EOF(
    filter_chains:
      filters:
    )EOF";
  }

  static std::string api_listener_config() {
    return R"EOF(
name: api_listener
address:
  socket_address:
    address: 127.0.0.1
    port_value: 1
api_listener:
  api_listener:
    "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
    stat_prefix: hcm
    route_config:
      virtual_hosts:
        name: integration
        routes:
          route:
            cluster: cluster_0
          match:
            prefix: "/"
        domains: "*"
      name: route_config_0
    http_filters:
      - name: envoy.router
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.http.router.v3.Router
      )EOF";
  }

  NiceMock<Http::MockStreamEncoder> stream_encoder_;
};

INSTANTIATE_TEST_SUITE_P(IpVersions, ApiListenerIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(ApiListenerIntegrationTest, Basic) {
  ConditionalInitializer test_ran;
  test_server_->server().dispatcher().post([this, &test_ran]() -> void {
    ASSERT_TRUE(test_server_->server().listenerManager().apiListener().has_value());
    ASSERT_EQ("api_listener", test_server_->server().listenerManager().apiListener()->get().name());
    ASSERT_TRUE(test_server_->server().listenerManager().apiListener()->get().http().has_value());
    auto& http_api_listener =
        test_server_->server().listenerManager().apiListener()->get().http()->get();

    ON_CALL(stream_encoder_, getStream()).WillByDefault(ReturnRef(stream_encoder_.stream_));
    auto& stream_decoder = http_api_listener.newStream(stream_encoder_);

    // The AutonomousUpstream responds with 200 OK and a body of 10 bytes.
    // In the http1 codec the end stream is encoded with encodeData and 0 bytes.
    Http::TestHeaderMapImpl expected_response_headers{{":status", "200"}};
    EXPECT_CALL(stream_encoder_, encodeHeaders(_, false));
    EXPECT_CALL(stream_encoder_, encodeData(_, false));
    EXPECT_CALL(stream_encoder_, encodeData(BufferStringEqual(""), true));

    // Send a headers-only request
    stream_decoder.decodeHeaders(
        Http::HeaderMapPtr(new Http::TestHeaderMapImpl{
            {":method", "GET"}, {":path", "/api"}, {":scheme", "http"}, {":authority", "host"}}),
        true);

    test_ran.setReady();
  });
  test_ran.waitReady();
}

} // namespace
} // namespace Envoy