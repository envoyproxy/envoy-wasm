#include "test/integration/integration_test.h"

#include <string>

#include "envoy/config/accesslog/v2/file.pb.h"

#include "common/http/header_map_impl.h"
#include "common/http/headers.h"
#include "common/network/utility.h"
#include "common/protobuf/utility.h"

#include "test/integration/autonomous_upstream.h"
#include "test/integration/filters/process_context_filter.h"
#include "test/integration/utility.h"
#include "test/mocks/http/mocks.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

using Envoy::Http::Headers;
using Envoy::Http::HeaderValueOf;
using Envoy::Http::HttpStatusIs;
using testing::EndsWith;
using testing::HasSubstr;
using testing::Not;

namespace Envoy {
namespace {

std::string normalizeDate(const std::string& s) {
  const std::regex date_regex("date:[^\r]+");
  return std::regex_replace(s, date_regex, "date: Mon, 01 Jan 2017 00:00:00 GMT");
}

void setDisallowAbsoluteUrl(
    envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
  hcm.mutable_http_protocol_options()->mutable_allow_absolute_url()->set_value(false);
};

void setAllowHttp10WithDefaultHost(
    envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
  hcm.mutable_http_protocol_options()->set_accept_http_10(true);
  hcm.mutable_http_protocol_options()->set_default_host_for_http_10("default.com");
}

} // namespace

INSTANTIATE_TEST_SUITE_P(IpVersions, IntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

// Make sure we have correctly specified per-worker performance stats.
TEST_P(IntegrationTest, PerWorkerStatsAndBalancing) {
  concurrency_ = 2;
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    auto* listener = bootstrap.mutable_static_resources()->mutable_listeners(0);
    listener->mutable_connection_balance_config()->mutable_exact_balance();
  });
  initialize();

  // Per-worker listener stats.
  auto check_listener_stats = [this](uint64_t cx_active, uint64_t cx_total) {
    if (GetParam() == Network::Address::IpVersion::v4) {
      test_server_->waitForGaugeEq("listener.127.0.0.1_0.worker_0.downstream_cx_active", cx_active);
      test_server_->waitForGaugeEq("listener.127.0.0.1_0.worker_1.downstream_cx_active", cx_active);
      test_server_->waitForCounterEq("listener.127.0.0.1_0.worker_0.downstream_cx_total", cx_total);
      test_server_->waitForCounterEq("listener.127.0.0.1_0.worker_1.downstream_cx_total", cx_total);
    } else {
      test_server_->waitForGaugeEq("listener.[__1]_0.worker_0.downstream_cx_active", cx_active);
      test_server_->waitForGaugeEq("listener.[__1]_0.worker_1.downstream_cx_active", cx_active);
      test_server_->waitForCounterEq("listener.[__1]_0.worker_0.downstream_cx_total", cx_total);
      test_server_->waitForCounterEq("listener.[__1]_0.worker_1.downstream_cx_total", cx_total);
    }
  };
  check_listener_stats(0, 0);

  // Main thread admin listener stats.
  EXPECT_NE(nullptr, test_server_->counter("listener.admin.main_thread.downstream_cx_total"));

  // Per-thread watchdog stats.
  EXPECT_NE(nullptr, test_server_->counter("server.main_thread.watchdog_miss"));
  EXPECT_NE(nullptr, test_server_->counter("server.worker_0.watchdog_miss"));
  EXPECT_NE(nullptr, test_server_->counter("server.worker_1.watchdog_miss"));

  codec_client_ = makeHttpConnection(lookupPort("http"));
  IntegrationCodecClientPtr codec_client2 = makeHttpConnection(lookupPort("http"));
  check_listener_stats(1, 1);

  codec_client_->close();
  codec_client2->close();
  check_listener_stats(0, 1);
}

// Validates that the drain actually drains the listeners.
TEST_P(IntegrationTest, AdminDrainDrainsListeners) {
  initialize();

  uint32_t http_port = lookupPort("http");
  codec_client_ = makeHttpConnection(http_port);
  Http::TestHeaderMapImpl request_headers{{":method", "HEAD"},
                                          {":path", "/test/long/url"},
                                          {":scheme", "http"},
                                          {":authority", "host"}};
  IntegrationStreamDecoderPtr response = codec_client_->makeHeaderOnlyRequest(request_headers);
  waitForNextUpstreamRequest(0);
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  upstream_request_->encodeHeaders(default_response_headers_, false);

  // Invoke drain listeners endpoint and validate that we can still work on inflight requests.
  BufferingStreamDecoderPtr admin_response = IntegrationUtil::makeSingleRequest(
      lookupPort("admin"), "POST", "/drain_listeners", "", downstreamProtocol(), version_);
  EXPECT_TRUE(admin_response->complete());
  EXPECT_EQ("200", admin_response->headers().Status()->value().getStringView());
  EXPECT_EQ("OK\n", admin_response->body());

  upstream_request_->encodeData(512, true);

  ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());

  // Wait for the response to be read by the codec client.
  response->waitForEndStream();

  ASSERT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), Http::HttpStatusIs("200"));

  // Validate that the listeners have been stopped.
  test_server_->waitForCounterEq("listener_manager.listener_stopped", 1);

  // Validate that port is closed and can be bound by other sockets.
  EXPECT_NO_THROW(Network::TcpListenSocket(
      Network::Utility::getAddressWithPort(*Network::Test::getCanonicalLoopbackAddress(GetParam()),
                                           http_port),
      nullptr, true));
}

