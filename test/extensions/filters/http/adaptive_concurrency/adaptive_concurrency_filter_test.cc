#include <chrono>

#include "extensions/filters/http/adaptive_concurrency/adaptive_concurrency_filter.h"
#include "extensions/filters/http/adaptive_concurrency/concurrency_controller/concurrency_controller.h"

#include "test/mocks/http/mocks.h"
#include "test/mocks/stream_info/mocks.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Return;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AdaptiveConcurrency {
namespace {

using ConcurrencyController::RequestForwardingAction;

class MockConcurrencyController : public ConcurrencyController::ConcurrencyController {
public:
  MOCK_METHOD0(forwardingDecision, RequestForwardingAction());
  MOCK_METHOD0(cancelLatencySample, void());
  MOCK_METHOD1(recordLatencySample, void(std::chrono::nanoseconds));

  uint32_t concurrencyLimit() const override { return 0; }
};

class AdaptiveConcurrencyFilterTest : public testing::Test {
public:
  AdaptiveConcurrencyFilterTest() = default;

  void SetUp() override {
    const envoy::config::filter::http::adaptive_concurrency::v2alpha::AdaptiveConcurrency config;
    auto config_ptr = std::make_shared<AdaptiveConcurrencyFilterConfig>(
        config, runtime_, "testprefix.", stats_, time_system_);

    filter_ = std::make_unique<AdaptiveConcurrencyFilter>(config_ptr, controller_);
    filter_->setDecoderFilterCallbacks(decoder_callbacks_);
    filter_->setEncoderFilterCallbacks(encoder_callbacks_);
  }

  void TearDown() override { filter_.reset(); }

  Event::SimulatedTimeSystem time_system_;
  Stats::IsolatedStoreImpl stats_;
  NiceMock<Runtime::MockLoader> runtime_;
  std::shared_ptr<MockConcurrencyController> controller_{new MockConcurrencyController()};
  NiceMock<Http::MockStreamDecoderFilterCallbacks> decoder_callbacks_;
  NiceMock<Http::MockStreamEncoderFilterCallbacks> encoder_callbacks_;
  std::unique_ptr<AdaptiveConcurrencyFilter> filter_;
};

TEST_F(AdaptiveConcurrencyFilterTest, DecodeHeadersTestForwarding) {
  Http::TestHeaderMapImpl request_headers;

  EXPECT_CALL(*controller_, forwardingDecision())
      .WillOnce(Return(RequestForwardingAction::Forward));
  EXPECT_CALL(*controller_, recordLatencySample(_));

  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, false));

  Buffer::OwnedImpl request_body;
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(request_body, false));

  Http::TestHeaderMapImpl request_trailers;
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(request_trailers));
}

TEST_F(AdaptiveConcurrencyFilterTest, DecodeHeadersTestBlock) {
  Http::TestHeaderMapImpl request_headers;

  EXPECT_CALL(*controller_, forwardingDecision()).WillOnce(Return(RequestForwardingAction::Block));
  EXPECT_CALL(decoder_callbacks_, sendLocalReply(Http::Code::ServiceUnavailable, _, _, _, _));
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter_->decodeHeaders(request_headers, true));
}

TEST_F(AdaptiveConcurrencyFilterTest, RecordSampleInDestructor) {
  // Verify that the request latency is always sampled even if encodeComplete() is never called.
  EXPECT_CALL(*controller_, forwardingDecision())
      .WillOnce(Return(RequestForwardingAction::Forward));
  Http::TestHeaderMapImpl request_headers;
  filter_->decodeHeaders(request_headers, true);

  EXPECT_CALL(*controller_, recordLatencySample(_));
  filter_.reset();
}

TEST_F(AdaptiveConcurrencyFilterTest, RecordSampleOmission) {
  // Verify that the request latency is not sampled if forwardingDecision blocks the request.
  EXPECT_CALL(*controller_, forwardingDecision()).WillOnce(Return(RequestForwardingAction::Block));
  Http::TestHeaderMapImpl request_headers;
  filter_->decodeHeaders(request_headers, true);

  filter_.reset();
}

TEST_F(AdaptiveConcurrencyFilterTest, OnDestroyCleanupResetTest) {
  // Get the filter to record the request start time via decode.
  Http::TestHeaderMapImpl request_headers;
  EXPECT_CALL(*controller_, forwardingDecision())
      .WillOnce(Return(RequestForwardingAction::Forward));
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));

  EXPECT_CALL(*controller_, cancelLatencySample());

  // Encode step is not performed prior to destruction.
  filter_->onDestroy();
}

TEST_F(AdaptiveConcurrencyFilterTest, OnDestroyCleanupTest) {
  // Get the filter to record the request start time via decode.
  Http::TestHeaderMapImpl request_headers;
  EXPECT_CALL(*controller_, forwardingDecision())
      .WillOnce(Return(RequestForwardingAction::Forward));
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));

  const auto advance_time = std::chrono::nanoseconds(42);
  time_system_.sleep(advance_time);

  Http::TestHeaderMapImpl response_headers;
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->encodeHeaders(response_headers, false));
  EXPECT_CALL(*controller_, recordLatencySample(advance_time));
  filter_->encodeComplete();

  filter_->onDestroy();
}

TEST_F(AdaptiveConcurrencyFilterTest, EncodeHeadersValidTest) {
  auto mt = time_system_.monotonicTime();
  time_system_.setMonotonicTime(mt + std::chrono::nanoseconds(123));

  // Get the filter to record the request start time via decode.
  Http::TestHeaderMapImpl request_headers;
  EXPECT_CALL(*controller_, forwardingDecision())
      .WillOnce(Return(RequestForwardingAction::Forward));
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));

  const auto advance_time = std::chrono::nanoseconds(42);
  mt = time_system_.monotonicTime();
  time_system_.setMonotonicTime(mt + advance_time);

  Http::TestHeaderMapImpl response_headers;
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->encodeHeaders(response_headers, false));
  EXPECT_CALL(*controller_, recordLatencySample(advance_time));
  filter_->encodeComplete();
}

} // namespace
} // namespace AdaptiveConcurrency
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
