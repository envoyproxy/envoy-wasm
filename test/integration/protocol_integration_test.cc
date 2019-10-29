#include <functional>
#include <list>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "envoy/buffer/buffer.h"
#include "envoy/event/dispatcher.h"
#include "envoy/http/header_map.h"
#include "envoy/registry/registry.h"

#include "common/api/api_impl.h"
#include "common/buffer/buffer_impl.h"
#include "common/common/fmt.h"
#include "common/common/thread_annotations.h"
#include "common/http/headers.h"
#include "common/network/utility.h"
#include "common/protobuf/utility.h"
#include "common/runtime/runtime_impl.h"
#include "common/upstream/upstream_impl.h"

#include "test/common/upstream/utility.h"
#include "test/integration/autonomous_upstream.h"
#include "test/integration/http_integration.h"
#include "test/integration/http_protocol_integration.h"
#include "test/integration/test_host_predicate_config.h"
#include "test/integration/utility.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/registry.h"

#include "gtest/gtest.h"

using testing::HasSubstr;

namespace Envoy {

void setDoNotValidateRouteConfig(
    envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
  auto* route_config = hcm.mutable_route_config();
  route_config->mutable_validate_clusters()->set_value(false);
};

// Tests for DownstreamProtocolIntegrationTest will be run with all protocols
// (H1/H2 downstream) but only H1 upstreams.
//
// This is useful for things which will likely not differ based on upstream
// behavior, for example "how does Envoy handle duplicate content lengths from
// downstream"?
class DownstreamProtocolIntegrationTest : public HttpProtocolIntegrationTest {
protected:
  void changeHeadersForStopAllTests(Http::TestHeaderMapImpl& headers, bool set_buffer_limit) {
    headers.addCopy("content_size", std::to_string(count_ * size_));
    headers.addCopy("added_size", std::to_string(added_decoded_data_size_));
    headers.addCopy("is_first_trigger", "value");
    if (set_buffer_limit) {
      headers.addCopy("buffer_limit", std::to_string(buffer_limit_));
    }
  }

  void verifyUpStreamRequestAfterStopAllFilter() {
    if (downstreamProtocol() == Http::CodecClient::Type::HTTP2) {
      // decode-headers-return-stop-all-filter calls addDecodedData in decodeData and
      // decodeTrailers. 2 decoded data were added.
      EXPECT_EQ(count_ * size_ + added_decoded_data_size_ * 2, upstream_request_->bodyLength());
    } else {
      EXPECT_EQ(count_ * size_ + added_decoded_data_size_ * 1, upstream_request_->bodyLength());
    }
    EXPECT_EQ(true, upstream_request_->complete());
  }

  const int count_ = 70;
  const int size_ = 1000;
  const int added_decoded_data_size_ = 1;
  const int buffer_limit_ = 100;
};

// Tests for ProtocolIntegrationTest will be run with the full mesh of H1/H2
// downstream and H1/H2 upstreams.
using ProtocolIntegrationTest = HttpProtocolIntegrationTest;

TEST_P(ProtocolIntegrationTest, ShutdownWithActiveConnPoolConnections) {
  auto response = makeHeaderOnlyRequest(nullptr, 0);
  // Shut down the server with active connection pool connections.
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);
  test_server_.reset();
  checkSimpleRequestSuccess(0U, 0U, response.get());
}

// Change the default route to be restrictive, and send a request to an alternate route.
TEST_P(ProtocolIntegrationTest, RouterNotFound) { testRouterNotFound(); }

// Change the default route to be restrictive, and send a POST to an alternate route.
TEST_P(DownstreamProtocolIntegrationTest, RouterNotFoundBodyNoBuffer) {
  testRouterNotFoundWithBody();
}

// Add a route that uses unknown cluster (expect 404 Not Found).
TEST_P(DownstreamProtocolIntegrationTest, RouterClusterNotFound404) {
  config_helper_.addConfigModifier(&setDoNotValidateRouteConfig);
  auto host = config_helper_.createVirtualHost("foo.com", "/unknown", "unknown_cluster");
  host.mutable_routes(0)->mutable_route()->set_cluster_not_found_response_code(
      envoy::api::v2::route::RouteAction::NOT_FOUND);
  config_helper_.addVirtualHost(host);
  initialize();

  BufferingStreamDecoderPtr response = IntegrationUtil::makeSingleRequest(
      lookupPort("http"), "GET", "/unknown", "", downstream_protocol_, version_, "foo.com");
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("404", response->headers().Status()->value().getStringView());
}

// Add a route that uses unknown cluster (expect 503 Service Unavailable).
TEST_P(DownstreamProtocolIntegrationTest, RouterClusterNotFound503) {
  config_helper_.addConfigModifier(&setDoNotValidateRouteConfig);
  auto host = config_helper_.createVirtualHost("foo.com", "/unknown", "unknown_cluster");
  host.mutable_routes(0)->mutable_route()->set_cluster_not_found_response_code(
      envoy::api::v2::route::RouteAction::SERVICE_UNAVAILABLE);
  config_helper_.addVirtualHost(host);
  initialize();

  BufferingStreamDecoderPtr response = IntegrationUtil::makeSingleRequest(
      lookupPort("http"), "GET", "/unknown", "", downstream_protocol_, version_, "foo.com");
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
}

