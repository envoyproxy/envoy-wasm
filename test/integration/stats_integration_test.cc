#include <memory>

#include "envoy/config/bootstrap/v2/bootstrap.pb.h"
#include "envoy/config/metrics/v2/stats.pb.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats.h"

#include "common/config/well_known_names.h"
#include "common/memory/stats.h"
#include "common/stats/symbol_table_creator.h"

#include "test/common/stats/stat_test_utility.h"
#include "test/config/utility.h"
#include "test/integration/integration.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace {

class StatsIntegrationTest : public testing::TestWithParam<Network::Address::IpVersion>,
                             public BaseIntegrationTest {
public:
  StatsIntegrationTest() : BaseIntegrationTest(GetParam()) {}

  void TearDown() override {
    test_server_.reset();
    fake_upstreams_.clear();
  }

  void initialize() override { BaseIntegrationTest::initialize(); }
};

INSTANTIATE_TEST_SUITE_P(IpVersions, StatsIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(StatsIntegrationTest, WithDefaultConfig) {
  initialize();

  auto live = test_server_->gauge("server.live");
  EXPECT_EQ(live->value(), 1);
  EXPECT_EQ(live->tags().size(), 0);

  auto counter = test_server_->counter("http.config_test.rq_total");
  EXPECT_EQ(counter->tags().size(), 1);
  EXPECT_EQ(counter->tags()[0].name_, "envoy.http_conn_manager_prefix");
  EXPECT_EQ(counter->tags()[0].value_, "config_test");
}

TEST_P(StatsIntegrationTest, WithoutDefaultTagExtractors) {
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    bootstrap.mutable_stats_config()->mutable_use_all_default_tags()->set_value(false);
  });
  initialize();

  auto counter = test_server_->counter("http.config_test.rq_total");
  EXPECT_EQ(counter->tags().size(), 0);
}

TEST_P(StatsIntegrationTest, WithDefaultTagExtractors) {
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    bootstrap.mutable_stats_config()->mutable_use_all_default_tags()->set_value(true);
  });
  initialize();

  auto counter = test_server_->counter("http.config_test.rq_total");
  EXPECT_EQ(counter->tags().size(), 1);
  EXPECT_EQ(counter->tags()[0].name_, "envoy.http_conn_manager_prefix");
  EXPECT_EQ(counter->tags()[0].value_, "config_test");
}

// Given: a. use_all_default_tags = false, b. a tag specifier has the same name
// as a default tag extractor name but also has use defined regex.
// In this case we don't use default tag extractors (since use_all_default_tags
// is set to false explicitly) and just treat the tag specifier as a normal tag
// specifier having use defined regex.
TEST_P(StatsIntegrationTest, WithDefaultTagExtractorNameWithUserDefinedRegex) {
  std::string tag_name = Config::TagNames::get().HTTP_CONN_MANAGER_PREFIX;
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    bootstrap.mutable_stats_config()->mutable_use_all_default_tags()->set_value(false);
    auto tag_specifier = bootstrap.mutable_stats_config()->mutable_stats_tags()->Add();
    tag_specifier->set_tag_name(tag_name);
    tag_specifier->set_regex("((.*))");
  });
  initialize();

  auto counter = test_server_->counter("http.config_test.rq_total");
  EXPECT_EQ(counter->tags().size(), 1);
  EXPECT_EQ(counter->tags()[0].name_, tag_name);
  EXPECT_EQ(counter->tags()[0].value_, "http.config_test.rq_total");
}

TEST_P(StatsIntegrationTest, WithTagSpecifierMissingTagValue) {
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    bootstrap.mutable_stats_config()->mutable_use_all_default_tags()->set_value(false);
    auto tag_specifier = bootstrap.mutable_stats_config()->mutable_stats_tags()->Add();
    tag_specifier->set_tag_name("envoy.http_conn_manager_prefix");
  });
  initialize();

  auto counter = test_server_->counter("http.config_test.rq_total");
  EXPECT_EQ(counter->tags().size(), 1);
  EXPECT_EQ(counter->tags()[0].name_, "envoy.http_conn_manager_prefix");
  EXPECT_EQ(counter->tags()[0].value_, "config_test");
}

