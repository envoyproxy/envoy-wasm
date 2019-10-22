#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/network/utility.h"
#include "common/upstream/load_balancer_impl.h"
#include "common/upstream/upstream_impl.h"

#include "test/common/upstream/utility.h"
#include "test/mocks/runtime/mocks.h"
#include "test/mocks/upstream/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::ElementsAre;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

namespace Envoy {
namespace Upstream {
namespace {

class LoadBalancerTestBase : public testing::TestWithParam<bool> {
protected:
  // Run all tests against both priority 0 and priority 1 host sets, to ensure
  // all the load balancers have equivalent functionality for failover host sets.
  MockHostSet& hostSet() { return GetParam() ? host_set_ : failover_host_set_; }

  LoadBalancerTestBase() : stats_(ClusterInfoImpl::generateStats(stats_store_)) {
    least_request_lb_config_.mutable_choice_count()->set_value(2);
  }

  Stats::IsolatedStoreImpl stats_store_;
  ClusterStats stats_;
  NiceMock<Runtime::MockLoader> runtime_;
  NiceMock<Runtime::MockRandomGenerator> random_;
  NiceMock<MockPrioritySet> priority_set_;
  MockHostSet& host_set_ = *priority_set_.getMockHostSet(0);
  MockHostSet& failover_host_set_ = *priority_set_.getMockHostSet(1);
  std::shared_ptr<MockClusterInfo> info_{new NiceMock<MockClusterInfo>()};
  envoy::api::v2::Cluster::CommonLbConfig common_config_;
  envoy::api::v2::Cluster::LeastRequestLbConfig least_request_lb_config_;
};

class TestLb : public LoadBalancerBase {
public:
  TestLb(const PrioritySet& priority_set, ClusterStats& stats, Runtime::Loader& runtime,
         Runtime::RandomGenerator& random,
         const envoy::api::v2::Cluster::CommonLbConfig& common_config)
      : LoadBalancerBase(priority_set, stats, runtime, random, common_config) {}
  using LoadBalancerBase::chooseHostSet;
  using LoadBalancerBase::isInPanic;
  using LoadBalancerBase::percentageDegradedLoad;
  using LoadBalancerBase::percentageLoad;

  HostConstSharedPtr chooseHostOnce(LoadBalancerContext*) override {
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
};

class LoadBalancerBaseTest : public LoadBalancerTestBase {
public:
  void updateHostSet(MockHostSet& host_set, uint32_t num_hosts, uint32_t num_healthy_hosts,
                     uint32_t num_degraded_hosts = 0, uint32_t num_excluded_hosts = 0) {
    ASSERT(num_healthy_hosts + num_degraded_hosts + num_excluded_hosts <= num_hosts);

    host_set.hosts_.clear();
    host_set.healthy_hosts_.clear();
    host_set.degraded_hosts_.clear();
    host_set.excluded_hosts_.clear();
    for (uint32_t i = 0; i < num_hosts; ++i) {
      host_set.hosts_.push_back(makeTestHost(info_, "tcp://127.0.0.1:80"));
    }
    uint32_t i = 0;
    for (; i < num_healthy_hosts; ++i) {
      host_set.healthy_hosts_.push_back(host_set.hosts_[i]);
    }
    for (; i < (num_healthy_hosts + num_degraded_hosts); ++i) {
      host_set.degraded_hosts_.push_back(host_set.hosts_[i]);
    }

    for (; i < (num_healthy_hosts + num_degraded_hosts + num_excluded_hosts); ++i) {
      host_set.degraded_hosts_.push_back(host_set.hosts_[i]);
      host_set.excluded_hosts_.push_back(host_set.hosts_[i]);
    }
    host_set.runCallbacks({}, {});
  }

  template <typename T, typename FUNC>
  std::vector<T> aggregatePrioritySetsValues(TestLb& lb, FUNC func) {
    std::vector<T> ret;

    for (size_t i = 0; i < priority_set_.host_sets_.size(); ++i) {
      ret.push_back((lb.*func)(i));
    }

    return ret;
  }

  std::vector<uint32_t> getLoadPercentage() {
    return aggregatePrioritySetsValues<uint32_t>(lb_, &TestLb::percentageLoad);
  }

  std::vector<uint32_t> getDegradedLoadPercentage() {
    return aggregatePrioritySetsValues<uint32_t>(lb_, &TestLb::percentageDegradedLoad);
  }

  std::vector<bool> getPanic() {
    return aggregatePrioritySetsValues<bool>(lb_, &TestLb::isInPanic);
  }

  envoy::api::v2::Cluster::CommonLbConfig common_config_;
  TestLb lb_{priority_set_, stats_, runtime_, random_, common_config_};
};

INSTANTIATE_TEST_SUITE_P(PrimaryOrFailover, LoadBalancerBaseTest, ::testing::Values(true));

// Basic test of host set selection.
TEST_P(LoadBalancerBaseTest, PrioritySelection) {
  NiceMock<Upstream::MockLoadBalancerContext> context;
  updateHostSet(host_set_, 1 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 1, 0);

  HealthyAndDegradedLoad priority_load{Upstream::HealthyLoad({100, 0, 0}),
                                       Upstream::DegradedLoad({0, 0, 0})};
  EXPECT_CALL(context, determinePriorityLoad(_, _)).WillRepeatedly(ReturnRef(priority_load));
  // With both the primary and failover hosts unhealthy, we should select an
  // unhealthy primary host.
  EXPECT_EQ(100, lb_.percentageLoad(0));
  EXPECT_EQ(0, lb_.percentageLoad(1));
  EXPECT_EQ(&host_set_, &lb_.chooseHostSet(&context).first);

  // Update the priority set with a new priority level P=2 and ensure the host
  // is chosen
  MockHostSet& tertiary_host_set_ = *priority_set_.getMockHostSet(2);
  updateHostSet(tertiary_host_set_, 1 /* num_hosts */, 1 /* num_healthy_hosts */);
  EXPECT_EQ(0, lb_.percentageLoad(0));
  EXPECT_EQ(0, lb_.percentageLoad(1));
  EXPECT_EQ(100, lb_.percentageLoad(2));
  priority_load.healthy_priority_load_ = HealthyLoad({0u, 0u, 100});
  EXPECT_EQ(&tertiary_host_set_, &lb_.chooseHostSet(&context).first);

  // Now add a healthy host in P=0 and make sure it is immediately selected.
  updateHostSet(host_set_, 1 /* num_hosts */, 1 /* num_healthy_hosts */);
  host_set_.healthy_hosts_ = host_set_.hosts_;
  host_set_.runCallbacks({}, {});
  EXPECT_EQ(100, lb_.percentageLoad(0));
  EXPECT_EQ(0, lb_.percentageLoad(2));
  priority_load.healthy_priority_load_ = HealthyLoad({100u, 0u, 0u});
  EXPECT_EQ(&host_set_, &lb_.chooseHostSet(&context).first);

  // Remove the healthy host and ensure we fail back over to tertiary_host_set_
  updateHostSet(host_set_, 1 /* num_hosts */, 0 /* num_healthy_hosts */);
  EXPECT_EQ(0, lb_.percentageLoad(0));
  EXPECT_EQ(100, lb_.percentageLoad(2));
  priority_load.healthy_priority_load_ = HealthyLoad({0u, 0u, 100});
  EXPECT_EQ(&tertiary_host_set_, &lb_.chooseHostSet(&context).first);
}

// Tests host selection with a randomized number of healthy, degraded and unhealthy hosts.
TEST_P(LoadBalancerBaseTest, PrioritySelectionFuzz) {
  TestRandomGenerator rand;

  // Determine total number of hosts.
  const auto total_hosts = 1 + (rand.random() % 10);

  NiceMock<Upstream::MockLoadBalancerContext> context;

  const auto host_set_hosts = rand.random() % total_hosts;

  if (host_set_hosts == 0) {
    updateHostSet(host_set_, 0, 0);
  } else {
    // We get on average 50% healthy hosts, 25% degraded hosts and 25% unhealthy hosts.
    const auto healthy_hosts = rand.random() % host_set_hosts;
    const auto degraded_hosts = rand.random() % (host_set_hosts - healthy_hosts);
    const auto unhealthy_hosts = host_set_hosts - healthy_hosts - degraded_hosts;

    updateHostSet(host_set_, host_set_hosts, unhealthy_hosts, degraded_hosts);
  }

  const auto failover_set_hosts = total_hosts - host_set_hosts;

  if (host_set_hosts == 0) {
    updateHostSet(failover_host_set_, 0, 0);
  } else {
    // We get on average 50% healthy hosts, 25% degraded hosts and 25% unhealthy hosts.
    const auto healthy_hosts = rand.random() % failover_set_hosts;
    const auto degraded_hosts = rand.random() % (failover_set_hosts - healthy_hosts);
    const auto unhealthy_hosts = failover_set_hosts - healthy_hosts - degraded_hosts;

    updateHostSet(failover_host_set_, failover_set_hosts, unhealthy_hosts, degraded_hosts);
  }

  EXPECT_CALL(context, determinePriorityLoad(_, _))
      .WillRepeatedly(
          Invoke([](const auto&, const auto& original_load) -> const HealthyAndDegradedLoad& {
            return original_load;
          }));

  for (uint64_t i = 0; i < total_hosts; ++i) {
    const auto hs = lb_.chooseHostSet(&context);
    switch (hs.second) {
    case LoadBalancerBase::HostAvailability::Healthy:
      // Either we selected one of the healthy hosts or we failed to select anything and defaulted
      // to healthy.
      EXPECT_TRUE(!hs.first.healthyHosts().empty() ||
                  (hs.first.healthyHosts().empty() && hs.first.degradedHosts().empty()));
      break;
    case LoadBalancerBase::HostAvailability::Degraded:
      EXPECT_FALSE(hs.first.degradedHosts().empty());
      break;
    }
  }
}

// Test of host set selection with priority filter
TEST_P(LoadBalancerBaseTest, PrioritySelectionWithFilter) {
  NiceMock<Upstream::MockLoadBalancerContext> context;

  HealthyAndDegradedLoad priority_load{Upstream::HealthyLoad({0u, 100u}),
                                       Upstream::DegradedLoad({0, 0})};
  // return a filter that excludes priority 0
  EXPECT_CALL(context, determinePriorityLoad(_, _)).WillRepeatedly(ReturnRef(priority_load));

  updateHostSet(host_set_, 1 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 1, 1);

  // Since we've excluded P0, we should pick the failover host set
  EXPECT_EQ(failover_host_set_.priority(), lb_.chooseHostSet(&context).first.priority());

  updateHostSet(host_set_, 1 /* num_hosts */, 0 /* num_healthy_hosts */,
                1 /* num_degraded_hosts */);
  updateHostSet(failover_host_set_, 1, 0, 1);

  // exclude priority 0 for degraded hosts
  priority_load.healthy_priority_load_ = Upstream::HealthyLoad({0, 0});
  priority_load.degraded_priority_load_ = Upstream::DegradedLoad({0, 100});

  // Since we've excluded P0, we should pick the failover host set
  EXPECT_EQ(failover_host_set_.priority(), lb_.chooseHostSet(&context).first.priority());
}

TEST_P(LoadBalancerBaseTest, OverProvisioningFactor) {
  // Default overprovisioning factor 1.4 makes P0 receives 70% load.
  updateHostSet(host_set_, 4, 2);
  updateHostSet(failover_host_set_, 4, 2);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(70, 30));

