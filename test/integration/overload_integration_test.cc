#include "test/integration/http_protocol_integration.h"

#include "absl/strings/str_cat.h"

namespace Envoy {

class OverloadIntegrationTest : public HttpProtocolIntegrationTest {
protected:
  OverloadIntegrationTest()
      : injected_resource_filename_(TestEnvironment::temporaryPath("injected_resource")),
        file_updater_(injected_resource_filename_) {}

  void initialize() override {
    config_helper_.addConfigModifier([this](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
      const std::string overload_config = fmt::format(R"EOF(
        refresh_interval:
          seconds: 0
          nanos: 1000000
        resource_monitors:
          - name: "envoy.resource_monitors.injected_resource"
            typed_config:
              "@type": type.googleapis.com/envoy.config.resource_monitor.injected_resource.v2alpha.InjectedResourceConfig
              filename: "{}"
        actions:
          - name: "envoy.overload_actions.stop_accepting_requests"
            triggers:
              - name: "envoy.resource_monitors.injected_resource"
                threshold:
                  value: 0.9
          - name: "envoy.overload_actions.disable_http_keepalive"
            triggers:
              - name: "envoy.resource_monitors.injected_resource"
                threshold:
                  value: 0.8
          - name: "envoy.overload_actions.stop_accepting_connections"
            triggers:
              - name: "envoy.resource_monitors.injected_resource"
                threshold:
                  value: 0.95
      )EOF",
                                                      injected_resource_filename_);
      *bootstrap.mutable_overload_manager() =
          TestUtility::parseYaml<envoy::config::overload::v2alpha::OverloadManager>(
              overload_config);
    });
    updateResource(0);
    HttpIntegrationTest::initialize();
  }

  void updateResource(double pressure) { file_updater_.update(absl::StrCat(pressure)); }

  const std::string injected_resource_filename_;
  AtomicFileUpdater file_updater_;
};

INSTANTIATE_TEST_SUITE_P(Protocols, OverloadIntegrationTest,
                         testing::ValuesIn(HttpProtocolIntegrationTest::getProtocolTestParams()),
                         HttpProtocolIntegrationTest::protocolTestParamsToString);

TEST_P(OverloadIntegrationTest, CloseStreamsWhenOverloaded) {
  initialize();
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  // Put envoy in overloaded state and check that it drops new requests.
  // Test both header-only and header+body requests since the code paths are slightly different.
  updateResource(0.9);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.stop_accepting_requests.active", 1);

  Http::TestHeaderMapImpl request_headers{
      {":method", "GET"}, {":path", "/test/long/url"}, {":scheme", "http"}, {":authority", "host"}};
  codec_client_ = makeHttpConnection(makeClientConnection((lookupPort("http"))));
  auto response = codec_client_->makeRequestWithBody(request_headers, 10);
  response->waitForEndStream();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ("envoy overloaded", response->body());
  codec_client_->close();

  codec_client_ = makeHttpConnection(makeClientConnection((lookupPort("http"))));
  response = codec_client_->makeHeaderOnlyRequest(request_headers);
  response->waitForEndStream();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ("envoy overloaded", response->body());
  codec_client_->close();

  // Deactivate overload state and check that new requests are accepted.
  updateResource(0.8);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.stop_accepting_requests.active", 0);

  codec_client_ = makeHttpConnection(makeClientConnection((lookupPort("http"))));
  response = sendRequestAndWaitForResponse(request_headers, 0, default_response_headers_, 0);

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(0U, response->body().size());
}

TEST_P(OverloadIntegrationTest, DisableKeepaliveWhenOverloaded) {
  if (downstreamProtocol() != Http::CodecClient::Type::HTTP1) {
    return; // only relevant for downstream HTTP1.x connections
  }

  initialize();
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  // Put envoy in overloaded state and check that it disables keepalive
  updateResource(0.8);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.disable_http_keepalive.active", 1);

  codec_client_ = makeHttpConnection(makeClientConnection((lookupPort("http"))));
  Http::TestHeaderMapImpl request_headers{
      {":method", "GET"}, {":path", "/test/long/url"}, {":scheme", "http"}, {":authority", "host"}};
  auto response = sendRequestAndWaitForResponse(request_headers, 1, default_response_headers_, 1);
  codec_client_->waitForDisconnect();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ("close", response->headers().Connection()->value().getStringView());

  // Deactivate overload state and check that keepalive is not disabled
  updateResource(0.7);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.disable_http_keepalive.active", 0);

  codec_client_ = makeHttpConnection(makeClientConnection((lookupPort("http"))));
  response = sendRequestAndWaitForResponse(request_headers, 1, default_response_headers_, 1);

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(nullptr, response->headers().Connection());
}

TEST_P(OverloadIntegrationTest, StopAcceptingConnectionsWhenOverloaded) {
  initialize();
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  // Put envoy in overloaded state and check that it doesn't accept the new client connection.
  updateResource(0.95);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.stop_accepting_connections.active",
                               1);
  codec_client_ = makeHttpConnection(makeClientConnection((lookupPort("http"))));
  Http::TestHeaderMapImpl request_headers{
      {":method", "GET"}, {":path", "/test/long/url"}, {":scheme", "http"}, {":authority", "host"}};
  auto response = codec_client_->makeRequestWithBody(request_headers, 10);
  EXPECT_FALSE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_,
                                                         std::chrono::milliseconds(1000)));

  // Reduce load a little to allow the connection to be accepted but then immediately reject the
  // request.
  updateResource(0.9);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.stop_accepting_connections.active",
                               0);
  response->waitForEndStream();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ("envoy overloaded", response->body());
  codec_client_->close();
}

} // namespace Envoy
