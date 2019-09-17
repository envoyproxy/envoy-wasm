#include <chrono>
#include <list>
#include <string>

#include "common/api/api_impl.h"
#include "common/config/well_known_names.h"
#include "common/json/json_loader.h"
#include "common/upstream/cluster_manager_impl.h"

#include "server/configuration_impl.h"

#include "extensions/stat_sinks/well_known_names.h"

#include "test/common/upstream/utility.h"
#include "test/mocks/common.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "fmt/printf.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Return;

namespace Envoy {
namespace Server {
namespace Configuration {
namespace {

TEST(FilterChainUtility, buildFilterChain) {
  Network::MockConnection connection;
  std::vector<Network::FilterFactoryCb> factories;
  ReadyWatcher watcher;
  Network::FilterFactoryCb factory = [&](Network::FilterManager&) -> void { watcher.ready(); };
  factories.push_back(factory);
  factories.push_back(factory);

  EXPECT_CALL(watcher, ready()).Times(2);
  EXPECT_CALL(connection, initializeReadFilters()).WillOnce(Return(true));
  EXPECT_EQ(FilterChainUtility::buildFilterChain(connection, factories), true);
}

TEST(FilterChainUtility, buildFilterChainFailWithBadFilters) {
  Network::MockConnection connection;
  std::vector<Network::FilterFactoryCb> factories;
  EXPECT_CALL(connection, initializeReadFilters()).WillOnce(Return(false));
  EXPECT_EQ(FilterChainUtility::buildFilterChain(connection, factories), false);
}

class ConfigurationImplTest : public testing::Test {
protected:
  ConfigurationImplTest()
      : api_(Api::createApiForTest()),
        cluster_manager_factory_(server_.admin(), server_.runtime(), server_.stats(),
                                 server_.threadLocal(), server_.random(), server_.dnsResolver(),
                                 server_.sslContextManager(), server_.dispatcher(),
                                 server_.localInfo(), server_.secretManager(),
                                 server_.messageValidationContext(), *api_, server_.httpContext(),
                                 server_.accessLogManager(), server_.singletonManager()) {}

