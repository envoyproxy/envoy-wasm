#include "test/integration/ads_integration.h"

#include "envoy/admin/v3/config_dump.pb.h"
#include "envoy/config/bootstrap/v3/bootstrap.pb.h"
#include "envoy/config/cluster/v3/cluster.pb.h"
#include "envoy/config/endpoint/v3/endpoint.pb.h"
#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/config/route/v3/route.pb.h"
#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"

#include "common/config/protobuf_link_hacks.h"
#include "common/protobuf/protobuf.h"
#include "common/protobuf/utility.h"

#include "test/test_common/network_utility.h"
#include "test/test_common/resources.h"
#include "test/test_common/utility.h"

using testing::AssertionResult;

namespace Envoy {

AdsIntegrationTest::AdsIntegrationTest()
    : HttpIntegrationTest(
          Http::CodecClient::Type::HTTP2, ipVersion(),
          AdsIntegrationConfig(sotwOrDelta() == Grpc::SotwOrDelta::Sotw ? "GRPC" : "DELTA_GRPC")) {
  use_lds_ = false;
  create_xds_upstream_ = true;
  tls_xds_upstream_ = true;
  sotw_or_delta_ = sotwOrDelta();
}

void AdsIntegrationTest::TearDown() {
  cleanUpXdsConnection();
  test_server_.reset();
  fake_upstreams_.clear();
}

envoy::config::cluster::v3::Cluster AdsIntegrationTest::buildCluster(const std::string& name) {
  return TestUtility::parseYaml<envoy::config::cluster::v3::Cluster>(fmt::format(R"EOF(
      name: {}
      connect_timeout: 5s
      type: EDS
      eds_cluster_config: {{ eds_config: {{ ads: {{}} }} }}
      lb_policy: ROUND_ROBIN
      http2_protocol_options: {{}}
    )EOF",
                                                                                 name));
}

envoy::config::cluster::v3::Cluster AdsIntegrationTest::buildRedisCluster(const std::string& name) {
  return TestUtility::parseYaml<envoy::config::cluster::v3::Cluster>(fmt::format(R"EOF(
      name: {}
      connect_timeout: 5s
      type: EDS
      eds_cluster_config: {{ eds_config: {{ ads: {{}} }} }}
      lb_policy: MAGLEV
    )EOF",
                                                                                 name));
}

envoy::config::endpoint::v3::ClusterLoadAssignment
AdsIntegrationTest::buildClusterLoadAssignment(const std::string& name) {
  return TestUtility::parseYaml<envoy::config::endpoint::v3::ClusterLoadAssignment>(
      fmt::format(R"EOF(
      cluster_name: {}
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: {}
                port_value: {}
    )EOF",
                  name, Network::Test::getLoopbackAddressString(ipVersion()),
                  fake_upstreams_[0]->localAddress()->ip()->port()));
}

envoy::config::listener::v3::Listener
AdsIntegrationTest::buildListener(const std::string& name, const std::string& route_config,
                                  const std::string& stat_prefix) {
  return TestUtility::parseYaml<envoy::config::listener::v3::Listener>(fmt::format(
      R"EOF(
      name: {}
      address:
        socket_address:
          address: {}
          port_value: 0
      filter_chains:
        filters:
        - name: http
          typed_config:
            "@type": type.googleapis.com/envoy.config.filter.network.http_connection_manager.v2.HttpConnectionManager
            stat_prefix: {}
            codec_type: HTTP2
            rds:
              route_config_name: {}
              config_source: {{ ads: {{}} }}
            http_filters: [{{ name: envoy.filters.http.router }}]
    )EOF",
      name, Network::Test::getLoopbackAddressString(ipVersion()), stat_prefix, route_config));
}

envoy::config::listener::v3::Listener
AdsIntegrationTest::buildRedisListener(const std::string& name, const std::string& cluster) {
  return TestUtility::parseYaml<envoy::config::listener::v3::Listener>(fmt::format(
      R"EOF(
      name: {}
      address:
        socket_address:
          address: {}
          port_value: 0
      filter_chains:
        filters:
        - name: redis
          typed_config:
            "@type": type.googleapis.com/envoy.config.filter.network.redis_proxy.v2.RedisProxy
            settings: 
              op_timeout: 1s
            stat_prefix: {}
            prefix_routes:
              catch_all_route: 
                cluster: {}
    )EOF",
      name, Network::Test::getLoopbackAddressString(ipVersion()), name, cluster));
}

envoy::config::route::v3::RouteConfiguration
AdsIntegrationTest::buildRouteConfig(const std::string& name, const std::string& cluster) {
  return TestUtility::parseYaml<envoy::config::route::v3::RouteConfiguration>(fmt::format(R"EOF(
      name: {}
      virtual_hosts:
      - name: integration
        domains: ["*"]
        routes:
        - match: {{ prefix: "/" }}
          route: {{ cluster: {} }}
    )EOF",
                                                                                          name,
                                                                                          cluster));
}

void AdsIntegrationTest::makeSingleRequest() {
  registerTestServerPorts({"http"});
  testRouterHeaderOnlyRequestAndResponse();
  cleanupUpstreamAndDownstream();
}

