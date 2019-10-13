#include "common/network/utility.h"

#include "test/test_common/network_utility.h"

#include "gtest/gtest.h"
#include "stress_test_downstream.h"
#include "stress_test_upstream.h"

namespace Envoy {
namespace Stress {

/**
 * Test of the StressTest::Client against the StressTest::Server without an Envoy intermediary.
 */
class StressTestSelfTest
    : public testing::TestWithParam<std::tuple<std::string, std::string, std::string>>,
      protected Logger::Loggable<Logger::Id::testing> {
public:
  StressTestSelfTest()
      : transport_socket_factory_(), ip_version_(ipVersion(std::get<1>(GetParam()))),
        http_type_(httpType(std::get<0>(GetParam()))),
        use_grpc_(0 == std::get<2>(GetParam()).compare("gRPC")),
        listening_socket_(Network::Utility::parseInternetAddressAndPort(fmt::format(
                              "{}:{}", Network::Test::getAnyAddressUrlString(ip_version_), 0)),
                          nullptr, true),
        client_("client"),
        server_("server", listening_socket_, transport_socket_factory_, http_type_) {}

protected:
  Network::RawBufferSocketFactory transport_socket_factory_;
  Network::Address::IpVersion ip_version_;
  Http::CodecClient::Type http_type_;
  bool use_grpc_;

  Network::TcpListenSocket listening_socket_;
  Client client_;
  Server server_;
};

INSTANTIATE_TEST_SUITE_P(RuntimesAndLanguages, StressTestSelfTest,
                         testing::Combine(testing::Values("http1", "http2"),
                                          testing::Values("IPv4", "IPv6"),
                                          testing::Values("HTTP")));

TEST_P(StressTestSelfTest, HappyPath) {
  // Logger::Registry::setLogLevel(spdlog::level::info);
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;

  //
  // Server Setup
  //

  try {
    // Take a really long time (500 msec) to send a 200 OK response.
    ServerCallbackHelper server_callbacks(
        [use_grpc = use_grpc_](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
          if (use_grpc) {
            ProtobufWkt::Value response;
            response.set_string_value("response");
            stream.sendGrpcResponse(Grpc::Status::Ok, response);
            return;
          }

          Http::TestHeaderMapImpl response{{":status", "200"}};
          stream.sendResponseHeaders(response);
        });
    server_.start(server_callbacks);

    //
    // Client setup
    //

    Network::Address::InstanceConstSharedPtr address = listening_socket_.localAddress();
    LoadGenerator load_generator(client_, transport_socket_factory_, http_type_, address);

    //
    // Exec test and wait for it to finish
    //

    Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{
        {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "host"}}};
    load_generator.run(connections_to_initiate, requests_to_send, std::move(request));

    // wait until the server has closed all connections created by the client
    server_callbacks.wait(load_generator.connectionsEstablished());

    //
    // Evaluate test
    //

    // All client connections are successfully established.
    EXPECT_EQ(load_generator.stats().connect_successes_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().connect_failures_.value(), 0);
    // Client close callback called for every client connection.
    EXPECT_EQ(load_generator.stats().local_closes_.value(), connections_to_initiate);
    // Client response callback is called for every request sent
    EXPECT_EQ(load_generator.stats().responses_received_.value(), requests_to_send);
    // Every response was a 2xx class
    EXPECT_EQ(load_generator.stats().class_2xx_.value(), requests_to_send);
    EXPECT_EQ(load_generator.stats().class_3xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_4xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_5xx_.value(), 0);
    // No client sockets are rudely closed by server / no client sockets are reset.
    EXPECT_EQ(load_generator.stats().remote_closes_.value(), 0);
    EXPECT_EQ(load_generator.stats().response_timeouts_.value(), 0);

    // Server accept callback is called for every client connection initiated.
    EXPECT_EQ(server_callbacks.stats().accepts_.value(), connections_to_initiate);
    // Server request callback is called for every client request sent
    EXPECT_EQ(server_callbacks.stats().requests_received_.value(), requests_to_send);
    // Server does not close its own sockets but instead relies on the client to initiate the close
    EXPECT_EQ(server_callbacks.stats().local_closes_.value(), 0);
    // Server sees a client-initiated close for every socket it accepts
    EXPECT_EQ(server_callbacks.stats().remote_closes_.value(),
              server_callbacks.stats().accepts_.value());
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ip_version_) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }
}

