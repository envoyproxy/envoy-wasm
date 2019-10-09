#include <chrono>
#include <iostream>

#include "envoy/config/filter/http/adaptive_concurrency/v2alpha/adaptive_concurrency.pb.h"
#include "envoy/config/filter/http/adaptive_concurrency/v2alpha/adaptive_concurrency.pb.validate.h"

#include "common/stats/isolated_store_impl.h"

#include "extensions/filters/http/adaptive_concurrency/adaptive_concurrency_filter.h"
#include "extensions/filters/http/adaptive_concurrency/concurrency_controller/concurrency_controller.h"
#include "extensions/filters/http/adaptive_concurrency/concurrency_controller/gradient_controller.h"

#include "test/mocks/event/mocks.h"
#include "test/mocks/runtime/mocks.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::AllOf;
using testing::Ge;
using testing::Le;
using testing::NiceMock;
using testing::Return;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AdaptiveConcurrency {
namespace ConcurrencyController {
namespace {

class GradientControllerConfigTest : public testing::Test {
public:
  GradientControllerConfigTest() = default;
};

class GradientControllerTest : public testing::Test {
public:
  GradientControllerTest()
      : api_(Api::createApiForTest(time_system_)), dispatcher_(api_->allocateDispatcher()) {}

  GradientControllerSharedPtr makeController(const std::string& yaml_config) {
    return std::make_shared<GradientController>(makeConfig(yaml_config), *dispatcher_, runtime_,
                                                "test_prefix.", stats_, random_);
  }

protected:
  GradientControllerConfig makeConfig(const std::string& yaml_config) {
    envoy::config::filter::http::adaptive_concurrency::v2alpha::GradientControllerConfig proto =
        TestUtility::parseYaml<
            envoy::config::filter::http::adaptive_concurrency::v2alpha::GradientControllerConfig>(
            yaml_config);
    return proto;
  }

  // Helper function that will attempt to pull forwarding decisions.
  void tryForward(const GradientControllerSharedPtr& controller,
                  const bool expect_forward_response) {
    const auto expected_resp =
        expect_forward_response ? RequestForwardingAction::Forward : RequestForwardingAction::Block;
    EXPECT_EQ(expected_resp, controller->forwardingDecision());
  }

  // Gets the controller past the initial minRTT stage.
  void advancePastMinRTTStage(const GradientControllerSharedPtr& controller,
                              const std::string& yaml_config,
                              std::chrono::milliseconds latency = std::chrono::milliseconds(5)) {
    const auto config = makeConfig(yaml_config);
    for (uint32_t i = 0; i <= config.minRTTAggregateRequestCount(); ++i) {
      tryForward(controller, true);
      controller->recordLatencySample(latency);
    }
  }

  Event::SimulatedTimeSystem time_system_;
  Stats::IsolatedStoreImpl stats_;
  NiceMock<Runtime::MockLoader> runtime_;
  Api::ApiPtr api_;
  Event::DispatcherPtr dispatcher_;
  NiceMock<Runtime::MockRandomGenerator> random_;
};

TEST_F(GradientControllerConfigTest, BasicTest) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 42
concurrency_limit_params:
  max_gradient: 2.1
  max_concurrency_limit: 1337
  concurrency_update_interval: 0.123s
min_rtt_calc_params:
  jitter:
    value: 13.2
  interval: 31s
  request_count: 52
)EOF";

  envoy::config::filter::http::adaptive_concurrency::v2alpha::GradientControllerConfig proto =
      TestUtility::parseYaml<
          envoy::config::filter::http::adaptive_concurrency::v2alpha::GradientControllerConfig>(
          yaml);
  GradientControllerConfig config(proto);

  EXPECT_EQ(config.minRTTCalcInterval(), std::chrono::seconds(31));
  EXPECT_EQ(config.sampleRTTCalcInterval(), std::chrono::milliseconds(123));
  EXPECT_EQ(config.maxConcurrencyLimit(), 1337);
  EXPECT_EQ(config.minRTTAggregateRequestCount(), 52);
  EXPECT_EQ(config.maxGradient(), 2.1);
  EXPECT_EQ(config.jitterPercent(), .132);
  EXPECT_EQ(config.sampleAggregatePercentile(), 0.42);
}

TEST_F(GradientControllerConfigTest, DefaultValuesTest) {
  const std::string yaml = R"EOF(
concurrency_limit_params:
  concurrency_update_interval: 0.123s
min_rtt_calc_params:
  interval: 31s
)EOF";

  envoy::config::filter::http::adaptive_concurrency::v2alpha::GradientControllerConfig proto =
      TestUtility::parseYaml<
          envoy::config::filter::http::adaptive_concurrency::v2alpha::GradientControllerConfig>(
          yaml);
  GradientControllerConfig config(proto);

  EXPECT_EQ(config.minRTTCalcInterval(), std::chrono::seconds(31));
  EXPECT_EQ(config.sampleRTTCalcInterval(), std::chrono::milliseconds(123));
  EXPECT_EQ(config.maxConcurrencyLimit(), 1000);
  EXPECT_EQ(config.minRTTAggregateRequestCount(), 50);
  EXPECT_EQ(config.maxGradient(), 2.0);
  EXPECT_EQ(config.jitterPercent(), .15);
  EXPECT_EQ(config.sampleAggregatePercentile(), 0.5);
}

