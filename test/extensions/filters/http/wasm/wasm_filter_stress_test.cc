#include "envoy/server/wasm_config.h"

#include "test/stress/stress_test.h"

using namespace Envoy::Stress;

namespace Envoy {
namespace Extensions {
namespace Wasm {

static const std::string STAT_PREFIX{"wasm_filter_stress_test"};

class WasmStressTest : public Stress::StressTest,
                       public testing::TestWithParam<
                           std::tuple<std::string, std::string, std::string, std::string>> {
public:
  WasmStressTest()
      : StressTest(::ipVersion(std::get<3>(GetParam())), ::httpType(std::get<2>(GetParam()))),
        wasm_vm_{std::get<0>(GetParam())}, wasm_lang_{std::get<1>(GetParam())} {}

  const std::string& wasmVM() const { return wasm_vm_; }

  const std::string& wasmLang() const { return wasm_lang_; }

private:
  std::string wasm_vm_;
  std::string wasm_lang_;
};

class GrpcWasmStressTest : public WasmStressTest {};

class HttpWasmStressTest : public WasmStressTest {};

INSTANTIATE_TEST_SUITE_P(RuntimesAndLanguages, GrpcWasmStressTest,
                         testing::Combine(testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                                              "envoy.wasm.runtime.v8", "envoy.wasm.runtime.wavm"
#elif defined(ENVOY_WASM_V8)
                                              "envoy.wasm.runtime.v8"
#elif defined(ENVOY_WASM_WAVM)
                                              "envoy.wasm.runtime.wavm"
#endif
                                              ),
                                          testing::Values("cpp"), testing::Values("http2"),
                                          testing::Values("IPv4", "IPv6")));

INSTANTIATE_TEST_SUITE_P(RuntimesAndLanguages, HttpWasmStressTest,
                         testing::Combine(testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                                              "envoy.wasm.runtime.v8", "envoy.wasm.runtime.wavm"
#elif defined(ENVOY_WASM_V8)
                                              "envoy.wasm.runtime.v8"
#elif defined(ENVOY_WASM_WAVM)
                                              "envoy.wasm.runtime.wavm"
#endif
                                              ),
                                          testing::Values("cpp"), testing::Values("http1", "http2"),
                                          testing::Values("IPv4", "IPv6")));

TEST_P(GrpcWasmStressTest, CalloutHappyPath) {
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;
  const std::string wasm_file = absl::StrCat(
      TestEnvironment::runfilesDirectory(),
      "/test/extensions/filters/http/wasm/test_data/grpc_callout_", wasmLang(), ".wasm");
  // Must match cluster name in the wasm bundle:
  const std::string callout_cluster_name{"callout_cluster"};

  //
  // Configure the wasm filter
  //

  config_helper_.addFilter(fmt::format(R"EOF(
            name: envoy.filters.http.wasm
            typed_config:
              "@type": type.googleapis.com/envoy.config.filter.http.wasm.v2.Wasm
              config:
                vm_config:
                  runtime: "{}"
                  code:
                    filename: "{}"
)EOF",
                                       wasmVM(), wasm_file));

  //
  // This test first sends a request from wasm to a callout cluster:
  //    Client -> Envoy -> Wasm -> Callout Cluster
  // Envoy then unconditionally forwards the request to the origin cluster
  //              Envoy -> Origin Cluster
  //
  // Both Callout and Origin clusters have a single server that immediately
  // returns a 200 OK response.
  //

  addCluster(std::make_unique<ClusterHelper>(StressTest::ORIGIN_CLUSTER_NAME))
      .addServer(std::make_unique<ServerCallbackHelper>(
          [](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
            ENVOY_LOG(debug, "Origin server received request");
            Http::TestHeaderMapImpl response{{":status", "200"}};
            stream.sendResponseHeaders(response);
          }));
  addCluster(std::make_unique<ClusterHelper>(callout_cluster_name))
      .addServer(std::make_unique<ServerCallbackHelper>(
          [](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
            ENVOY_LOG(debug, "Callout server received request");
            ProtobufWkt::Value response;
            response.set_string_value("response");
            stream.sendGrpcResponse(Grpc::Status::Ok, response);
          }));