  // Set overprovisioning factor to 1, now it should be proportioned to healthy ratio.
  host_set_.setOverprovisioningFactor(100);
  updateHostSet(host_set_, 4, 2);
  failover_host_set_.setOverprovisioningFactor(100);
  updateHostSet(failover_host_set_, 4, 2);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(50, 50));
}

TEST_P(LoadBalancerBaseTest, GentleFailover) {
  // With 100% of P=0 hosts healthy, P=0 gets all the load.
  // None of the levels is in Panic mode
  updateHostSet(host_set_, 1, 1);
  updateHostSet(failover_host_set_, 1, 1);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(100, 0));
  ASSERT_THAT(getPanic(), ElementsAre(false, false));

  // Health P=0 == 50*1.4 == 70
  // Total health = 70 + 70 >= 100%. None of the levels should be in panic mode.
  updateHostSet(host_set_, 2 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 2 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(70, 30));
  ASSERT_THAT(getPanic(), ElementsAre(false, false));

  // Health P=0 == 25*1.4 == 35   P=1 is healthy so takes all spillover.
  // Total health = 35+100 >= 100%. P=0 is below Panic level but it is ignored, because
  // Total health >= 100%.
  updateHostSet(host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 2 /* num_hosts */, 2 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(35, 65));
  ASSERT_THAT(getPanic(), ElementsAre(false, false));

  // Health P=0 == 25*1.4 == 35   P=1 == 35
  // Health is then scaled up by (100 / (35 + 35) == 50)
  // Total health = 35% + 35% is less than 100%. Panic levels per priority kick in.
  updateHostSet(host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(50, 50));
  ASSERT_THAT(getPanic(), ElementsAre(true, true));

  // Health P=0 == 100*1.4 == 35 P=1 == 35
  // Since 3 hosts are excluded, P=0 should be considered fully healthy.
  // Total health = 100% + 35% is greater than 100%. Panic should not trigger.
  updateHostSet(host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */, 0 /* num_degraded_hosts */,
                3 /* num_excluded_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(100, 0));
  ASSERT_THAT(getPanic(), ElementsAre(false, false));

  // Health P=0 == 100*1.4 == 35 P=1 == 35
  // Since 4 hosts are excluded and are unhealthy, P=0 should be considered fully unavailable.
  // Total health = 35% is less than 100%. Panic should trigger.
  updateHostSet(host_set_, 4 /* num_hosts */, 0 /* num_healthy_hosts */, 0 /* num_degraded_hosts */,
                4 /* num_excluded_hosts */);
  updateHostSet(failover_host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 100));
  ASSERT_THAT(getPanic(), ElementsAre(true, true));
}