TEST_P(IntegrationTest, RouterDirectResponse) {
  const std::string body = "Response body";
  const std::string file_path = TestEnvironment::writeStringToFileForTest("test_envoy", body);
  static const std::string domain("direct.example.com");
  static const std::string prefix("/");
  static const Http::Code status(Http::Code::OK);
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        auto* route_config = hcm.mutable_route_config();
        auto* header_value_option = route_config->mutable_response_headers_to_add()->Add();
        header_value_option->mutable_header()->set_key("x-additional-header");
        header_value_option->mutable_header()->set_value("example-value");
        header_value_option->mutable_append()->set_value(false);
        header_value_option = route_config->mutable_response_headers_to_add()->Add();
        header_value_option->mutable_header()->set_key("content-type");
        header_value_option->mutable_header()->set_value("text/html");
        header_value_option->mutable_append()->set_value(false);
        auto* virtual_host = route_config->add_virtual_hosts();
        virtual_host->set_name(domain);
        virtual_host->add_domains(domain);
        virtual_host->add_routes()->mutable_match()->set_prefix(prefix);
        virtual_host->mutable_routes(0)->mutable_direct_response()->set_status(
            static_cast<uint32_t>(status));
        virtual_host->mutable_routes(0)->mutable_direct_response()->mutable_body()->set_filename(
            file_path);
      });
  initialize();

  BufferingStreamDecoderPtr response = IntegrationUtil::makeSingleRequest(
      lookupPort("http"), "GET", "/", "", downstream_protocol_, version_, "direct.example.com");
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ("example-value", response->headers()
                                 .get(Envoy::Http::LowerCaseString("x-additional-header"))
                                 ->value()
                                 .getStringView());
  EXPECT_EQ("text/html", response->headers().ContentType()->value().getStringView());
  EXPECT_EQ(body, response->body());
}

TEST_P(IntegrationTest, ConnectionClose) {
  config_helper_.addFilter(ConfigHelper::DEFAULT_HEALTH_CHECK_FILTER);
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      codec_client_->makeHeaderOnlyRequest(Http::TestHeaderMapImpl{{":method", "GET"},
                                                                   {":path", "/healthcheck"},
                                                                   {":authority", "host"},
                                                                   {"connection", "close"}});
  response->waitForEndStream();
  codec_client_->waitForDisconnect();

  EXPECT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("200"));
}

TEST_P(IntegrationTest, RouterRequestAndResponseWithBodyNoBuffer) {
  testRouterRequestAndResponseWithBody(1024, 512, false);
}

TEST_P(IntegrationTest, FlowControlOnAndGiantBody) {
  config_helper_.setBufferLimits(1024, 1024);
  testRouterRequestAndResponseWithBody(1024 * 1024, 1024 * 1024, false);
}

TEST_P(IntegrationTest, RouterRequestAndResponseLargeHeaderNoBuffer) {
  testRouterRequestAndResponseWithBody(1024, 512, true);
}

TEST_P(IntegrationTest, RouterHeaderOnlyRequestAndResponseNoBuffer) {
  testRouterHeaderOnlyRequestAndResponse();
}

TEST_P(IntegrationTest, RouterUpstreamDisconnectBeforeRequestcomplete) {
  testRouterUpstreamDisconnectBeforeRequestComplete();
}

TEST_P(IntegrationTest, RouterUpstreamDisconnectBeforeResponseComplete) {
  testRouterUpstreamDisconnectBeforeResponseComplete();
}

TEST_P(IntegrationTest, RouterDownstreamDisconnectBeforeRequestComplete) {
  testRouterDownstreamDisconnectBeforeRequestComplete();
}

TEST_P(IntegrationTest, RouterDownstreamDisconnectBeforeResponseComplete) {
  testRouterDownstreamDisconnectBeforeResponseComplete();
}

TEST_P(IntegrationTest, RouterUpstreamResponseBeforeRequestComplete) {
  testRouterUpstreamResponseBeforeRequestComplete();
}

