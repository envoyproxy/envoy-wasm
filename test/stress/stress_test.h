#pragma once

#include "test/integration/http_integration.h"
#include "test/stress/stress_test_downstream.h"
#include "test/stress/stress_test_upstream.h"

namespace Envoy {
namespace Stress {

class StressTest : public HttpIntegrationTest {
public:
  static const std::string ORIGIN_CLUSTER_NAME;

  StressTest(Network::Address::IpVersion ip_protocol, Http::CodecClient::Type http_type)
      : HttpIntegrationTest(http_type, ip_protocol, baseBootstrap(ip_protocol)),
        ip_version_(ip_protocol), http_type_{http_type},
        transport_socket_factory_{}, client_{"client"} {
    // Tell the base class that we will create our own upstream origin server
    fake_upstreams_count_ = 0;
  }

protected:
  Network::Address::IpVersion ipVersion() const { return ip_version_; }

  Http::CodecClient::Type httpType() const { return http_type_; }

  ClusterHelper& addCluster(ClusterHelperPtr&& cluster_helper);

  void bind();

  LoadGeneratorPtr start();

  uint16_t envoyPort() { return static_cast<uint16_t>(lookupPort("http")); }

  uint16_t firstPortInCluster(const std::string& cluster_name) const;

  const ClusterHelper& findCluster(const std::string& cluster_name) const;

  void stopServers();

  using CounterMap = std::unordered_map<std::string, double>;

  // Must be called before Envoy is stopped
  void extractCounters(CounterMap& counters, const std::string& prefix = "");

  void dumpCounters(CounterMap& counters);

private:
  static std::string baseBootstrap(Network::Address::IpVersion ip_protocol);

  class Cluster {
  public:
    Cluster(ClusterHelperPtr&& cluster_helper,
            Network::TransportSocketFactory& transport_socket_factory,
            Network::Address::IpVersion ip_version, Http::CodecClient::Type http_type)
        : transport_socket_factory_{transport_socket_factory}, ip_version_{ip_version},
          http_type_{http_type}, cluster_helper_{std::move(cluster_helper)} {}

    void wait() { cluster_helper_->wait(); }

    void bind();

    uint16_t firstPort() const {
      return static_cast<uint16_t>(listeners_[0]->localAddress()->ip()->port());
    }

    const ClusterHelper& clusterHelper() const { return *cluster_helper_; }
    ClusterHelper& clusterHelper() { return *cluster_helper_; }

    void addClusterToBootstrap(ConfigHelper& config_helper, std::vector<uint32_t>& ports) const;

    void start();

  private:
    bool bound_{false};
    Network::TransportSocketFactory& transport_socket_factory_;
    Network::Address::IpVersion ip_version_;
    Http::CodecClient::Type http_type_;
    ClusterHelperPtr cluster_helper_;
    std::vector<Network::TcpListenSocketPtr> listeners_;
    std::vector<ServerPtr> servers_;
  };

  typedef std::unique_ptr<Cluster> ClusterPtr;

  Network::Address::IpVersion ip_version_;
  Http::CodecClient::Type http_type_;
  Network::RawBufferSocketFactory transport_socket_factory_;
  Client client_;
  std::unordered_map<std::string, ClusterPtr> clusters_;
};

} // namespace Stress
} // namespace Envoy
