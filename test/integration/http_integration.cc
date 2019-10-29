#include "test/integration/http_integration.h"

#include <functional>
#include <list>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "envoy/buffer/buffer.h"
#include "envoy/event/dispatcher.h"
#include "envoy/http/header_map.h"
#include "envoy/network/address.h"
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
#include "test/integration/test_host_predicate_config.h"
#include "test/integration/utility.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/registry.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace {

envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager::CodecType
typeToCodecType(Http::CodecClient::Type type) {
  switch (type) {
  case Http::CodecClient::Type::HTTP1:
    return envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager::
        HTTP1;
  case Http::CodecClient::Type::HTTP2:
    return envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager::
        HTTP2;
  default:
    RELEASE_ASSERT(0, "");
  }
}

} // namespace

IntegrationCodecClient::IntegrationCodecClient(
    Event::Dispatcher& dispatcher, Network::ClientConnectionPtr&& conn,
    Upstream::HostDescriptionConstSharedPtr host_description, CodecClient::Type type)
    : CodecClientProd(type, std::move(conn), host_description, dispatcher), dispatcher_(dispatcher),
      callbacks_(*this), codec_callbacks_(*this) {
  connection_->addConnectionCallbacks(callbacks_);
  setCodecConnectionCallbacks(codec_callbacks_);
  dispatcher.run(Event::Dispatcher::RunType::Block);
}

void IntegrationCodecClient::flushWrite() {
  connection_->dispatcher().run(Event::Dispatcher::RunType::NonBlock);
  // NOTE: We should run blocking until all the body data is flushed.
}

IntegrationStreamDecoderPtr
IntegrationCodecClient::makeHeaderOnlyRequest(const Http::HeaderMap& headers) {
  auto response = std::make_unique<IntegrationStreamDecoder>(dispatcher_);
  Http::StreamEncoder& encoder = newStream(*response);
  encoder.getStream().addCallbacks(*response);
  encoder.encodeHeaders(headers, true);
  flushWrite();
  return response;
}

IntegrationStreamDecoderPtr
IntegrationCodecClient::makeRequestWithBody(const Http::HeaderMap& headers, uint64_t body_size) {
  return makeRequestWithBody(headers, std::string(body_size, 'a'));
}

IntegrationStreamDecoderPtr
IntegrationCodecClient::makeRequestWithBody(const Http::HeaderMap& headers,
                                            const std::string& body) {
  auto response = std::make_unique<IntegrationStreamDecoder>(dispatcher_);
  Http::StreamEncoder& encoder = newStream(*response);
  encoder.getStream().addCallbacks(*response);
  encoder.encodeHeaders(headers, false);
  Buffer::OwnedImpl data(body);
  encoder.encodeData(data, true);
  flushWrite();
  return response;
}

void IntegrationCodecClient::sendData(Http::StreamEncoder& encoder, absl::string_view data,
                                      bool end_stream) {
  Buffer::OwnedImpl buffer_data(data.data(), data.size());
  encoder.encodeData(buffer_data, end_stream);
  flushWrite();
}

void IntegrationCodecClient::sendData(Http::StreamEncoder& encoder, Buffer::Instance& data,
                                      bool end_stream) {
  encoder.encodeData(data, end_stream);
  flushWrite();
}

void IntegrationCodecClient::sendData(Http::StreamEncoder& encoder, uint64_t size,
                                      bool end_stream) {
  Buffer::OwnedImpl data(std::string(size, 'a'));
  sendData(encoder, data, end_stream);
}

void IntegrationCodecClient::sendTrailers(Http::StreamEncoder& encoder,
                                          const Http::HeaderMap& trailers) {
  encoder.encodeTrailers(trailers);
  flushWrite();
}

void IntegrationCodecClient::sendReset(Http::StreamEncoder& encoder) {
  encoder.getStream().resetStream(Http::StreamResetReason::LocalReset);
  flushWrite();
}