TEST_P(LoadBalancerBaseTest, GentleFailoverWithExtraLevels) {
  // Add a third host set. Again with P=0 healthy, all traffic goes there.
  MockHostSet& tertiary_host_set_ = *priority_set_.getMockHostSet(2);
  updateHostSet(host_set_, 1, 1);
  updateHostSet(failover_host_set_, 1, 1);
  updateHostSet(tertiary_host_set_, 1, 1);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(100, 0, 0));
  ASSERT_THAT(getPanic(), ElementsAre(false, false, false));

  // Health P=0 == 50*1.4 == 70
  // Health P=0 == 50, so can take the 30% spillover.
  updateHostSet(host_set_, 2 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 2 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 2 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(70, 30, 0));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));

  // Health P=0 == 25*1.4 == 35   P=1 is healthy so takes all spillover.
  updateHostSet(host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 2 /* num_hosts */, 2 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 2 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(35, 65, 0));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));

  // This is the first test where health (P=0 + P=1 < 100)
  // Health P=0 == 25*1.4 == 35   P=1 == 35  P=2 == 35
  updateHostSet(host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(35, 35, 30));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));

  // This is the first test where (health P=0 + P=1 < 100)
  // Health P=0 == 25*1.4 == 35   P=1 == 35  P=2 == 35
  updateHostSet(host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 4 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(35, 35, 30));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));

  // Now all health is (20% * 1.5 == 28). 28 * 3 < 100 so we have to scale.
  // Each Priority level gets 33% of the load, with P=0 picking up the rounding error.
  updateHostSet(host_set_, 5 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 1 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(34, 33, 33));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));
  ASSERT_THAT(getPanic(), ElementsAre(true, true, true));

  // Levels P=0 and P=1 are totally down. P=2 is totally healthy.
  // 100% of the traffic should go to P=2 and P=0 and P=1 should
  // not be in panic mode.
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 5 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 0, 100));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));
  ASSERT_THAT(getPanic(), ElementsAre(false, false, false));

  // Levels P=0 and P=1 are totally down. P=2 is 80*1.4 >= 100% healthy.
  // 100% of the traffic should go to P=2 and P=0 and P=1 should
  // not be in panic mode.
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 4 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 0, 100));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));
  ASSERT_THAT(getPanic(), ElementsAre(false, false, false));

  // Levels P=0 and P=1 are totally down. P=2 is 40*1.4=56%% healthy.
  // 100% of the traffic should go to P=2. All levels P=0, P=1 and P=2 should
  // be in panic mode even though P=0 and P=1 do not receive any load.
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 2 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 0, 100));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));
  ASSERT_THAT(getPanic(), ElementsAre(true, true, true));

  // Level P=0 is totally degraded. P=1 is 40*1.4=56% healthy and 40*1.4=56% degraded. P=2 is
  // 40*1.4=56%% healthy. 100% of the traffic should go to P=2. No priorities should be in panic
  // mode.
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */,
                5 /* num_degraded_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 2 /* num_healthy_hosts */,
                2 /* num_degraded_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 2 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 56, 44));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));
  ASSERT_THAT(getPanic(), ElementsAre(false, false, false));

  // All levels are completely down. 100% of traffic should go to P=0
  // and P=0 should be in panic mode
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(100, _, _));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));
  ASSERT_THAT(getPanic(), ElementsAre(true, _, _));

  // Rounding errors should be picked up by the first healthy priority.
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 2 /* num_healthy_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 67, 33));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 0, 0));

  // Load should spill over into degraded.
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */,
                1 /* num_degraded_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */,
                5 /* num_degraded_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 1 /* num_healthy_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 0, 28));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(28, 44, 0));

  // Rounding errors should be picked up by the first priority with degraded hosts when
  // there are no healthy priorities.
  updateHostSet(host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */);
  updateHostSet(failover_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */,
                2 /* num_degraded_hosts */);
  updateHostSet(tertiary_host_set_, 5 /* num_hosts */, 0 /* num_healthy_hosts */,
                1 /* num_degraded_hosts */);
  ASSERT_THAT(getLoadPercentage(), ElementsAre(0, 0, 0));
  ASSERT_THAT(getDegradedLoadPercentage(), ElementsAre(0, 67, 33));
}

TEST_P(LoadBalancerBaseTest, BoundaryConditions) {
  TestRandomGenerator rand;
  uint32_t num_priorities = rand.random() % 10;

  for (uint32_t i = 0; i < num_priorities; ++i) {
    uint32_t num_hosts = rand.random() % 100;
    uint32_t healthy_hosts = std::min<uint32_t>(num_hosts, rand.random() % 100);
    // Make sure random health situations don't trigger the assert in recalculatePerPriorityState
    updateHostSet(*priority_set_.getMockHostSet(i), num_hosts, healthy_hosts);
  }
}

class RoundRobinLoadBalancerTest : public LoadBalancerTestBase {
public:
  void init(bool need_local_cluster) {
    if (need_local_cluster) {
      local_priority_set_.reset(new PrioritySetImpl());
      local_priority_set_->getOrCreateHostSet(0);
    }
    lb_.reset(new RoundRobinLoadBalancer(priority_set_, local_priority_set_.get(), stats_, runtime_,
                                         random_, common_config_));
  }

  // Updates priority 0 with the given hosts and hosts_per_locality.
  void updateHosts(HostVectorConstSharedPtr hosts,
                   HostsPerLocalityConstSharedPtr hosts_per_locality) {
    local_priority_set_->updateHosts(
        0,
        updateHostsParams(hosts, hosts_per_locality,
                          std::make_shared<const HealthyHostVector>(*hosts), hosts_per_locality),
        {}, empty_host_vector_, empty_host_vector_, absl::nullopt);
  }

  std::shared_ptr<PrioritySetImpl> local_priority_set_;
  std::shared_ptr<LoadBalancer> lb_;
  HostsPerLocalityConstSharedPtr empty_locality_;
  HostVector empty_host_vector_;
}; // namespace

// For the tests which mutate primary and failover host sets explicitly, only
// run once.
using FailoverTest = RoundRobinLoadBalancerTest;

// Ensure if all the hosts with priority 0 unhealthy, the next priority hosts are used.
TEST_P(FailoverTest, BasicFailover) {
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  failover_host_set_.healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:82")};
  failover_host_set_.hosts_ = failover_host_set_.healthy_hosts_;
  init(false);
  EXPECT_EQ(failover_host_set_.healthy_hosts_[0], lb_->chooseHost(nullptr));
}

// Ensure if all the hosts with priority 0 degraded, the first priority degraded hosts are used.
TEST_P(FailoverTest, BasicDegradedHosts) {
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  host_set_.degraded_hosts_ = host_set_.hosts_;
  failover_host_set_.hosts_ = failover_host_set_.healthy_hosts_;
  init(false);
  EXPECT_EQ(host_set_.degraded_hosts_[0], lb_->chooseHost(nullptr));
}

// Ensure if all the hosts with priority 0 degraded, but healthy hosts in the failover, the healthy
// hosts in the second priority are used.
TEST_P(FailoverTest, BasicFailoverDegradedHosts) {
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  host_set_.degraded_hosts_ = host_set_.hosts_;
  failover_host_set_.healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:82")};
  failover_host_set_.hosts_ = failover_host_set_.healthy_hosts_;
  init(false);
  EXPECT_EQ(failover_host_set_.healthy_hosts_[0], lb_->chooseHost(nullptr));
}

