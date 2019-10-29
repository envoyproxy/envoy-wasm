#include <chrono>
#include <memory>
#include <string>

#include "envoy/admin/v2alpha/config_dump.pb.h"
#include "envoy/admin/v2alpha/config_dump.pb.validate.h"
#include "envoy/stats/scope.h"

#include "common/config/utility.h"
#include "common/json/json_loader.h"
#include "common/router/rds_impl.h"

#include "server/http/admin.h"

#include "test/mocks/init/mocks.h"
#include "test/mocks/local_info/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/printers.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::InSequence;
using testing::Invoke;
using testing::ReturnRef;

namespace Envoy {
namespace Router {
namespace {

envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager
parseHttpConnectionManagerFromYaml(const std::string& yaml_string) {
  envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager
      http_connection_manager;
  TestUtility::loadFromYaml(yaml_string, http_connection_manager);
  return http_connection_manager;
}

class RdsTestBase : public testing::Test {
public:
  RdsTestBase() {
    // For server_factory_context
    ON_CALL(mock_factory_context_, getServerFactoryContext())
        .WillByDefault(ReturnRef(server_factory_context_));
    ON_CALL(server_factory_context_, scope()).WillByDefault(ReturnRef(scope_));
    ON_CALL(mock_factory_context_, scope()).WillByDefault(ReturnRef(scope_));

    ON_CALL(mock_factory_context_, initManager()).WillByDefault(ReturnRef(outer_init_manager_));
    ON_CALL(outer_init_manager_, add(_)).WillByDefault(Invoke([this](const Init::Target& target) {
      init_target_handle_ = target.createHandle("test");
    }));
    ON_CALL(outer_init_manager_, initialize(_))
        .WillByDefault(Invoke(
            [this](const Init::Watcher& watcher) { init_target_handle_->initialize(watcher); }));
  }

  Event::SimulatedTimeSystem& timeSystem() { return time_system_; }

  Event::SimulatedTimeSystem time_system_;
  NiceMock<Server::Configuration::MockFactoryContext> mock_factory_context_;
  NiceMock<Init::MockManager> outer_init_manager_;
  NiceMock<Server::Configuration::MockServerFactoryContext> server_factory_context_;
  Init::ExpectableWatcherImpl init_watcher_;
  Init::TargetHandlePtr init_target_handle_;
  Envoy::Config::SubscriptionCallbacks* rds_callbacks_{};
  NiceMock<Stats::MockIsolatedStatsStore> scope_;
};

class RdsImplTest : public RdsTestBase {
public:
  RdsImplTest() {
    EXPECT_CALL(server_factory_context_.admin_.config_tracker_, add_("routes", _));
    route_config_provider_manager_ =
        std::make_unique<RouteConfigProviderManagerImpl>(server_factory_context_.admin_);
  }
  ~RdsImplTest() override { server_factory_context_.thread_local_.shutdownThread(); }

  void setup() {
    const std::string config_yaml = R"EOF(
rds:
  config_source:
    api_config_source:
      api_type: REST
      cluster_names:
      - foo_cluster
      refresh_delay: 1s
  route_config_name: foo_route_config
codec_type: auto
stat_prefix: foo
http_filters:
- name: http_dynamo_filter
  config: {}
    )EOF";

    EXPECT_CALL(outer_init_manager_, add(_));
    rds_ = RouteConfigProviderUtil::create(parseHttpConnectionManagerFromYaml(config_yaml),
                                           mock_factory_context_, "foo.",
                                           *route_config_provider_manager_);
    rds_callbacks_ = server_factory_context_.cluster_manager_.subscription_factory_.callbacks_;
    EXPECT_CALL(*server_factory_context_.cluster_manager_.subscription_factory_.subscription_,
                start(_));
    outer_init_manager_.initialize(init_watcher_);
  }

  RouteConstSharedPtr route(Http::TestHeaderMapImpl headers) {
    NiceMock<Envoy::StreamInfo::MockStreamInfo> stream_info;
    headers.addCopy("x-forwarded-proto", "http");
    return rds_->config()->route(headers, stream_info, 0);
  }

