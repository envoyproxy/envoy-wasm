#include <memory>
#include <string>
#include <vector>

#include "envoy/config/filter/network/ext_authz/v2/ext_authz.pb.validate.h"
#include "envoy/stats/stats.h"

#include "common/buffer/buffer_impl.h"
#include "common/json/json_loader.h"
#include "common/network/address_impl.h"
#include "common/protobuf/utility.h"

#include "extensions/filters/network/ext_authz/ext_authz.h"

#include "test/extensions/filters/common/ext_authz/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/runtime/mocks.h"
#include "test/mocks/tracing/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/printers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::InSequence;
using testing::Invoke;
using testing::NiceMock;
using testing::ReturnRef;
using testing::WithArgs;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace ExtAuthz {

class ExtAuthzFilterTest : public testing::Test {
public:
  ExtAuthzFilterTest() {
    std::string json = R"EOF(
    {
      "grpc_service": {
          "envoy_grpc": { "cluster_name": "ext_authz_server" }
      },
      "failure_mode_allow": true,
      "stat_prefix": "name"
    }
    )EOF";

    envoy::config::filter::network::ext_authz::v2::ExtAuthz proto_config{};
    TestUtility::loadFromJson(json, proto_config);
    config_.reset(new Config(proto_config, stats_store_));
    client_ = new Filters::Common::ExtAuthz::MockClient();
    filter_ = std::make_unique<Filter>(config_, Filters::Common::ExtAuthz::ClientPtr{client_});
    filter_->initializeReadFilterCallbacks(filter_callbacks_);
    addr_ = std::make_shared<Network::Address::PipeInstance>("/test/test.sock");

    // NOP currently.
    filter_->onAboveWriteBufferHighWatermark();
    filter_->onBelowWriteBufferLowWatermark();
  }

  Filters::Common::ExtAuthz::ResponsePtr
  makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus status) {
    Filters::Common::ExtAuthz::ResponsePtr response =
        std::make_unique<Filters::Common::ExtAuthz::Response>();
    response->status = status;
    return response;
  }

  ~ExtAuthzFilterTest() override {
    for (const Stats::GaugeSharedPtr& gauge : stats_store_.gauges()) {
      EXPECT_EQ(0U, gauge->value());
    }
  }

  Stats::IsolatedStoreImpl stats_store_;
  ConfigSharedPtr config_;
  Filters::Common::ExtAuthz::MockClient* client_;
  std::unique_ptr<Filter> filter_;
  NiceMock<Network::MockReadFilterCallbacks> filter_callbacks_;
  Network::Address::InstanceConstSharedPtr addr_;
  Filters::Common::ExtAuthz::RequestCallbacks* request_callbacks_{};
};

TEST_F(ExtAuthzFilterTest, BadExtAuthzConfig) {
  std::string json_string = R"EOF(
  {
    "stat_prefix": "my_stat_prefix",
    "grpc_service": {}
  }
  )EOF";

  envoy::config::filter::network::ext_authz::v2::ExtAuthz proto_config{};
  TestUtility::loadFromJson(json_string, proto_config);

  EXPECT_THROW(TestUtility::downcastAndValidate<
                   const envoy::config::filter::network::ext_authz::v2::ExtAuthz&>(proto_config),
               ProtoValidationException);
}

TEST_F(ExtAuthzFilterTest, OKWithOnData) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(*client_, check(_, _, testing::A<Tracing::Span&>()))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  // Confirm that the invocation of onNewConnection did NOT increment the active or total count!
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(
      0U,
      stats_store_.gauge("ext_authz.name.active", Stats::Gauge::ImportMode::Accumulate).value());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));
  // Confirm that the invocation of onData does increment the active and total count!
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(
      1U,
      stats_store_.gauge("ext_authz.name.active", Stats::Gauge::ImportMode::Accumulate).value());

  EXPECT_CALL(filter_callbacks_, continueReading());
  request_callbacks_->onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::OK));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(data, false));

  EXPECT_CALL(*client_, cancel()).Times(0);
  filter_callbacks_.connection_.raiseEvent(Network::ConnectionEvent::LocalClose);

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