// Test that extending the priority set with an existing LB causes the correct updates.
TEST_P(FailoverTest, PriorityUpdatesWithLocalHostSet) {
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  failover_host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:81")};
  init(false);
  // With both the primary and failover hosts unhealthy, we should select an
  // unhealthy primary host.
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Update the priority set with a new priority level P=2 and ensure the host
  // is chosen
  MockHostSet& tertiary_host_set_ = *priority_set_.getMockHostSet(2);
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:82")}));
  tertiary_host_set_.hosts_ = *hosts;
  tertiary_host_set_.healthy_hosts_ = tertiary_host_set_.hosts_;
  HostVector add_hosts;
  add_hosts.push_back(tertiary_host_set_.hosts_[0]);
  tertiary_host_set_.runCallbacks(add_hosts, {});
  EXPECT_EQ(tertiary_host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Now add a healthy host in P=0 and make sure it is immediately selected.
  host_set_.healthy_hosts_ = host_set_.hosts_;
  host_set_.runCallbacks(add_hosts, {});
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Remove the healthy host and ensure we fail back over to tertiary_host_set_
  host_set_.healthy_hosts_ = {};
  host_set_.runCallbacks({}, {});
  EXPECT_EQ(tertiary_host_set_.hosts_[0], lb_->chooseHost(nullptr));
}

// Test that extending the priority set with an existing LB causes the correct updates when the
// cluster is configured to disable on panic.
TEST_P(FailoverTest, PriorityUpdatesWithLocalHostSetDisableOnPanic) {
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  failover_host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:81")};
  common_config_.mutable_zone_aware_lb_config()->set_fail_traffic_on_panic(true);

  init(false);
  // With both the primary and failover hosts unhealthy, we should select no host.
  EXPECT_EQ(nullptr, lb_->chooseHost(nullptr));

  // Update the priority set with a new priority level P=2 and ensure the host
  // is chosen
  MockHostSet& tertiary_host_set_ = *priority_set_.getMockHostSet(2);
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:82")}));
  tertiary_host_set_.hosts_ = *hosts;
  tertiary_host_set_.healthy_hosts_ = tertiary_host_set_.hosts_;
  HostVector add_hosts;
  add_hosts.push_back(tertiary_host_set_.hosts_[0]);
  tertiary_host_set_.runCallbacks(add_hosts, {});
  EXPECT_EQ(tertiary_host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Now add a healthy host in P=0 and make sure it is immediately selected.
  host_set_.healthy_hosts_ = host_set_.hosts_;
  host_set_.runCallbacks(add_hosts, {});
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Remove the healthy host and ensure we fail back over to tertiary_host_set_
  host_set_.healthy_hosts_ = {};
  host_set_.runCallbacks({}, {});
  EXPECT_EQ(tertiary_host_set_.hosts_[0], lb_->chooseHost(nullptr));
}

// Test extending the priority set.
TEST_P(FailoverTest, ExtendPrioritiesUpdatingPrioritySet) {
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  failover_host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:81")};
  init(true);
  // With both the primary and failover hosts unhealthy, we should select an
  // unhealthy primary host.
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Update the priority set with a new priority level P=2
  // As it has healthy hosts, it should be selected.
  MockHostSet& tertiary_host_set_ = *priority_set_.getMockHostSet(2);
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:82")}));
  tertiary_host_set_.hosts_ = *hosts;
  tertiary_host_set_.healthy_hosts_ = tertiary_host_set_.hosts_;
  HostVector add_hosts;
  add_hosts.push_back(tertiary_host_set_.hosts_[0]);
  tertiary_host_set_.runCallbacks(add_hosts, {});
  EXPECT_EQ(tertiary_host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Now add a healthy host in P=0 and make sure it is immediately selected.
  host_set_.healthy_hosts_ = host_set_.hosts_;
  host_set_.runCallbacks(add_hosts, {});
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));
}

TEST_P(FailoverTest, ExtendPrioritiesWithLocalPrioritySet) {
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  failover_host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:81")};
  init(true);
  // With both the primary and failover hosts unhealthy, we should select an
  // unhealthy primary host.
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Update the host set with a new priority level. We should start selecting
  // hosts from that level as it has viable hosts.
  MockHostSet& tertiary_host_set_ = *priority_set_.getMockHostSet(2);
  HostVectorSharedPtr hosts2(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:84")}));
  tertiary_host_set_.hosts_ = *hosts2;
  tertiary_host_set_.healthy_hosts_ = tertiary_host_set_.hosts_;
  HostVector add_hosts;
  add_hosts.push_back(tertiary_host_set_.hosts_[0]);
  tertiary_host_set_.runCallbacks(add_hosts, {});
  EXPECT_EQ(tertiary_host_set_.hosts_[0], lb_->chooseHost(nullptr));

  // Update the local hosts. We're not doing locality based routing in this
  // test, but it should at least do no harm.
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:82")}));
  updateHosts(hosts, HostsPerLocalityImpl::empty());
  EXPECT_EQ(tertiary_host_set_.hosts_[0], lb_->chooseHost(nullptr));
}

// Verifies that the number of warmed hosts is used to compute priority spillover.
TEST_P(FailoverTest, PrioritiesWithNotAllWarmedHosts) {
  // To begin with we set up the following:
  // P0: 1 healthy, 1 unhealthy, 1 warmed.
  // P1: 1 healthy.
  // We then expect no spillover, since P0 is still overprovisioned.
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                      makeTestHost(info_, "tcp://127.0.0.1:81")};
  host_set_.healthy_hosts_ = {host_set_.hosts_[0]};
  failover_host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:82")};
  failover_host_set_.healthy_hosts_ = failover_host_set_.hosts_;
  init(true);

  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(host_set_.hosts_[0], lb_->chooseHost(nullptr));
}

// Verifies that we handle zero warmed hosts.
TEST_P(FailoverTest, PrioritiesWithZeroWarmedHosts) {
  // To begin with we set up the following:
  // P0: 2 unhealthy, 0 warmed.
  // P1: 1 healthy.
  // We then expect all the traffic to spill over to P1 since P0 has an effective load of zero.
  host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                      makeTestHost(info_, "tcp://127.0.0.1:81")};
  failover_host_set_.hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:82")};
  failover_host_set_.healthy_hosts_ = failover_host_set_.hosts_;

  init(true);

  EXPECT_EQ(failover_host_set_.hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(failover_host_set_.hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(failover_host_set_.hosts_[0], lb_->chooseHost(nullptr));
}

INSTANTIATE_TEST_SUITE_P(PrimaryOrFailover, FailoverTest, ::testing::Values(true));

TEST_P(RoundRobinLoadBalancerTest, NoHosts) {
  init(false);
  EXPECT_EQ(nullptr, lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, SingleHost) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  hostSet().hosts_ = hostSet().healthy_hosts_;
  init(false);
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, Normal) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                              makeTestHost(info_, "tcp://127.0.0.1:81")};
  hostSet().hosts_ = hostSet().healthy_hosts_;
  init(false);
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
}

// Validate that the RNG seed influences pick order.
TEST_P(RoundRobinLoadBalancerTest, Seed) {
  hostSet().healthy_hosts_ = {
      makeTestHost(info_, "tcp://127.0.0.1:80"),
      makeTestHost(info_, "tcp://127.0.0.1:81"),
      makeTestHost(info_, "tcp://127.0.0.1:82"),
  };
  hostSet().hosts_ = hostSet().healthy_hosts_;
  EXPECT_CALL(random_, random()).WillRepeatedly(Return(1));
  init(false);
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, Locality) {
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:80"),
                                            makeTestHost(info_, "tcp://127.0.0.1:81"),
                                            makeTestHost(info_, "tcp://127.0.0.1:82")}));
  HostsPerLocalitySharedPtr hosts_per_locality =
      makeHostsPerLocality({{(*hosts)[1]}, {(*hosts)[0]}, {(*hosts)[2]}});
  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_ = *hosts;
  hostSet().healthy_hosts_per_locality_ = hosts_per_locality;
  init(false);
  // chooseHealthyLocality() return value determines which locality we use.
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(0));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(1));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(0));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(1));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(0));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  // When there is no locality, we RR over all available hosts.
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(absl::optional<uint32_t>()));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(absl::optional<uint32_t>()));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(absl::optional<uint32_t>()));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, DegradedLocality) {
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:80"),
                                            makeTestHost(info_, "tcp://127.0.0.1:81"),
                                            makeTestHost(info_, "tcp://127.0.0.1:84")}));
  HostVectorSharedPtr healthy_hosts(new HostVector({(*hosts)[0]}));
  HostVectorSharedPtr degraded_hosts(new HostVector({(*hosts)[1], (*hosts)[2]}));
  HostsPerLocalitySharedPtr hosts_per_locality =
      makeHostsPerLocality({{(*hosts)[0]}, {(*hosts)[1], (*hosts)[2]}});
  HostsPerLocalitySharedPtr healthy_hosts_per_locality = makeHostsPerLocality({{(*hosts)[0]}, {}});
  HostsPerLocalitySharedPtr degraded_hosts_per_locality =
      makeHostsPerLocality({{}, {(*hosts)[1], (*hosts)[2]}});

  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_ = *healthy_hosts;
  hostSet().degraded_hosts_ = *degraded_hosts;
  hostSet().hosts_per_locality_ = hosts_per_locality;
  hostSet().healthy_hosts_per_locality_ = healthy_hosts_per_locality;
  hostSet().degraded_hosts_per_locality_ = degraded_hosts_per_locality;
  init(false);

  EXPECT_CALL(random_, random()).WillOnce(Return(50)).WillOnce(Return(0));
  // Since we're split between healthy and degraded, the LB should call into both
  // chooseHealthyLocality and chooseDegradedLocality.
  EXPECT_CALL(hostSet(), chooseDegradedLocality()).WillOnce(Return(1));
  EXPECT_EQ(hostSet().degraded_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_CALL(hostSet(), chooseHealthyLocality()).WillOnce(Return(0));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, Weighted) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80", 1),
                              makeTestHost(info_, "tcp://127.0.0.1:81", 2)};
  hostSet().hosts_ = hostSet().healthy_hosts_;
  init(false);
  // Initial weights respected.
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  // Modify weights, we converge on new weighting after one pick cycle.
  hostSet().healthy_hosts_[0]->weight(2);
  hostSet().healthy_hosts_[1]->weight(1);
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  // Add a host, it should participate in next round of scheduling.
  hostSet().healthy_hosts_.push_back(makeTestHost(info_, "tcp://127.0.0.1:82", 3));
  hostSet().hosts_.push_back(hostSet().healthy_hosts_.back());
  hostSet().runCallbacks({hostSet().healthy_hosts_.back()}, {});
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  // Remove last two hosts, add a new one with different weights.
  HostVector removed_hosts = {hostSet().hosts_[1], hostSet().hosts_[2]};
  hostSet().healthy_hosts_.pop_back();
  hostSet().healthy_hosts_.pop_back();
  hostSet().hosts_.pop_back();
  hostSet().hosts_.pop_back();
  hostSet().healthy_hosts_.push_back(makeTestHost(info_, "tcp://127.0.0.1:83", 4));
  hostSet().hosts_.push_back(hostSet().healthy_hosts_.back());
  hostSet().healthy_hosts_[0]->weight(1);
  hostSet().runCallbacks({hostSet().healthy_hosts_.back()}, removed_hosts);
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
}