  NiceMock<Server::MockInstance> server_;
  std::unique_ptr<RouteConfigProviderManagerImpl> route_config_provider_manager_;
  RouteConfigProviderPtr rds_;
};

TEST_F(RdsImplTest, RdsAndStatic) {
  const std::string config_yaml = R"EOF(
rds: {}
route_config: {}
codec_type: auto
stat_prefix: foo
http_filters:
- name: http_dynamo_filter
  config: {}
    )EOF";

  EXPECT_THROW(RouteConfigProviderUtil::create(parseHttpConnectionManagerFromYaml(config_yaml),
                                               mock_factory_context_, "foo.",
                                               *route_config_provider_manager_),
               EnvoyException);
}

TEST_F(RdsImplTest, DestroyDuringInitialize) {
  InSequence s;
  setup();
  // EXPECT_CALL(server_factory_context_, scope());
  EXPECT_CALL(init_watcher_, ready());
  rds_.reset();
}

TEST_F(RdsImplTest, Basic) {
  InSequence s;
  Buffer::OwnedImpl empty;
  Buffer::OwnedImpl data;

  setup();

  // Make sure the initial empty route table works.
  EXPECT_EQ(nullptr, route(Http::TestHeaderMapImpl{{":authority", "foo"}}));

  // Initial request.
  const std::string response1_json = R"EOF(
{
  "version_info": "1",
  "resources": [
    {
      "@type": "type.googleapis.com/envoy.api.v2.RouteConfiguration",
      "name": "foo_route_config",
      "virtual_hosts": null
    }
  ]
}
)EOF";
  auto response1 = TestUtility::parseYaml<envoy::api::v2::DiscoveryResponse>(response1_json);

  EXPECT_CALL(init_watcher_, ready());
  rds_callbacks_->onConfigUpdate(response1.resources(), response1.version_info());
  EXPECT_EQ(nullptr, route(Http::TestHeaderMapImpl{{":authority", "foo"}}));

  // 2nd request with same response. Based on hash should not reload config.
  rds_callbacks_->onConfigUpdate(response1.resources(), response1.version_info());
  EXPECT_EQ(nullptr, route(Http::TestHeaderMapImpl{{":authority", "foo"}}));

  // Load the config and verified shared count.
  ConfigConstSharedPtr config = rds_->config();
  EXPECT_EQ(2, config.use_count());

  // Third request.
  const std::string response2_json = R"EOF(
{
  "version_info": "2",
  "resources": [
    {
      "@type": "type.googleapis.com/envoy.api.v2.RouteConfiguration",
      "name": "foo_route_config",
      "virtual_hosts": [
        {
          "name": "integration",
          "domains": [
            "*"
          ],
          "routes": [
            {
              "match": {
                "prefix": "/foo"
              },
              "route": {
                "cluster_header": ":authority"
              }
            }
          ]
        }
      ]
    }
  ]
}
  )EOF";
  auto response2 = TestUtility::parseYaml<envoy::api::v2::DiscoveryResponse>(response2_json);

  // Make sure we don't lookup/verify clusters.
  EXPECT_CALL(mock_factory_context_.cluster_manager_, get(Eq("bar"))).Times(0);
  rds_callbacks_->onConfigUpdate(response2.resources(), response2.version_info());
  EXPECT_EQ("foo", route(Http::TestHeaderMapImpl{{":authority", "foo"}, {":path", "/foo"}})
                       ->routeEntry()
                       ->clusterName());

  // Old config use count should be 1 now.
  EXPECT_EQ(1, config.use_count());
  EXPECT_EQ(2UL, scope_.counter("foo.rds.foo_route_config.config_reload").value());
}

// Validate behavior when the config is delivered but it fails PGV validation.
TEST_F(RdsImplTest, FailureInvalidConfig) {
  InSequence s;

  setup();

  const std::string response1_json = R"EOF(
{
  "version_info": "1",
  "resources": [
    {
      "@type": "type.googleapis.com/envoy.api.v2.RouteConfiguration",
      "name": "INVALID_NAME_FOR_route_config",
      "virtual_hosts": null
    }
  ]
}
)EOF";
  auto response1 = TestUtility::parseYaml<envoy::api::v2::DiscoveryResponse>(response1_json);

  EXPECT_CALL(init_watcher_, ready());
  EXPECT_THROW_WITH_MESSAGE(
      rds_callbacks_->onConfigUpdate(response1.resources(), response1.version_info()),
      EnvoyException,
      "Unexpected RDS configuration (expecting foo_route_config): INVALID_NAME_FOR_route_config");
}