// Add a route which redirects HTTP to HTTPS, and verify Envoy sends a 301
TEST_P(ProtocolIntegrationTest, RouterRedirect) {
  auto host = config_helper_.createVirtualHost("www.redirect.com", "/");
  host.set_require_tls(envoy::api::v2::route::VirtualHost::ALL);
  config_helper_.addVirtualHost(host);
  initialize();

  BufferingStreamDecoderPtr response = IntegrationUtil::makeSingleRequest(
      lookupPort("http"), "GET", "/foo", "", downstream_protocol_, version_, "www.redirect.com");
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("301", response->headers().Status()->value().getStringView());
  EXPECT_EQ("https://www.redirect.com/foo",
            response->headers().get(Http::Headers::get().Location)->value().getStringView());
}

// Add a health check filter and verify correct computation of health based on upstream status.
TEST_P(ProtocolIntegrationTest, ComputedHealthCheck) {
  config_helper_.addFilter(R"EOF(
name: envoy.health_check
typed_config:
    "@type": type.googleapis.com/envoy.config.filter.http.health_check.v2.HealthCheck
    pass_through_mode: false
    cluster_min_healthy_percentages:
        example_cluster_name: { value: 75 }
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = codec_client_->makeHeaderOnlyRequest(Http::TestHeaderMapImpl{
      {":method", "GET"}, {":path", "/healthcheck"}, {":scheme", "http"}, {":authority", "host"}});
  response->waitForEndStream();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
}

// Add a health check filter and verify correct computation of health based on upstream status.
TEST_P(ProtocolIntegrationTest, ModifyBuffer) {
  config_helper_.addFilter(R"EOF(
name: envoy.health_check
typed_config:
    "@type": type.googleapis.com/envoy.config.filter.http.health_check.v2.HealthCheck
    pass_through_mode: false
    cluster_min_healthy_percentages:
        example_cluster_name: { value: 75 }
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = codec_client_->makeHeaderOnlyRequest(Http::TestHeaderMapImpl{
      {":method", "GET"}, {":path", "/healthcheck"}, {":scheme", "http"}, {":authority", "host"}});
  response->waitForEndStream();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
}

TEST_P(ProtocolIntegrationTest, AddEncodedTrailers) {
  config_helper_.addFilter(R"EOF(
name: add-trailers-filter
typed_config:
  "@type": type.googleapis.com/google.protobuf.Empty
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 128);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);
  upstream_request_->encodeData(128, true);
  response->waitForEndStream();

  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP2) {
    EXPECT_EQ("decode", upstream_request_->trailers()->GrpcMessage()->value().getStringView());
  }
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  if (downstream_protocol_ == Http::CodecClient::Type::HTTP2) {
    EXPECT_EQ("encode", response->trailers()->GrpcMessage()->value().getStringView());
  }
}

// Add a health check filter and verify correct behavior when draining.
TEST_P(ProtocolIntegrationTest, DrainClose) {
  config_helper_.addFilter(ConfigHelper::DEFAULT_HEALTH_CHECK_FILTER);
  initialize();

  test_server_->drainManager().draining_ = true;
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = codec_client_->makeHeaderOnlyRequest(default_request_headers_);
  response->waitForEndStream();
  codec_client_->waitForDisconnect();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  if (downstream_protocol_ == Http::CodecClient::Type::HTTP2) {
    EXPECT_TRUE(codec_client_->sawGoAway());
  }

  test_server_->drainManager().draining_ = false;
}

TEST_P(ProtocolIntegrationTest, Retry) {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/test/long/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"},
                                                                 {"x-forwarded-for", "10.0.0.1"},
                                                                 {"x-envoy-retry-on", "5xx"}},
                                         1024);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);

  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
    ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
  }
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);

  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(1024U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
}

// Tests that the x-envoy-attempt-count header is properly set on the upstream request
// and updated after the request is retried.
TEST_P(DownstreamProtocolIntegrationTest, RetryAttemptCountHeader) {
  auto host = config_helper_.createVirtualHost("host", "/test_retry");
  host.set_include_request_attempt_count(true);
  config_helper_.addVirtualHost(host);
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/test_retry"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"},
                                                                 {"x-forwarded-for", "10.0.0.1"},
                                                                 {"x-envoy-retry-on", "5xx"}},
                                         1024);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);

  EXPECT_EQ(
      atoi(std::string(upstream_request_->headers().EnvoyAttemptCount()->value().getStringView())
               .c_str()),
      1);

  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
    ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
  }
  waitForNextUpstreamRequest();
  EXPECT_EQ(
      atoi(std::string(upstream_request_->headers().EnvoyAttemptCount()->value().getStringView())
               .c_str()),
      2);
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);

  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(1024U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
}