TEST_P(StatsIntegrationTest, WithTagSpecifierWithRegex) {
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    bootstrap.mutable_stats_config()->mutable_use_all_default_tags()->set_value(false);
    auto tag_specifier = bootstrap.mutable_stats_config()->mutable_stats_tags()->Add();
    tag_specifier->set_tag_name("my.http_conn_manager_prefix");
    tag_specifier->set_regex(R"(^(?:|listener(?=\.).*?\.)http\.((.*?)\.))");
  });
  initialize();

  auto counter = test_server_->counter("http.config_test.rq_total");
  EXPECT_EQ(counter->tags().size(), 1);
  EXPECT_EQ(counter->tags()[0].name_, "my.http_conn_manager_prefix");
  EXPECT_EQ(counter->tags()[0].value_, "config_test");
}

TEST_P(StatsIntegrationTest, WithTagSpecifierWithFixedValue) {
  config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
    auto tag_specifier = bootstrap.mutable_stats_config()->mutable_stats_tags()->Add();
    tag_specifier->set_tag_name("test.x");
    tag_specifier->set_fixed_value("xxx");
  });
  initialize();

  auto live = test_server_->gauge("server.live");
  EXPECT_EQ(live->value(), 1);
  EXPECT_EQ(live->tags().size(), 1);
  EXPECT_EQ(live->tags()[0].name_, "test.x");
  EXPECT_EQ(live->tags()[0].value_, "xxx");
}

// TODO(cmluciano) Refactor once https://github.com/envoyproxy/envoy/issues/5624 is solved
// TODO(cmluciano) Add options to measure multiple workers & without stats
// This class itself does not add additional tests. It is a helper for use in other tests measuring
// cluster overhead.
class ClusterMemoryTestHelper : public BaseIntegrationTest {
public:
  ClusterMemoryTestHelper()
      : BaseIntegrationTest(testing::TestWithParam<Network::Address::IpVersion>::GetParam()) {}

  static size_t computeMemoryDelta(int initial_num_clusters, int initial_num_hosts,
                                   int final_num_clusters, int final_num_hosts, bool allow_stats) {
    // Use the same number of fake upstreams for both helpers in order to exclude memory overhead
    // added by the fake upstreams.
    int fake_upstreams_count = 1 + final_num_clusters * final_num_hosts;

    size_t initial_memory;
    {
      ClusterMemoryTestHelper helper;
      helper.setUpstreamCount(fake_upstreams_count);
      helper.skipPortUsageValidation();
      initial_memory =
          helper.clusterMemoryHelper(initial_num_clusters, initial_num_hosts, allow_stats);
    }

    ClusterMemoryTestHelper helper;
    helper.setUpstreamCount(fake_upstreams_count);
    return helper.clusterMemoryHelper(final_num_clusters, final_num_hosts, allow_stats) -
           initial_memory;
  }

private:
  /**
   * @param num_clusters number of clusters appended to bootstrap_config
   * @param allow_stats if false, enable set_reject_all in stats_config
   * @return size_t the total memory allocated
   */
  size_t clusterMemoryHelper(int num_clusters, int num_hosts, bool allow_stats) {
    Stats::TestUtil::MemoryTest memory_test;
    config_helper_.addConfigModifier([&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
      if (!allow_stats) {
        bootstrap.mutable_stats_config()->mutable_stats_matcher()->set_reject_all(true);
      }
      for (int i = 1; i < num_clusters; ++i) {
        auto* cluster = bootstrap.mutable_static_resources()->add_clusters();
        cluster->set_name(fmt::format("cluster_{}", i));
      }

      for (int i = 0; i < num_clusters; ++i) {
        auto* cluster = bootstrap.mutable_static_resources()->mutable_clusters(i);
        for (int j = 0; j < num_hosts; ++j) {
          auto* host = cluster->add_hosts();
          auto* socket_address = host->mutable_socket_address();
          socket_address->set_protocol(envoy::api::v2::core::SocketAddress::TCP);
          socket_address->set_address("0.0.0.0");
          socket_address->set_port_value(80);
        }
      }
    });
    initialize();

    return memory_test.consumedBytes();
  }
};
class ClusterMemoryTestRunner : public testing::TestWithParam<Network::Address::IpVersion> {
protected:
  ClusterMemoryTestRunner() : save_use_fakes_(Stats::SymbolTableCreator::useFakeSymbolTables()) {}
  ~ClusterMemoryTestRunner() override {
    Stats::TestUtil::SymbolTableCreatorTestPeer::setUseFakeSymbolTables(save_use_fakes_);
  }

private:
  const bool save_use_fakes_;
};