TEST_F(ExtAuthzFilterTest, DeniedWithOnData) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  // Confirm that the invocation of onNewConnection did NOT increment the active or total count!
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(
      0U,
      stats_store_.gauge("ext_authz.name.active", Stats::Gauge::ImportMode::Accumulate).value());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));
  // Confirm that the invocation of onData does increment the active and total count!
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(
      1U,
      stats_store_.gauge("ext_authz.name.active", Stats::Gauge::ImportMode::Accumulate).value());

  EXPECT_CALL(filter_callbacks_.connection_, close(Network::ConnectionCloseType::NoFlush));
  EXPECT_CALL(*client_, cancel()).Times(0);
  request_callbacks_->onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::Denied));

  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

TEST_F(ExtAuthzFilterTest, FailOpen) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));

  EXPECT_CALL(filter_callbacks_.connection_, close(_)).Times(0);
  EXPECT_CALL(*client_, cancel()).Times(0);
  EXPECT_CALL(filter_callbacks_, continueReading());
  request_callbacks_->onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::Error));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(data, false));

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

TEST_F(ExtAuthzFilterTest, FailClose) {
  InSequence s;
  // Explicitly set the failure_mode_allow to false.
  config_->setFailModeAllow(false);

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));

  EXPECT_CALL(filter_callbacks_.connection_, close(_)).Times(1);
  EXPECT_CALL(filter_callbacks_, continueReading()).Times(0);
  request_callbacks_->onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::Error));

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

// Test to verify that when callback from the authorization service has completed the filter
// does not invoke Cancel on RemoteClose event.
TEST_F(ExtAuthzFilterTest, DoNotCallCancelonRemoteClose) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));

  EXPECT_CALL(filter_callbacks_, continueReading());
  request_callbacks_->onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::Error));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(data, false));

  EXPECT_CALL(*client_, cancel()).Times(0);
  filter_callbacks_.connection_.raiseEvent(Network::ConnectionEvent::RemoteClose);

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

// Test to verify that Cancel is invoked when a RemoteClose event occurs while the call
// to the authorization service was in progress.
TEST_F(ExtAuthzFilterTest, VerifyCancelOnRemoteClose) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));

  EXPECT_CALL(*client_, cancel());
  filter_callbacks_.connection_.raiseEvent(Network::ConnectionEvent::RemoteClose);

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

// Test to verify that on stack response from the authorization service does NOT
// result in calling cancel.
TEST_F(ExtAuthzFilterTest, ImmediateOK) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_, continueReading()).Times(0);
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            callbacks.onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::OK));
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(data, false));
  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(data, false));

  EXPECT_CALL(*client_, cancel()).Times(0);
  filter_callbacks_.connection_.raiseEvent(Network::ConnectionEvent::RemoteClose);

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

// Test to verify that on stack denied response from the authorization service does
// result in stoppage of the filter chain.
TEST_F(ExtAuthzFilterTest, ImmediateNOK) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_, continueReading()).Times(0);
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            callbacks.onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::Denied));
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter_->onData(data, false));

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

// Test to verify that on stack Error response when failure_mode_allow is configured
// result in request being allowed.
TEST_F(ExtAuthzFilterTest, ImmediateErrorFailOpen) {
  InSequence s;

  EXPECT_CALL(filter_callbacks_.connection_, remoteAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_.connection_, localAddress()).WillOnce(ReturnRef(addr_));
  EXPECT_CALL(filter_callbacks_, continueReading()).Times(0);
  EXPECT_CALL(*client_, check(_, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::ExtAuthz::RequestCallbacks& callbacks) -> void {
            callbacks.onComplete(makeAuthzResponse(Filters::Common::ExtAuthz::CheckStatus::Error));
          })));

  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(data, false));
  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(data, false));

  EXPECT_CALL(*client_, cancel()).Times(0);
  filter_callbacks_.connection_.raiseEvent(Network::ConnectionEvent::RemoteClose);

  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.total").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.error").value());
  EXPECT_EQ(1U, stats_store_.counter("ext_authz.name.failure_mode_allowed").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.denied").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.ok").value());
  EXPECT_EQ(0U, stats_store_.counter("ext_authz.name.cx_closed").value());
}

} // namespace ExtAuthz
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