  try {
    bind();
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ipVersion()) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }

  LoadGeneratorPtr client = start();

  //
  // Exec test and wait for it to finish
  //

  Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{
      {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "host"}}};
  client->run(connections_to_initiate, requests_to_send, std::move(request));

  CounterMap counters;
  extractCounters(counters);

  //
  // Block until all servers exit
  //

  stopServers();

  //
  // Evaluate test
  //

  // All client connections are successfully established.
  EXPECT_EQ(client->connectSuccesses(), connections_to_initiate);
  EXPECT_EQ(client->connectFailures(), 0);
  // Client close callback called for every client connection.
  EXPECT_EQ(client->localCloses(), connections_to_initiate);
  // Client response callback is called for every request sent
  EXPECT_EQ(client->responsesReceived(), requests_to_send);
  // Every response was a 2xx class
  EXPECT_EQ(client->class2xxResponses(), requests_to_send);
  EXPECT_EQ(client->class4xxResponses(), 0);
  EXPECT_EQ(client->class5xxResponses(), 0);
  EXPECT_EQ(client->responseTimeouts(), 0);
  // No client sockets are rudely closed by server / no client sockets are
  // reset.
  EXPECT_EQ(client->remoteCloses(), 0);

  // assert that the callout server and origin server see every request
  EXPECT_EQ(findCluster(StressTest::ORIGIN_CLUSTER_NAME).requestsReceived(), requests_to_send);
  EXPECT_EQ(findCluster(callout_cluster_name).requestsReceived(), requests_to_send);

  // dumpCounters(counters);

  // And the wasm filter should have successfully created the callout_successes
  // counter and received a successful gRPC response for every inbound request.
  EXPECT_EQ(counters["test_callout_successes"], requests_to_send);
}

TEST_P(GrpcWasmStressTest, CalloutErrorResponse) {
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;
  const std::string wasm_file = absl::StrCat(
      TestEnvironment::runfilesDirectory(),
      "/test/extensions/filters/http/wasm/test_data/grpc_callout_", wasmLang(), ".wasm");
  // Must match cluster name in the wasm bundle:
  const std::string callout_cluster_name{"callout_cluster"};

  //
  // Configure the wasm filter
  //

  config_helper_.addFilter(fmt::format(R"EOF(
            name: envoy.filters.http.wasm
            config:
              config:
                vm_config:
                  runtime: "{}"
                  code:
                    filename: "{}"
)EOF",
                                       wasmVM(), wasm_file));

  //
  // This test first sends a request from wasm to a callout cluster:
  //    Client -> Envoy -> Wasm -> Callout Cluster
  // The callout cluster sends an error response which prevents Envoy from
  // forwarding the data plane request to the origin cluster.
  //

  addCluster(std::make_unique<ClusterHelper>(StressTest::ORIGIN_CLUSTER_NAME))
      .addServer(std::make_unique<ServerCallbackHelper>(
          [](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
            ENVOY_LOG(debug, "Origin server received request");
            Http::TestHeaderMapImpl response{{":status", "200"}};
            stream.sendResponseHeaders(response);
          }));
  addCluster(std::make_unique<ClusterHelper>(callout_cluster_name))
      .addServer(std::make_unique<ServerCallbackHelper>(
          [](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
            ENVOY_LOG(debug, "Callout server received request");
            ProtobufWkt::Value response;
            response.set_string_value("response");
            stream.sendGrpcResponse(Grpc::Status::PermissionDenied, response);
          }));

  try {
    bind();
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ipVersion()) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }

  LoadGeneratorPtr client = start();

  //
  // Exec test and wait for it to finish
  //

  Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{
      {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "host"}}};
  client->run(connections_to_initiate, requests_to_send, std::move(request));

  CounterMap counters;
  extractCounters(counters);

  //
  // Block until all servers exit
  //

  stopServers();

  //
  // Evaluate test
  //

  // All client connections are successfully established.
  EXPECT_EQ(client->connectSuccesses(), connections_to_initiate);
  EXPECT_EQ(client->connectFailures(), 0);
  // Client close callback called for every client connection.
  EXPECT_EQ(client->localCloses(), connections_to_initiate);
  // Client response callback is called for every request sent
  EXPECT_EQ(client->responsesReceived(), requests_to_send);
  // Every response was a 5xx class
  EXPECT_EQ(client->class2xxResponses(), 0);
  EXPECT_EQ(client->class4xxResponses(), 0);
  EXPECT_EQ(client->class5xxResponses(), requests_to_send);
  EXPECT_EQ(client->responseTimeouts(), 0);
  // No client sockets are rudely closed by server / no client sockets are
  // reset.
  EXPECT_EQ(client->remoteCloses(), 0);

  // assert that the callout server saw every request and prevented the
  // origin server from seeing it.
  EXPECT_EQ(findCluster(StressTest::ORIGIN_CLUSTER_NAME).requestsReceived(), 0);
  EXPECT_EQ(findCluster(callout_cluster_name).requestsReceived(), requests_to_send);

  // And the wasm filter should have successfully created the failure
  // counter and received a gRPC error response for every inbound request.
  EXPECT_EQ(counters["test_callout_failures"], requests_to_send);
}