INSTANTIATE_TEST_SUITE_P(IpVersions, ClusterMemoryTestRunner,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(ClusterMemoryTestRunner, MemoryLargeClusterSizeWithFakeSymbolTable) {
  Stats::TestUtil::SymbolTableCreatorTestPeer::setUseFakeSymbolTables(true);

  // A unique instance of ClusterMemoryTest allows for multiple runs of Envoy with
  // differing configuration. This is necessary for measuring the memory consumption
  // between the different instances within the same test.
  const size_t m1000 = ClusterMemoryTestHelper::computeMemoryDelta(1, 0, 1001, 0, true);
  const size_t m_per_cluster = (m1000) / 1000;

  // Note: if you are increasing this golden value because you are adding a
  // stat, please confirm that this will be generally useful to most Envoy
  // users. Otherwise you are adding to the per-cluster memory overhead, which
  // will be significant for Envoy installations that are massively
  // multi-tenant.
  //
  // History of golden values:
  //
  // Date        PR       Bytes Per Cluster   Notes
  //                      exact upper-bound
  // ----------  -----    -----------------   -----
  // 2019/03/20  6329     59015               Initial version
  // 2019/04/12  6477     59576               Implementing Endpoint lease...
  // 2019/04/23  6659     59512               Reintroduce dispatcher stats...
  // 2019/04/24  6161     49415               Pack tags and tag extracted names
  // 2019/05/07  6794     49957               Stats for excluded hosts in cluster
  // 2019/04/27  6733     50213               Use SymbolTable API for HTTP codes
  // 2019/05/31  6866     50157               libstdc++ upgrade in CI
  // 2019/06/03  7199     49393               absl update
  // 2019/06/06  7208     49650               make memory targets approximate
  // 2019/06/17  7243     49412       49700   macros for exact/upper-bound memory checks
  // 2019/06/29  7364     45685       46000   combine 2 levels of stat ref-counting into 1
  // 2019/06/30  7428     42742       43000   remove stats multiple inheritance, inline HeapStatData
  // 2019/07/06  7477     42742       43000   fork gauge representation to drop pending_increment_
  // 2019/07/15  7555     42806       43000   static link libstdc++ in tests
  // 2019/07/24  7503     43030       44000   add upstream filters to clusters
  // 2019/08/13  7877     42838       44000   skip EdfScheduler creation if all host weights equal
  // 2019/09/02  8118     42830       43000   Share symbol-tables in cluster/host stats.
  // 2019/09/16  8100     42894       43000   Add transport socket matcher in cluster.
  // 2019/09/25  8226     43022       44000   dns: enable dns failure refresh rate configuration
  // 2019/09/30  8354     43310       44000   Implement transport socket match.
  // 2019/10/17  8537     43308       44000   add new enum value HTTP3
  // 2019/10/17  8484     43340       44000   stats: add unit support to histogram

  // Note: when adjusting this value: EXPECT_MEMORY_EQ is active only in CI
  // 'release' builds, where we control the platform and tool-chain. So you
  // will need to find the correct value only after failing CI and looking
  // at the logs.
  //
  // On a local clang8/libstdc++/linux flow, the memory usage was observed in
  // June 2019 to be 64 bytes higher than it is in CI/release. Your mileage may
  // vary.
  //
  // If you encounter a failure here, please see
  // https://github.com/envoyproxy/envoy/blob/master/source/docs/stats.md#stats-memory-tests
  // for details on how to fix.
  EXPECT_MEMORY_EQ(m_per_cluster, 43340); // 104 bytes higher than a debug build.
  EXPECT_MEMORY_LE(m_per_cluster, 44000);
}

TEST_P(ClusterMemoryTestRunner, MemoryLargeClusterSizeWithRealSymbolTable) {
  Stats::TestUtil::SymbolTableCreatorTestPeer::setUseFakeSymbolTables(false);

  // A unique instance of ClusterMemoryTest allows for multiple runs of Envoy with
  // differing configuration. This is necessary for measuring the memory consumption
  // between the different instances within the same test.
  const size_t m1000 = ClusterMemoryTestHelper::computeMemoryDelta(1, 0, 1001, 0, true);
  const size_t m_per_cluster = (m1000) / 1000;

  // Note: if you are increasing this golden value because you are adding a
  // stat, please confirm that this will be generally useful to most Envoy
  // users. Otherwise you are adding to the per-cluster memory overhead, which
  // will be significant for Envoy installations that are massively
  // multi-tenant.
  //
  // History of golden values:
  //
  // Date        PR       Bytes Per Cluster   Notes
  //                      exact upper-bound
  // ----------  -----    -----------------   -----
  // 2019/08/09  7882     35489       36000   Initial version
  // 2019/09/02  8118     34585       34500   Share symbol-tables in cluster/host stats.
  // 2019/09/16  8100     34585       34500   Add transport socket matcher in cluster.
  // 2019/09/25  8226     34777       35000   dns: enable dns failure refresh rate configuration
  // 2019/09/30  8354     34969       35000   Implement transport socket match.
  // 2019/10/17  8537     34966       35000   add new enum value HTTP3
  // 2019/10/17  8484     34998       35000   stats: add unit support to histogram

  // Note: when adjusting this value: EXPECT_MEMORY_EQ is active only in CI
  // 'release' builds, where we control the platform and tool-chain. So you
  // will need to find the correct value only after failing CI and looking
  // at the logs.
  //
  // On a local clang8/libstdc++/linux flow, the memory usage was observed in
  // June 2019 to be 64 bytes higher than it is in CI/release. Your mileage may
  // vary.
  //
  // If you encounter a failure here, please see
  // https://github.com/envoyproxy/envoy/blob/master/source/docs/stats.md#stats-memory-tests
  // for details on how to fix.
  EXPECT_MEMORY_EQ(m_per_cluster, 34998); // 104 bytes higher than a debug build.
  EXPECT_MEMORY_LE(m_per_cluster, 36000);
}

TEST_P(ClusterMemoryTestRunner, MemoryLargeHostSizeWithStats) {
  Stats::TestUtil::SymbolTableCreatorTestPeer::setUseFakeSymbolTables(false);

  // A unique instance of ClusterMemoryTest allows for multiple runs of Envoy with
  // differing configuration. This is necessary for measuring the memory consumption
  // between the different instances within the same test.
  const size_t m1000 = ClusterMemoryTestHelper::computeMemoryDelta(1, 1, 1, 1001, true);
  const size_t m_per_host = (m1000) / 1000;

  // Note: if you are increasing this golden value because you are adding a
  // stat, please confirm that this will be generally useful to most Envoy
  // users. Otherwise you are adding to the per-host memory overhead, which
  // will be significant for Envoy installations configured to talk to large
  // numbers of hosts.
  //
  // History of golden values:
  //
  // Date        PR       Bytes Per Host      Notes
  //                      exact upper-bound
  // ----------  -----    -----------------   -----
  // 2019/09/09  8189     2739         3100   Initial per-host memory snapshot
  // 2019/09/10  8216     1283         1315   Use primitive counters for host stats

  // Note: when adjusting this value: EXPECT_MEMORY_EQ is active only in CI
  // 'release' builds, where we control the platform and tool-chain. So you
  // will need to find the correct value only after failing CI and looking
  // at the logs.
  //
  // If you encounter a failure here, please see
  // https://github.com/envoyproxy/envoy/blob/master/source/docs/stats.md#stats-memory-tests
  // for details on how to fix.
  EXPECT_MEMORY_EQ(m_per_host, 1283);
  EXPECT_MEMORY_LE(m_per_host, 1315);
}

} // namespace
} // namespace Envoy