void IntegrationCodecClient::sendMetadata(Http::StreamEncoder& encoder,
                                          Http::MetadataMap metadata_map) {
  Http::MetadataMapPtr metadata_map_ptr = std::make_unique<Http::MetadataMap>(metadata_map);
  Http::MetadataMapVector metadata_map_vector;
  metadata_map_vector.push_back(std::move(metadata_map_ptr));
  encoder.encodeMetadata(metadata_map_vector);
  flushWrite();
}

std::pair<Http::StreamEncoder&, IntegrationStreamDecoderPtr>
IntegrationCodecClient::startRequest(const Http::HeaderMap& headers) {
  auto response = std::make_unique<IntegrationStreamDecoder>(dispatcher_);
  Http::StreamEncoder& encoder = newStream(*response);
  encoder.getStream().addCallbacks(*response);
  encoder.encodeHeaders(headers, false);
  flushWrite();
  return {encoder, std::move(response)};
}

bool IntegrationCodecClient::waitForDisconnect(std::chrono::milliseconds time_to_wait) {
  Event::TimerPtr wait_timer;
  bool wait_timer_triggered = false;
  if (time_to_wait.count()) {
    wait_timer = connection_->dispatcher().createTimer([this, &wait_timer_triggered] {
      connection_->dispatcher().exit();
      wait_timer_triggered = true;
    });
    wait_timer->enableTimer(time_to_wait);
  }

  connection_->dispatcher().run(Event::Dispatcher::RunType::Block);

  // Disable the timer if it was created. This call is harmless if the timer already triggered.
  if (wait_timer) {
    wait_timer->disableTimer();
  }

  if (wait_timer_triggered && !disconnected_) {
    return false;
  }
  EXPECT_TRUE(disconnected_);

  return true;
}

void IntegrationCodecClient::ConnectionCallbacks::onEvent(Network::ConnectionEvent event) {
  parent_.last_connection_event_ = event;
  if (event == Network::ConnectionEvent::Connected) {
    parent_.connected_ = true;
    parent_.connection_->dispatcher().exit();
  } else if (event == Network::ConnectionEvent::RemoteClose) {
    parent_.disconnected_ = true;
    parent_.connection_->dispatcher().exit();
  } else {
    parent_.disconnected_ = true;
  }
}

IntegrationCodecClientPtr HttpIntegrationTest::makeHttpConnection(uint32_t port) {
  return makeHttpConnection(makeClientConnection(port));
}

IntegrationCodecClientPtr
HttpIntegrationTest::makeRawHttpConnection(Network::ClientConnectionPtr&& conn) {
  std::shared_ptr<Upstream::MockClusterInfo> cluster{new NiceMock<Upstream::MockClusterInfo>()};
  cluster->max_response_headers_count_ = 200;
  cluster->http2_settings_.allow_connect_ = true;
  cluster->http2_settings_.allow_metadata_ = true;
  Upstream::HostDescriptionConstSharedPtr host_description{Upstream::makeTestHostDescription(
      cluster, fmt::format("tcp://{}:80", Network::Test::getLoopbackAddressUrlString(version_)))};
  return std::make_unique<IntegrationCodecClient>(*dispatcher_, std::move(conn), host_description,
                                                  downstream_protocol_);
}

IntegrationCodecClientPtr
HttpIntegrationTest::makeHttpConnection(Network::ClientConnectionPtr&& conn) {
  auto codec = makeRawHttpConnection(std::move(conn));
  EXPECT_TRUE(codec->connected());
  return codec;
}

HttpIntegrationTest::HttpIntegrationTest(Http::CodecClient::Type downstream_protocol,
                                         Network::Address::IpVersion version,
                                         const std::string& config)
    : HttpIntegrationTest::HttpIntegrationTest(
          downstream_protocol,
          [version](int) {
            return Network::Utility::parseInternetAddress(
                Network::Test::getAnyAddressString(version), 0);
          },
          version, config) {}

HttpIntegrationTest::HttpIntegrationTest(Http::CodecClient::Type downstream_protocol,
                                         const InstanceConstSharedPtrFn& upstream_address_fn,
                                         Network::Address::IpVersion version,
                                         const std::string& config)
    : BaseIntegrationTest(upstream_address_fn, version, config),
      downstream_protocol_(downstream_protocol) {
  // Legacy integration tests expect the default listener to be named "http" for
  // lookupPort calls.
  config_helper_.renameListener("http");
  config_helper_.setClientCodec(typeToCodecType(downstream_protocol_));
}