TEST_P(StressTestSelfTest, AcceptAndClose) {
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;

  //
  // Server Setup
  //

  try {
    // Immediately close any connection accepted.
    ServerCallbackHelper server_callbacks(
        [](ServerConnection&, ServerStream&, Http::HeaderMapPtr&&) {
          GTEST_FATAL_FAILURE_("Connections immediately closed so no response should be received");
        },
        [](ServerConnection&) -> ServerCallbackResult { return ServerCallbackResult::CLOSE; });

    server_.start(server_callbacks);

    //
    // Client setup
    //

    Network::Address::InstanceConstSharedPtr address = listening_socket_.localAddress();
    LoadGenerator load_generator(client_, transport_socket_factory_, http_type_, address);

    //
    // Exec test and wait for it to finish
    //

    Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{
        {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "host"}}};
    load_generator.run(connections_to_initiate, requests_to_send, std::move(request));

    // wait until the server has closed all connections created by the client
    server_callbacks.wait(load_generator.connectionsEstablished());

    //
    // Evaluate test
    //

    // Assert that all connections succeed but no responses are received and the server closes the
    // connections.
    EXPECT_EQ(load_generator.stats().connect_successes_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().connect_failures_.value(), 0);
    EXPECT_EQ(load_generator.stats().remote_closes_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().local_closes_.value(), 0);
    EXPECT_EQ(load_generator.stats().responses_received_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_2xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_3xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_4xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_5xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().response_timeouts_.value(), 0);

    // Server accept callback is called for every client connection initiated.
    EXPECT_EQ(server_callbacks.stats().accepts_.value(), connections_to_initiate);
    // Server request callback is never called
    EXPECT_EQ(server_callbacks.stats().requests_received_.value(), 0);
    // Server closes every connection
    EXPECT_EQ(server_callbacks.stats().accepts_.value(),
              server_callbacks.stats().local_closes_.value());
    EXPECT_EQ(server_callbacks.stats().remote_closes_.value(), 0);
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ip_version_) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }
}

TEST_P(StressTestSelfTest, SlowResponse) {
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;

  //
  // Server Setup
  //

  try {
    // Take a really long time (500 msec) to send a 200 OK response.
    ServerCallbackHelper server_callbacks(
        [](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
          Http::TestHeaderMapImpl response{{":status", "200"}};
          stream.sendResponseHeaders(response, std::chrono::milliseconds(500));
        });

    server_.start(server_callbacks);

    //
    // Client setup
    //

    Network::Address::InstanceConstSharedPtr address = listening_socket_.localAddress();
    LoadGenerator load_generator(client_, transport_socket_factory_, http_type_, address);

    //
    // Exec test and wait for it to finish
    //

    Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{
        {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "host"}}};
    load_generator.run(connections_to_initiate, requests_to_send, std::move(request),
                       std::chrono::milliseconds(250));

    // wait until the server has closed all connections created by the client
    server_callbacks.wait(load_generator.connectionsEstablished());

    //
    // Evaluate test
    //

    // Assert that all connections succeed but all responses timeout leading to local closing of all
    // connections.
    EXPECT_EQ(load_generator.stats().connect_successes_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().connect_failures_.value(), 0);
    EXPECT_EQ(load_generator.stats().response_timeouts_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().local_closes_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().remote_closes_.value(), 0);
    EXPECT_EQ(load_generator.stats().responses_received_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_2xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_3xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_4xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_5xx_.value(), 0);

    // Server accept callback is called for every client connection initiated.
    EXPECT_EQ(server_callbacks.stats().accepts_.value(), connections_to_initiate);
    // Server receives a request on each connection
    EXPECT_EQ(server_callbacks.stats().requests_received_.value(), connections_to_initiate);
    // Server sees that the client closes each connection after it gives up
    EXPECT_EQ(server_callbacks.stats().accepts_.value(),
              server_callbacks.stats().remote_closes_.value());
    EXPECT_EQ(server_callbacks.stats().local_closes_.value(), 0);
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ip_version_) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }
}