  Api::ApiPtr api_;
  NiceMock<Server::MockInstance> server_;
  Upstream::ProdClusterManagerFactory cluster_manager_factory_;
};

TEST_F(ConfigurationImplTest, DefaultStatsFlushInterval) {
  envoy::config::bootstrap::v2::Bootstrap bootstrap;

  MainImpl config;
  config.initialize(bootstrap, server_, cluster_manager_factory_);

  EXPECT_EQ(std::chrono::milliseconds(5000), config.statsFlushInterval());
}

TEST_F(ConfigurationImplTest, CustomStatsFlushInterval) {
  std::string json = R"EOF(
  {
    "stats_flush_interval": "0.500s",

    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);

  MainImpl config;
  config.initialize(bootstrap, server_, cluster_manager_factory_);

  EXPECT_EQ(std::chrono::milliseconds(500), config.statsFlushInterval());
}

TEST_F(ConfigurationImplTest, SetUpstreamClusterPerConnectionBufferLimit) {
  const std::string json = R"EOF(
  {
    "static_resources": {
      "listeners" : [],
      "clusters": [
        {
          "name": "test_cluster",
          "type": "static",
          "connect_timeout": "0.01s",
          "per_connection_buffer_limit_bytes": 8192,
          "lb_policy": "round_robin",
          "hosts": [
            {
              "socket_address" : {
                "address": "127.0.0.1",
                "port_value": 9999
              }
            }
          ]
        }
      ]
    },
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);

  MainImpl config;
  config.initialize(bootstrap, server_, cluster_manager_factory_);

  ASSERT_EQ(1U, config.clusterManager()->clusters().count("test_cluster"));
  EXPECT_EQ(8192U, config.clusterManager()
                       ->clusters()
                       .find("test_cluster")
                       ->second.get()
                       .info()
                       ->perConnectionBufferLimitBytes());
  server_.thread_local_.shutdownThread();
}

TEST_F(ConfigurationImplTest, NullTracerSetWhenTracingConfigurationAbsent) {
  std::string json = R"EOF(
  {
    "static_resources": {
      "listeners" : [
        {
          "address": {
            "socket_address": {
              "address": "127.0.0.1",
              "port_value": 1234
            }
          },
          "filter_chains": []
        }
      ],
      "clusters": []
    },
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);

  server_.local_info_.node_.set_cluster("");
  MainImpl config;
  config.initialize(bootstrap, server_, cluster_manager_factory_);

  EXPECT_NE(nullptr, dynamic_cast<Tracing::HttpNullTracer*>(&config.httpTracer()));
}

TEST_F(ConfigurationImplTest, NullTracerSetWhenHttpKeyAbsentFromTracerConfiguration) {
  std::string json = R"EOF(
  {
    "static_resources": {
      "listeners" : [
        {
          "address": {
            "socket_address": {
              "address": "127.0.0.1",
              "port_value": 1234
            }
          },
          "filter_chains": []
        }
      ],
      "clusters": []
    },
    "tracing": {},
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);

  server_.local_info_.node_.set_cluster("");
  MainImpl config;
  config.initialize(bootstrap, server_, cluster_manager_factory_);

  EXPECT_NE(nullptr, dynamic_cast<Tracing::HttpNullTracer*>(&config.httpTracer()));
}

TEST_F(ConfigurationImplTest, ConfigurationFailsWhenInvalidTracerSpecified) {
  std::string json = R"EOF(
  {
    "static_resources": {
      "listeners" : [
        {
          "address": {
            "socket_address": {
              "address": "127.0.0.1",
              "port_value": 1234
            }
          },
          "filter_chains": []
        }
      ],
      "clusters": []
    },
    "tracing": {
      "http": {
        "name": "invalid",
        "typed_config": {
          "@type": "type.googleapis.com/envoy.config.trace.v2.LightstepConfig",
          "collector_cluster": "cluster_0",
          "access_token_file": "/etc/envoy/envoy.cfg"
        }
      }
    },
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);
  MainImpl config;
  EXPECT_THROW_WITH_MESSAGE(config.initialize(bootstrap, server_, cluster_manager_factory_),
                            EnvoyException,
                            "Didn't find a registered implementation for name: 'invalid'");
}

TEST_F(ConfigurationImplTest, ProtoSpecifiedStatsSink) {
  std::string json = R"EOF(
  {
    "static_resources": {
      "listeners": [],
      "clusters": []
    },
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);

  auto& sink = *bootstrap.mutable_stats_sinks()->Add();
  sink.set_name(Extensions::StatSinks::StatsSinkNames::get().Statsd);
  auto& field_map = *sink.mutable_config()->mutable_fields();
  field_map["tcp_cluster_name"].set_string_value("fake_cluster");

  MainImpl config;
  config.initialize(bootstrap, server_, cluster_manager_factory_);

  EXPECT_EQ(1, config.statsSinks().size());
}

TEST_F(ConfigurationImplTest, StatsSinkWithInvalidName) {
  std::string json = R"EOF(
  {
    "static_resources": {
      "listeners": [],
      "clusters": []
    },
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);

  envoy::config::metrics::v2::StatsSink& sink = *bootstrap.mutable_stats_sinks()->Add();
  sink.set_name("envoy.invalid");
  auto& field_map = *sink.mutable_config()->mutable_fields();
  field_map["tcp_cluster_name"].set_string_value("fake_cluster");

  MainImpl config;
  EXPECT_THROW_WITH_MESSAGE(config.initialize(bootstrap, server_, cluster_manager_factory_),
                            EnvoyException,
                            "Didn't find a registered implementation for name: 'envoy.invalid'");
}

TEST_F(ConfigurationImplTest, StatsSinkWithNoName) {
  std::string json = R"EOF(
  {
    "static_resources": {
      "listeners": [],
      "clusters": []
    },
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      }
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);

  auto& sink = *bootstrap.mutable_stats_sinks()->Add();
  auto& field_map = *sink.mutable_config()->mutable_fields();
  field_map["tcp_cluster_name"].set_string_value("fake_cluster");

  MainImpl config;
  EXPECT_THROW_WITH_MESSAGE(config.initialize(bootstrap, server_, cluster_manager_factory_),
                            EnvoyException,
                            "Provided name for static registration lookup was empty.");
}

// An explicit non-empty LayeredRuntime is available to the server with no
// changes made.
TEST(InitialImplTest, LayeredRuntime) {
  const std::string yaml = R"EOF(
  layered_runtime:
    layers:
    - name: base
      static_layer:
        health_check:
          min_interval: 5
    - name: root
      disk_layer: { symlink_root: /srv/runtime/current, subdirectory: envoy }
    - name: override
      disk_layer: { symlink_root: /srv/runtime/current, subdirectory: envoy_override, append_service_cluster: true }
    - name: admin
      admin_layer: {}
  )EOF";
  const auto bootstrap = TestUtility::parseYaml<envoy::config::bootstrap::v2::Bootstrap>(yaml);
  InitialImpl config(bootstrap);
  EXPECT_THAT(config.runtime(), ProtoEq(bootstrap.layered_runtime()));
}

// An empty LayeredRuntime has an admin layer injected.
TEST(InitialImplTest, EmptyLayeredRuntime) {
  const std::string bootstrap_yaml = R"EOF(
  layered_runtime: {}
  )EOF";
  const auto bootstrap =
      TestUtility::parseYaml<envoy::config::bootstrap::v2::Bootstrap>(bootstrap_yaml);
  InitialImpl config(bootstrap);