// Verifies that a retry priority can be configured and affect the host selected during retries.
// The retry priority will always target P1, which would otherwise never be hit due to P0 being
// healthy.
TEST_P(DownstreamProtocolIntegrationTest, RetryPriority) {
  const Upstream::HealthyLoad healthy_priority_load({0u, 100u});
  const Upstream::DegradedLoad degraded_priority_load({0u, 100u});
  NiceMock<Upstream::MockRetryPriority> retry_priority(healthy_priority_load,
                                                       degraded_priority_load);
  Upstream::MockRetryPriorityFactory factory(retry_priority);

  Registry::InjectFactory<Upstream::RetryPriorityFactory> inject_factory(factory);

  // Add route with custom retry policy
  auto host = config_helper_.createVirtualHost("host", "/test_retry");
  host.set_include_request_attempt_count(true);
  auto retry_policy = host.mutable_routes(0)->mutable_route()->mutable_retry_policy();
  retry_policy->mutable_retry_priority()->set_name(factory.name());
  config_helper_.addVirtualHost(host);

  // Use load assignments instead of static hosts. Necessary in order to use priorities.
  config_helper_.addConfigModifier([](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
    auto cluster = bootstrap.mutable_static_resources()->mutable_clusters(0);
    auto load_assignment = cluster->mutable_load_assignment();
    load_assignment->set_cluster_name(cluster->name());
    const auto& host_address = cluster->hosts(0).socket_address().address();

    for (int i = 0; i < 2; ++i) {
      auto locality = load_assignment->add_endpoints();
      locality->set_priority(i);
      locality->mutable_locality()->set_region("region");
      locality->mutable_locality()->set_zone("zone");
      locality->mutable_locality()->set_sub_zone("sub_zone" + std::to_string(i));
      auto lb_endpoint = locality->add_lb_endpoints();
      lb_endpoint->mutable_endpoint()->mutable_address()->mutable_socket_address()->set_address(
          host_address);
      lb_endpoint->mutable_endpoint()->mutable_address()->mutable_socket_address()->set_port_value(
          0);
    }

    cluster->clear_hosts();
  });

  fake_upstreams_count_ = 2;
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/test_retry"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"},
                                                                 {"x-forwarded-for", "10.0.0.1"},
                                                                 {"x-envoy-retry-on", "5xx"}},
                                         1024);

  // Note how we're expecting each upstream request to hit the same upstream.
  waitForNextUpstreamRequest(0);
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);

  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
    ASSERT_TRUE(fake_upstreams_[1]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
  }

  waitForNextUpstreamRequest(1);
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);

  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(1024U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
}

//
// Verifies that a retry host filter can be configured and affect the host selected during retries.
// The predicate will keep track of the first host attempted, and attempt to route all requests to
// the same host. With a total of two upstream hosts, this should result in us continuously sending
// requests to the same host.
TEST_P(DownstreamProtocolIntegrationTest, RetryHostPredicateFilter) {
  TestHostPredicateFactory predicate_factory;
  Registry::InjectFactory<Upstream::RetryHostPredicateFactory> inject_factory(predicate_factory);

  // Add route with custom retry policy
  auto host = config_helper_.createVirtualHost("host", "/test_retry");
  host.set_include_request_attempt_count(true);
  auto retry_policy = host.mutable_routes(0)->mutable_route()->mutable_retry_policy();
  retry_policy->add_retry_host_predicate()->set_name(predicate_factory.name());
  config_helper_.addVirtualHost(host);

  // We want to work with a cluster with two hosts.
  config_helper_.addConfigModifier([](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
    auto* new_host = bootstrap.mutable_static_resources()->mutable_clusters(0)->add_hosts();
    new_host->MergeFrom(bootstrap.static_resources().clusters(0).hosts(0));
  });
  fake_upstreams_count_ = 2;
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/test_retry"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"},
                                                                 {"x-forwarded-for", "10.0.0.1"},
                                                                 {"x-envoy-retry-on", "5xx"}},
                                         1024);

  // Note how we're expecting each upstream request to hit the same upstream.
  auto upstream_idx = waitForNextUpstreamRequest({0, 1});
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);

  if (fake_upstreams_[upstream_idx]->httpType() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
    ASSERT_TRUE(fake_upstreams_[upstream_idx]->waitForHttpConnection(*dispatcher_,
                                                                     fake_upstream_connection_));
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
  }

  waitForNextUpstreamRequest(upstream_idx);
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);

  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(1024U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
}

// Very similar set-up to testRetry but with a 16k request the request will not
// be buffered and the 503 will be returned to the user.
TEST_P(ProtocolIntegrationTest, RetryHittingBufferLimit) {
  config_helper_.setBufferLimits(1024, 1024); // Set buffer limits upstream and downstream.
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/test/long/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"},
                                                                 {"x-forwarded-for", "10.0.0.1"},
                                                                 {"x-envoy-retry-on", "5xx"}},
                                         1024 * 65);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, true);

  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(66560U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
}

// Very similar set-up to RetryHittingBufferLimits but using the route specific cap.
TEST_P(ProtocolIntegrationTest, RetryHittingRouteLimits) {
  auto host = config_helper_.createVirtualHost("nobody.com", "/");
  host.mutable_per_request_buffer_limit_bytes()->set_value(0);
  config_helper_.addVirtualHost(host);
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "nobody.com"},
                                                                 {"x-forwarded-for", "10.0.0.1"},
                                                                 {"x-envoy-retry-on", "5xx"}},
                                         1);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, true);

  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(1U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
}