TEST_P(IntegrationTest, EnvoyProxyingEarly100ContinueWithEncoderFilter) {
  testEnvoyProxying100Continue(true, true);
}

TEST_P(IntegrationTest, EnvoyProxyingLate100ContinueWithEncoderFilter) {
  testEnvoyProxying100Continue(false, true);
}

// This is a regression for https://github.com/envoyproxy/envoy/issues/2715 and validates that a
// pending request is not sent on a connection that has been half-closed.
TEST_P(IntegrationTest, UpstreamDisconnectWithTwoRequests) {
  config_helper_.addConfigModifier([](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
    auto* static_resources = bootstrap.mutable_static_resources();
    auto* cluster = static_resources->mutable_clusters(0);
    // Ensure we only have one connection upstream, one request active at a time.
    cluster->mutable_max_requests_per_connection()->set_value(1);
    auto* circuit_breakers = cluster->mutable_circuit_breakers();
    circuit_breakers->add_thresholds()->mutable_max_connections()->set_value(1);
  });
  initialize();
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Request 1.
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 1024);
  waitForNextUpstreamRequest();

  // Request 2.
  IntegrationCodecClientPtr codec_client2 = makeHttpConnection(lookupPort("http"));
  auto response2 = codec_client2->makeRequestWithBody(default_request_headers_, 512);

  // Validate one request active, the other pending.
  test_server_->waitForGaugeEq("cluster.cluster_0.upstream_rq_active", 1);
  test_server_->waitForGaugeEq("cluster.cluster_0.upstream_rq_pending_active", 1);

  // Response 1.
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);
  ASSERT_TRUE(fake_upstream_connection_->close());
  response->waitForEndStream();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_cx_total", 1);
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_rq_200", 1);

  // Response 2.
  ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  fake_upstream_connection_.reset();
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(1024, true);
  response2->waitForEndStream();
  codec_client2->close();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_TRUE(response2->complete());
  EXPECT_EQ("200", response2->headers().Status()->value().getStringView());
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_cx_total", 2);
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_rq_200", 2);
}

// Test hitting the bridge filter with too many response bytes to buffer. Given
// the headers are not proxied, the connection manager will send a local error reply.
TEST_P(IntegrationTest, HittingGrpcFilterLimitBufferingHeaders) {
  config_helper_.addFilter("{ name: envoy.grpc_http1_bridge, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");
  config_helper_.setBufferLimits(1024, 1024);

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response = codec_client_->makeHeaderOnlyRequest(
      Http::TestHeaderMapImpl{{":method", "POST"},
                              {":path", "/test/long/url"},
                              {":scheme", "http"},
                              {":authority", "host"},
                              {"content-type", "application/grpc"},
                              {"x-envoy-retry-grpc-on", "cancelled"}});
  waitForNextUpstreamRequest();

  // Send the overly large response. Because the grpc_http1_bridge filter buffers and buffer
  // limits are exceeded, this will be translated into an unknown gRPC error.
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "200"}}, false);
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);
  upstream_request_->encodeData(1024 * 65, false);
  ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());

  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("200"));
  EXPECT_THAT(response->headers(),
              HeaderValueOf(Headers::get().GrpcStatus, "2")); // Unknown gRPC error
}

TEST_P(IntegrationTest, BadFirstline) {
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"), "hello", &response);
  EXPECT_EQ("HTTP/1.1 400 Bad Request\r\ncontent-length: 0\r\nconnection: close\r\n\r\n", response);
}

TEST_P(IntegrationTest, MissingDelimiter) {
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"),
                                "GET / HTTP/1.1\r\nHost: host\r\nfoo bar\r\n\r\n", &response);
  EXPECT_EQ("HTTP/1.1 400 Bad Request\r\ncontent-length: 0\r\nconnection: close\r\n\r\n", response);
}

TEST_P(IntegrationTest, InvalidCharacterInFirstline) {
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"), "GE(T / HTTP/1.1\r\nHost: host\r\n\r\n",
                                &response);
  EXPECT_EQ("HTTP/1.1 400 Bad Request\r\ncontent-length: 0\r\nconnection: close\r\n\r\n", response);
}

TEST_P(IntegrationTest, InvalidVersion) {
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"), "GET / HTTP/1.01\r\nHost: host\r\n\r\n",
                                &response);
  EXPECT_EQ("HTTP/1.1 400 Bad Request\r\ncontent-length: 0\r\nconnection: close\r\n\r\n", response);
}

TEST_P(IntegrationTest, Http10Disabled) {
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"), "GET / HTTP/1.0\r\n\r\n", &response, true);
  EXPECT_TRUE(response.find("HTTP/1.1 426 Upgrade Required\r\n") == 0);
}