// Validate that the RNG seed influences pick order when weighted RR.
TEST_P(RoundRobinLoadBalancerTest, WeightedSeed) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80", 1),
                              makeTestHost(info_, "tcp://127.0.0.1:81", 2)};
  hostSet().hosts_ = hostSet().healthy_hosts_;
  EXPECT_CALL(random_, random()).WillRepeatedly(Return(1));
  init(false);
  // Initial weights respected.
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, MaxUnhealthyPanic) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                              makeTestHost(info_, "tcp://127.0.0.1:81")};
  hostSet().hosts_ = {
      makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81"),
      makeTestHost(info_, "tcp://127.0.0.1:82"), makeTestHost(info_, "tcp://127.0.0.1:83"),
      makeTestHost(info_, "tcp://127.0.0.1:84"), makeTestHost(info_, "tcp://127.0.0.1:85")};

  init(false);
  EXPECT_EQ(hostSet().hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().hosts_[2], lb_->chooseHost(nullptr));

  // Take the threshold back above the panic threshold.
  hostSet().healthy_hosts_ = {
      makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81"),
      makeTestHost(info_, "tcp://127.0.0.1:82"), makeTestHost(info_, "tcp://127.0.0.1:83")};
  hostSet().runCallbacks({}, {});

  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));

  EXPECT_EQ(3UL, stats_.lb_healthy_panic_.value());
}

// Test that no hosts are selected when fail_traffic_on_panic is enabled.
TEST_P(RoundRobinLoadBalancerTest, MaxUnhealthyPanicDisableOnPanic) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                              makeTestHost(info_, "tcp://127.0.0.1:81")};
  hostSet().hosts_ = {
      makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81"),
      makeTestHost(info_, "tcp://127.0.0.1:82"), makeTestHost(info_, "tcp://127.0.0.1:83"),
      makeTestHost(info_, "tcp://127.0.0.1:84"), makeTestHost(info_, "tcp://127.0.0.1:85")};

  common_config_.mutable_zone_aware_lb_config()->set_fail_traffic_on_panic(true);

  init(false);
  EXPECT_EQ(nullptr, lb_->chooseHost(nullptr));

  // Take the threshold back above the panic threshold.
  hostSet().healthy_hosts_ = {
      makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81"),
      makeTestHost(info_, "tcp://127.0.0.1:82"), makeTestHost(info_, "tcp://127.0.0.1:83")};
  hostSet().runCallbacks({}, {});

  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));

  EXPECT_EQ(1UL, stats_.lb_healthy_panic_.value());
}

// Ensure if the panic threshold is 0%, panic mode is disabled.
TEST_P(RoundRobinLoadBalancerTest, DisablePanicMode) {
  hostSet().healthy_hosts_ = {};
  hostSet().hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};

  common_config_.mutable_healthy_panic_threshold()->set_value(0);

  init(false);
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 50))
      .WillRepeatedly(Return(0));
  EXPECT_EQ(nullptr, lb_->chooseHost(nullptr));
  EXPECT_EQ(0UL, stats_.lb_healthy_panic_.value());
}

// Test of host set selection with host filter
TEST_P(RoundRobinLoadBalancerTest, HostSelectionWithFilter) {
  NiceMock<Upstream::MockLoadBalancerContext> context;

  HostVectorSharedPtr hosts(new HostVector(
      {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81")}));
  HostsPerLocalitySharedPtr hosts_per_locality = makeHostsPerLocality(
      {{makeTestHost(info_, "tcp://127.0.0.1:80")}, {makeTestHost(info_, "tcp://127.0.0.1:81")}});

  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_ = *hosts;
  hostSet().healthy_hosts_per_locality_ = hosts_per_locality;

  init(false);

  // return a predicate that only accepts the first host
  EXPECT_CALL(context, shouldSelectAnotherHost(_))
      .WillRepeatedly(Invoke([&](const Host& host) -> bool {
        return host.address()->asString() != hostSet().hosts_[0]->address()->asString();
      }));
  HealthyAndDegradedLoad priority_load{Upstream::HealthyLoad({0, 0}),
                                       Upstream::DegradedLoad({0, 0})};

  if (GetParam()) {
    priority_load.healthy_priority_load_ = HealthyLoad({100u, 0u});
  } else {
    priority_load.healthy_priority_load_ = HealthyLoad({0u, 100u});
  }
  EXPECT_CALL(context, determinePriorityLoad(_, _)).WillRepeatedly(ReturnRef(priority_load));
  EXPECT_CALL(context, hostSelectionRetryCount()).WillRepeatedly(Return(2));

  // Calling chooseHost multiple times always returns host one, since the filter will reject
  // the other host.
  EXPECT_EQ(hostSet().hosts_[0], lb_->chooseHost(&context));
  EXPECT_EQ(hostSet().hosts_[0], lb_->chooseHost(&context));
  EXPECT_EQ(hostSet().hosts_[0], lb_->chooseHost(&context));

  // By setting the retry counter to zero, we effectively disable the filter.
  EXPECT_CALL(context, hostSelectionRetryCount()).WillRepeatedly(Return(0));

  EXPECT_EQ(hostSet().hosts_[1], lb_->chooseHost(&context));
  EXPECT_EQ(hostSet().hosts_[0], lb_->chooseHost(&context));
  EXPECT_EQ(hostSet().hosts_[1], lb_->chooseHost(&context));
}

TEST_P(RoundRobinLoadBalancerTest, ZoneAwareSmallCluster) {
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:80"),
                                            makeTestHost(info_, "tcp://127.0.0.1:81"),
                                            makeTestHost(info_, "tcp://127.0.0.1:82")}));
  HostsPerLocalitySharedPtr hosts_per_locality =
      makeHostsPerLocality({{makeTestHost(info_, "tcp://127.0.0.1:81")},
                            {makeTestHost(info_, "tcp://127.0.0.1:80")},
                            {makeTestHost(info_, "tcp://127.0.0.1:82")}});

  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_ = *hosts;
  hostSet().healthy_hosts_per_locality_ = hosts_per_locality;
  common_config_.mutable_healthy_panic_threshold()->set_value(0);
  common_config_.mutable_zone_aware_lb_config()->mutable_routing_enabled()->set_value(98);
  common_config_.mutable_zone_aware_lb_config()->mutable_min_cluster_size()->set_value(7);
  init(true);
  updateHosts(hosts, hosts_per_locality);

  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 0))
      .WillRepeatedly(Return(50));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 98))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 7))
      .WillRepeatedly(Return(7));

  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[2], lb_->chooseHost(nullptr));

  if (&hostSet() == &host_set_) {
    // Cluster size is computed once at zone aware struct regeneration point.
    EXPECT_EQ(1U, stats_.lb_zone_cluster_too_small_.value());
  } else {
    EXPECT_EQ(0U, stats_.lb_zone_cluster_too_small_.value());
    return;
  }
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 7))
      .WillRepeatedly(Return(1));
  // Trigger reload.
  updateHosts(hosts, hosts_per_locality);
  EXPECT_EQ(hostSet().healthy_hosts_per_locality_->get()[0][0], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, NoZoneAwareDifferentZoneSize) {
  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:80"),
                                            makeTestHost(info_, "tcp://127.0.0.1:81"),
                                            makeTestHost(info_, "tcp://127.0.0.1:82")}));
  HostsPerLocalitySharedPtr upstream_hosts_per_locality =
      makeHostsPerLocality({{makeTestHost(info_, "tcp://127.0.0.1:81")},
                            {makeTestHost(info_, "tcp://127.0.0.1:80")},
                            {makeTestHost(info_, "tcp://127.0.0.1:82")}});
  HostsPerLocalitySharedPtr local_hosts_per_locality = makeHostsPerLocality(
      {{makeTestHost(info_, "tcp://127.0.0.1:81")}, {makeTestHost(info_, "tcp://127.0.0.1:80")}});

  hostSet().healthy_hosts_ = *hosts;
  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_per_locality_ = upstream_hosts_per_locality;
  common_config_.mutable_healthy_panic_threshold()->set_value(100);
  common_config_.mutable_zone_aware_lb_config()->mutable_routing_enabled()->set_value(98);
  common_config_.mutable_zone_aware_lb_config()->mutable_min_cluster_size()->set_value(7);
  init(true);
  updateHosts(hosts, local_hosts_per_locality);

  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 100))
      .WillRepeatedly(Return(50));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 98))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 7))
      .WillRepeatedly(Return(7));

  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(1U, stats_.lb_zone_number_differs_.value());
}