// Test hitting the dynamo filter with too many request bytes to buffer. Ensure the connection
// manager sends a 413.
TEST_P(DownstreamProtocolIntegrationTest, HittingDecoderFilterLimit) {
  config_helper_.addFilter("{ name: envoy.http_dynamo_filter, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");
  config_helper_.setBufferLimits(1024, 1024);
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Envoy will likely connect and proxy some unspecified amount of data before
  // hitting the buffer limit and disconnecting. Ignore this if it happens.
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/dynamo/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"},
                                                                 {"x-forwarded-for", "10.0.0.1"},
                                                                 {"x-envoy-retry-on", "5xx"}},
                                         1024 * 65);

  response->waitForEndStream();
  // With HTTP/1 there's a possible race where if the connection backs up early,
  // the 413-and-connection-close may be sent while the body is still being
  // sent, resulting in a write error and the connection being closed before the
  // response is read.
  if (downstream_protocol_ == Http::CodecClient::Type::HTTP2) {
    ASSERT_TRUE(response->complete());
  }
  if (response->complete()) {
    EXPECT_EQ("413", response->headers().Status()->value().getStringView());
  }
}

// Test hitting the dynamo filter with too many response bytes to buffer. Given the request headers
// are sent on early, the stream/connection will be reset.
TEST_P(ProtocolIntegrationTest, HittingEncoderFilterLimit) {
  useAccessLog();
  config_helper_.addFilter("{ name: envoy.http_dynamo_filter, typed_config: { \"@type\": "
                           "type.googleapis.com/google.protobuf.Empty } }");
  config_helper_.setBufferLimits(1024, 1024);
  initialize();

  // Send the request.
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  auto downstream_request = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  Buffer::OwnedImpl data(R"({"TableName":"locations"})");
  codec_client_->sendData(*downstream_request, data, true);
  waitForNextUpstreamRequest();

  // Send the response headers.
  upstream_request_->encodeHeaders(default_response_headers_, false);

  // Now send an overly large response body. At some point, too much data will
  // be buffered, the stream will be reset, and the connection will disconnect.
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);
  upstream_request_->encodeData(1024 * 65, false);
  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("500", response->headers().Status()->value().getStringView());
  EXPECT_THAT(waitForAccessLog(access_log_name_), HasSubstr("500"));
  test_server_->waitForCounterEq("http.config_test.downstream_rq_5xx", 1);
}

TEST_P(ProtocolIntegrationTest, EnvoyHandling100Continue) { testEnvoyHandling100Continue(); }

TEST_P(ProtocolIntegrationTest, EnvoyHandlingDuplicate100Continue) {
  testEnvoyHandling100Continue(true);
}

TEST_P(ProtocolIntegrationTest, EnvoyProxyingEarly100Continue) {
  testEnvoyProxying100Continue(true);
}

TEST_P(ProtocolIntegrationTest, EnvoyProxyingLate100Continue) {
  testEnvoyProxying100Continue(false);
}

TEST_P(ProtocolIntegrationTest, TwoRequests) { testTwoRequests(); }

TEST_P(ProtocolIntegrationTest, TwoRequestsWithForcedBackup) { testTwoRequests(true); }

TEST_P(DownstreamProtocolIntegrationTest, ValidZeroLengthContent) {
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  Http::TestHeaderMapImpl request_headers{{":method", "POST"},
                                          {":path", "/test/long/url"},
                                          {":scheme", "http"},
                                          {":authority", "host"},
                                          {"content-length", "0"}};
  auto response = sendRequestAndWaitForResponse(request_headers, 0, default_response_headers_, 0);

  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
}

// Validate that lots of tiny cookies doesn't cause a DoS (single cookie header).
TEST_P(DownstreamProtocolIntegrationTest, LargeCookieParsingConcatenated) {
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  Http::TestHeaderMapImpl request_headers{{":method", "POST"},
                                          {":path", "/test/long/url"},
                                          {":scheme", "http"},
                                          {":authority", "host"},
                                          {"content-length", "0"}};
  std::vector<std::string> cookie_pieces;
  cookie_pieces.reserve(7000);
  for (int i = 0; i < 7000; i++) {
    cookie_pieces.push_back(fmt::sprintf("a%x=b", i));
  }
  request_headers.addCopy("cookie", absl::StrJoin(cookie_pieces, "; "));
  auto response = sendRequestAndWaitForResponse(request_headers, 0, default_response_headers_, 0);

  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
}

// Validate that lots of tiny cookies doesn't cause a DoS (many cookie headers).
TEST_P(DownstreamProtocolIntegrationTest, LargeCookieParsingMany) {
  // Set header count limit to 2010.
  uint32_t max_count = 2010;
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_common_http_protocol_options()->mutable_max_headers_count()->set_value(
            max_count);
      });
  max_request_headers_count_ = max_count;

  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  Http::TestHeaderMapImpl request_headers{{":method", "POST"},
                                          {":path", "/test/long/url"},
                                          {":scheme", "http"},
                                          {":authority", "host"},
                                          {"content-length", "0"}};
  for (int i = 0; i < 2000; i++) {
    request_headers.addCopy("cookie", fmt::sprintf("a%x=b", i));
  }
  auto response = sendRequestAndWaitForResponse(request_headers, 0, default_response_headers_, 0);

  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
}

TEST_P(DownstreamProtocolIntegrationTest, InvalidContentLength) {
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":authority", "host"},
                                                          {"content-length", "-1"}});
  auto response = std::move(encoder_decoder.second);

  codec_client_->waitForDisconnect();

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    ASSERT_TRUE(response->complete());
    EXPECT_EQ("400", response->headers().Status()->value().getStringView());
  } else {
    ASSERT_TRUE(response->reset());
    EXPECT_EQ(Http::StreamResetReason::ConnectionTermination, response->reset_reason());
  }
}