// Validate behavior when the config fails delivery at the subscription level.
TEST_F(RdsImplTest, FailureSubscription) {
  InSequence s;

  setup();

  EXPECT_CALL(init_watcher_, ready());
  // onConfigUpdateFailed() should not be called for gRPC stream connection failure
  rds_callbacks_->onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason::FetchTimedout, {});
}

class RouteConfigProviderManagerImplTest : public RdsTestBase {
public:
  void setup() {
    // Get a RouteConfigProvider. This one should create an entry in the RouteConfigProviderManager.
    rds_.set_route_config_name("foo_route_config");
    rds_.mutable_config_source()->set_path("foo_path");
    provider_ = route_config_provider_manager_->createRdsRouteConfigProvider(
        rds_, mock_factory_context_, "foo_prefix.", outer_init_manager_);
    rds_callbacks_ = server_factory_context_.cluster_manager_.subscription_factory_.callbacks_;
  }

  RouteConfigProviderManagerImplTest() {
    EXPECT_CALL(server_factory_context_.admin_.config_tracker_, add_("routes", _));
    route_config_provider_manager_ =
        std::make_unique<RouteConfigProviderManagerImpl>(server_factory_context_.admin_);
  }

  ~RouteConfigProviderManagerImplTest() override {
    server_factory_context_.thread_local_.shutdownThread();
  }

  envoy::config::filter::network::http_connection_manager::v2::Rds rds_;
  std::unique_ptr<RouteConfigProviderManagerImpl> route_config_provider_manager_;
  RouteConfigProviderPtr provider_;
};

envoy::api::v2::RouteConfiguration parseRouteConfigurationFromV2Yaml(const std::string& yaml) {
  envoy::api::v2::RouteConfiguration route_config;
  TestUtility::loadFromYaml(yaml, route_config);
  return route_config;
}

TEST_F(RouteConfigProviderManagerImplTest, ConfigDump) {
  auto message_ptr =
      server_factory_context_.admin_.config_tracker_.config_tracker_callbacks_["routes"]();
  const auto& route_config_dump =
      TestUtility::downcastAndValidate<const envoy::admin::v2alpha::RoutesConfigDump&>(
          *message_ptr);

  // No routes at all, no last_updated timestamp
  envoy::admin::v2alpha::RoutesConfigDump expected_route_config_dump;
  TestUtility::loadFromYaml(R"EOF(
static_route_configs:
dynamic_route_configs:
)EOF",
                            expected_route_config_dump);
  EXPECT_EQ(expected_route_config_dump.DebugString(), route_config_dump.DebugString());

  const std::string config_yaml = R"EOF(
name: foo
virtual_hosts:
  - name: bar
    domains: ["*"]
    routes:
      - match: { prefix: "/" }
        route: { cluster: baz }
)EOF";

  timeSystem().setSystemTime(std::chrono::milliseconds(1234567891234));

  // Only static route.
  RouteConfigProviderPtr static_config =
      route_config_provider_manager_->createStaticRouteConfigProvider(
          parseRouteConfigurationFromV2Yaml(config_yaml), mock_factory_context_);
  message_ptr =
      server_factory_context_.admin_.config_tracker_.config_tracker_callbacks_["routes"]();
  const auto& route_config_dump2 =
      TestUtility::downcastAndValidate<const envoy::admin::v2alpha::RoutesConfigDump&>(
          *message_ptr);
  TestUtility::loadFromYaml(R"EOF(
static_route_configs:
  - route_config:
      name: foo
      virtual_hosts:
        - name: bar
          domains: ["*"]
          routes:
            - match: { prefix: "/" }
              route: { cluster: baz }
    last_updated:
      seconds: 1234567891
      nanos: 234000000
dynamic_route_configs:
)EOF",
                            expected_route_config_dump);
  EXPECT_EQ(expected_route_config_dump.DebugString(), route_config_dump2.DebugString());

  // Static + dynamic.
  setup();
  EXPECT_CALL(*server_factory_context_.cluster_manager_.subscription_factory_.subscription_,
              start(_));
  outer_init_manager_.initialize(init_watcher_);

  const std::string response1_json = R"EOF(
{
  "version_info": "1",
  "resources": [
    {
      "@type": "type.googleapis.com/envoy.api.v2.RouteConfiguration",
      "name": "foo_route_config",
      "virtual_hosts": null
    }
  ]
}
)EOF";
  auto response1 = TestUtility::parseYaml<envoy::api::v2::DiscoveryResponse>(response1_json);

  EXPECT_CALL(init_watcher_, ready());
  rds_callbacks_->onConfigUpdate(response1.resources(), response1.version_info());
  message_ptr =
      server_factory_context_.admin_.config_tracker_.config_tracker_callbacks_["routes"]();
  const auto& route_config_dump3 =
      TestUtility::downcastAndValidate<const envoy::admin::v2alpha::RoutesConfigDump&>(
          *message_ptr);
  TestUtility::loadFromYaml(R"EOF(
static_route_configs:
  - route_config:
      name: foo
      virtual_hosts:
        - name: bar
          domains: ["*"]
          routes:
            - match: { prefix: "/" }
              route: { cluster: baz }
    last_updated:
      seconds: 1234567891
      nanos: 234000000
dynamic_route_configs:
  - version_info: "1"
    route_config:
      name: foo_route_config
      virtual_hosts:
    last_updated:
      seconds: 1234567891
      nanos: 234000000
)EOF",
                            expected_route_config_dump);
  EXPECT_EQ(expected_route_config_dump.DebugString(), route_config_dump3.DebugString());
}

