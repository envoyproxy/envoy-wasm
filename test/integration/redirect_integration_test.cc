#include "envoy/config/route/v3alpha/route_components.pb.h"
#include "envoy/extensions/filters/network/http_connection_manager/v3alpha/http_connection_manager.pb.h"

#include "test/integration/http_protocol_integration.h"

namespace Envoy {

class RedirectIntegrationTest : public HttpProtocolIntegrationTest {
public:
  void initialize() override {
    envoy::config::route::v3alpha::RetryPolicy retry_policy;

    auto pass_through = config_helper_.createVirtualHost("pass.through.internal.redirect");
    config_helper_.addVirtualHost(pass_through);

    auto handle = config_helper_.createVirtualHost("handle.internal.redirect");
    handle.mutable_routes(0)->mutable_route()->set_internal_redirect_action(
        envoy::config::route::v3alpha::RouteAction::HANDLE_INTERNAL_REDIRECT);
    config_helper_.addVirtualHost(handle);

    HttpProtocolIntegrationTest::initialize();
  }

protected:
  Http::TestHeaderMapImpl redirect_response_{
      {":status", "302"}, {"content-length", "0"}, {"location", "http://authority2/new/url"}};
};

// By default if internal redirects are not configured, redirects are proxied.
TEST_P(RedirectIntegrationTest, RedirectNotConfigured) {
  // Use base class initialize.
  HttpProtocolIntegrationTest::initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = sendRequestAndWaitForResponse(default_request_headers_, 0, redirect_response_, 0);
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("302", response->headers().Status()->value().getStringView());
}

// Now test a route with redirects configured on in pass-through mode.
TEST_P(RedirectIntegrationTest, InternalRedirectPassedThrough) {
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  default_request_headers_.setHost("pass.through.internal.redirect");
  auto response = sendRequestAndWaitForResponse(default_request_headers_, 0, redirect_response_, 0);
  EXPECT_EQ("302", response->headers().Status()->value().getStringView());
  EXPECT_EQ(
      0,
      test_server_->counter("cluster.cluster_0.upstream_internal_redirect_failed_total")->value());
}

TEST_P(RedirectIntegrationTest, BasicInternalRedirect) {
  // Validate that header sanitization is only called once.
  config_helper_.addConfigModifier(
      [](envoy::extensions::filters::network::http_connection_manager::v3alpha::
             HttpConnectionManager& hcm) { hcm.set_via("via_value"); });
  initialize();
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  codec_client_ = makeHttpConnection(lookupPort("http"));

  default_request_headers_.setHost("handle.internal.redirect");
  IntegrationStreamDecoderPtr response =
      codec_client_->makeHeaderOnlyRequest(default_request_headers_);

  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(redirect_response_, true);

  waitForNextUpstreamRequest();
  ASSERT(upstream_request_->headers().EnvoyOriginalUrl() != nullptr);
  EXPECT_EQ("http://handle.internal.redirect/test/long/url",
            upstream_request_->headers().EnvoyOriginalUrl()->value().getStringView());
  EXPECT_EQ("/new/url", upstream_request_->headers().Path()->value().getStringView());
  EXPECT_EQ("authority2", upstream_request_->headers().Host()->value().getStringView());
  EXPECT_EQ("via_value", upstream_request_->headers().Via()->value().getStringView());

  upstream_request_->encodeHeaders(default_response_headers_, true);

  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(1, test_server_->counter("cluster.cluster_0.upstream_internal_redirect_succeeded_total")
                   ->value());
}

TEST_P(RedirectIntegrationTest, InternalRedirectToDestinationWithBody) {
  // Validate that header sanitization is only called once.
  config_helper_.addConfigModifier(
      [](envoy::extensions::filters::network::http_connection_manager::v3alpha::
             HttpConnectionManager& hcm) { hcm.set_via("via_value"); });
  config_helper_.addFilter(R"EOF(
  name: pause-filter
  typed_config:
    "@type": type.googleapis.com/google.protobuf.Empty
  )EOF");
  initialize();
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  codec_client_ = makeHttpConnection(lookupPort("http"));

  default_request_headers_.setHost("handle.internal.redirect");
  IntegrationStreamDecoderPtr response =
      codec_client_->makeHeaderOnlyRequest(default_request_headers_);

  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(redirect_response_, true);

  waitForNextUpstreamRequest();
  ASSERT(upstream_request_->headers().EnvoyOriginalUrl() != nullptr);
  EXPECT_EQ("http://handle.internal.redirect/test/long/url",
            upstream_request_->headers().EnvoyOriginalUrl()->value().getStringView());
  EXPECT_EQ("/new/url", upstream_request_->headers().Path()->value().getStringView());
  EXPECT_EQ("authority2", upstream_request_->headers().Host()->value().getStringView());
  EXPECT_EQ("via_value", upstream_request_->headers().Via()->value().getStringView());

  Http::TestHeaderMapImpl response_with_big_body(
      {{":status", "200"}, {"content-length", "2000000"}});
  upstream_request_->encodeHeaders(response_with_big_body, false);
  upstream_request_->encodeData(2000000, true);

  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(1, test_server_->counter("cluster.cluster_0.upstream_internal_redirect_succeeded_total")
                   ->value());
}

TEST_P(RedirectIntegrationTest, InvalidRedirect) {
  initialize();

  redirect_response_.setLocation("invalid_url");

  // Send the same request as above, only send an invalid URL as the response.
  // The request should not be redirected.
  codec_client_ = makeHttpConnection(lookupPort("http"));
  default_request_headers_.setHost("handle.internal.redirect");
  auto response = sendRequestAndWaitForResponse(default_request_headers_, 0, redirect_response_, 0);
  EXPECT_EQ("302", response->headers().Status()->value().getStringView());
  EXPECT_EQ(
      1,
      test_server_->counter("cluster.cluster_0.upstream_internal_redirect_failed_total")->value());
}

INSTANTIATE_TEST_SUITE_P(Protocols, RedirectIntegrationTest,
                         testing::ValuesIn(HttpProtocolIntegrationTest::getProtocolTestParams()),
                         HttpProtocolIntegrationTest::protocolTestParamsToString);

} // namespace Envoy