// TODO(PiotrSikora): move this HTTP/2 only variant to http2_integration_test.cc.
TEST_P(DownstreamProtocolIntegrationTest, InvalidContentLengthAllowed) {
  config_helper_.addConfigModifier(
      [](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_http2_protocol_options()->set_stream_error_on_invalid_http_messaging(true);
      });

  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":authority", "host"},
                                                          {"content-length", "-1"}});
  auto response = std::move(encoder_decoder.second);

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    codec_client_->waitForDisconnect();
  } else {
    response->waitForReset();
    codec_client_->close();
  }

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    ASSERT_TRUE(response->complete());
    EXPECT_EQ("400", response->headers().Status()->value().getStringView());
  } else {
    ASSERT_TRUE(response->reset());
    EXPECT_EQ(Http::StreamResetReason::RemoteReset, response->reset_reason());
  }
}

TEST_P(DownstreamProtocolIntegrationTest, MultipleContentLengths) {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":authority", "host"},
                                                          {"content-length", "3,2"}});
  auto response = std::move(encoder_decoder.second);

  codec_client_->waitForDisconnect();

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    ASSERT_TRUE(response->complete());
    EXPECT_EQ("400", response->headers().Status()->value().getStringView());
  } else {
    ASSERT_TRUE(response->reset());
    EXPECT_EQ(Http::StreamResetReason::ConnectionTermination, response->reset_reason());
  }
}

// TODO(PiotrSikora): move this HTTP/2 only variant to http2_integration_test.cc.
TEST_P(DownstreamProtocolIntegrationTest, MultipleContentLengthsAllowed) {
  config_helper_.addConfigModifier(
      [](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_http2_protocol_options()->set_stream_error_on_invalid_http_messaging(true);
      });

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":authority", "host"},
                                                          {"content-length", "3,2"}});
  auto response = std::move(encoder_decoder.second);

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    codec_client_->waitForDisconnect();
  } else {
    response->waitForReset();
    codec_client_->close();
  }

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    ASSERT_TRUE(response->complete());
    EXPECT_EQ("400", response->headers().Status()->value().getStringView());
  } else {
    ASSERT_TRUE(response->reset());
    EXPECT_EQ(Http::StreamResetReason::RemoteReset, response->reset_reason());
  }
}

TEST_P(DownstreamProtocolIntegrationTest, HeadersOnlyFilterEncoding) {
  config_helper_.addFilter(R"EOF(
name: encode-headers-only
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "GET"},
                                                                 {":path", "/test/long/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"}},
                                         128);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);
  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->waitForEndStream(*dispatcher_));
  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ(0, response->body().size());
}

TEST_P(DownstreamProtocolIntegrationTest, HeadersOnlyFilterDecoding) {
  config_helper_.addFilter(R"EOF(
name: decode-headers-only
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/test/long/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"}},
                                         128);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);
  upstream_request_->encodeData(128, true);
  response->waitForEndStream();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ(128, response->body().size());
}

TEST_P(DownstreamProtocolIntegrationTest, HeadersOnlyFilterEncodingIntermediateFilters) {
  config_helper_.addFilter(R"EOF(
name: passthrough-filter
)EOF");
  config_helper_.addFilter(R"EOF(
name: encode-headers-only
)EOF");
  config_helper_.addFilter(R"EOF(
name: passthrough-filter
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "GET"},
                                                                 {":path", "/test/long/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"}},
                                         128);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);
  response->waitForEndStream();
  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ(0, response->body().size());
}

TEST_P(DownstreamProtocolIntegrationTest, HeadersOnlyFilterDecodingIntermediateFilters) {
  config_helper_.addFilter(R"EOF(
name: passthrough-filter
)EOF");
  config_helper_.addFilter(R"EOF(
name: decode-headers-only
)EOF");
  config_helper_.addFilter(R"EOF(
name: passthrough-filter
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response =
      codec_client_->makeRequestWithBody(Http::TestHeaderMapImpl{{":method", "POST"},
                                                                 {":path", "/test/long/url"},
                                                                 {":scheme", "http"},
                                                                 {":authority", "host"}},
                                         128);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);
  upstream_request_->encodeData(128, true);
  response->waitForEndStream();

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ(128, response->body().size());
}

// Verifies behavior when request data is encoded after the request has been
// turned into a headers-only request and the response has already begun.
TEST_P(DownstreamProtocolIntegrationTest, HeadersOnlyFilterInterleaved) {
  config_helper_.addFilter(R"EOF(
name: decode-headers-only
)EOF");
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  // First send the request headers. The filter should turn this into a header-only
  // request.
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "GET"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);

  // Wait for the upstream request and begin sending a response with end_stream = false.
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(Http::TestHeaderMapImpl{{":status", "503"}}, false);

  // Simulate additional data after the request has been turned into a headers only request.
  Buffer::OwnedImpl data(std::string(128, 'a'));
  request_encoder_->encodeData(data, false);

  // End the response.
  upstream_request_->encodeData(128, true);

  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ(0, upstream_request_->body().length());
}