  const std::string expected_yaml = R"EOF(
  layers:
  - admin_layer: {}
  )EOF";
  const auto expected_runtime =
      TestUtility::parseYaml<envoy::config::bootstrap::v2::LayeredRuntime>(expected_yaml);
  EXPECT_THAT(config.runtime(), ProtoEq(expected_runtime));
}

// An empty deprecated Runtime has an empty static and admin layer injected.
TEST(InitialImplTest, EmptyDeprecatedRuntime) {
  const auto bootstrap = TestUtility::parseYaml<envoy::config::bootstrap::v2::Bootstrap>("{}");
  InitialImpl config(bootstrap);

  const std::string expected_yaml = R"EOF(
  layers:
  - name: base
    static_layer: {}
  - name: admin
    admin_layer: {}
  )EOF";
  const auto expected_runtime =
      TestUtility::parseYaml<envoy::config::bootstrap::v2::LayeredRuntime>(expected_yaml);
  EXPECT_THAT(config.runtime(), ProtoEq(expected_runtime));
}

// A deprecated Runtime is transformed to the equivalent LayeredRuntime.
TEST(InitialImplTest, DeprecatedRuntimeTranslation) {
  const std::string bootstrap_yaml = R"EOF(
  runtime:
    symlink_root: /srv/runtime/current
    subdirectory: envoy
    override_subdirectory: envoy_override
    base:
      health_check:
        min_interval: 5
  )EOF";
  const auto bootstrap =
      TestUtility::parseYaml<envoy::config::bootstrap::v2::Bootstrap>(bootstrap_yaml);
  InitialImpl config(bootstrap);

  const std::string expected_yaml = R"EOF(
  layers:
  - name: base
    static_layer:
      health_check:
        min_interval: 5
  - name: root
    disk_layer: { symlink_root: /srv/runtime/current, subdirectory: envoy }
  - name: override
    disk_layer: { symlink_root: /srv/runtime/current, subdirectory: envoy_override, append_service_cluster: true }
  - name: admin
    admin_layer: {}
  )EOF";
  const auto expected_runtime =
      TestUtility::parseYaml<envoy::config::bootstrap::v2::LayeredRuntime>(expected_yaml);
  EXPECT_THAT(config.runtime(), ProtoEq(expected_runtime));
}

TEST_F(ConfigurationImplTest, AdminSocketOptions) {
  std::string json = R"EOF(
  {
    "admin": {
      "access_log_path": "/dev/null",
      "address": {
        "socket_address": {
          "address": "1.2.3.4",
          "port_value": 5678
        }
      },
      "socket_options": [
         {
           "level": 1,
           "name": 2,
           "int_value": 3,
           "state": "STATE_PREBIND"
         },
         {
           "level": 4,
           "name": 5,
           "int_value": 6,
           "state": "STATE_BOUND"
         },
      ]
    }
  }
  )EOF";

  auto bootstrap = Upstream::parseBootstrapFromV2Json(json);
  InitialImpl config(bootstrap);
  Network::MockListenSocket socket_mock;

  ASSERT_EQ(config.admin().socketOptions()->size(), 2);
  auto detail = config.admin().socketOptions()->at(0)->getOptionDetails(
      socket_mock, envoy::api::v2::core::SocketOption::STATE_PREBIND);
  ASSERT_NE(detail, absl::nullopt);
  EXPECT_EQ(detail->name_, Envoy::Network::SocketOptionName(1, 2, "1/2"));
  detail = config.admin().socketOptions()->at(1)->getOptionDetails(
      socket_mock, envoy::api::v2::core::SocketOption::STATE_BOUND);
  ASSERT_NE(detail, absl::nullopt);
  EXPECT_EQ(detail->name_, Envoy::Network::SocketOptionName(4, 5, "4/5"));
}

} // namespace
} // namespace Configuration
} // namespace Server
} // namespace Envoy