TEST_P(RoundRobinLoadBalancerTest, ZoneAwareRoutingLargeZoneSwitchOnOff) {
  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:80"),
                                            makeTestHost(info_, "tcp://127.0.0.1:81"),
                                            makeTestHost(info_, "tcp://127.0.0.1:82")}));
  HostsPerLocalitySharedPtr hosts_per_locality =
      makeHostsPerLocality({{makeTestHost(info_, "tcp://127.0.0.1:81")},
                            {makeTestHost(info_, "tcp://127.0.0.1:80")},
                            {makeTestHost(info_, "tcp://127.0.0.1:82")}});

  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 50))
      .WillRepeatedly(Return(50));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 100))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 6))
      .WillRepeatedly(Return(3));

  hostSet().healthy_hosts_ = *hosts;
  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_per_locality_ = hosts_per_locality;
  init(true);
  updateHosts(hosts, hosts_per_locality);

  // There is only one host in the given zone for zone aware routing.
  EXPECT_EQ(hostSet().healthy_hosts_per_locality_->get()[0][0], lb_->chooseHost(nullptr));
  EXPECT_EQ(1U, stats_.lb_zone_routing_all_directly_.value());
  EXPECT_EQ(hostSet().healthy_hosts_per_locality_->get()[0][0], lb_->chooseHost(nullptr));
  EXPECT_EQ(2U, stats_.lb_zone_routing_all_directly_.value());

  // Disable runtime global zone routing.
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 100))
      .WillRepeatedly(Return(false));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, ZoneAwareRoutingSmallZone) {
  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  HostVectorSharedPtr upstream_hosts(new HostVector(
      {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81"),
       makeTestHost(info_, "tcp://127.0.0.1:82"), makeTestHost(info_, "tcp://127.0.0.1:83"),
       makeTestHost(info_, "tcp://127.0.0.1:84")}));
  HostVectorSharedPtr local_hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:0"),
                                                  makeTestHost(info_, "tcp://127.0.0.1:1"),
                                                  makeTestHost(info_, "tcp://127.0.0.1:2")}));

  HostsPerLocalitySharedPtr upstream_hosts_per_locality = makeHostsPerLocality(
      {{makeTestHost(info_, "tcp://127.0.0.1:81")},
       {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:82")},
       {makeTestHost(info_, "tcp://127.0.0.1:83"), makeTestHost(info_, "tcp://127.0.0.1:84")}});

  HostsPerLocalitySharedPtr local_hosts_per_locality =
      makeHostsPerLocality({{makeTestHost(info_, "tcp://127.0.0.1:0")},
                            {makeTestHost(info_, "tcp://127.0.0.1:1")},
                            {makeTestHost(info_, "tcp://127.0.0.1:2")}});

  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 50))
      .WillRepeatedly(Return(50));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 100))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 6))
      .WillRepeatedly(Return(5));

  hostSet().healthy_hosts_ = *upstream_hosts;
  hostSet().hosts_ = *upstream_hosts;
  hostSet().healthy_hosts_per_locality_ = upstream_hosts_per_locality;
  init(true);
  updateHosts(local_hosts, local_hosts_per_locality);

  // There is only one host in the given zone for zone aware routing.
  EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(100));
  EXPECT_EQ(hostSet().healthy_hosts_per_locality_->get()[0][0], lb_->chooseHost(nullptr));
  EXPECT_EQ(1U, stats_.lb_zone_routing_sampled_.value());

  // Force request out of small zone.
  EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(9999)).WillOnce(Return(2));
  EXPECT_EQ(hostSet().healthy_hosts_per_locality_->get()[1][0], lb_->chooseHost(nullptr));
  EXPECT_EQ(1U, stats_.lb_zone_routing_cross_zone_.value());
}

TEST_P(RoundRobinLoadBalancerTest, LowPrecisionForDistribution) {
  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  // upstream_hosts and local_hosts do not matter, zone aware routing is based on per zone hosts.
  HostVectorSharedPtr upstream_hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:80")}));
  hostSet().healthy_hosts_ = *upstream_hosts;
  hostSet().hosts_ = *upstream_hosts;
  HostVectorSharedPtr local_hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:0")}));

  std::vector<HostVector> upstream_hosts_per_locality;
  std::vector<HostVector> local_hosts_per_locality;

  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 50))
      .WillRepeatedly(Return(50));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 100))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 6))
      .WillRepeatedly(Return(1));

  // The following host distribution with current precision should lead to the no_capacity_left
  // situation.
  // Reuse the same host in all of the structures below to reduce time test takes and this does not
  // impact load balancing logic.
  HostSharedPtr host = makeTestHost(info_, "tcp://127.0.0.1:80");
  HostVector current(45000);

  for (int i = 0; i < 45000; ++i) {
    current[i] = host;
  }
  local_hosts_per_locality.push_back(current);

  current.resize(55000);
  for (int i = 0; i < 55000; ++i) {
    current[i] = host;
  }
  local_hosts_per_locality.push_back(current);

  current.resize(44999);
  for (int i = 0; i < 44999; ++i) {
    current[i] = host;
  }
  upstream_hosts_per_locality.push_back(current);

  current.resize(55001);
  for (int i = 0; i < 55001; ++i) {
    current[i] = host;
  }
  upstream_hosts_per_locality.push_back(current);

  hostSet().healthy_hosts_per_locality_ =
      makeHostsPerLocality(std::move(upstream_hosts_per_locality));
  init(true);

  // To trigger update callback.
  auto local_hosts_per_locality_shared = makeHostsPerLocality(std::move(local_hosts_per_locality));
  updateHosts(local_hosts, local_hosts_per_locality_shared);

  // Force request out of small zone and to randomly select zone.
  EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(9999)).WillOnce(Return(2));
  lb_->chooseHost(nullptr);
  EXPECT_EQ(1U, stats_.lb_zone_no_capacity_left_.value());
}