void HttpIntegrationTest::useAccessLog() {
  access_log_name_ = TestEnvironment::temporaryPath(TestUtility::uniqueFilename());
  ASSERT_TRUE(config_helper_.setAccessLog(access_log_name_));
}

HttpIntegrationTest::~HttpIntegrationTest() {
  cleanupUpstreamAndDownstream();
  test_server_.reset();
  fake_upstreams_.clear();
}

std::string HttpIntegrationTest::waitForAccessLog(const std::string& filename) {
  // Wait a max of 1s for logs to flush to disk.
  for (int i = 0; i < 1000; ++i) {
    std::string contents = TestEnvironment::readFileToStringForTest(filename, false);
    if (contents.length() > 0) {
      return contents;
    }
    usleep(1000);
  }
  RELEASE_ASSERT(0, "Timed out waiting for access log");
  return "";
}

void HttpIntegrationTest::setDownstreamProtocol(Http::CodecClient::Type downstream_protocol) {
  downstream_protocol_ = downstream_protocol;
  config_helper_.setClientCodec(typeToCodecType(downstream_protocol_));
}

IntegrationStreamDecoderPtr HttpIntegrationTest::sendRequestAndWaitForResponse(
    const Http::TestHeaderMapImpl& request_headers, uint32_t request_body_size,
    const Http::TestHeaderMapImpl& response_headers, uint32_t response_size, int upstream_index,
    std::chrono::milliseconds time) {
  ASSERT(codec_client_ != nullptr);
  // Send the request to Envoy.
  IntegrationStreamDecoderPtr response;
  if (request_body_size) {
    response = codec_client_->makeRequestWithBody(request_headers, request_body_size);
  } else {
    response = codec_client_->makeHeaderOnlyRequest(request_headers);
  }
  waitForNextUpstreamRequest(upstream_index, time);
  // Send response headers, and end_stream if there is no response body.
  upstream_request_->encodeHeaders(response_headers, response_size == 0);
  // Send any response data, with end_stream true.
  if (response_size) {
    upstream_request_->encodeData(response_size, true);
  }
  // Wait for the response to be read by the codec client.
  response->waitForEndStream();
  return response;
}

void HttpIntegrationTest::cleanupUpstreamAndDownstream() {
  // Close the upstream connection first. If there's an outstanding request,
  // closing the client may result in a FIN being sent upstream, and FakeConnectionBase::close
  // will interpret that as an unexpected disconnect. The codec client is not
  // subject to the same failure mode.
  if (fake_upstream_connection_) {
    AssertionResult result = fake_upstream_connection_->close();
    RELEASE_ASSERT(result, result.message());
    result = fake_upstream_connection_->waitForDisconnect();
    RELEASE_ASSERT(result, result.message());
    fake_upstream_connection_.reset();
  }
  if (codec_client_) {
    codec_client_->close();
  }
}

void HttpIntegrationTest::sendRequestAndVerifyResponse(
    const Http::TestHeaderMapImpl& request_headers, const int request_size,
    const Http::TestHeaderMapImpl& response_headers, const int response_size,
    const int backend_idx) {
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto response = sendRequestAndWaitForResponse(request_headers, request_size, response_headers,
                                                response_size, backend_idx);
  verifyResponse(std::move(response), "200", response_headers, std::string(response_size, 'a'));

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(request_size, upstream_request_->bodyLength());
  cleanupUpstreamAndDownstream();
}

void HttpIntegrationTest::verifyResponse(IntegrationStreamDecoderPtr response,
                                         const std::string& response_code,
                                         const Http::TestHeaderMapImpl& expected_headers,
                                         const std::string& expected_body) {
  EXPECT_TRUE(response->complete());
  EXPECT_EQ(response_code, response->headers().Status()->value().getStringView());
  expected_headers.iterate(
      [](const Http::HeaderEntry& header, void* context) -> Http::HeaderMap::Iterate {
        auto response_headers = static_cast<Http::HeaderMap*>(context);
        const Http::HeaderEntry* entry =
            response_headers->get(Http::LowerCaseString{std::string(header.key().getStringView())});
        EXPECT_NE(entry, nullptr);
        EXPECT_EQ(header.value().getStringView(), entry->value().getStringView());
        return Http::HeaderMap::Iterate::Continue;
      },
      const_cast<void*>(static_cast<const void*>(&response->headers())));

  EXPECT_EQ(response->body(), expected_body);
}