// TODO fix test. Currently fails with a:
// terminate called after throwing an instance of 'Extensions::Common::Wasm::WasmException'
//  what():  emscripten cxa_allocate_exception
TEST_P(HttpWasmStressTest, DISABLED_CalloutHappyPath) {
  constexpr uint32_t connections_to_initiate = 30;
  constexpr uint32_t requests_to_send = 30 * connections_to_initiate;
  const std::string wasm_vm{std::get<0>(GetParam())};
  const std::string wasm_file =
      absl::StrCat(TestEnvironment::runfilesDirectory(),
                   "/test/extensions/filters/http/wasm/test_data/http_callout_",
                   std::get<1>(GetParam()), ".wasm");
  const std::string callout_cluster_name{"callout_cluster"};

  //
  // Configure the wasm filter
  //

  config_helper_.addFilter(fmt::format(R"EOF(
            name: envoy.filters.http.wasm
            config:
              config:
                vm_config:
                  runtime: "{}"
                  code:
                    filename: "{}"
)EOF",
                                       wasm_vm, wasm_file));

  //
  // This test first sends a request from wasm to a callout cluster:
  //    Client -> Envoy -> Wasm -> Callout Cluster
  // Envoy then unconditionally forwards the request to the origin cluster
  //              Envoy -> Origin Cluster
  //
  // Both Callout and Origin clusters have a single server that immediately
  // returns a 200 OK response.
  //
  addCluster(std::make_unique<ClusterHelper>(StressTest::ORIGIN_CLUSTER_NAME))
      .addServer(std::make_unique<ServerCallbackHelper>(
          [](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
            ENVOY_LOG(debug, "Origin server received request");
            Http::TestHeaderMapImpl response{{":status", "200"}};
            stream.sendResponseHeaders(response);
          }));
  addCluster(std::make_unique<ClusterHelper>(callout_cluster_name))
      .addServer(std::make_unique<ServerCallbackHelper>(
          [](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
            ENVOY_LOG(debug, "Callout server received request");
            Http::TestHeaderMapImpl response{{":status", "200"}};
            stream.sendResponseHeaders(response);
          }));

  try {
    bind();
  } catch (Network::SocketBindException& ex) {
    if (Network::Address::IpVersion::v6 == ipVersion()) {
      ENVOY_LOG(info, "Environment does not support IPv6, skipping test");
      GTEST_SKIP();
    }
    throw ex;
  }

  LoadGeneratorPtr client = start();

  //
  // Exec test and wait for it to finish (pass callout URL to wasm via http header)
  //

  std::string callout_url =
      fmt::format("http://{}:{}/", Network::Test::getLoopbackAddressString(ipVersion()),
                  firstPortInCluster(callout_cluster_name));

  Http::HeaderMapPtr request{new Http::TestHeaderMapImpl{{":method", "GET"},
                                                         {":path", "/"},
                                                         {":scheme", "http"},
                                                         {":authority", "host"},
                                                         {"x-callout-url", callout_url}}};
  client->run(connections_to_initiate, requests_to_send, std::move(request));

  //
  // Block until all servers exit
  //

  stopServers();

  //
  // Evaluate test
  //

  // All client connections are successfully established.
  EXPECT_EQ(client->connectSuccesses(), connections_to_initiate);
  EXPECT_EQ(client->connectFailures(), 0);
  // Client close callback called for every client connection.
  EXPECT_EQ(client->localCloses(), connections_to_initiate);
  // Client response callback is called for every request sent
  EXPECT_EQ(client->responsesReceived(), requests_to_send);
  // Every response was a 2xx class
  EXPECT_EQ(client->class2xxResponses(), requests_to_send);
  EXPECT_EQ(client->class4xxResponses(), 0);
  EXPECT_EQ(client->class5xxResponses(), 0);
  EXPECT_EQ(client->responseTimeouts(), 0);
  // No client sockets are rudely closed by server / no client sockets are
  // reset.
  EXPECT_EQ(client->remoteCloses(), 0);

  // assert that the callout server and origin server see every request
  EXPECT_EQ(findCluster(StressTest::ORIGIN_CLUSTER_NAME).requestsReceived(), requests_to_send);
  EXPECT_EQ(findCluster(callout_cluster_name).requestsReceived(), requests_to_send);
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
