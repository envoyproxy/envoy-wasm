#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include "envoy/stats/scope.h"

#include "common/common/empty_string.h"
#include "common/upstream/cluster_factory_impl.h"
#include "common/upstream/logical_host.h"
#include "common/upstream/upstream_impl.h"

#include "extensions/clusters/well_known_names.h"

namespace Envoy {
namespace Upstream {

/**
 * The LogicalDnsCluster is a type of cluster that creates a single logical host that wraps
 * an async DNS resolver. The DNS resolver will continuously resolve, and swap in the first IP
 * address in the resolution set. However the logical owning host will not change. Any created
 * connections against this host will use the currently resolved IP. This means that a connection
 * pool using the logical host may end up with connections to many different real IPs.
 *
 * This cluster type is useful for large web services that use DNS in a round robin fashion, such
 * that DNS resolution may repeatedly return different results. A single connection pool can be
 * created that will internally have connections to different backends, while still allowing long
 * connection lengths and keep alive. The cluster type should only be used when an IP address change
 * means that connections using the IP should not drain.
 */
class LogicalDnsCluster : public ClusterImplBase {
public:
  LogicalDnsCluster(const envoy::api::v2::Cluster& cluster, Runtime::Loader& runtime,
                    Network::DnsResolverSharedPtr dns_resolver,
                    Server::Configuration::TransportSocketFactoryContext& factory_context,
                    Stats::ScopePtr&& stats_scope, bool added_via_api);

  ~LogicalDnsCluster() override;

  // Upstream::Cluster
  InitializePhase initializePhase() const override { return InitializePhase::Primary; }

private:
  const envoy::api::v2::endpoint::LocalityLbEndpoints& localityLbEndpoint() const {
    // This is checked in the constructor, i.e. at config load time.
    ASSERT(load_assignment_.endpoints().size() == 1);
    return load_assignment_.endpoints()[0];
  }

  const envoy::api::v2::endpoint::LbEndpoint& lbEndpoint() const {
    // This is checked in the constructor, i.e. at config load time.
    ASSERT(localityLbEndpoint().lb_endpoints().size() == 1);
    return localityLbEndpoint().lb_endpoints()[0];
  }

  void startResolve();

  // ClusterImplBase
  void startPreInit() override;

  Network::DnsResolverSharedPtr dns_resolver_;
  const std::chrono::milliseconds dns_refresh_rate_ms_;
  BackOffStrategyPtr failure_backoff_strategy_;
  const bool respect_dns_ttl_;
  Network::DnsLookupFamily dns_lookup_family_;
  Event::TimerPtr resolve_timer_;
  std::string dns_url_;
  std::string hostname_;
  Network::Address::InstanceConstSharedPtr current_resolved_address_;
  LogicalHostSharedPtr logical_host_;
  Network::ActiveDnsQuery* active_dns_query_{};
  const LocalInfo::LocalInfo& local_info_;
  const envoy::api::v2::ClusterLoadAssignment load_assignment_;
};

class LogicalDnsClusterFactory : public ClusterFactoryImplBase {
public:
  LogicalDnsClusterFactory()
      : ClusterFactoryImplBase(Extensions::Clusters::ClusterTypes::get().LogicalDns) {}

private:
  std::pair<ClusterImplBaseSharedPtr, ThreadAwareLoadBalancerPtr>
  createClusterImpl(const envoy::api::v2::Cluster& cluster, ClusterFactoryContext& context,
                    Server::Configuration::TransportSocketFactoryContext& socket_factory_context,
                    Stats::ScopePtr&& stats_scope) override;
};

DECLARE_FACTORY(LogicalDnsClusterFactory);

} // namespace Upstream
} // namespace Envoy