TEST_P(DownstreamProtocolIntegrationTest, LargeRequestHeadersRejected) {
  // Send one 95 kB header with limit 60 kB and 100 headers.
  testLargeRequestHeaders(95, 1, 60, 100);
}

TEST_P(DownstreamProtocolIntegrationTest, LargeRequestHeadersAccepted) {
  // Send one 95 kB header with limit 96 kB and 100 headers.
  testLargeRequestHeaders(95, 1, 96, 100);
}

TEST_P(DownstreamProtocolIntegrationTest, ManyRequestHeadersRejected) {
  // Send 101 empty headers with limit 60 kB and 100 headers.
  testLargeRequestHeaders(0, 101, 60, 80);
}

TEST_P(DownstreamProtocolIntegrationTest, ManyRequestHeadersAccepted) {
  // Send 145 empty headers with limit 60 kB and 150 headers.
  testLargeRequestHeaders(0, 140, 60, 150);
}

TEST_P(DownstreamProtocolIntegrationTest, ManyRequestTrailersRejected) {
  // Default header (and trailer) count limit is 100.
  Http::TestHeaderMapImpl request_trailers;
  for (int i = 0; i < 150; i++) {
    request_trailers.addCopy("trailer", std::string(1, 'a'));
  }

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  codec_client_->sendData(*request_encoder_, 1, false);
  codec_client_->sendTrailers(*request_encoder_, request_trailers);

  // Only relevant to Http2Downstream.
  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    // Http1 Downstream ignores trailers.
    waitForNextUpstreamRequest();
    upstream_request_->encodeHeaders(default_response_headers_, true);
    response->waitForEndStream();
    EXPECT_TRUE(response->complete());
    EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  } else {
    // Expect rejection.
    // TODO(asraa): we shouldn't need this unexpected disconnect, but some tests hit unparented
    // connections without it. Likely need to reconsider whether the waits/closes should be on
    // client or upstream.
    fake_upstreams_[0]->set_allow_unexpected_disconnects(true);
    ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
    response->waitForReset();
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }
}

TEST_P(DownstreamProtocolIntegrationTest, ManyRequestTrailersAccepted) {
  // Set header (and trailer) count limit to 200.
  uint32_t max_count = 200;
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_common_http_protocol_options()->mutable_max_headers_count()->set_value(
            max_count);
      });
  max_request_headers_count_ = max_count;
  Http::TestHeaderMapImpl request_trailers;
  for (int i = 0; i < 150; i++) {
    request_trailers.addCopy("trailer", std::string(1, 'a'));
  }

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  codec_client_->sendData(*request_encoder_, 1, false);
  codec_client_->sendTrailers(*request_encoder_, request_trailers);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
}

TEST_P(DownstreamProtocolIntegrationTest, ManyRequestHeadersTimeout) {
  // Set timeout for 5 seconds, and ensure that a request with 20k+ headers can be sent.
  testManyRequestHeaders(std::chrono::milliseconds(5000));
}

TEST_P(DownstreamProtocolIntegrationTest, LargeRequestTrailersAccepted) {
  testLargeRequestTrailers(60, 96);
}

TEST_P(DownstreamProtocolIntegrationTest, LargeRequestTrailersRejected) {
  testLargeRequestTrailers(66, 60);
}

TEST_P(DownstreamProtocolIntegrationTest, ManyTrailerHeaders) {
  max_request_headers_kb_ = 96;
  max_request_headers_count_ = 20005;

  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_max_request_headers_kb()->set_value(max_request_headers_kb_);
        hcm.mutable_common_http_protocol_options()->mutable_max_headers_count()->set_value(
            max_request_headers_count_);
      });

  Http::TestHeaderMapImpl request_trailers{};
  for (int i = 0; i < 20000; i++) {
    request_trailers.addCopy(std::to_string(i), "");
  }

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  codec_client_->sendTrailers(*request_encoder_, request_trailers);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
}

// Tests StopAllIterationAndBuffer. Verifies decode-headers-return-stop-all-filter calls decodeData
// once after iteration is resumed.
TEST_P(DownstreamProtocolIntegrationTest, testDecodeHeadersReturnsStopAll) {
  config_helper_.addFilter(R"EOF(
name: call-decodedata-once-filter
)EOF");
  config_helper_.addFilter(R"EOF(
name: decode-headers-return-stop-all-filter
)EOF");
  config_helper_.addFilter(R"EOF(
name: passthrough-filter
)EOF");

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Sends a request with headers and data.
  changeHeadersForStopAllTests(default_request_headers_, false);
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  for (int i = 0; i < count_ - 1; i++) {
    codec_client_->sendData(*request_encoder_, size_, false);
  }
  // Sleeps for 1s in order to be consistent with testDecodeHeadersReturnsStopAllWatermark.
  sleep(1);
  codec_client_->sendData(*request_encoder_, size_, true);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  EXPECT_EQ(count_ * size_ + added_decoded_data_size_, upstream_request_->bodyLength());
  EXPECT_EQ(true, upstream_request_->complete());

  // Sends a request with headers, data, and trailers.
  auto encoder_decoder_2 = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder_2.first;
  response = std::move(encoder_decoder_2.second);
  for (int i = 0; i < count_; i++) {
    codec_client_->sendData(*request_encoder_, size_, false);
  }
  Http::TestHeaderMapImpl request_trailers{{"trailer", "trailer"}};
  codec_client_->sendTrailers(*request_encoder_, request_trailers);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  verifyUpStreamRequestAfterStopAllFilter();
}

