#pragma once

#include "envoy/config/cluster/v3/cluster.pb.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "common/upstream/thread_aware_lb_impl.h"
#include "common/upstream/upstream_impl.h"

namespace Envoy {
namespace Upstream {

/**
 * All Maglev load balancer stats. @see stats_macros.h
 */
#define ALL_MAGLEV_LOAD_BALANCER_STATS(GAUGE)                                                      \
  GAUGE(max_entries_per_host, Accumulate)                                                          \
  GAUGE(min_entries_per_host, Accumulate)

/**
 * Struct definition for all Maglev load balancer stats. @see stats_macros.h
 */
struct MaglevLoadBalancerStats {
  ALL_MAGLEV_LOAD_BALANCER_STATS(GENERATE_GAUGE_STRUCT)
};

/**
 * This is an implementation of Maglev consistent hashing as described in:
 * https://static.googleusercontent.com/media/research.google.com/en//pubs/archive/44824.pdf
 * section 3.4. Specifically, the algorithm shown in pseudocode listing 1 is implemented with a
 * fixed table size of 65537. This is the recommended table size in section 5.3.
 */
class MaglevTable : public ThreadAwareLoadBalancerBase::HashingLoadBalancer,
                    Logger::Loggable<Logger::Id::upstream> {
public:
  MaglevTable(const NormalizedHostWeightVector& normalized_host_weights,
              double max_normalized_weight, uint64_t table_size, MaglevLoadBalancerStats& stats);

  // ThreadAwareLoadBalancerBase::HashingLoadBalancer
  HostConstSharedPtr chooseHost(uint64_t hash) const override;

  // Recommended table size in section 5.3 of the paper.
  static const uint64_t DefaultTableSize = 65537;

private:
  struct TableBuildEntry {
    TableBuildEntry(const HostConstSharedPtr& host, uint64_t offset, uint64_t skip, double weight)
        : host_(host), offset_(offset), skip_(skip), weight_(weight) {}

    HostConstSharedPtr host_;
    const uint64_t offset_;
    const uint64_t skip_;
    const double weight_;
    double target_weight_{};
    uint64_t next_{};
    uint64_t count_{};
  };

  uint64_t permutation(const TableBuildEntry& entry);

  const uint64_t table_size_;
  std::vector<HostConstSharedPtr> table_;
  MaglevLoadBalancerStats& stats_;
};

/**
 * Thread aware load balancer implementation for Maglev.
 */
class MaglevLoadBalancer : public ThreadAwareLoadBalancerBase {
public:
  MaglevLoadBalancer(const PrioritySet& priority_set, ClusterStats& stats, Stats::Scope& scope,
                     Runtime::Loader& runtime, Runtime::RandomGenerator& random,
                     const envoy::config::cluster::v3::Cluster::CommonLbConfig& common_config,
                     uint64_t table_size = MaglevTable::DefaultTableSize);

  const MaglevLoadBalancerStats& stats() const { return stats_; }

private:
  // ThreadAwareLoadBalancerBase
  HashingLoadBalancerSharedPtr
  createLoadBalancer(const NormalizedHostWeightVector& normalized_host_weights,
                     double /* min_normalized_weight */, double max_normalized_weight) override {
    return std::make_shared<MaglevTable>(normalized_host_weights, max_normalized_weight,
                                         table_size_, stats_);
  }

  static MaglevLoadBalancerStats generateStats(Stats::Scope& scope);

  Stats::ScopePtr scope_;
  MaglevLoadBalancerStats stats_;
  const uint64_t table_size_;
};

} // namespace Upstream
} // namespace Envoy