TEST_P(IntegrationTest, Http10DisabledWithUpgrade) {
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"), "GET / HTTP/1.0\r\nUpgrade: h2c\r\n\r\n",
                                &response, true);
  EXPECT_TRUE(response.find("HTTP/1.1 426 Upgrade Required\r\n") == 0);
}

// Turn HTTP/1.0 support on and verify 09 style requests work.
TEST_P(IntegrationTest, Http09Enabled) {
  useAccessLog();
  autonomous_upstream_ = true;
  config_helper_.addConfigModifier(&setAllowHttp10WithDefaultHost);
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"), "GET /\r\n\r\n", &response, false);
  EXPECT_THAT(response, HasSubstr("HTTP/1.0 200 OK\r\n"));
  EXPECT_THAT(response, HasSubstr("connection: close"));
  EXPECT_THAT(response, Not(HasSubstr("transfer-encoding: chunked\r\n")));

  std::unique_ptr<Http::TestHeaderMapImpl> upstream_headers =
      reinterpret_cast<AutonomousUpstream*>(fake_upstreams_.front().get())->lastRequestHeaders();
  ASSERT_TRUE(upstream_headers != nullptr);
  EXPECT_EQ(upstream_headers->Host()->value(), "default.com");

  EXPECT_THAT(waitForAccessLog(access_log_name_), HasSubstr("HTTP/1.0"));
}

// Turn HTTP/1.0 support on and verify the request is proxied and the default host is sent upstream.
TEST_P(IntegrationTest, Http10Enabled) {
  autonomous_upstream_ = true;
  config_helper_.addConfigModifier(&setAllowHttp10WithDefaultHost);
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"), "GET / HTTP/1.0\r\n\r\n", &response, false);
  EXPECT_THAT(response, HasSubstr("HTTP/1.0 200 OK\r\n"));
  EXPECT_THAT(response, HasSubstr("connection: close"));
  EXPECT_THAT(response, Not(HasSubstr("transfer-encoding: chunked\r\n")));

  std::unique_ptr<Http::TestHeaderMapImpl> upstream_headers =
      reinterpret_cast<AutonomousUpstream*>(fake_upstreams_.front().get())->lastRequestHeaders();
  ASSERT_TRUE(upstream_headers != nullptr);
  EXPECT_EQ(upstream_headers->Host()->value(), "default.com");

  sendRawHttpAndWaitForResponse(lookupPort("http"), "HEAD / HTTP/1.0\r\n\r\n", &response, false);
  EXPECT_THAT(response, HasSubstr("HTTP/1.0 200 OK\r\n"));
  EXPECT_THAT(response, HasSubstr("connection: close"));
  EXPECT_THAT(response, Not(HasSubstr("transfer-encoding: chunked\r\n")));
}

TEST_P(IntegrationTest, TestInlineHeaders) {
  autonomous_upstream_ = true;
  config_helper_.addConfigModifier(&setAllowHttp10WithDefaultHost);
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"),
                                "GET / HTTP/1.1\r\n"
                                "Host: foo.com\r\n"
                                "Foo: bar\r\n"
                                "Cache-control: public\r\n"
                                "Cache-control: 123\r\n"
                                "Eep: baz\r\n\r\n",
                                &response, true);
  EXPECT_THAT(response, HasSubstr("HTTP/1.1 200 OK\r\n"));

  std::unique_ptr<Http::TestHeaderMapImpl> upstream_headers =
      reinterpret_cast<AutonomousUpstream*>(fake_upstreams_.front().get())->lastRequestHeaders();
  ASSERT_TRUE(upstream_headers != nullptr);
  EXPECT_EQ(upstream_headers->Host()->value(), "foo.com");
  EXPECT_EQ(upstream_headers->CacheControl()->value(), "public,123");
  ASSERT_TRUE(upstream_headers->get(Envoy::Http::LowerCaseString("foo")) != nullptr);
  EXPECT_EQ("bar",
            upstream_headers->get(Envoy::Http::LowerCaseString("foo"))->value().getStringView());
  ASSERT_TRUE(upstream_headers->get(Envoy::Http::LowerCaseString("eep")) != nullptr);
  EXPECT_EQ("baz",
            upstream_headers->get(Envoy::Http::LowerCaseString("eep"))->value().getStringView());
}