uint64_t
HttpIntegrationTest::waitForNextUpstreamRequest(const std::vector<uint64_t>& upstream_indices,
                                                std::chrono::milliseconds connection_wait_timeout) {
  uint64_t upstream_with_request;
  // If there is no upstream connection, wait for it to be established.
  if (!fake_upstream_connection_) {

    AssertionResult result = AssertionFailure();
    for (auto upstream_index : upstream_indices) {
      result = fake_upstreams_[upstream_index]->waitForHttpConnection(
          *dispatcher_, fake_upstream_connection_, connection_wait_timeout, max_request_headers_kb_,
          max_request_headers_count_);
      if (result) {
        upstream_with_request = upstream_index;
        break;
      }
    }
    RELEASE_ASSERT(result, result.message());
  }
  // Wait for the next stream on the upstream connection.
  AssertionResult result =
      fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_);
  RELEASE_ASSERT(result, result.message());
  // Wait for the stream to be completely received.
  result = upstream_request_->waitForEndStream(*dispatcher_);
  RELEASE_ASSERT(result, result.message());

  return upstream_with_request;
}

void HttpIntegrationTest::waitForNextUpstreamRequest(
    uint64_t upstream_index, std::chrono::milliseconds connection_wait_timeout) {
  waitForNextUpstreamRequest(std::vector<uint64_t>({upstream_index}), connection_wait_timeout);
}

void HttpIntegrationTest::checkSimpleRequestSuccess(uint64_t expected_request_size,
                                                    uint64_t expected_response_size,
                                                    IntegrationStreamDecoder* response) {
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(expected_request_size, upstream_request_->bodyLength());

  ASSERT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(expected_response_size, response->body().size());
}

void HttpIntegrationTest::testRouterRequestAndResponseWithBody(
    uint64_t request_size, uint64_t response_size, bool big_header,
    ConnectionCreationFunction* create_connection) {
  initialize();
  codec_client_ = makeHttpConnection(
      create_connection ? ((*create_connection)()) : makeClientConnection((lookupPort("http"))));
  Http::TestHeaderMapImpl request_headers{
      {":method", "POST"},    {":path", "/test/long/url"}, {":scheme", "http"},
      {":authority", "host"}, {"x-lyft-user-id", "123"},   {"x-forwarded-for", "10.0.0.1"}};
  if (big_header) {
    request_headers.addCopy("big", std::string(4096, 'a'));
  }
  auto response = sendRequestAndWaitForResponse(request_headers, request_size,
                                                default_response_headers_, response_size);
  checkSimpleRequestSuccess(request_size, response_size, response.get());
}

IntegrationStreamDecoderPtr
HttpIntegrationTest::makeHeaderOnlyRequest(ConnectionCreationFunction* create_connection,
                                           int upstream_index, const std::string& path,
                                           const std::string& authority) {
  // This is called multiple times per test in ads_integration_test. Only call
  // initialize() the first time.
  if (!initialized()) {
    initialize();
  }
  codec_client_ = makeHttpConnection(
      create_connection ? ((*create_connection)()) : makeClientConnection((lookupPort("http"))));
  Http::TestHeaderMapImpl request_headers{{":method", "GET"},
                                          {":path", path},
                                          {":scheme", "http"},
                                          {":authority", authority},
                                          {"x-lyft-user-id", "123"}};
  return sendRequestAndWaitForResponse(request_headers, 0, default_response_headers_, 0,
                                       upstream_index);
}

void HttpIntegrationTest::testRouterHeaderOnlyRequestAndResponse(
    ConnectionCreationFunction* create_connection, int upstream_index, const std::string& path,
    const std::string& authority) {
  auto response = makeHeaderOnlyRequest(create_connection, upstream_index, path, authority);
  checkSimpleRequestSuccess(0U, 0U, response.get());
}

