#include "stress_test.h"

namespace Envoy {
namespace Stress {

const std::string StressTest::ORIGIN_CLUSTER_NAME{"origin_cluster"};

static const std::string BOOTSTRAP_CONFIG = R"EOF(
admin:
  access_log_path: /dev/null
  address:
    socket_address:
      address: {}
      port_value: 0
dynamic_resources:
  lds_config:
    path: /dev/null
static_resources:
  listeners:
    name: listener_0
    address:
      socket_address:
        address: {}
        port_value: 0
    filter_chains:
      filters:
        name: envoy.http_connection_manager
        typed_config:
          "@type": type.googleapis.com/envoy.config.filter.network.http_connection_manager.v2.HttpConnectionManager
          stat_prefix: config_test
          http_filters:
            name: envoy.router
          codec_type: auto
          access_log:
            name: envoy.file_access_log
            filter:
              not_health_check_filter:  {{}}
            config:
              path: /dev/null
          route_config:
            virtual_hosts:
              name: integration
              routes:
                route:
                  cluster: {}
                match:
                  prefix: "/"
              domains: "*"
            name: route_config_0
)EOF";

std::string StressTest::baseBootstrap(Network::Address::IpVersion ip_version) {
  return fmt::format(BOOTSTRAP_CONFIG, Network::Test::getLoopbackAddressString(ip_version),
                     Network::Test::getLoopbackAddressString(ip_version), ORIGIN_CLUSTER_NAME);
}

ClusterHelper& StressTest::addCluster(ClusterHelperPtr&& cluster_helper) {
  const std::string& name = cluster_helper->name();
  auto it = clusters_.emplace(std::make_pair(
      name, std::make_unique<Cluster>(std::move(cluster_helper), transport_socket_factory_,
                                      ip_version_, http_type_)));

  if (!it.second) {
    throw EnvoyException(fmt::format("Duplicate cluster named '{}'", name));
  }

  return it.first->second->clusterHelper();
}

void StressTest::bind() {
  for (auto& it : clusters_) {
    it.second->bind();
  }
}

LoadGeneratorPtr StressTest::start() {
  {
    const auto& it = clusters_.find(ORIGIN_CLUSTER_NAME);
    if (it == clusters_.end()) {
      throw EnvoyException(fmt::format("One cluster must be named '{}'", ORIGIN_CLUSTER_NAME));
    }
  }

  for (auto& it : clusters_) {
    it.second->start();
    it.second->addClusterToBootstrap(config_helper_, ports_);
  }

  setUpstreamProtocol(Http::CodecClient::Type::HTTP2 == http_type_
                          ? FakeHttpConnection::Type::HTTP2
                          : FakeHttpConnection::Type::HTTP1);
  // Start envoy
  HttpIntegrationTest::initialize();

  ENVOY_LOG(debug, "Bootstrap Config:\n{}",
            MessageUtil::getYamlStringFromMessage(config_helper_.bootstrap(), true));

  Network::Address::InstanceConstSharedPtr address{
      loopbackAddress(ip_version_, lookupPort("http"))};
  return std::make_unique<LoadGenerator>(client_, transport_socket_factory_, http_type_, address);
}

uint16_t StressTest::firstPortInCluster(const std::string& cluster_name) const {
  const auto& it = clusters_.find(cluster_name);
  return it == clusters_.end() ? 0 : it->second->firstPort();
}

const ClusterHelper& StressTest::findCluster(const std::string& cluster_name) const {
  const auto& it = clusters_.find(cluster_name);
  if (it == clusters_.end()) {
    throw EnvoyException(fmt::format("Cannot find cluster '{}'", cluster_name));
  }
  return it->second->clusterHelper();
}

void StressTest::stopServers() {
  // Stop envoy by destroying it.
  test_server_ = nullptr;

  // Wait until all clusters have no more active connections
  for (auto& it : clusters_) {
    it.second->wait();
  }
}

// Must be called before Envoy is stopped
void StressTest::extractCounters(StressTest::CounterMap& counters, const std::string& prefix) {
  for (const auto& it : test_server_->stat_store().counters()) {
    if (!absl::StartsWith(it->name(), prefix)) {
      continue;
    }
    counters[it->name()] = it->value();
  }
}

void StressTest::dumpCounters(StressTest::CounterMap& counters) {
  for (const auto& it : counters) {
    ENVOY_LOG(info, "{} = {}", it.first, it.second);
  }
}

void StressTest::Cluster::bind() {
  if (bound_) {
    return;
  }
  for (size_t i = 0; i < cluster_helper_->servers().size(); ++i) {
    listeners_.emplace_back(new LocalListenSocket(ip_version_));
    ENVOY_LOG(debug, "{} bound port {}", cluster_helper_->name(),
              listeners_.back()->localAddress()->ip()->port());
  }
  bound_ = true;
}

void StressTest::Cluster::addClusterToBootstrap(ConfigHelper& config_helper,
                                                std::vector<uint32_t>& ports) const {
  config_helper.addConfigModifier([this](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
    auto cluster = bootstrap.mutable_static_resources()->add_clusters();

    cluster->set_name(cluster_helper_->name());
    cluster->set_type(envoy::api::v2::Cluster_DiscoveryType::Cluster_DiscoveryType_STATIC);
    cluster->set_lb_policy(envoy::api::v2::Cluster_LbPolicy::Cluster_LbPolicy_ROUND_ROBIN);

    if (http_type_ == Http::CodecClient::Type::HTTP1) {
      auto opts = cluster->mutable_http_protocol_options();
      opts->set_accept_http_10(false);
    } else {
      auto opts = cluster->mutable_http2_protocol_options();
      auto value = opts->mutable_max_concurrent_streams();
      value->set_value(2147483647U);
    }

    for (const auto& listener : listeners_) {
      auto hosts = cluster->add_hosts();
      auto address = hosts->mutable_socket_address();
      address->set_address(Network::Test::getLoopbackAddressString(ip_version_));
      address->set_port_value(listener->localAddress()->ip()->port());
    }
  });

  // This avoids "assert failure: ports.size() > port_idx" complaints from
  // ConfigHelper::finalize()
  for (const auto& listener : listeners_) {
    ports.push_back(listener->localAddress()->ip()->port());
  }
}

void StressTest::Cluster::start() {
  bind();
  for (size_t i = 0; i < cluster_helper_->servers().size(); ++i) {
    servers_.emplace_back(new Server(fmt::format("{}-{}", cluster_helper_->name(), i),
                                     *listeners_[i], transport_socket_factory_, http_type_));
    servers_.back()->start(*cluster_helper_->servers()[i]);
  }
}

} // namespace Stress
} // namespace Envoy