// Verify for HTTP/1.0 a keep-alive header results in no connection: close.
// Also verify existing host headers are passed through for the HTTP/1.0 case.
TEST_P(IntegrationTest, Http10WithHostandKeepAlive) {
  autonomous_upstream_ = true;
  config_helper_.addConfigModifier(&setAllowHttp10WithDefaultHost);
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"),
                                "GET / HTTP/1.0\r\nHost: foo.com\r\nConnection:Keep-alive\r\n\r\n",
                                &response, true);
  EXPECT_THAT(response, HasSubstr("HTTP/1.0 200 OK\r\n"));
  EXPECT_THAT(response, Not(HasSubstr("connection: close")));
  EXPECT_THAT(response, Not(HasSubstr("transfer-encoding: chunked\r\n")));

  std::unique_ptr<Http::TestHeaderMapImpl> upstream_headers =
      reinterpret_cast<AutonomousUpstream*>(fake_upstreams_.front().get())->lastRequestHeaders();
  ASSERT_TRUE(upstream_headers != nullptr);
  EXPECT_EQ(upstream_headers->Host()->value(), "foo.com");
}

TEST_P(IntegrationTest, Pipeline) {
  autonomous_upstream_ = true;
  initialize();
  std::string response;

  Buffer::OwnedImpl buffer("GET / HTTP/1.1\r\nHost: host\r\n\r\nGET / HTTP/1.1\r\n\r\n");
  RawConnectionDriver connection(
      lookupPort("http"), buffer,
      [&](Network::ClientConnection&, const Buffer::Instance& data) -> void {
        response.append(data.toString());
      },
      version_);
  // First response should be success.
  while (response.find("200") == std::string::npos) {
    connection.run(Event::Dispatcher::RunType::NonBlock);
  }
  EXPECT_THAT(response, HasSubstr("HTTP/1.1 200 OK\r\n"));

  // Second response should be 400 (no host)
  while (response.find("400") == std::string::npos) {
    connection.run(Event::Dispatcher::RunType::NonBlock);
  }
  EXPECT_THAT(response, HasSubstr("HTTP/1.1 400 Bad Request\r\n"));
  connection.close();
}

TEST_P(IntegrationTest, NoHost) {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  Http::TestHeaderMapImpl request_headers{
      {":method", "GET"}, {":path", "/test/long/url"}, {":scheme", "http"}};
  auto response = codec_client_->makeHeaderOnlyRequest(request_headers);
  response->waitForEndStream();

  ASSERT_TRUE(response->complete());
  EXPECT_EQ("400", response->headers().Status()->value().getStringView());
}

TEST_P(IntegrationTest, BadPath) {
  config_helper_.addConfigModifier(&setDisallowAbsoluteUrl);
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"),
                                "GET http://api.lyft.com HTTP/1.1\r\nHost: host\r\n\r\n", &response,
                                true);
  EXPECT_TRUE(response.find("HTTP/1.1 404 Not Found\r\n") == 0);
}

TEST_P(IntegrationTest, AbsolutePath) {
  // Configure www.redirect.com to send a redirect, and ensure the redirect is
  // encountered via absolute URL.
  auto host = config_helper_.createVirtualHost("www.redirect.com", "/");
  host.set_require_tls(envoy::api::v2::route::VirtualHost::ALL);
  config_helper_.addVirtualHost(host);

  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"),
                                "GET http://www.redirect.com HTTP/1.1\r\nHost: host\r\n\r\n",
                                &response, true);
  EXPECT_FALSE(response.find("HTTP/1.1 404 Not Found\r\n") == 0);
}

TEST_P(IntegrationTest, AbsolutePathWithPort) {
  // Configure www.namewithport.com:1234 to send a redirect, and ensure the redirect is
  // encountered via absolute URL with a port.
  auto host = config_helper_.createVirtualHost("www.namewithport.com:1234", "/");
  host.set_require_tls(envoy::api::v2::route::VirtualHost::ALL);
  config_helper_.addVirtualHost(host);
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(
      lookupPort("http"), "GET http://www.namewithport.com:1234 HTTP/1.1\r\nHost: host\r\n\r\n",
      &response, true);
  EXPECT_FALSE(response.find("HTTP/1.1 404 Not Found\r\n") == 0);
}

TEST_P(IntegrationTest, AbsolutePathWithoutPort) {
  // Add a restrictive default match, to avoid the request hitting the * / catchall.
  config_helper_.setDefaultHostAndRoute("foo.com", "/found");
  // Set a matcher for www.namewithport.com:1234 and verify http://www.namewithport.com does not
  // match
  auto host = config_helper_.createVirtualHost("www.namewithport.com:1234", "/");
  host.set_require_tls(envoy::api::v2::route::VirtualHost::ALL);
  config_helper_.addVirtualHost(host);
  initialize();
  std::string response;
  sendRawHttpAndWaitForResponse(lookupPort("http"),
                                "GET http://www.namewithport.com HTTP/1.1\r\nHost: host\r\n\r\n",
                                &response, true);
  EXPECT_TRUE(response.find("HTTP/1.1 404 Not Found\r\n") == 0) << response;
}