void AdsIntegrationTest::initialize() { initializeAds(false); }

void AdsIntegrationTest::initializeAds(const bool rate_limiting) {
  config_helper_.addConfigModifier([this, &rate_limiting](
                                       envoy::config::bootstrap::v3::Bootstrap& bootstrap) {
    auto* ads_config = bootstrap.mutable_dynamic_resources()->mutable_ads_config();
    if (rate_limiting) {
      ads_config->mutable_rate_limit_settings();
    }
    auto* grpc_service = ads_config->add_grpc_services();
    setGrpcService(*grpc_service, "ads_cluster", xds_upstream_->localAddress());
    auto* ads_cluster = bootstrap.mutable_static_resources()->add_clusters();
    ads_cluster->MergeFrom(bootstrap.static_resources().clusters()[0]);
    ads_cluster->set_name("ads_cluster");
    envoy::extensions::transport_sockets::tls::v3::UpstreamTlsContext context;
    auto* validation_context = context.mutable_common_tls_context()->mutable_validation_context();
    validation_context->mutable_trusted_ca()->set_filename(
        TestEnvironment::runfilesPath("test/config/integration/certs/upstreamcacert.pem"));
    validation_context->add_match_subject_alt_names()->set_suffix("lyft.com");
    if (clientType() == Grpc::ClientType::GoogleGrpc) {
      auto* google_grpc = grpc_service->mutable_google_grpc();
      auto* ssl_creds = google_grpc->mutable_channel_credentials()->mutable_ssl_credentials();
      ssl_creds->mutable_root_certs()->set_filename(
          TestEnvironment::runfilesPath("test/config/integration/certs/upstreamcacert.pem"));
    }
    ads_cluster->mutable_transport_socket()->set_name("envoy.transport_sockets.tls");
    ads_cluster->mutable_transport_socket()->mutable_typed_config()->PackFrom(context);
  });
  setUpstreamProtocol(FakeHttpConnection::Type::HTTP2);
  HttpIntegrationTest::initialize();
  if (xds_stream_ == nullptr) {
    createXdsConnection();
    AssertionResult result = xds_connection_->waitForNewStream(*dispatcher_, xds_stream_);
    RELEASE_ASSERT(result, result.message());
    xds_stream_->startGrpcStream();
  }
}

