#include "envoy/admin/v3/clusters.pb.h"

#include "test/server/admin/admin_instance.h"

using testing::Return;
using testing::ReturnPointee;
using testing::ReturnRef;

namespace Envoy {
namespace Server {

INSTANTIATE_TEST_SUITE_P(IpVersions, AdminInstanceTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(AdminInstanceTest, ClustersJson) {
  Upstream::ClusterManager::ClusterInfoMap cluster_map;
  ON_CALL(server_.cluster_manager_, clusters()).WillByDefault(ReturnPointee(&cluster_map));

  NiceMock<Upstream::MockClusterMockPrioritySet> cluster;
  cluster_map.emplace(cluster.info_->name_, cluster);

  NiceMock<Upstream::Outlier::MockDetector> outlier_detector;
  ON_CALL(Const(cluster), outlierDetector()).WillByDefault(Return(&outlier_detector));
  ON_CALL(outlier_detector,
          successRateEjectionThreshold(
              Upstream::Outlier::DetectorHostMonitor::SuccessRateMonitorType::ExternalOrigin))
      .WillByDefault(Return(6.0));
  ON_CALL(outlier_detector,
          successRateEjectionThreshold(
              Upstream::Outlier::DetectorHostMonitor::SuccessRateMonitorType::LocalOrigin))
      .WillByDefault(Return(9.0));

  ON_CALL(*cluster.info_, addedViaApi()).WillByDefault(Return(true));

  Upstream::MockHostSet* host_set = cluster.priority_set_.getMockHostSet(0);
  auto host = std::make_shared<NiceMock<Upstream::MockHost>>();

  envoy::config::core::v3::Locality locality;
  locality.set_region("test_region");
  locality.set_zone("test_zone");
  locality.set_sub_zone("test_sub_zone");
  ON_CALL(*host, locality()).WillByDefault(ReturnRef(locality));

  host_set->hosts_.emplace_back(host);
  Network::Address::InstanceConstSharedPtr address =
      Network::Utility::resolveUrl("tcp://1.2.3.4:80");
  ON_CALL(*host, address()).WillByDefault(Return(address));
  const std::string hostname = "foo.com";
  ON_CALL(*host, hostname()).WillByDefault(ReturnRef(hostname));

  // Add stats in random order and validate that they come in order.
  Stats::PrimitiveCounter test_counter;
  test_counter.add(10);
  Stats::PrimitiveCounter rest_counter;
  rest_counter.add(10);
  Stats::PrimitiveCounter arest_counter;
  arest_counter.add(5);
  std::vector<std::pair<absl::string_view, Stats::PrimitiveCounterReference>> counters = {
      {"arest_counter", arest_counter},
      {"rest_counter", rest_counter},
      {"test_counter", test_counter},
  };
  Stats::PrimitiveGauge test_gauge;
  test_gauge.set(11);
  Stats::PrimitiveGauge atest_gauge;
  atest_gauge.set(10);
  std::vector<std::pair<absl::string_view, Stats::PrimitiveGaugeReference>> gauges = {
      {"atest_gauge", atest_gauge},
      {"test_gauge", test_gauge},
  };
  ON_CALL(*host, counters()).WillByDefault(Invoke([&counters]() { return counters; }));
  ON_CALL(*host, gauges()).WillByDefault(Invoke([&gauges]() { return gauges; }));

  ON_CALL(*host, healthFlagGet(Upstream::Host::HealthFlag::FAILED_ACTIVE_HC))
      .WillByDefault(Return(true));
  ON_CALL(*host, healthFlagGet(Upstream::Host::HealthFlag::FAILED_OUTLIER_CHECK))
      .WillByDefault(Return(true));
  ON_CALL(*host, healthFlagGet(Upstream::Host::HealthFlag::FAILED_EDS_HEALTH))
      .WillByDefault(Return(false));
  ON_CALL(*host, healthFlagGet(Upstream::Host::HealthFlag::DEGRADED_ACTIVE_HC))
      .WillByDefault(Return(true));
  ON_CALL(*host, healthFlagGet(Upstream::Host::HealthFlag::DEGRADED_EDS_HEALTH))
      .WillByDefault(Return(true));
  ON_CALL(*host, healthFlagGet(Upstream::Host::HealthFlag::PENDING_DYNAMIC_REMOVAL))
      .WillByDefault(Return(true));

  ON_CALL(
      host->outlier_detector_,
      successRate(Upstream::Outlier::DetectorHostMonitor::SuccessRateMonitorType::ExternalOrigin))
      .WillByDefault(Return(43.2));
  ON_CALL(*host, weight()).WillByDefault(Return(5));
  ON_CALL(host->outlier_detector_,
          successRate(Upstream::Outlier::DetectorHostMonitor::SuccessRateMonitorType::LocalOrigin))
      .WillByDefault(Return(93.2));
  ON_CALL(*host, priority()).WillByDefault(Return(6));

  Buffer::OwnedImpl response;
  Http::TestResponseHeaderMapImpl header_map;
  EXPECT_EQ(Http::Code::OK, getCallback("/clusters?format=json", header_map, response));
  std::string output_json = response.toString();
  envoy::admin::v3::Clusters output_proto;
  TestUtility::loadFromJson(output_json, output_proto);

  const std::string expected_json = R"EOF({
 "cluster_statuses": [
  {
   "name": "fake_cluster",
   "success_rate_ejection_threshold": {
    "value": 6
   },
   "local_origin_success_rate_ejection_threshold": {
    "value": 9
   },
   "added_via_api": true,
   "circuit_breakers": {
    "thresholds": [
     {
      "max_connections": 1,
      "max_pending_requests": 1024,
      "max_requests": 1024,
      "max_retries": 1
     },
     {
      "priority": "HIGH",
      "max_connections": 1,
      "max_pending_requests": 1024,
      "max_requests": 1024,
      "max_retries": 1
     }
    ]
   },
   "host_statuses": [
    {
     "address": {
      "socket_address": {
       "protocol": "TCP",
       "address": "1.2.3.4",
       "port_value": 80
      }
     },
     "stats": [
       {
       "name": "arest_counter",
       "value": "5",
       "type": "COUNTER"
       },
       {
       "name": "rest_counter",
       "value": "10",
       "type": "COUNTER"
      },
      {
       "name": "test_counter",
       "value": "10",
       "type": "COUNTER"
      },
      {
       "name": "atest_gauge",
       "value": "10",
       "type": "GAUGE"
      },
      {
       "name": "test_gauge",
       "value": "11",
       "type": "GAUGE"
      }
     ],
     "health_status": {
      "eds_health_status": "DEGRADED",
      "failed_active_health_check": true,
      "failed_outlier_check": true,
      "failed_active_degraded_check": true,
      "pending_dynamic_removal": true
     },
     "success_rate": {
      "value": 43.2
     },
     "weight": 5,
     "hostname": "foo.com",
     "priority": 6,
     "local_origin_success_rate": {
      "value": 93.2
     },
     "locality": {
       "region": "test_region",
       "zone": "test_zone",
       "sub_zone": "test_sub_zone"
     }
    }
   ]
  }
 ]
}
)EOF";

