#include "common/upstream/health_checker_impl.h"

#include "extensions/health_checkers/redis/config.h"

#include "test/common/upstream/utility.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/runtime/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/mocks/upstream/mocks.h"

namespace Envoy {
namespace Extensions {
namespace HealthCheckers {
namespace RedisHealthChecker {
namespace {

using CustomRedisHealthChecker = Extensions::HealthCheckers::RedisHealthChecker::RedisHealthChecker;

TEST(HealthCheckerFactoryTest, CreateRedis) {
  const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 1
    healthy_threshold: 1
    custom_health_check:
      name: envoy.health_checkers.redis
      config:
        key: foo
    )EOF";

  NiceMock<Server::Configuration::MockHealthCheckerFactoryContext> context;

  RedisHealthCheckerFactory factory;
  EXPECT_NE(
      nullptr,
      dynamic_cast<CustomRedisHealthChecker*>(
          factory.createCustomHealthChecker(Upstream::parseHealthCheckFromV2Yaml(yaml), context)
              .get()));
}

TEST(HealthCheckerFactoryTest, CreateRedisWithoutKey) {
  const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 1
    healthy_threshold: 1
    custom_health_check:
      name: envoy.health_checkers.redis
      config:
    )EOF";

  NiceMock<Server::Configuration::MockHealthCheckerFactoryContext> context;

  RedisHealthCheckerFactory factory;
  EXPECT_NE(
      nullptr,
      dynamic_cast<CustomRedisHealthChecker*>(
          factory.createCustomHealthChecker(Upstream::parseHealthCheckFromV2Yaml(yaml), context)
              .get()));
}

TEST(HealthCheckerFactoryTest, CreateRedisWithLogHCFailure) {
  const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 1
    healthy_threshold: 1
    custom_health_check:
      name: envoy.health_checkers.redis
      config:
    always_log_health_check_failures: true
    )EOF";

  NiceMock<Server::Configuration::MockHealthCheckerFactoryContext> context;

  RedisHealthCheckerFactory factory;
  EXPECT_NE(
      nullptr,
      dynamic_cast<CustomRedisHealthChecker*>(
          factory.createCustomHealthChecker(Upstream::parseHealthCheckFromV2Yaml(yaml), context)
              .get()));
}

TEST(HealthCheckerFactoryTest, CreateRedisViaUpstreamHealthCheckerFactory) {
  const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 1
    healthy_threshold: 1
    custom_health_check:
      name: envoy.health_checkers.redis
      config:
        key: foo
    )EOF";

  NiceMock<Upstream::MockClusterMockPrioritySet> cluster;
  Runtime::MockLoader runtime;
  Runtime::MockRandomGenerator random;
  Event::MockDispatcher dispatcher;
  AccessLog::MockAccessLogManager log_manager;
  NiceMock<Api::MockApi> api;

  EXPECT_NE(nullptr,
            dynamic_cast<CustomRedisHealthChecker*>(
                Upstream::HealthCheckerFactory::create(
                    Upstream::parseHealthCheckFromV2Yaml(yaml), cluster, runtime, random,
                    dispatcher, log_manager, ProtobufMessage::getStrictValidationVisitor(), api)
                    .get()));
}
} // namespace
} // namespace RedisHealthChecker
} // namespace HealthCheckers
} // namespace Extensions
} // namespace Envoy