// Change the default route to be restrictive, and send a request to an alternate route.
void HttpIntegrationTest::testRouterNotFound() {
  config_helper_.setDefaultHostAndRoute("foo.com", "/found");
  initialize();

  BufferingStreamDecoderPtr response = IntegrationUtil::makeSingleRequest(
      lookupPort("http"), "GET", "/notfound", "", downstream_protocol_, version_);
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("404", response->headers().Status()->value().getStringView());
}

// Change the default route to be restrictive, and send a POST to an alternate route.
void HttpIntegrationTest::testRouterNotFoundWithBody() {
  config_helper_.setDefaultHostAndRoute("foo.com", "/found");
  initialize();

  BufferingStreamDecoderPtr response = IntegrationUtil::makeSingleRequest(
      lookupPort("http"), "POST", "/notfound", "foo", downstream_protocol_, version_);
  ASSERT_TRUE(response->complete());
  EXPECT_EQ("404", response->headers().Status()->value().getStringView());
}

void HttpIntegrationTest::testRouterUpstreamDisconnectBeforeRequestComplete() {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  auto response = std::move(encoder_decoder.second);

  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));

  ASSERT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));
  ASSERT_TRUE(upstream_request_->waitForHeadersComplete());
  ASSERT_TRUE(fake_upstream_connection_->close());
  ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  response->waitForEndStream();

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    codec_client_->waitForDisconnect();
  } else {
    codec_client_->close();
  }

  EXPECT_FALSE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("503", response->headers().Status()->value().getStringView());
  EXPECT_EQ("upstream connect error or disconnect/reset before headers. reset reason: connection "
            "termination",
            response->body());
}

void HttpIntegrationTest::testRouterUpstreamDisconnectBeforeResponseComplete(
    ConnectionCreationFunction* create_connection) {
  initialize();
  codec_client_ = makeHttpConnection(
      create_connection ? ((*create_connection)()) : makeClientConnection((lookupPort("http"))));
  auto response = codec_client_->makeHeaderOnlyRequest(default_request_headers_);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, false);
  ASSERT_TRUE(fake_upstream_connection_->close());
  ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    codec_client_->waitForDisconnect();
  } else {
    response->waitForReset();
    codec_client_->close();
  }

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());

  EXPECT_FALSE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(0U, response->body().size());
}

void HttpIntegrationTest::testRouterDownstreamDisconnectBeforeRequestComplete(
    ConnectionCreationFunction* create_connection) {
  initialize();

  codec_client_ = makeHttpConnection(
      create_connection ? ((*create_connection)()) : makeClientConnection((lookupPort("http"))));
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  auto response = std::move(encoder_decoder.second);
  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  ASSERT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));
  ASSERT_TRUE(upstream_request_->waitForHeadersComplete());
  codec_client_->close();

  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  EXPECT_FALSE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());

  EXPECT_FALSE(response->complete());
}

void HttpIntegrationTest::testRouterDownstreamDisconnectBeforeResponseComplete(
    ConnectionCreationFunction* create_connection) {
#ifdef __APPLE__
  // Skip this test on macOS: we can't detect the early close on macOS, and we
  // won't clean up the upstream connection until it times out. See #4294.
  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    return;
  }
#endif
  initialize();
  codec_client_ = makeHttpConnection(
      create_connection ? ((*create_connection)()) : makeClientConnection((lookupPort("http"))));
  auto response = codec_client_->makeHeaderOnlyRequest(default_request_headers_);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, false);
  response->waitForBodyData(512);
  codec_client_->close();

  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());

  EXPECT_FALSE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
}

void HttpIntegrationTest::testRouterUpstreamResponseBeforeRequestComplete() {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  auto response = std::move(encoder_decoder.second);
  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  ASSERT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));
  ASSERT_TRUE(upstream_request_->waitForHeadersComplete());
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);
  response->waitForEndStream();

  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
    codec_client_->waitForDisconnect();
  } else {
    codec_client_->close();
  }

  EXPECT_FALSE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
}