// Ensure that connect behaves the same with allow_absolute_url enabled and without
TEST_P(IntegrationTest, Connect) {
  const std::string& request = "CONNECT www.somewhere.com:80 HTTP/1.1\r\nHost: host\r\n\r\n";
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    // Clone the whole listener.
    auto static_resources = bootstrap.mutable_static_resources();
    auto* old_listener = static_resources->mutable_listeners(0);
    auto* cloned_listener = static_resources->add_listeners();
    cloned_listener->CopyFrom(*old_listener);
    old_listener->set_name("http_forward");
  });
  // Set the first listener to disallow absolute URLs.
  config_helper_.addConfigModifier(&setDisallowAbsoluteUrl);
  initialize();

  std::string response1;
  sendRawHttpAndWaitForResponse(lookupPort("http"), request.c_str(), &response1, true);

  std::string response2;
  sendRawHttpAndWaitForResponse(lookupPort("http_forward"), request.c_str(), &response2, true);

  EXPECT_EQ(normalizeDate(response1), normalizeDate(response2));
}

TEST_P(IntegrationTest, UpstreamProtocolError) {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder = codec_client_->startRequest(Http::TestHeaderMapImpl{
      {":method", "GET"}, {":path", "/test/long/url"}, {":authority", "host"}});
  auto response = std::move(encoder_decoder.second);

  FakeRawConnectionPtr fake_upstream_connection;
  ASSERT_TRUE(fake_upstreams_[0]->waitForRawConnection(fake_upstream_connection));
  // TODO(mattklein123): Waiting for exact amount of data is a hack. This needs to
  // be fixed.
  std::string data;
  ASSERT_TRUE(fake_upstream_connection->waitForData(187, &data));
  ASSERT_TRUE(fake_upstream_connection->write("bad protocol data!"));
  ASSERT_TRUE(fake_upstream_connection->waitForDisconnect());
  codec_client_->waitForDisconnect();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
}

TEST_P(IntegrationTest, TestHead) {
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  Http::TestHeaderMapImpl head_request{{":method", "HEAD"},
                                       {":path", "/test/long/url"},
                                       {":scheme", "http"},
                                       {":authority", "host"}};

  // Without an explicit content length, assume we chunk for HTTP/1.1
  auto response = sendRequestAndWaitForResponse(head_request, 0, default_response_headers_, 0);
  ASSERT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("200"));
  EXPECT_EQ(response->headers().ContentLength(), nullptr);
  EXPECT_THAT(response->headers(),
              HeaderValueOf(Headers::get().TransferEncoding,
                            Http::Headers::get().TransferEncodingValues.Chunked));
  EXPECT_EQ(0, response->body().size());

  // Preserve explicit content length.
  Http::TestHeaderMapImpl content_length_response{{":status", "200"}, {"content-length", "12"}};
  response = sendRequestAndWaitForResponse(head_request, 0, content_length_response, 0);
  ASSERT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("200"));
  EXPECT_THAT(response->headers(), HeaderValueOf(Headers::get().ContentLength, "12"));
  EXPECT_EQ(response->headers().TransferEncoding(), nullptr);
  EXPECT_EQ(0, response->body().size());

  cleanupUpstreamAndDownstream();
}

// The Envoy HTTP/1.1 codec ASSERTs that T-E headers are cleared in
// encodeHeaders, so to test upstreams explicitly sending T-E: chunked we have
// to send raw HTTP.
TEST_P(IntegrationTest, TestHeadWithExplicitTE) {
  initialize();

  auto tcp_client = makeTcpConnection(lookupPort("http"));
  tcp_client->write("HEAD / HTTP/1.1\r\nHost: host\r\n\r\n");
  FakeRawConnectionPtr fake_upstream_connection;
  ASSERT_TRUE(fake_upstreams_[0]->waitForRawConnection(fake_upstream_connection));
  std::string data;
  ASSERT_TRUE(fake_upstream_connection->waitForData(
      FakeRawConnection::waitForInexactMatch("\r\n\r\n"), &data));

  ASSERT_TRUE(
      fake_upstream_connection->write("HTTP/1.1 200 OK\r\nTransfer-encoding: chunked\r\n\r\n"));
  tcp_client->waitForData("\r\n\r\n", false);
  std::string response = tcp_client->data();

  EXPECT_THAT(response, HasSubstr("HTTP/1.1 200 OK\r\n"));
  EXPECT_THAT(response, Not(HasSubstr("content-length")));
  EXPECT_THAT(response, HasSubstr("transfer-encoding: chunked\r\n"));
  EXPECT_THAT(response, EndsWith("\r\n\r\n"));

  ASSERT_TRUE(fake_upstream_connection->close());
  ASSERT_TRUE(fake_upstream_connection->waitForDisconnect());
  tcp_client->close();
}