TEST_P(RoundRobinLoadBalancerTest, NoZoneAwareRoutingOneZone) {
  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  HostVectorSharedPtr hosts(new HostVector({makeTestHost(info_, "tcp://127.0.0.1:80")}));
  HostsPerLocalitySharedPtr hosts_per_locality =
      makeHostsPerLocality({{makeTestHost(info_, "tcp://127.0.0.1:81")}});

  hostSet().healthy_hosts_ = *hosts;
  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_per_locality_ = hosts_per_locality;
  init(true);
  updateHosts(hosts, hosts_per_locality);
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, NoZoneAwareRoutingNotHealthy) {
  HostVectorSharedPtr hosts(new HostVector(
      {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.2:80")}));
  HostsPerLocalitySharedPtr hosts_per_locality = makeHostsPerLocality(
      {{}, {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.2:80")}});

  hostSet().healthy_hosts_ = *hosts;
  hostSet().hosts_ = *hosts;
  hostSet().healthy_hosts_per_locality_ = hosts_per_locality;
  init(true);
  updateHosts(hosts, hosts_per_locality);

  // local zone has no healthy hosts, take from the all healthy hosts.
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
}

TEST_P(RoundRobinLoadBalancerTest, NoZoneAwareRoutingLocalEmpty) {
  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  HostVectorSharedPtr upstream_hosts(new HostVector(
      {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81")}));
  HostVectorSharedPtr local_hosts(new HostVector({}, {}));

  HostsPerLocalitySharedPtr upstream_hosts_per_locality = makeHostsPerLocality(
      {{makeTestHost(info_, "tcp://127.0.0.1:80")}, {makeTestHost(info_, "tcp://127.0.0.1:81")}});
  HostsPerLocalitySharedPtr local_hosts_per_locality = makeHostsPerLocality({{}, {}});

  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 50))
      .WillOnce(Return(50))
      .WillOnce(Return(50));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 100))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 6))
      .WillOnce(Return(1));

  hostSet().healthy_hosts_ = *upstream_hosts;
  hostSet().hosts_ = *upstream_hosts;
  hostSet().healthy_hosts_per_locality_ = upstream_hosts_per_locality;
  init(true);
  updateHosts(local_hosts, local_hosts_per_locality);

  // Local cluster is not OK, we'll do regular routing.
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(0U, stats_.lb_healthy_panic_.value());
  EXPECT_EQ(1U, stats_.lb_local_cluster_not_ok_.value());
}

TEST_P(RoundRobinLoadBalancerTest, NoZoneAwareRoutingLocalEmptyFailTrafficOnPanic) {
  common_config_.mutable_zone_aware_lb_config()->set_fail_traffic_on_panic(true);

  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  HostVectorSharedPtr upstream_hosts(new HostVector(
      {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81")}));
  HostVectorSharedPtr local_hosts(new HostVector({}, {}));

  HostsPerLocalitySharedPtr upstream_hosts_per_locality = makeHostsPerLocality(
      {{makeTestHost(info_, "tcp://127.0.0.1:80")}, {makeTestHost(info_, "tcp://127.0.0.1:81")}});
  HostsPerLocalitySharedPtr local_hosts_per_locality = makeHostsPerLocality({{}, {}});

  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.healthy_panic_threshold", 50))
      .WillOnce(Return(50))
      .WillOnce(Return(50));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("upstream.zone_routing.enabled", 100))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("upstream.zone_routing.min_cluster_size", 6))
      .WillOnce(Return(1));

  hostSet().healthy_hosts_ = *upstream_hosts;
  hostSet().hosts_ = *upstream_hosts;
  hostSet().healthy_hosts_per_locality_ = upstream_hosts_per_locality;
  init(true);
  updateHosts(local_hosts, local_hosts_per_locality);

  // Local cluster is not OK, we'll do regular routing (and select no host, since we're in global
  // panic).
  EXPECT_EQ(nullptr, lb_->chooseHost(nullptr));
  EXPECT_EQ(0U, stats_.lb_healthy_panic_.value());
  EXPECT_EQ(1U, stats_.lb_local_cluster_not_ok_.value());
}

// Validate that if we have healthy host lists >= 2, but there is no local
// locality included, that we skip zone aware routing and fallback.
TEST_P(RoundRobinLoadBalancerTest, NoZoneAwareRoutingNoLocalLocality) {
  if (&hostSet() == &failover_host_set_) { // P = 1 does not support zone-aware routing.
    return;
  }
  HostVectorSharedPtr upstream_hosts(new HostVector(
      {makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81")}));
  HostVectorSharedPtr local_hosts(new HostVector({}, {}));

  HostsPerLocalitySharedPtr upstream_hosts_per_locality = makeHostsPerLocality(
      {{makeTestHost(info_, "tcp://127.0.0.1:80")}, {makeTestHost(info_, "tcp://127.0.0.1:81")}},
      true);
  const HostsPerLocalitySharedPtr& local_hosts_per_locality = upstream_hosts_per_locality;

  hostSet().healthy_hosts_ = *upstream_hosts;
  hostSet().hosts_ = *upstream_hosts;
  hostSet().healthy_hosts_per_locality_ = upstream_hosts_per_locality;
  init(true);
  updateHosts(local_hosts, local_hosts_per_locality);

  // Local cluster is not OK, we'll do regular routing.
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_EQ(0U, stats_.lb_healthy_panic_.value());
  EXPECT_EQ(1U, stats_.lb_local_cluster_not_ok_.value());
}

INSTANTIATE_TEST_SUITE_P(PrimaryOrFailover, RoundRobinLoadBalancerTest,
                         ::testing::Values(true, false));

class LeastRequestLoadBalancerTest : public LoadBalancerTestBase {
public:
  LeastRequestLoadBalancer lb_{
      priority_set_, nullptr, stats_, runtime_, random_, common_config_, least_request_lb_config_};
};

TEST_P(LeastRequestLoadBalancerTest, NoHosts) { EXPECT_EQ(nullptr, lb_.chooseHost(nullptr)); }

TEST_P(LeastRequestLoadBalancerTest, SingleHost) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80")};
  hostSet().hosts_ = hostSet().healthy_hosts_;
  hostSet().runCallbacks({}, {}); // Trigger callbacks. The added/removed lists are not relevant.

  // Host weight is 1.
  {
    EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(2)).WillOnce(Return(3));
    stats_.max_host_weight_.set(1UL);
    EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  }

  // Host weight is 100.
  {
    EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(2)).WillOnce(Return(3));
    stats_.max_host_weight_.set(100UL);
    EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  }

  HostVector empty;
  {
    hostSet().runCallbacks(empty, empty);
    EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(2)).WillOnce(Return(3));
    EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  }

  {
    HostVector remove_hosts;
    remove_hosts.push_back(hostSet().hosts_[0]);
    hostSet().healthy_hosts_.clear();
    hostSet().hosts_.clear();
    hostSet().runCallbacks(empty, remove_hosts);
    EXPECT_CALL(random_, random()).WillOnce(Return(0));
    EXPECT_EQ(nullptr, lb_.chooseHost(nullptr));
  }
}

TEST_P(LeastRequestLoadBalancerTest, Normal) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                              makeTestHost(info_, "tcp://127.0.0.1:81")};
  stats_.max_host_weight_.set(1UL);
  hostSet().hosts_ = hostSet().healthy_hosts_;
  hostSet().runCallbacks({}, {}); // Trigger callbacks. The added/removed lists are not relevant.

  hostSet().healthy_hosts_[0]->stats().rq_active_.set(1);
  hostSet().healthy_hosts_[1]->stats().rq_active_.set(2);
  EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(2)).WillOnce(Return(3));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));

  hostSet().healthy_hosts_[0]->stats().rq_active_.set(2);
  hostSet().healthy_hosts_[1]->stats().rq_active_.set(1);
  EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(2)).WillOnce(Return(3));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
}

TEST_P(LeastRequestLoadBalancerTest, PNC) {
  hostSet().healthy_hosts_ = {
      makeTestHost(info_, "tcp://127.0.0.1:80"), makeTestHost(info_, "tcp://127.0.0.1:81"),
      makeTestHost(info_, "tcp://127.0.0.1:82"), makeTestHost(info_, "tcp://127.0.0.1:83")};
  stats_.max_host_weight_.set(1UL);
  hostSet().hosts_ = hostSet().healthy_hosts_;
  hostSet().runCallbacks({}, {}); // Trigger callbacks. The added/removed lists are not relevant.

  hostSet().healthy_hosts_[0]->stats().rq_active_.set(4);
  hostSet().healthy_hosts_[1]->stats().rq_active_.set(3);
  hostSet().healthy_hosts_[2]->stats().rq_active_.set(2);
  hostSet().healthy_hosts_[3]->stats().rq_active_.set(1);

  // Creating various load balancer objects with different choice configs.
  envoy::api::v2::Cluster::LeastRequestLbConfig lr_lb_config;
  lr_lb_config.mutable_choice_count()->set_value(2);
  LeastRequestLoadBalancer lb_2{priority_set_, nullptr,        stats_,      runtime_,
                                random_,       common_config_, lr_lb_config};
  lr_lb_config.mutable_choice_count()->set_value(5);
  LeastRequestLoadBalancer lb_5{priority_set_, nullptr,        stats_,      runtime_,
                                random_,       common_config_, lr_lb_config};

  // Verify correct number of choices.

  // 0 choices configured should default to P2C.
  EXPECT_CALL(random_, random()).Times(3).WillRepeatedly(Return(0));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));

  // 2 choices configured results in P2C.
  EXPECT_CALL(random_, random()).Times(3).WillRepeatedly(Return(0));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_2.chooseHost(nullptr));

  // 5 choices configured results in P5C.
  EXPECT_CALL(random_, random()).Times(6).WillRepeatedly(Return(0));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_5.chooseHost(nullptr));

  // Verify correct host chosen in P5C scenario.
  EXPECT_CALL(random_, random())
      .Times(6)
      .WillOnce(Return(0))
      .WillOnce(Return(3))
      .WillOnce(Return(0))
      .WillOnce(Return(3))
      .WillOnce(Return(2))
      .WillOnce(Return(1));
  EXPECT_EQ(hostSet().healthy_hosts_[3], lb_5.chooseHost(nullptr));
}