  envoy::admin::v3::Clusters expected_proto;
  TestUtility::loadFromJson(expected_json, expected_proto);

  // Ensure the protos created from each JSON are equivalent.
  EXPECT_THAT(output_proto, ProtoEq(expected_proto));

  // Ensure that the normal text format is used by default.
  Buffer::OwnedImpl response2;
  EXPECT_EQ(Http::Code::OK, getCallback("/clusters", header_map, response2));
  const std::string expected_text = R"EOF(fake_cluster::outlier::success_rate_average::0
fake_cluster::outlier::success_rate_ejection_threshold::6
fake_cluster::outlier::local_origin_success_rate_average::0
fake_cluster::outlier::local_origin_success_rate_ejection_threshold::9
fake_cluster::default_priority::max_connections::1
fake_cluster::default_priority::max_pending_requests::1024
fake_cluster::default_priority::max_requests::1024
fake_cluster::default_priority::max_retries::1
fake_cluster::high_priority::max_connections::1
fake_cluster::high_priority::max_pending_requests::1024
fake_cluster::high_priority::max_requests::1024
fake_cluster::high_priority::max_retries::1
fake_cluster::added_via_api::true
fake_cluster::1.2.3.4:80::arest_counter::5
fake_cluster::1.2.3.4:80::atest_gauge::10
fake_cluster::1.2.3.4:80::rest_counter::10
fake_cluster::1.2.3.4:80::test_counter::10
fake_cluster::1.2.3.4:80::test_gauge::11
fake_cluster::1.2.3.4:80::hostname::foo.com
fake_cluster::1.2.3.4:80::health_flags::/failed_active_hc/failed_outlier_check/degraded_active_hc/degraded_eds_health/pending_dynamic_removal
fake_cluster::1.2.3.4:80::weight::5
fake_cluster::1.2.3.4:80::region::test_region
fake_cluster::1.2.3.4:80::zone::test_zone
fake_cluster::1.2.3.4:80::sub_zone::test_sub_zone
fake_cluster::1.2.3.4:80::canary::false
fake_cluster::1.2.3.4:80::priority::6
fake_cluster::1.2.3.4:80::success_rate::43.2
fake_cluster::1.2.3.4:80::local_origin_success_rate::93.2
)EOF";
  EXPECT_EQ(expected_text, response2.toString());
}

} // namespace Server
} // namespace Envoy