TEST_P(IntegrationTest, TestBind) {
  std::string address_string;
  if (GetParam() == Network::Address::IpVersion::v4) {
    address_string = TestUtility::getIpv4Loopback();
  } else {
    address_string = "::1";
  }
  config_helper_.setSourceAddress(address_string);
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "GET"},
                                                                 {":path", "/test/long/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"}},
                                         1024);
  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  ASSERT_NE(fake_upstream_connection_, nullptr);
  std::string address =
      fake_upstream_connection_->connection().remoteAddress()->ip()->addressAsString();
  EXPECT_EQ(address, address_string);
  ASSERT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));
  ASSERT_NE(upstream_request_, nullptr);
  ASSERT_TRUE(upstream_request_->waitForEndStream(*dispatcher_));

  cleanupUpstreamAndDownstream();
}

TEST_P(IntegrationTest, TestFailedBind) {
  config_helper_.setSourceAddress("8.8.8.8");

  initialize();
  // Envoy will create and close some number of connections when trying to bind.
  // Make sure they don't cause assertion failures when we ignore them.
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);
  codec_client_ = makeHttpConnection(lookupPort("http"));
  // With no ability to successfully bind on an upstream connection Envoy should
  // send a 500.
  auto response = codec_client_->makeHeaderOnlyRequest(
      Http::TestHeaderMapImpl{{":method", "GET"},
                              {":path", "/test/long/url"},
                              {":scheme", "http"},
                              {":authority", "host"},
                              {"x-forwarded-for", "10.0.0.1"},
                              {"x-envoy-upstream-rq-timeout-ms", "1000"}});
  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("503"));
  EXPECT_LT(0, test_server_->counter("cluster.cluster_0.bind_errors")->value());
}

ConfigHelper::HttpModifierFunction setVia(const std::string& via) {
  return
      [via](
          envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
        hcm.set_via(via);
      };
}

// Validate in a basic header-only request we get via header insertion.
TEST_P(IntegrationTest, ViaAppendHeaderOnly) {
  config_helper_.addConfigModifier(setVia("bar"));
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      codec_client_->makeHeaderOnlyRequest(Http::TestHeaderMapImpl{{":method", "GET"},
                                                                   {":path", "/test/long/url"},
                                                                   {":authority", "host"},
                                                                   {"via", "foo"},
                                                                   {"connection", "close"}});
  waitForNextUpstreamRequest();
  EXPECT_THAT(upstream_request_->headers(), HeaderValueOf(Headers::get().Via, "foo, bar"));
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "200"}}, true);
  response->waitForEndStream();
  codec_client_->waitForDisconnect();
  EXPECT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("200"));
  EXPECT_THAT(response->headers(), HeaderValueOf(Headers::get().Via, "bar"));
}

// Validate that 100-continue works as expected with via header addition on both request and
// response path.
TEST_P(IntegrationTest, ViaAppendWith100Continue) {
  config_helper_.addConfigModifier(setVia("foo"));
  testEnvoyHandling100Continue(false, "foo");
}

// Test delayed close semantics for downstream HTTP/1.1 connections. When an early response is
// sent by Envoy, it will wait for response acknowledgment (via FIN/RST) from the client before
// closing the socket (with a timeout for ensuring cleanup).
TEST_P(IntegrationTest, TestDelayedConnectionTeardownOnGracefulClose) {
  // This test will trigger an early 413 Payload Too Large response due to buffer limits being
  // exceeded. The following filter is needed since the router filter will never trigger a 413.
  config_helper_.addFilter("{ name: envoy.http_dynamo_filter, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");
  config_helper_.setBufferLimits(1024, 1024);
  initialize();

  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);

  codec_client_->sendData(*request_encoder_, 1024 * 65, false);

  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("413", response->headers().Status()->value().getStringView());
  // With no delayed close processing, Envoy will close the connection immediately after flushing
  // and this should instead return true.
  EXPECT_FALSE(codec_client_->waitForDisconnect(std::chrono::milliseconds(500)));

  // Issue a local close and check that the client did not pick up a remote close which can happen
  // when delayed close semantics are disabled.
  codec_client_->connection()->close(Network::ConnectionCloseType::NoFlush);
  EXPECT_EQ(codec_client_->last_connection_event(), Network::ConnectionEvent::LocalClose);
}