TEST_P(StressTestSelfTest, NoServer) {
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;

  // Create a listening socket bound to an ephemeral port picked by the kernel, but don't create a
  // server to call listen() on it. Result will be ECONNREFUSEDs and we won't accidentally send
  // connects to another process.

  try {
    Network::TcpListenSocket listening_socket(loopbackAddress(ip_version_, 0), nullptr, true);
    Network::Address::InstanceConstSharedPtr address{
        loopbackAddress(ip_version_, listening_socket.localAddress()->ip()->port())};

    //
    // Client setup
    //

    LoadGenerator load_generator(client_, transport_socket_factory_, http_type_, address);

    //
    // Exec test and wait for it to finish
    //

    Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{
        {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "host"}}};
    load_generator.run(connections_to_initiate, requests_to_send, std::move(request));

    //
    // Evaluate test
    //

    // All client connections fail
    EXPECT_EQ(load_generator.stats().connect_failures_.value(), connections_to_initiate);
    // And nothing else happens
    EXPECT_EQ(load_generator.stats().connect_successes_.value(), 0);
    EXPECT_EQ(load_generator.stats().local_closes_.value(), 0);
    EXPECT_EQ(load_generator.stats().response_timeouts_.value(), 0);
    EXPECT_EQ(load_generator.stats().responses_received_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_2xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_3xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_4xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_5xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().remote_closes_.value(), 0);
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ip_version_) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }
}

TEST_P(StressTestSelfTest, NoAccept) {
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;

  //
  // Server Setup
  //

  try {

    ServerCallbackHelper server_callbacks; // sends a 200 OK to everything
    server_.start(server_callbacks);

    // but don't call accept() on the listening socket
    server_.stopAcceptingConnections();

    //
    // Client setup
    //

    Network::Address::InstanceConstSharedPtr address = listening_socket_.localAddress();
    LoadGenerator load_generator(client_, transport_socket_factory_, http_type_, address);

    //
    // Exec test and wait for it to finish
    //

    Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{
        {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "host"}}};
    load_generator.run(connections_to_initiate, requests_to_send, std::move(request),
                       std::chrono::milliseconds(250));

    //
    // Evaluate test
    //

    // Assert that all connections succeed but all responses timeout leading to local closing of all
    // connections.
    EXPECT_EQ(load_generator.stats().connect_successes_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().connect_failures_.value(), 0);
    EXPECT_EQ(load_generator.stats().response_timeouts_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().local_closes_.value(), connections_to_initiate);
    EXPECT_EQ(load_generator.stats().remote_closes_.value(), 0);
    EXPECT_EQ(load_generator.stats().responses_received_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_2xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_3xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_4xx_.value(), 0);
    EXPECT_EQ(load_generator.stats().class_5xx_.value(), 0);

    // From the server point of view, nothing happened
    EXPECT_EQ(server_callbacks.stats().accepts_.value(), 0);
    EXPECT_EQ(server_callbacks.stats().requests_received_.value(), 0);
    EXPECT_EQ(server_callbacks.stats().remote_closes_.value(), 0);
    EXPECT_EQ(server_callbacks.stats().local_closes_.value(), 0);
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ip_version_) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }
}

} // namespace Stress
} // namespace Envoy