TEST_F(GradientControllerTest, MinRTTLogicTest) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 2.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 50
)EOF";

  auto controller = makeController(yaml);
  const auto min_rtt = std::chrono::milliseconds(13);

  // The controller should be measuring minRTT upon creation, so the concurrency window is 1.
  EXPECT_EQ(controller->concurrencyLimit(), 1);
  tryForward(controller, true);
  tryForward(controller, false);
  tryForward(controller, false);
  controller->recordLatencySample(min_rtt);

  // 49 more requests should cause the minRTT to be done calculating.
  for (int i = 0; i < 49; ++i) {
    EXPECT_EQ(controller->concurrencyLimit(), 1);
    tryForward(controller, true);
    tryForward(controller, false);
    controller->recordLatencySample(min_rtt);
  }

  // Verify the minRTT value measured is accurate.
  EXPECT_EQ(
      13, stats_.gauge("test_prefix.min_rtt_msecs", Stats::Gauge::ImportMode::NeverImport).value());
}

TEST_F(GradientControllerTest, CancelLatencySample) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 2.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 5
)EOF";

  auto controller = makeController(yaml);

  for (int i = 1; i <= 5; ++i) {
    tryForward(controller, true);
    controller->recordLatencySample(std::chrono::milliseconds(i));
  }
  EXPECT_EQ(
      3, stats_.gauge("test_prefix.min_rtt_msecs", Stats::Gauge::ImportMode::NeverImport).value());
}

TEST_F(GradientControllerTest, SamplePercentileProcessTest) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 2.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 5
)EOF";

  auto controller = makeController(yaml);

  tryForward(controller, true);
  tryForward(controller, false);
  controller->cancelLatencySample();
  tryForward(controller, true);
  tryForward(controller, false);
}

TEST_F(GradientControllerTest, ConcurrencyLimitBehaviorTestBasic) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 2.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 5
)EOF";

  auto controller = makeController(yaml);
  EXPECT_EQ(controller->concurrencyLimit(), 1);

  // Force a minRTT of 5ms.
  advancePastMinRTTStage(controller, yaml, std::chrono::milliseconds(5));
  EXPECT_EQ(
      5, stats_.gauge("test_prefix.min_rtt_msecs", Stats::Gauge::ImportMode::NeverImport).value());

  // Ensure that the concurrency window increases on its own due to the headroom calculation.
  time_system_.sleep(std::chrono::milliseconds(101));
  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_GT(controller->concurrencyLimit(), 1);

  // Make it seem as if the recorded latencies are consistently lower than the measured minRTT.
  // Ensure that it grows.
  for (int recalcs = 0; recalcs < 10; ++recalcs) {
    const auto last_concurrency = controller->concurrencyLimit();
    for (int i = 1; i <= 5; ++i) {
      tryForward(controller, true);
      controller->recordLatencySample(std::chrono::milliseconds(4));
    }
    time_system_.sleep(std::chrono::milliseconds(101));
    dispatcher_->run(Event::Dispatcher::RunType::Block);
    EXPECT_GT(controller->concurrencyLimit(), last_concurrency);
  }

  // Verify that the concurrency limit can now shrink as necessary.
  for (int recalcs = 0; recalcs < 10; ++recalcs) {
    const auto last_concurrency = controller->concurrencyLimit();
    for (int i = 1; i <= 5; ++i) {
      tryForward(controller, true);
      controller->recordLatencySample(std::chrono::milliseconds(6));
    }
    time_system_.sleep(std::chrono::milliseconds(101));
    dispatcher_->run(Event::Dispatcher::RunType::Block);
    EXPECT_LT(controller->concurrencyLimit(), last_concurrency);
  }
}