// Test configuration of the delayed close timeout on downstream HTTP/1.1 connections. A value of 0
// disables delayed close processing.
TEST_P(IntegrationTest, TestDelayedConnectionTeardownConfig) {
  config_helper_.addFilter("{ name: envoy.http_dynamo_filter, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");
  config_helper_.setBufferLimits(1024, 1024);
  config_helper_.addConfigModifier(
      [](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
        hcm.mutable_delayed_close_timeout()->set_seconds(0);
      });
  initialize();

  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);

  codec_client_->sendData(*request_encoder_, 1024 * 65, false);

  response->waitForEndStream();
  // There is a potential race in the client's response processing when delayed close logic is
  // disabled in Envoy (see https://github.com/envoyproxy/envoy/issues/2929). Depending on timing,
  // a client may receive an RST prior to reading the response data from the socket, which may clear
  // the receive buffers. Also, clients which don't flush the receive buffer upon receiving a remote
  // close may also lose data (Envoy is susceptible to this).
  // Therefore, avoid checking response code/payload here and instead simply look for the remote
  // close.
  EXPECT_TRUE(codec_client_->waitForDisconnect(std::chrono::milliseconds(500)));
  EXPECT_EQ(codec_client_->last_connection_event(), Network::ConnectionEvent::RemoteClose);
}

// Test that delay closed connections are eventually force closed when the timeout triggers.
TEST_P(IntegrationTest, TestDelayedConnectionTeardownTimeoutTrigger) {
  config_helper_.addFilter("{ name: envoy.http_dynamo_filter, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");
  config_helper_.setBufferLimits(1024, 1024);
  config_helper_.addConfigModifier(
      [](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
        // 200ms.
        hcm.mutable_delayed_close_timeout()->set_nanos(200000000);
      });

  initialize();

  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);

  codec_client_->sendData(*request_encoder_, 1024 * 65, false);

  response->waitForEndStream();
  // The delayed close timeout should trigger since client is not closing the connection.
  EXPECT_TRUE(codec_client_->waitForDisconnect(std::chrono::milliseconds(2000)));
  EXPECT_EQ(codec_client_->last_connection_event(), Network::ConnectionEvent::RemoteClose);
  EXPECT_EQ(test_server_->counter("http.config_test.downstream_cx_delayed_close_timeout")->value(),
            1);
}

// Test that if the route cache is cleared, it doesn't cause problems.
TEST_P(IntegrationTest, TestClearingRouteCacheFilter) {
  config_helper_.addFilter("{ name: clear-route-cache, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  sendRequestAndWaitForResponse(default_request_headers_, 0, default_response_headers_, 0);
}

// Test that if no connection pools are free, Envoy fails to establish an upstream connection.
TEST_P(IntegrationTest, NoConnectionPoolsFree) {
  config_helper_.addConfigModifier([](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
    auto* static_resources = bootstrap.mutable_static_resources();
    auto* cluster = static_resources->mutable_clusters(0);

    // Somewhat contrived with 0, but this is the simplest way to test right now.
    auto* circuit_breakers = cluster->mutable_circuit_breakers();
    circuit_breakers->add_thresholds()->mutable_max_connection_pools()->set_value(0);
  });

  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Request 1.
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 1024);

  // Validate none active.
  test_server_->waitForGaugeEq("cluster.cluster_0.upstream_rq_active", 0);
  test_server_->waitForGaugeEq("cluster.cluster_0.upstream_rq_pending_active", 0);

  response->waitForEndStream();

  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_rq_503", 1);

  EXPECT_EQ(test_server_->counter("cluster.cluster_0.upstream_cx_pool_overflow")->value(), 1);
}

TEST_P(IntegrationTest, ProcessObjectHealthy) {
  config_helper_.addFilter("{ name: process-context-filter, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");

  ProcessObjectForFilter healthy_object(true);
  process_object_ = healthy_object;
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      codec_client_->makeHeaderOnlyRequest(Http::TestHeaderMapImpl{{":method", "GET"},
                                                                   {":path", "/healthcheck"},
                                                                   {":authority", "host"},
                                                                   {"connection", "close"}});
  response->waitForEndStream();
  codec_client_->waitForDisconnect();

  EXPECT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("200"));
}

TEST_P(IntegrationTest, ProcessObjectUnealthy) {
  config_helper_.addFilter("{ name: process-context-filter, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");

  ProcessObjectForFilter unhealthy_object(false);
  process_object_ = unhealthy_object;
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      codec_client_->makeHeaderOnlyRequest(Http::TestHeaderMapImpl{{":method", "GET"},
                                                                   {":path", "/healthcheck"},
                                                                   {":authority", "host"},
                                                                   {"connection", "close"}});
  response->waitForEndStream();
  codec_client_->waitForDisconnect();

  EXPECT_TRUE(response->complete());
  EXPECT_THAT(response->headers(), HttpStatusIs("500"));
}

INSTANTIATE_TEST_SUITE_P(IpVersions, UpstreamEndpointIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(UpstreamEndpointIntegrationTest, TestUpstreamEndpointAddress) {
  initialize();
  EXPECT_STREQ(fake_upstreams_[0]->localAddress()->ip()->addressAsString().c_str(),
               Network::Test::getLoopbackAddressString(GetParam()).c_str());
}

} // namespace Envoy