TEST_P(LeastRequestLoadBalancerTest, WeightImbalance) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80", 1),
                              makeTestHost(info_, "tcp://127.0.0.1:81", 2)};
  stats_.max_host_weight_.set(2UL);

  hostSet().hosts_ = hostSet().healthy_hosts_;
  hostSet().runCallbacks({}, {}); // Trigger callbacks. The added/removed lists are not relevant.

  EXPECT_CALL(random_, random()).WillRepeatedly(Return(0));

  // We should see 2:1 ratio for hosts[1] to hosts[0].
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));

  // Bringing hosts[1] to an active request should yield a 1:1 ratio.
  hostSet().healthy_hosts_[1]->stats().rq_active_.set(1);
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));

  // Settings hosts[0] to an active request and hosts[1] to no active requests should yield a 4:1
  // ratio.
  hostSet().healthy_hosts_[0]->stats().rq_active_.set(1);
  hostSet().healthy_hosts_[1]->stats().rq_active_.set(0);
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
}

TEST_P(LeastRequestLoadBalancerTest, WeightImbalanceCallbacks) {
  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80", 1),
                              makeTestHost(info_, "tcp://127.0.0.1:81", 2)};
  stats_.max_host_weight_.set(2UL);

  hostSet().hosts_ = hostSet().healthy_hosts_;
  hostSet().runCallbacks({}, {}); // Trigger callbacks. The added/removed lists are not relevant.

  EXPECT_CALL(random_, random()).WillRepeatedly(Return(0));

  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_.chooseHost(nullptr));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));

  // Remove and verify we get other host.
  HostVector empty;
  HostVector hosts_removed;
  hosts_removed.push_back(hostSet().hosts_[1]);
  hostSet().hosts_.erase(hostSet().hosts_.begin() + 1);
  hostSet().healthy_hosts_.erase(hostSet().healthy_hosts_.begin() + 1);
  hostSet().runCallbacks(empty, hosts_removed);
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_.chooseHost(nullptr));
}

INSTANTIATE_TEST_SUITE_P(PrimaryOrFailover, LeastRequestLoadBalancerTest,
                         ::testing::Values(true, false));

class RandomLoadBalancerTest : public LoadBalancerTestBase {
public:
  void init() {
    lb_.reset(
        new RandomLoadBalancer(priority_set_, nullptr, stats_, runtime_, random_, common_config_));
  }
  std::shared_ptr<LoadBalancer> lb_;
};

TEST_P(RandomLoadBalancerTest, NoHosts) {
  init();
  EXPECT_EQ(nullptr, lb_->chooseHost(nullptr));
}

TEST_P(RandomLoadBalancerTest, Normal) {
  init();

  hostSet().healthy_hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                              makeTestHost(info_, "tcp://127.0.0.1:81")};
  hostSet().hosts_ = hostSet().healthy_hosts_;
  hostSet().runCallbacks({}, {}); // Trigger callbacks. The added/removed lists are not relevant.
  EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(2));
  EXPECT_EQ(hostSet().healthy_hosts_[0], lb_->chooseHost(nullptr));
  EXPECT_CALL(random_, random()).WillOnce(Return(0)).WillOnce(Return(3));
  EXPECT_EQ(hostSet().healthy_hosts_[1], lb_->chooseHost(nullptr));
}

TEST_P(RandomLoadBalancerTest, FailClusterOnPanic) {
  common_config_.mutable_zone_aware_lb_config()->set_fail_traffic_on_panic(true);
  init();

  hostSet().healthy_hosts_ = {};
  hostSet().hosts_ = {makeTestHost(info_, "tcp://127.0.0.1:80"),
                      makeTestHost(info_, "tcp://127.0.0.1:81")};
  hostSet().runCallbacks({}, {}); // Trigger callbacks. The added/removed lists are not relevant.
  EXPECT_EQ(nullptr, lb_->chooseHost(nullptr));
}

INSTANTIATE_TEST_SUITE_P(PrimaryOrFailover, RandomLoadBalancerTest, ::testing::Values(true, false));

TEST(LoadBalancerSubsetInfoImplTest, DefaultConfigIsDiabled) {
  auto subset_info =
      LoadBalancerSubsetInfoImpl(envoy::api::v2::Cluster::LbSubsetConfig::default_instance());

  EXPECT_FALSE(subset_info.isEnabled());
  EXPECT_TRUE(subset_info.fallbackPolicy() == envoy::api::v2::Cluster::LbSubsetConfig::NO_FALLBACK);
  EXPECT_EQ(subset_info.defaultSubset().fields_size(), 0);
  EXPECT_EQ(subset_info.subsetSelectors().size(), 0);
}

TEST(LoadBalancerSubsetInfoImplTest, SubsetConfig) {
  auto subset_value = ProtobufWkt::Value();
  subset_value.set_string_value("the value");

  auto subset_config = envoy::api::v2::Cluster::LbSubsetConfig::default_instance();
  subset_config.set_fallback_policy(envoy::api::v2::Cluster::LbSubsetConfig::DEFAULT_SUBSET);
  subset_config.mutable_default_subset()->mutable_fields()->insert({"key", subset_value});
  auto subset_selector1 = subset_config.mutable_subset_selectors()->Add();
  subset_selector1->add_keys("selector_key1");
  auto subset_selector2 = subset_config.mutable_subset_selectors()->Add();
  subset_selector2->add_keys("selector_key2");
  subset_selector2->set_fallback_policy(
      envoy::api::v2::Cluster::LbSubsetConfig::LbSubsetSelector::ANY_ENDPOINT);

  auto subset_info = LoadBalancerSubsetInfoImpl(subset_config);

  EXPECT_TRUE(subset_info.isEnabled());
  EXPECT_TRUE(subset_info.fallbackPolicy() ==
              envoy::api::v2::Cluster::LbSubsetConfig::DEFAULT_SUBSET);
  EXPECT_EQ(subset_info.defaultSubset().fields_size(), 1);
  EXPECT_EQ(subset_info.defaultSubset().fields().at("key").string_value(),
            std::string("the value"));
  EXPECT_EQ(subset_info.subsetSelectors().size(), 2);
  EXPECT_EQ(subset_info.subsetSelectors()[0]->selector_keys_,
            std::set<std::string>({"selector_key1"}));
  EXPECT_EQ(subset_info.subsetSelectors()[0]->fallback_policy_,
            envoy::api::v2::Cluster::LbSubsetConfig::LbSubsetSelector::NOT_DEFINED);
  EXPECT_EQ(subset_info.subsetSelectors()[1]->selector_keys_,
            std::set<std::string>({"selector_key2"}));
  EXPECT_EQ(subset_info.subsetSelectors()[1]->fallback_policy_,
            envoy::api::v2::Cluster::LbSubsetConfig::LbSubsetSelector::ANY_ENDPOINT);
}

} // namespace
} // namespace Upstream
} // namespace Envoy