TEST_F(GradientControllerTest, MaxGradientTest) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 3.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 5
)EOF";

  auto controller = makeController(yaml);
  EXPECT_EQ(controller->concurrencyLimit(), 1);

  // Force a minRTT of 5 seconds.
  advancePastMinRTTStage(controller, yaml, std::chrono::seconds(5));

  // circllhist approximates the percentiles, so we can expect it to be within a certain range.
  EXPECT_THAT(
      stats_.gauge("test_prefix.min_rtt_msecs", Stats::Gauge::ImportMode::NeverImport).value(),
      AllOf(Ge(4950), Le(5050)));

  // Now verify max gradient value by forcing dramatically faster latency measurements..
  for (int i = 1; i <= 5; ++i) {
    tryForward(controller, true);
    controller->recordLatencySample(std::chrono::milliseconds(4));
  }
  time_system_.sleep(std::chrono::milliseconds(101));
  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_EQ(3.0,
            stats_.gauge("test_prefix.gradient", Stats::Gauge::ImportMode::NeverImport).value());
}

TEST_F(GradientControllerTest, MinRTTReturnToPreviousLimit) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 3.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 5
)EOF";

  auto controller = makeController(yaml);
  EXPECT_EQ(controller->concurrencyLimit(), 1);

  // Get initial minRTT measurement out of the way.
  advancePastMinRTTStage(controller, yaml, std::chrono::milliseconds(5));

  // Force the limit calculation to run a few times from some measurements.
  for (int sample_iters = 0; sample_iters < 5; ++sample_iters) {
    const auto last_concurrency = controller->concurrencyLimit();
    for (int i = 1; i <= 5; ++i) {
      tryForward(controller, true);
      controller->recordLatencySample(std::chrono::milliseconds(4));
    }
    time_system_.sleep(std::chrono::milliseconds(101));
    dispatcher_->run(Event::Dispatcher::RunType::Block);
    // Verify the value is growing.
    EXPECT_GT(controller->concurrencyLimit(), last_concurrency);
  }

  const auto limit_val = controller->concurrencyLimit();

  // Wait until the minRTT recalculation is triggered again and verify the limit drops.
  time_system_.sleep(std::chrono::seconds(31));
  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_EQ(controller->concurrencyLimit(), 1);

  // 49 more requests should cause the minRTT to be done calculating.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(controller->concurrencyLimit(), 1);
    tryForward(controller, true);
    controller->recordLatencySample(std::chrono::milliseconds(13));
  }

  // Check that we restored the old concurrency limit value.
  EXPECT_EQ(limit_val, controller->concurrencyLimit());
}

TEST_F(GradientControllerTest, MinRTTRescheduleTest) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 3.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 5
)EOF";

  auto controller = makeController(yaml);
  EXPECT_EQ(controller->concurrencyLimit(), 1);

  // Get initial minRTT measurement out of the way.
  advancePastMinRTTStage(controller, yaml, std::chrono::milliseconds(5));

  // Force the limit calculation to run a few times from some measurements.
  for (int sample_iters = 0; sample_iters < 5; ++sample_iters) {
    const auto last_concurrency = controller->concurrencyLimit();
    for (int i = 1; i <= 5; ++i) {
      tryForward(controller, true);
      controller->recordLatencySample(std::chrono::milliseconds(4));
    }
    time_system_.sleep(std::chrono::milliseconds(101));
    dispatcher_->run(Event::Dispatcher::RunType::Block);
    // Verify the value is growing.
    EXPECT_GT(controller->concurrencyLimit(), last_concurrency);
  }

  // Wait until the minRTT recalculation is triggered again and verify the limit drops.
  time_system_.sleep(std::chrono::seconds(31));
  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_EQ(controller->concurrencyLimit(), 1);

  // Verify sample recalculation doesn't occur during the minRTT window.
  time_system_.sleep(std::chrono::milliseconds(101));
  dispatcher_->run(Event::Dispatcher::RunType::Block);
  EXPECT_EQ(controller->concurrencyLimit(), 1);
}