void HttpIntegrationTest::testRetry() {
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
void HttpIntegrationTest::testRetryAttemptCountHeader() {
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

void HttpIntegrationTest::testGrpcRetry() {
  Http::TestHeaderMapImpl response_trailers{{"response1", "trailer1"}, {"grpc-status", "0"}};
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"},
                                                          {"x-forwarded-for", "10.0.0.1"},
                                                          {"x-envoy-retry-grpc-on", "cancelled"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  codec_client_->sendData(*request_encoder_, 1024, true);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(
      Http::TestHeaderMapImpl{{":status", "200"}, {"grpc-status", "1"}}, false);
  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
    ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
  }
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512,
                                fake_upstreams_[0]->httpType() != FakeHttpConnection::Type::HTTP2);
  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP2) {
    upstream_request_->encodeTrailers(response_trailers);
  }

  response->waitForEndStream();
  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(1024U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP2) {
    EXPECT_THAT(*response->trailers(), HeaderMapEqualRef(&response_trailers));
  }
}

void HttpIntegrationTest::testEnvoyHandling100Continue(bool additional_continue_from_upstream,
                                                       const std::string& via) {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/dynamo/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"},
                                                          {"expect", "100-continue"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  // The continue headers should arrive immediately.
  response->waitForContinueHeaders();
  ASSERT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));

  // Send the rest of the request.
  codec_client_->sendData(*request_encoder_, 10, true);
  ASSERT_TRUE(upstream_request_->waitForEndStream(*dispatcher_));
  // Verify the Expect header is stripped.
  EXPECT_EQ(nullptr, upstream_request_->headers().get(Http::Headers::get().Expect));
  if (via.empty()) {
    EXPECT_EQ(nullptr, upstream_request_->headers().get(Http::Headers::get().Via));
  } else {
    EXPECT_EQ(via,
              upstream_request_->headers().get(Http::Headers::get().Via)->value().getStringView());
  }

  if (additional_continue_from_upstream) {
    // Make sure if upstream sends an 100-Continue Envoy doesn't send its own and proxy the one
    // from upstream!
    upstream_request_->encode100ContinueHeaders(Http::TestHeaderMapImpl{{":status", "100"}});
  }
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(12, true);

  response->waitForEndStream();
  ASSERT_TRUE(response->complete());
  ASSERT(response->continue_headers() != nullptr);
  EXPECT_EQ("100", response->continue_headers()->Status()->value().getStringView());
  EXPECT_EQ(nullptr, response->continue_headers()->Via());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  if (via.empty()) {
    EXPECT_EQ(nullptr, response->headers().Via());
  } else {
    EXPECT_EQ(via.c_str(), response->headers().Via()->value().getStringView());
  }
}

void HttpIntegrationTest::testEnvoyProxying100Continue(bool continue_before_upstream_complete,
                                                       bool with_encoder_filter) {
  if (with_encoder_filter) {
    // Because 100-continue only affects encoder filters, make sure it plays well with one.
    config_helper_.addFilter("name: envoy.cors");
    config_helper_.addConfigModifier(
        [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
            -> void {
          auto* route_config = hcm.mutable_route_config();
          auto* virtual_host = route_config->mutable_virtual_hosts(0);
          {
            auto* cors = virtual_host->mutable_cors();
            cors->mutable_allow_origin_string_match()->Add()->set_exact("*");
            cors->set_allow_headers("content-type,x-grpc-web");
            cors->set_allow_methods("GET,POST");
          }
        });
  }
  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void { hcm.set_proxy_100_continue(true); });
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "GET"},
                                                          {":path", "/dynamo/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"},
                                                          {"expect", "100-continue"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);

  // Wait for the request headers to be received upstream.
  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  ASSERT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));

  if (continue_before_upstream_complete) {
    // This case tests sending on 100-Continue headers before the client has sent all the
    // request data.
    upstream_request_->encode100ContinueHeaders(Http::TestHeaderMapImpl{{":status", "100"}});
    response->waitForContinueHeaders();
  }
  // Send all of the request data and wait for it to be received upstream.
  codec_client_->sendData(*request_encoder_, 10, true);
  ASSERT_TRUE(upstream_request_->waitForEndStream(*dispatcher_));

  if (!continue_before_upstream_complete) {
    // This case tests forwarding 100-Continue after the client has sent all data.
    upstream_request_->encode100ContinueHeaders(Http::TestHeaderMapImpl{{":status", "100"}});
    response->waitForContinueHeaders();
  }
  // Now send the rest of the response.
  upstream_request_->encodeHeaders(default_response_headers_, true);
  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  ASSERT(response->continue_headers() != nullptr);
  EXPECT_EQ("100", response->continue_headers()->Status()->value().getStringView());

  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
}