TEST_F(RouteConfigProviderManagerImplTest, Basic) {
  Buffer::OwnedImpl data;

  // Get a RouteConfigProvider. This one should create an entry in the RouteConfigProviderManager.
  setup();

  EXPECT_FALSE(provider_->configInfo().has_value());

  Protobuf::RepeatedPtrField<ProtobufWkt::Any> route_configs;
  route_configs.Add()->PackFrom(parseRouteConfigurationFromV2Yaml(R"EOF(
name: foo_route_config
virtual_hosts:
  - name: bar
    domains: ["*"]
    routes:
      - match: { prefix: "/" }
        route: { cluster: baz }
)EOF"));

  server_factory_context_.cluster_manager_.subscription_factory_.callbacks_->onConfigUpdate(
      route_configs, "1");

  RouteConfigProviderPtr provider2 = route_config_provider_manager_->createRdsRouteConfigProvider(
      rds_, mock_factory_context_, "foo_prefix", outer_init_manager_);

  // provider2 should have route config immediately after create
  EXPECT_TRUE(provider2->configInfo().has_value());

  // So this means that both provider have same subscription.
  EXPECT_EQ(&dynamic_cast<RdsRouteConfigProviderImpl&>(*provider_).subscription(),
            &dynamic_cast<RdsRouteConfigProviderImpl&>(*provider2).subscription());
  EXPECT_EQ(&provider_->configInfo().value().config_, &provider2->configInfo().value().config_);

  envoy::config::filter::network::http_connection_manager::v2::Rds rds2;
  rds2.set_route_config_name("foo_route_config");
  rds2.mutable_config_source()->set_path("bar_path");
  RouteConfigProviderPtr provider3 = route_config_provider_manager_->createRdsRouteConfigProvider(
      rds2, mock_factory_context_, "foo_prefix", mock_factory_context_.initManager());
  EXPECT_NE(provider3, provider_);
  server_factory_context_.cluster_manager_.subscription_factory_.callbacks_->onConfigUpdate(
      route_configs, "provider3");
  EXPECT_EQ(2UL,
            route_config_provider_manager_->dumpRouteConfigs()->dynamic_route_configs().size());

  provider_.reset();
  provider2.reset();

  // All shared_ptrs to the provider pointed at by provider1, and provider2 have been deleted, so
  // now we should only have the provider pointed at by provider3.
  auto dynamic_route_configs =
      route_config_provider_manager_->dumpRouteConfigs()->dynamic_route_configs();
  EXPECT_EQ(1UL, dynamic_route_configs.size());

  // Make sure the left one is provider3
  EXPECT_EQ("provider3", dynamic_route_configs[0].version_info());

  provider3.reset();

  EXPECT_EQ(0UL,
            route_config_provider_manager_->dumpRouteConfigs()->dynamic_route_configs().size());
}

// Negative test for protoc-gen-validate constraints.
TEST_F(RouteConfigProviderManagerImplTest, ValidateFail) {
  setup();
  Protobuf::RepeatedPtrField<ProtobufWkt::Any> route_configs;
  envoy::api::v2::RouteConfiguration route_config;
  route_config.set_name("foo_route_config");
  route_config.mutable_virtual_hosts()->Add();
  route_configs.Add()->PackFrom(route_config);
  EXPECT_THROW(
      server_factory_context_.cluster_manager_.subscription_factory_.callbacks_->onConfigUpdate(
          route_configs, ""),
      ProtoValidationException);
}

TEST_F(RouteConfigProviderManagerImplTest, onConfigUpdateEmpty) {
  setup();
  EXPECT_CALL(*server_factory_context_.cluster_manager_.subscription_factory_.subscription_,
              start(_));
  outer_init_manager_.initialize(init_watcher_);
  EXPECT_CALL(init_watcher_, ready());
  server_factory_context_.cluster_manager_.subscription_factory_.callbacks_->onConfigUpdate({}, "");
}

TEST_F(RouteConfigProviderManagerImplTest, onConfigUpdateWrongSize) {
  setup();
  EXPECT_CALL(*server_factory_context_.cluster_manager_.subscription_factory_.subscription_,
              start(_));
  outer_init_manager_.initialize(init_watcher_);
  Protobuf::RepeatedPtrField<ProtobufWkt::Any> route_configs;
  route_configs.Add();
  route_configs.Add();
  EXPECT_CALL(init_watcher_, ready());
  EXPECT_THROW_WITH_MESSAGE(
      server_factory_context_.cluster_manager_.subscription_factory_.callbacks_->onConfigUpdate(
          route_configs, ""),
      EnvoyException, "Unexpected RDS resource length: 2");
}

// Regression test for https://github.com/envoyproxy/envoy/issues/7939
TEST_F(RouteConfigProviderManagerImplTest, ConfigDumpAfterConfigRejected) {
  auto message_ptr =
      server_factory_context_.admin_.config_tracker_.config_tracker_callbacks_["routes"]();
  const auto& route_config_dump =
      TestUtility::downcastAndValidate<const envoy::admin::v2alpha::RoutesConfigDump&>(
          *message_ptr);

  // No routes at all, no last_updated timestamp
  envoy::admin::v2alpha::RoutesConfigDump expected_route_config_dump;
  TestUtility::loadFromYaml(R"EOF(
static_route_configs:
dynamic_route_configs:
)EOF",
                            expected_route_config_dump);
  EXPECT_EQ(expected_route_config_dump.DebugString(), route_config_dump.DebugString());

  timeSystem().setSystemTime(std::chrono::milliseconds(1234567891234));

  // dynamic.
  setup();
  EXPECT_CALL(*server_factory_context_.cluster_manager_.subscription_factory_.subscription_,
              start(_));
  outer_init_manager_.initialize(init_watcher_);

  const std::string response1_yaml = R"EOF(
version_info: '1'
resources:
- "@type": type.googleapis.com/envoy.api.v2.RouteConfiguration
  name: foo_route_config
  virtual_hosts:
  - name: integration
    domains:
    - "*"
    routes:
    - match:
        prefix: "/foo"
      route:
        cluster_header: ":authority"
  - name: duplicate
    domains:
    - "*"
    routes:
    - match:
        prefix: "/foo"
      route:
        cluster_header: ":authority"
)EOF";
  auto response1 = TestUtility::parseYaml<envoy::api::v2::DiscoveryResponse>(response1_yaml);

  EXPECT_CALL(init_watcher_, ready());

  EXPECT_THROW_WITH_MESSAGE(
      rds_callbacks_->onConfigUpdate(response1.resources(), response1.version_info()),
      EnvoyException, "Only a single wildcard domain is permitted");

  message_ptr =
      server_factory_context_.admin_.config_tracker_.config_tracker_callbacks_["routes"]();
  const auto& route_config_dump3 =
      TestUtility::downcastAndValidate<const envoy::admin::v2alpha::RoutesConfigDump&>(
          *message_ptr);
  TestUtility::loadFromYaml(R"EOF(
static_route_configs:
dynamic_route_configs:
)EOF",
                            expected_route_config_dump);
  EXPECT_EQ(expected_route_config_dump.DebugString(), route_config_dump3.DebugString());
}

} // namespace
} // namespace Router
} // namespace Envoy