// Tests StopAllIterationAndWatermark. decode-headers-return-stop-all-watermark-filter sets buffer
// limit to 100. Verifies data pause when limit is reached, and resume after iteration continues.
TEST_P(DownstreamProtocolIntegrationTest, testDecodeHeadersReturnsStopAllWatermark) {
  config_helper_.addFilter(R"EOF(
name: decode-headers-return-stop-all-filter
)EOF");
  config_helper_.addFilter(R"EOF(
name: passthrough-filter
)EOF");

  // Sets initial stream window to min value to make the client sensitive to a low watermark.
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_http2_protocol_options()->mutable_initial_stream_window_size()->set_value(
            Http::Http2Settings::MIN_INITIAL_STREAM_WINDOW_SIZE);
      });

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Sends a request with headers and data.
  changeHeadersForStopAllTests(default_request_headers_, true);
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  for (int i = 0; i < count_ - 1; i++) {
    codec_client_->sendData(*request_encoder_, size_, false);
  }
  // Gives buffer 1s to react to buffer limit.
  sleep(1);
  codec_client_->sendData(*request_encoder_, size_, true);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  EXPECT_EQ(count_ * size_ + added_decoded_data_size_, upstream_request_->bodyLength());
  EXPECT_EQ(true, upstream_request_->complete());

  // Sends a request with headers, data, and trailers.
  auto encoder_decoder_2 = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder_2.first;
  response = std::move(encoder_decoder_2.second);
  for (int i = 0; i < count_ - 1; i++) {
    codec_client_->sendData(*request_encoder_, size_, false);
  }
  // Gives buffer 1s to react to buffer limit.
  sleep(1);
  codec_client_->sendData(*request_encoder_, size_, false);
  Http::TestHeaderMapImpl request_trailers{{"trailer", "trailer"}};
  codec_client_->sendTrailers(*request_encoder_, request_trailers);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  verifyUpStreamRequestAfterStopAllFilter();
}

// Test two filters that return StopAllIterationAndBuffer back-to-back.
TEST_P(DownstreamProtocolIntegrationTest, testTwoFiltersDecodeHeadersReturnsStopAll) {
  config_helper_.addFilter(R"EOF(
name: decode-headers-return-stop-all-filter
)EOF");
  config_helper_.addFilter(R"EOF(
name: decode-headers-return-stop-all-filter
)EOF");
  config_helper_.addFilter(R"EOF(
name: passthrough-filter
)EOF");

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Sends a request with headers and data.
  changeHeadersForStopAllTests(default_request_headers_, false);
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  for (int i = 0; i < count_ - 1; i++) {
    codec_client_->sendData(*request_encoder_, size_, false);
  }
  codec_client_->sendData(*request_encoder_, size_, true);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  EXPECT_EQ(count_ * size_ + added_decoded_data_size_, upstream_request_->bodyLength());
  EXPECT_EQ(true, upstream_request_->complete());

  // Sends a request with headers, data, and trailers.
  auto encoder_decoder_2 = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder_2.first;
  response = std::move(encoder_decoder_2.second);
  for (int i = 0; i < count_; i++) {
    codec_client_->sendData(*request_encoder_, size_, false);
  }
  Http::TestHeaderMapImpl request_trailers{{"trailer", "trailer"}};
  codec_client_->sendTrailers(*request_encoder_, request_trailers);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  verifyUpStreamRequestAfterStopAllFilter();
}

// Tests encodeHeaders() returns StopAllIterationAndBuffer.
TEST_P(DownstreamProtocolIntegrationTest, testEncodeHeadersReturnsStopAll) {
  config_helper_.addFilter(R"EOF(
name: encode-headers-return-stop-all-filter
)EOF");
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void { hcm.mutable_http2_protocol_options()->set_allow_metadata(true); });

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Upstream responds with headers, data and trailers.
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 10);
  waitForNextUpstreamRequest();

  changeHeadersForStopAllTests(default_response_headers_, false);
  upstream_request_->encodeHeaders(default_response_headers_, false);
  for (int i = 0; i < count_ - 1; i++) {
    upstream_request_->encodeData(size_, false);
  }
  // Sleeps for 1s in order to be consistent with testEncodeHeadersReturnsStopAllWatermark.
  sleep(1);
  upstream_request_->encodeData(size_, false);
  Http::TestHeaderMapImpl response_trailers{{"response", "trailer"}};
  upstream_request_->encodeTrailers(response_trailers);

  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  EXPECT_EQ(count_ * size_ + added_decoded_data_size_, response->body().size());
}