void HttpIntegrationTest::testTwoRequests(bool network_backup) {
  // if network_backup is false, this simply tests that Envoy can handle multiple
  // requests on a connection.
  //
  // If network_backup is true, the first request will explicitly set the TCP level flow control
  // as blocked as it finishes the encode and set a timer to unblock. The second stream should be
  // created while the socket appears to be in the high watermark state, and regression tests that
  // flow control will be corrected as the socket "becomes unblocked"
  if (network_backup) {
    config_helper_.addFilter(R"EOF(
  name: pause-filter
  typed_config:
    "@type": type.googleapis.com/google.protobuf.Empty
  )EOF");
  }
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  // Request 1.
  auto response = codec_client_->makeRequestWithBody(default_request_headers_, 1024);
  waitForNextUpstreamRequest();

  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, true);
  response->waitForEndStream();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(1024U, upstream_request_->bodyLength());
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());

  // Request 2.
  response = codec_client_->makeRequestWithBody(default_request_headers_, 512);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(1024, true);
  response->waitForEndStream();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(512U, upstream_request_->bodyLength());
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(1024U, response->body().size());
}

void HttpIntegrationTest::testLargeRequestHeaders(uint32_t size, uint32_t count, uint32_t max_size,
                                                  uint32_t max_count) {
  // `size` parameter dictates the size of each header that will be added to the request and `count`
  // parameter is the number of headers to be added. The actual request byte size will exceed `size`
  // due to the keys and other headers. The actual request header count will exceed `count` by four
  // due to default headers.

  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_max_request_headers_kb()->set_value(max_size);
        hcm.mutable_common_http_protocol_options()->mutable_max_headers_count()->set_value(
            max_count);
      });
  max_request_headers_kb_ = max_size;
  max_request_headers_count_ = max_count;

  Http::TestHeaderMapImpl big_headers{
      {":method", "GET"}, {":path", "/test/long/url"}, {":scheme", "http"}, {":authority", "host"}};
  // Already added four headers.
  for (unsigned int i = 0; i < count; i++) {
    big_headers.addCopy(std::to_string(i), std::string(size * 1024, 'a'));
  }

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  if (size >= max_size || count > max_count) {
    // header size includes keys too, so expect rejection when equal
    auto encoder_decoder = codec_client_->startRequest(big_headers);
    auto response = std::move(encoder_decoder.second);

    if (downstream_protocol_ == Http::CodecClient::Type::HTTP1) {
      codec_client_->waitForDisconnect();
      EXPECT_TRUE(response->complete());
      EXPECT_EQ("431", response->headers().Status()->value().getStringView());
    } else {
      response->waitForReset();
      codec_client_->close();
    }
  } else {
    auto response = sendRequestAndWaitForResponse(big_headers, 0, default_response_headers_, 0);
    EXPECT_TRUE(response->complete());
    EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  }
}

void HttpIntegrationTest::testLargeRequestTrailers(uint32_t size, uint32_t max_size) {
  // `size` parameter is the size of the trailer that will be added to the
  // request. The actual request byte size will exceed `size` due to keys
  // and other headers.

  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void { hcm.mutable_max_request_headers_kb()->set_value(max_size); });
  max_request_headers_kb_ = max_size;
  Http::TestHeaderMapImpl request_trailers{{"trailer", "trailer"}};
  request_trailers.addCopy("big", std::string(size * 1024, 'a'));

  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  auto encoder_decoder = codec_client_->startRequest(default_request_headers_);
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  codec_client_->sendData(*request_encoder_, 10, false);
  codec_client_->sendTrailers(*request_encoder_, request_trailers);

  if (size >= max_size && downstream_protocol_ == Http::CodecClient::Type::HTTP2) {
    // For HTTP/2, expect a stream reset when the size of the trailers is larger than the maximum
    // limit.
    response->waitForReset();
    codec_client_->close();
    EXPECT_FALSE(response->complete());

  } else {
    waitForNextUpstreamRequest();
    upstream_request_->encodeHeaders(default_response_headers_, true);
    response->waitForEndStream();
    EXPECT_TRUE(response->complete());
  }
}