void AdsIntegrationTest::testBasicFlow() {
  // Send initial configuration, validate we can process a request.
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "", {}, {}, {}, true));
  sendDiscoveryResponse<envoy::config::cluster::v3::Cluster>(Config::TypeUrl::get().Cluster,
                                                             {buildCluster("cluster_0")},
                                                             {buildCluster("cluster_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().ClusterLoadAssignment, "",
                                      {"cluster_0"}, {"cluster_0"}, {}));
  sendDiscoveryResponse<envoy::config::endpoint::v3::ClusterLoadAssignment>(
      Config::TypeUrl::get().ClusterLoadAssignment, {buildClusterLoadAssignment("cluster_0")},
      {buildClusterLoadAssignment("cluster_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "1", {}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Listener, "", {}, {}, {}));
  sendDiscoveryResponse<envoy::config::listener::v3::Listener>(
      Config::TypeUrl::get().Listener, {buildListener("listener_0", "route_config_0")},
      {buildListener("listener_0", "route_config_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().ClusterLoadAssignment, "1",
                                      {"cluster_0"}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "",
                                      {"route_config_0"}, {"route_config_0"}, {}));
  sendDiscoveryResponse<envoy::config::route::v3::RouteConfiguration>(
      Config::TypeUrl::get().RouteConfiguration, {buildRouteConfig("route_config_0", "cluster_0")},
      {buildRouteConfig("route_config_0", "cluster_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Listener, "1", {}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "1",
                                      {"route_config_0"}, {}, {}));

  test_server_->waitForCounterGe("listener_manager.listener_create_success", 1);
  makeSingleRequest();
  const ProtobufWkt::Timestamp first_active_listener_ts_1 =
      getListenersConfigDump().dynamic_listeners(0).active_state().last_updated();
  const ProtobufWkt::Timestamp first_active_cluster_ts_1 =
      getClustersConfigDump().dynamic_active_clusters()[0].last_updated();
  const ProtobufWkt::Timestamp first_route_config_ts_1 =
      getRoutesConfigDump().dynamic_route_configs()[0].last_updated();

  // Upgrade RDS/CDS/EDS to a newer config, validate we can process a request.
  sendDiscoveryResponse<envoy::config::cluster::v3::Cluster>(
      Config::TypeUrl::get().Cluster, {buildCluster("cluster_1"), buildCluster("cluster_2")},
      {buildCluster("cluster_1"), buildCluster("cluster_2")}, {"cluster_0"}, "2");
  test_server_->waitForGaugeEq("cluster_manager.warming_clusters", 2);
  sendDiscoveryResponse<envoy::config::endpoint::v3::ClusterLoadAssignment>(
      Config::TypeUrl::get().ClusterLoadAssignment,
      {buildClusterLoadAssignment("cluster_1"), buildClusterLoadAssignment("cluster_2")},
      {buildClusterLoadAssignment("cluster_1"), buildClusterLoadAssignment("cluster_2")},
      {"cluster_0"}, "2");
  test_server_->waitForGaugeEq("cluster_manager.warming_clusters", 0);
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().ClusterLoadAssignment, "1",
                                      {"cluster_2", "cluster_1"}, {"cluster_2", "cluster_1"},
                                      {"cluster_0"}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "2", {}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().ClusterLoadAssignment, "2",
                                      {"cluster_2", "cluster_1"}, {}, {}));
  sendDiscoveryResponse<envoy::config::route::v3::RouteConfiguration>(
      Config::TypeUrl::get().RouteConfiguration, {buildRouteConfig("route_config_0", "cluster_1")},
      {buildRouteConfig("route_config_0", "cluster_1")}, {}, "2");
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "2",
                                      {"route_config_0"}, {}, {}));

  makeSingleRequest();
  const ProtobufWkt::Timestamp first_active_listener_ts_2 =
      getListenersConfigDump().dynamic_listeners(0).active_state().last_updated();
  const ProtobufWkt::Timestamp first_active_cluster_ts_2 =
      getClustersConfigDump().dynamic_active_clusters()[0].last_updated();
  const ProtobufWkt::Timestamp first_route_config_ts_2 =
      getRoutesConfigDump().dynamic_route_configs()[0].last_updated();

  // Upgrade LDS/RDS, validate we can process a request.
  sendDiscoveryResponse<envoy::config::listener::v3::Listener>(
      Config::TypeUrl::get().Listener,
      {buildListener("listener_1", "route_config_1"),
       buildListener("listener_2", "route_config_2")},
      {buildListener("listener_1", "route_config_1"),
       buildListener("listener_2", "route_config_2")},
      {"listener_0"}, "2");
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "2",
                                      {"route_config_2", "route_config_1", "route_config_0"},
                                      {"route_config_2", "route_config_1"}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Listener, "2", {}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "2",
                                      {"route_config_2", "route_config_1"}, {},
                                      {"route_config_0"}));
  sendDiscoveryResponse<envoy::config::route::v3::RouteConfiguration>(
      Config::TypeUrl::get().RouteConfiguration,
      {buildRouteConfig("route_config_1", "cluster_1"),
       buildRouteConfig("route_config_2", "cluster_1")},
      {buildRouteConfig("route_config_1", "cluster_1"),
       buildRouteConfig("route_config_2", "cluster_1")},
      {"route_config_0"}, "3");
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "3",
                                      {"route_config_2", "route_config_1"}, {}, {}));

  test_server_->waitForCounterGe("listener_manager.listener_create_success", 2);
  makeSingleRequest();
  const ProtobufWkt::Timestamp first_active_listener_ts_3 =
      getListenersConfigDump().dynamic_listeners(0).active_state().last_updated();
  const ProtobufWkt::Timestamp first_active_cluster_ts_3 =
      getClustersConfigDump().dynamic_active_clusters()[0].last_updated();
  const ProtobufWkt::Timestamp first_route_config_ts_3 =
      getRoutesConfigDump().dynamic_route_configs()[0].last_updated();

  // Expect last_updated timestamps to be updated in a predictable way
  // For the listener configs in this example, 1 == 2 < 3.
  EXPECT_EQ(first_active_listener_ts_2, first_active_listener_ts_1);
  EXPECT_GT(first_active_listener_ts_3, first_active_listener_ts_2);
  // For the cluster configs in this example, 1 < 2 == 3.
  EXPECT_GT(first_active_cluster_ts_2, first_active_cluster_ts_1);
  EXPECT_EQ(first_active_cluster_ts_3, first_active_cluster_ts_2);
  // For the route configs in this example, 1 < 2 < 3.
  EXPECT_GT(first_route_config_ts_2, first_route_config_ts_1);
  EXPECT_GT(first_route_config_ts_3, first_route_config_ts_2);
}

envoy::admin::v3::ClustersConfigDump AdsIntegrationTest::getClustersConfigDump() {
  auto message_ptr =
      test_server_->server().admin().getConfigTracker().getCallbacksMap().at("clusters")();
  return dynamic_cast<const envoy::admin::v3::ClustersConfigDump&>(*message_ptr);
}

envoy::admin::v3::ListenersConfigDump AdsIntegrationTest::getListenersConfigDump() {
  auto message_ptr =
      test_server_->server().admin().getConfigTracker().getCallbacksMap().at("listeners")();
  return dynamic_cast<const envoy::admin::v3::ListenersConfigDump&>(*message_ptr);
}

envoy::admin::v3::RoutesConfigDump AdsIntegrationTest::getRoutesConfigDump() {
  auto message_ptr =
      test_server_->server().admin().getConfigTracker().getCallbacksMap().at("routes")();
  return dynamic_cast<const envoy::admin::v3::RoutesConfigDump&>(*message_ptr);
}

} // namespace Envoy