// Tests encodeHeaders() returns StopAllIterationAndWatermark.
TEST_P(DownstreamProtocolIntegrationTest, testEncodeHeadersReturnsStopAllWatermark) {
  config_helper_.addFilter(R"EOF(
name: encode-headers-return-stop-all-filter
)EOF");
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void { hcm.mutable_http2_protocol_options()->set_allow_metadata(true); });

  // Sets initial stream window to min value to make the upstream sensitive to a low watermark.
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_http2_protocol_options()->mutable_initial_stream_window_size()->set_value(
            Http::Http2Settings::MIN_INITIAL_STREAM_WINDOW_SIZE);
      });

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Upstream responds with headers, data and trailers.
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 10);
  waitForNextUpstreamRequest();

  changeHeadersForStopAllTests(default_response_headers_, true);
  upstream_request_->encodeHeaders(default_response_headers_, false);
  for (int i = 0; i < count_ - 1; i++) {
    upstream_request_->encodeData(size_, false);
  }
  // Gives buffer 1s to react to buffer limit.
  sleep(1);
  upstream_request_->encodeData(size_, false);
  Http::TestHeaderMapImpl response_trailers{{"response", "trailer"}};
  upstream_request_->encodeTrailers(response_trailers);

  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  EXPECT_EQ(count_ * size_ + added_decoded_data_size_, response->body().size());
}

// Per https://github.com/envoyproxy/envoy/issues/7488 make sure we don't
// combine set-cookie headers
TEST_P(ProtocolIntegrationTest, MultipleSetCookies) {
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  Http::TestHeaderMapImpl response_headers{
      {":status", "200"}, {"set-cookie", "foo"}, {"set-cookie", "bar"}};

  auto response = sendRequestAndWaitForResponse(default_request_headers_, 0, response_headers, 0);

  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());

  std::vector<absl::string_view> out;
  Http::HeaderUtility::getAllOfHeader(response->headers(), "set-cookie", out);
  ASSERT_EQ(out.size(), 2);
  ASSERT_EQ(out[0], "foo");
  ASSERT_EQ(out[1], "bar");
}

// Resets the downstream stream immediately and verifies that we clean up everything.
TEST_P(ProtocolIntegrationTest, TestDownstreamResetIdleTimeout) {
  config_helper_.setDownstreamHttpIdleTimeout(std::chrono::milliseconds(100));

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);

  EXPECT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_,
                                                        TestUtility::DefaultTimeout,
                                                        max_request_headers_kb_));

  EXPECT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));

  if (downstreamProtocol() == Http::CodecClient::Type::HTTP1) {
    codec_client_->close();
  } else {
    codec_client_->sendReset(encoder_decoder.first);
  }

  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  codec_client_->waitForDisconnect();
}

// Test connection is closed after single request processed.
TEST_P(ProtocolIntegrationTest, ConnDurationTimeoutBasic) {
  config_helper_.setDownstreamMaxConnectionDuration(std::chrono::milliseconds(500));
  config_helper_.addConfigModifier(
      [](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
        hcm.mutable_drain_timeout()->set_seconds(1);
      });
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 1024);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);
  response->waitForEndStream();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_TRUE(response->complete());
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_cx_total", 1);
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_rq_200", 1);

  ASSERT_TRUE(codec_client_->waitForDisconnect(std::chrono::milliseconds(10000)));
  test_server_->waitForCounterGe("http.config_test.downstream_cx_max_duration_reached", 1);
}

// Test inflight request is processed correctly when timeout fires during request processing.
TEST_P(ProtocolIntegrationTest, ConnDurationInflightRequest) {
  config_helper_.setDownstreamMaxConnectionDuration(std::chrono::milliseconds(500));
  config_helper_.addConfigModifier(
      [](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
        hcm.mutable_drain_timeout()->set_seconds(1);
      });
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 1024);
  waitForNextUpstreamRequest();

  // block and wait for counter to increase
  test_server_->waitForCounterGe("http.config_test.downstream_cx_max_duration_reached", 1);

  // ensure request processed correctly
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);
  response->waitForEndStream();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_TRUE(response->complete());
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_cx_total", 1);
  test_server_->waitForCounterGe("cluster.cluster_0.upstream_rq_200", 1);

  ASSERT_TRUE(codec_client_->waitForDisconnect(std::chrono::milliseconds(10000)));
}

// Test connection is closed if no http requests were processed
TEST_P(ProtocolIntegrationTest, ConnDurationTimeoutNoHttpRequest) {
  config_helper_.setDownstreamMaxConnectionDuration(std::chrono::milliseconds(500));
  config_helper_.addConfigModifier(
      [](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm) {
        hcm.mutable_drain_timeout()->set_seconds(1);
      });
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  ASSERT_TRUE(codec_client_->waitForDisconnect(std::chrono::milliseconds(10000)));
  test_server_->waitForCounterGe("http.config_test.downstream_cx_max_duration_reached", 1);
}

// For tests which focus on downstream-to-Envoy behavior, and don't need to be
// run with both HTTP/1 and HTTP/2 upstreams.
INSTANTIATE_TEST_SUITE_P(Protocols, DownstreamProtocolIntegrationTest,
                         testing::ValuesIn(HttpProtocolIntegrationTest::getProtocolTestParams(
                             {Http::CodecClient::Type::HTTP1, Http::CodecClient::Type::HTTP2},
                             {FakeHttpConnection::Type::HTTP1})),
                         HttpProtocolIntegrationTest::protocolTestParamsToString);

INSTANTIATE_TEST_SUITE_P(Protocols, ProtocolIntegrationTest,
                         testing::ValuesIn(HttpProtocolIntegrationTest::getProtocolTestParams()),
                         HttpProtocolIntegrationTest::protocolTestParamsToString);

} // namespace Envoy