void HttpIntegrationTest::testManyRequestHeaders(std::chrono::milliseconds time) {
  max_request_headers_kb_ = 96;
  max_request_headers_count_ = 20005;

  config_helper_.addConfigModifier(
      [&](envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager& hcm)
          -> void {
        hcm.mutable_max_request_headers_kb()->set_value(max_request_headers_kb_);
        hcm.mutable_common_http_protocol_options()->mutable_max_headers_count()->set_value(
            max_request_headers_count_);
      });

  Http::TestHeaderMapImpl big_headers{
      {":method", "GET"}, {":path", "/test/long/url"}, {":scheme", "http"}, {":authority", "host"}};

  for (int i = 0; i < 20000; i++) {
    big_headers.addCopy(std::to_string(i), std::string(0, 'a'));
  }
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto response =
      sendRequestAndWaitForResponse(big_headers, 0, default_response_headers_, 0, 0, time);

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
}

void HttpIntegrationTest::testDownstreamResetBeforeResponseComplete() {
  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));

  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "GET"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"},
                                                          {"cookie", "a=b"},
                                                          {"cookie", "c=d"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  codec_client_->sendData(*request_encoder_, 0, true);
  waitForNextUpstreamRequest();

  EXPECT_EQ(upstream_request_->headers().get(Http::Headers::get().Cookie)->value(), "a=b; c=d");

  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(512, false);

  response->waitForBodyData(512);
  codec_client_->sendReset(*request_encoder_);

  if (upstreamProtocol() == FakeHttpConnection::Type::HTTP1) {
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  } else {
    ASSERT_TRUE(upstream_request_->waitForReset());
    ASSERT_TRUE(fake_upstream_connection_->close());
    ASSERT_TRUE(fake_upstream_connection_->waitForDisconnect());
  }

  codec_client_->close();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());

  EXPECT_FALSE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(512U, response->body().size());
}

void HttpIntegrationTest::testTrailers(uint64_t request_size, uint64_t response_size) {
  Http::TestHeaderMapImpl request_trailers{{"request1", "trailer1"}, {"request2", "trailer2"}};
  Http::TestHeaderMapImpl response_trailers{{"response1", "trailer1"}, {"response2", "trailer2"}};

  initialize();
  codec_client_ = makeHttpConnection(lookupPort("http"));
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"}});
  request_encoder_ = &encoder_decoder.first;
  auto response = std::move(encoder_decoder.second);
  codec_client_->sendData(*request_encoder_, request_size, false);
  codec_client_->sendTrailers(*request_encoder_, request_trailers);
  waitForNextUpstreamRequest();
  upstream_request_->encodeHeaders(default_response_headers_, false);
  upstream_request_->encodeData(response_size, false);
  upstream_request_->encodeTrailers(response_trailers);
  response->waitForEndStream();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(request_size, upstream_request_->bodyLength());
  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP2) {
    EXPECT_THAT(*upstream_request_->trailers(), HeaderMapEqualRef(&request_trailers));
  }

  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().Status()->value().getStringView());
  EXPECT_EQ(response_size, response->body().size());
  if (fake_upstreams_[0]->httpType() == FakeHttpConnection::Type::HTTP2) {
    EXPECT_THAT(*response->trailers(), HeaderMapEqualRef(&response_trailers));
  }
}

std::string HttpIntegrationTest::listenerStatPrefix(const std::string& stat_name) {
  if (version_ == Network::Address::IpVersion::v4) {
    return "listener.127.0.0.1_0." + stat_name;
  }
  return "listener.[__1]_0." + stat_name;
}
} // namespace Envoy