TEST_F(GradientControllerTest, NoSamplesTest) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 3.0
  max_concurrency_limit:
  concurrency_update_interval: 0.1s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 30s
  request_count: 5
)EOF";

  auto controller = makeController(yaml);
  EXPECT_EQ(controller->concurrencyLimit(), 1);

  // Get minRTT measurement out of the way.
  advancePastMinRTTStage(controller, yaml, std::chrono::milliseconds(5));

  // Force the limit calculation to run a few times from some measurements.
  for (int sample_iters = 0; sample_iters < 5; ++sample_iters) {
    const auto last_concurrency = controller->concurrencyLimit();
    for (int i = 1; i <= 5; ++i) {
      tryForward(controller, true);
      controller->recordLatencySample(std::chrono::milliseconds(4));
    }
    time_system_.sleep(std::chrono::milliseconds(101));
    dispatcher_->run(Event::Dispatcher::RunType::Block);
    // Verify the value is growing.
    EXPECT_GT(controller->concurrencyLimit(), last_concurrency);
  }

  // Now we make sure that the limit value doesn't change in the absence of samples.
  for (int sample_iters = 0; sample_iters < 5; ++sample_iters) {
    const auto old_limit = controller->concurrencyLimit();
    time_system_.sleep(std::chrono::milliseconds(101));
    dispatcher_->run(Event::Dispatcher::RunType::Block);
    EXPECT_EQ(old_limit, controller->concurrencyLimit());
  }
}

TEST_F(GradientControllerTest, TimerAccuracyTest) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 3.0
  max_concurrency_limit:
  concurrency_update_interval: 0.123s
min_rtt_calc_params:
  jitter:
    value: 10.0
  interval: 100s
  request_count: 5
)EOF";

  // Verify the configuration affects the timers that are kicked off.
  NiceMock<Event::MockDispatcher> fake_dispatcher;
  auto sample_timer = new NiceMock<Event::MockTimer>;
  auto rtt_timer = new NiceMock<Event::MockTimer>;

  // Expect the sample timer to trigger start immediately upon controller creation.
  EXPECT_CALL(fake_dispatcher, createTimer_(_))
      .Times(2)
      .WillOnce(Return(rtt_timer))
      .WillOnce(Return(sample_timer));
  EXPECT_CALL(*sample_timer, enableTimer(std::chrono::milliseconds(123), _));
  auto controller = std::make_shared<GradientController>(makeConfig(yaml), fake_dispatcher,
                                                         runtime_, "test_prefix.", stats_, random_);

  // Set the minRTT- this will trigger the timer for the next minRTT calculation.

  // Let's make sure the jitter value can't exceed the configured percentage as well by returning a
  // random value > 10% of the interval.
  EXPECT_CALL(random_, random()).WillOnce(Return(15000));
  EXPECT_CALL(*rtt_timer, enableTimer(std::chrono::milliseconds(105000), _));
  for (int i = 0; i < 6; ++i) {
    tryForward(controller, true);
    controller->recordLatencySample(std::chrono::milliseconds(5));
  }
}

TEST_F(GradientControllerTest, TimerAccuracyTestNoJitter) {
  const std::string yaml = R"EOF(
sample_aggregate_percentile:
  value: 50
concurrency_limit_params:
  max_gradient: 3.0
  max_concurrency_limit:
  concurrency_update_interval: 0.123s
min_rtt_calc_params:
  jitter:
    value: 0.0
  interval: 45s
  request_count: 5
)EOF";

  // Verify the configuration affects the timers that are kicked off.
  NiceMock<Event::MockDispatcher> fake_dispatcher;
  auto sample_timer = new NiceMock<Event::MockTimer>;
  auto rtt_timer = new NiceMock<Event::MockTimer>;

  // Expect the sample timer to trigger start immediately upon controller creation.
  EXPECT_CALL(fake_dispatcher, createTimer_(_))
      .Times(2)
      .WillOnce(Return(rtt_timer))
      .WillOnce(Return(sample_timer));
  EXPECT_CALL(*sample_timer, enableTimer(std::chrono::milliseconds(123), _));
  auto controller = std::make_shared<GradientController>(makeConfig(yaml), fake_dispatcher,
                                                         runtime_, "test_prefix.", stats_, random_);

  // Set the minRTT- this will trigger the timer for the next minRTT calculation.
  EXPECT_CALL(*rtt_timer, enableTimer(std::chrono::milliseconds(45000), _));
  for (int i = 0; i < 6; ++i) {
    tryForward(controller, true);
    controller->recordLatencySample(std::chrono::milliseconds(5));
  }
}

} // namespace
} // namespace ConcurrencyController
} // namespace AdaptiveConcurrency
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
