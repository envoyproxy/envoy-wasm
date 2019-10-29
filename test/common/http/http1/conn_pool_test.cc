#include <memory>
#include <vector>

#include "envoy/http/codec.h"

#include "common/buffer/buffer_impl.h"
#include "common/event/dispatcher_impl.h"
#include "common/http/codec_client.h"
#include "common/http/http1/conn_pool.h"
#include "common/network/utility.h"
#include "common/upstream/upstream_impl.h"

#include "test/common/http/common.h"
#include "test/common/upstream/utility.h"
#include "test/mocks/buffer/mocks.h"
#include "test/mocks/event/mocks.h"
#include "test/mocks/http/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/runtime/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/printers.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::DoAll;
using testing::InSequence;
using testing::Invoke;
using testing::NiceMock;
using testing::Property;
using testing::Return;
using testing::ReturnRef;
using testing::SaveArg;

namespace Envoy {
namespace Http {
namespace Http1 {
namespace {

/**
 * A test version of ConnPoolImpl that allows for mocking beneath the codec clients.
 */
class ConnPoolImplForTest : public ConnPoolImpl {
public:
  ConnPoolImplForTest(Event::MockDispatcher& dispatcher,
                      Upstream::ClusterInfoConstSharedPtr cluster,
                      NiceMock<Event::MockTimer>* upstream_ready_timer)
      : ConnPoolImpl(dispatcher, Upstream::makeTestHost(cluster, "tcp://127.0.0.1:9000"),
                     Upstream::ResourcePriority::Default, nullptr, Http1Settings(), nullptr),
        api_(Api::createApiForTest()), mock_dispatcher_(dispatcher),
        mock_upstream_ready_timer_(upstream_ready_timer) {}

  ~ConnPoolImplForTest() override {
    EXPECT_EQ(0U, ready_clients_.size());
    EXPECT_EQ(0U, busy_clients_.size());
    EXPECT_EQ(0U, pending_requests_.size());
  }

  struct TestCodecClient {
    Http::MockClientConnection* codec_;
    Network::MockClientConnection* connection_;
    CodecClient* codec_client_;
    Event::MockTimer* connect_timer_;
    Event::DispatcherPtr client_dispatcher_;
  };

  CodecClientPtr createCodecClient(Upstream::Host::CreateConnectionData& data) override {
    // We expect to own the connection, but already have it, so just release it to prevent it from
    // getting deleted.
    data.connection_.release();
    return CodecClientPtr{createCodecClient_()};
  }

  MOCK_METHOD0(createCodecClient_, CodecClient*());
  MOCK_METHOD0(onClientDestroy, void());

  void expectClientCreate(Protocol protocol = Protocol::Http11) {
    test_clients_.emplace_back();
    TestCodecClient& test_client = test_clients_.back();
    test_client.connection_ = new NiceMock<Network::MockClientConnection>();
    test_client.codec_ = new NiceMock<Http::MockClientConnection>();
    test_client.connect_timer_ = new NiceMock<Event::MockTimer>(&mock_dispatcher_);
    std::shared_ptr<Upstream::MockClusterInfo> cluster{new NiceMock<Upstream::MockClusterInfo>()};
    test_client.client_dispatcher_ = api_->allocateDispatcher();
    Network::ClientConnectionPtr connection{test_client.connection_};
    test_client.codec_client_ = new CodecClientForTest(
        CodecClient::Type::HTTP1, std::move(connection), test_client.codec_,
        [this](CodecClient* codec_client) -> void {
          for (auto i = test_clients_.begin(); i != test_clients_.end(); i++) {
            if (i->codec_client_ == codec_client) {
              onClientDestroy();
              test_clients_.erase(i);
              return;
            }
          }
        },
        Upstream::makeTestHost(cluster, "tcp://127.0.0.1:9000"), *test_client.client_dispatcher_);
    EXPECT_CALL(mock_dispatcher_, createClientConnection_(_, _, _, _))
        .WillOnce(Return(test_client.connection_));
    EXPECT_CALL(*this, createCodecClient_()).WillOnce(Return(test_client.codec_client_));
    EXPECT_CALL(*test_client.connect_timer_, enableTimer(_, _));
    ON_CALL(*test_client.codec_, protocol()).WillByDefault(Return(protocol));
  }

  void expectEnableUpstreamReady() {
    EXPECT_FALSE(upstream_ready_enabled_);
    EXPECT_CALL(*mock_upstream_ready_timer_, enableTimer(_, _)).Times(1).RetiresOnSaturation();
  }

  void expectAndRunUpstreamReady() {
    EXPECT_TRUE(upstream_ready_enabled_);
    mock_upstream_ready_timer_->invokeCallback();
    EXPECT_FALSE(upstream_ready_enabled_);
  }

  Api::ApiPtr api_;
  Event::MockDispatcher& mock_dispatcher_;
  NiceMock<Event::MockTimer>* mock_upstream_ready_timer_;
  std::vector<TestCodecClient> test_clients_;
};

/**
 * Test fixture for all connection pool tests.
 */
class Http1ConnPoolImplTest : public testing::Test {
public:
  Http1ConnPoolImplTest()
      : upstream_ready_timer_(new NiceMock<Event::MockTimer>(&dispatcher_)),
        conn_pool_(dispatcher_, cluster_, upstream_ready_timer_) {}

  ~Http1ConnPoolImplTest() override {
    EXPECT_TRUE(TestUtility::gaugesZeroed(cluster_->stats_store_.gauges()));
  }

  NiceMock<Event::MockDispatcher> dispatcher_;
  std::shared_ptr<Upstream::MockClusterInfo> cluster_{new NiceMock<Upstream::MockClusterInfo>()};
  NiceMock<Event::MockTimer>* upstream_ready_timer_;
  ConnPoolImplForTest conn_pool_;
  NiceMock<Runtime::MockLoader> runtime_;
};

/**
 * Helper for dealing with an active test request.
 */
struct ActiveTestRequest {
  enum class Type { Pending, CreateConnection, Immediate };

  ActiveTestRequest(Http1ConnPoolImplTest& parent, size_t client_index, Type type)
      : parent_(parent), client_index_(client_index) {
    uint64_t current_rq_total = parent_.cluster_->stats_.upstream_rq_total_.value();
    if (type == Type::CreateConnection) {
      parent.conn_pool_.expectClientCreate();
    }

    if (type == Type::Immediate) {
      expectNewStream();
    }

    handle_ = parent.conn_pool_.newStream(outer_decoder_, callbacks_);

    if (type == Type::Immediate) {
      EXPECT_EQ(nullptr, handle_);
    } else {
      EXPECT_NE(nullptr, handle_);
    }

    if (type == Type::CreateConnection) {
      EXPECT_CALL(*parent_.conn_pool_.test_clients_[client_index_].connect_timer_, disableTimer());
      expectNewStream();
      parent.conn_pool_.test_clients_[client_index_].connection_->raiseEvent(
          Network::ConnectionEvent::Connected);
    }
    if (type != Type::Pending) {
      EXPECT_EQ(current_rq_total + 1, parent_.cluster_->stats_.upstream_rq_total_.value());
    }
  }

  void completeResponse(bool with_body) {
    // Test additional metric writes also.
    Http::HeaderMapPtr response_headers(
        new TestHeaderMapImpl{{":status", "200"}, {"x-envoy-upstream-canary", "true"}});

    inner_decoder_->decodeHeaders(std::move(response_headers), !with_body);
    if (with_body) {
      Buffer::OwnedImpl data;
      inner_decoder_->decodeData(data, true);
    }
  }

  void expectNewStream() {
    EXPECT_CALL(*parent_.conn_pool_.test_clients_[client_index_].codec_, newStream(_))
        .WillOnce(DoAll(SaveArgAddress(&inner_decoder_), ReturnRef(request_encoder_)));
    EXPECT_CALL(callbacks_.pool_ready_, ready());
  }

  void startRequest() { callbacks_.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true); }

  Http1ConnPoolImplTest& parent_;
  size_t client_index_;
  NiceMock<Http::MockStreamDecoder> outer_decoder_;
  Http::ConnectionPool::Cancellable* handle_{};
  NiceMock<Http::MockStreamEncoder> request_encoder_;
  Http::StreamDecoder* inner_decoder_{};
  ConnPoolCallbacks callbacks_;
};

/**
 * Verify that the pool's host is a member of the cluster the pool was constructed with.
 */
TEST_F(Http1ConnPoolImplTest, Host) { EXPECT_EQ(cluster_.get(), &conn_pool_.host()->cluster()); }

/**
 * Verify that connections are drained when requested.
 */
TEST_F(Http1ConnPoolImplTest, DrainConnections) {
  cluster_->resetResourceManager(2, 1024, 1024, 1, 1);
  InSequence s;

  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::CreateConnection);
  r1.startRequest();

  ActiveTestRequest r2(*this, 1, ActiveTestRequest::Type::CreateConnection);
  r2.startRequest();

  r1.completeResponse(false);

  // This will destroy the ready client and set requests remaining to 1 on the busy client.
  conn_pool_.drainConnections();
  EXPECT_CALL(conn_pool_, onClientDestroy());
  dispatcher_.clearDeferredDeleteList();

  // This will destroy the busy client when the response finishes.
  r2.completeResponse(false);
  EXPECT_CALL(conn_pool_, onClientDestroy());
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Test all timing stats are set.
 */
TEST_F(Http1ConnPoolImplTest, VerifyTimingStats) {
  EXPECT_CALL(cluster_->stats_store_,
              deliverHistogramToSinks(Property(&Stats::Metric::name, "upstream_cx_connect_ms"), _));
  EXPECT_CALL(cluster_->stats_store_,
              deliverHistogramToSinks(Property(&Stats::Metric::name, "upstream_cx_length_ms"), _));

  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::CreateConnection);
  r1.startRequest();
  r1.completeResponse(false);

  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Test that buffer limits are set.
 */
TEST_F(Http1ConnPoolImplTest, VerifyBufferLimits) {
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  EXPECT_CALL(*cluster_, perConnectionBufferLimitBytes()).WillOnce(Return(8192));
  EXPECT_CALL(*conn_pool_.test_clients_.back().connection_, setBufferLimits(8192));
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);
  EXPECT_NE(nullptr, handle);

  EXPECT_CALL(conn_pool_, onClientDestroy());
  EXPECT_CALL(callbacks.pool_failure_, ready());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Verify that canceling pending connections within the callback works.
 */
TEST_F(Http1ConnPoolImplTest, VerifyCancelInCallback) {
  Http::ConnectionPool::Cancellable* handle1{};
  // In this scenario, all connections must succeed, so when
  // one fails, the others are canceled.
  // Note: We rely on the fact that the implementation cancels the second request first,
  // to simplify the test.
  ConnPoolCallbacks callbacks1;
  EXPECT_CALL(callbacks1.pool_failure_, ready()).Times(0);
  ConnPoolCallbacks callbacks2;
  EXPECT_CALL(callbacks2.pool_failure_, ready()).WillOnce(Invoke([&]() -> void {
    handle1->cancel();
  }));

  NiceMock<Http::MockStreamDecoder> outer_decoder;
  // Create the first client.
  conn_pool_.expectClientCreate();
  handle1 = conn_pool_.newStream(outer_decoder, callbacks1);
  ASSERT_NE(nullptr, handle1);

  // Create the second client.
  Http::ConnectionPool::Cancellable* handle2 = conn_pool_.newStream(outer_decoder, callbacks2);
  ASSERT_NE(nullptr, handle2);

  // Simulate connection failure.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Tests a request that generates a new connection, completes, and then a second request that uses
 * the same connection.
 */
TEST_F(Http1ConnPoolImplTest, MultipleRequestAndResponse) {
  InSequence s;

  // Request 1 should kick off a new connection.
  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::CreateConnection);
  r1.startRequest();
  r1.completeResponse(false);

  // Request 2 should not.
  ActiveTestRequest r2(*this, 0, ActiveTestRequest::Type::Immediate);
  r2.startRequest();
  r2.completeResponse(true);

  // Cause the connection to go away.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Test when we overflow max pending requests.
 */
TEST_F(Http1ConnPoolImplTest, MaxPendingRequests) {
  cluster_->resetResourceManager(1, 1, 1024, 1, 1);

  EXPECT_EQ(0U, cluster_->circuit_breakers_stats_.rq_pending_open_.value());

  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);
  EXPECT_NE(nullptr, handle);

  NiceMock<Http::MockStreamDecoder> outer_decoder2;
  ConnPoolCallbacks callbacks2;
  EXPECT_CALL(callbacks2.pool_failure_, ready());
  Http::ConnectionPool::Cancellable* handle2 = conn_pool_.newStream(outer_decoder2, callbacks2);
  EXPECT_EQ(nullptr, handle2);

  EXPECT_EQ(1U, cluster_->circuit_breakers_stats_.rq_pending_open_.value());

  handle->cancel();

  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(1U, cluster_->stats_.upstream_rq_pending_overflow_.value());
}

/**
 * Tests a connection failure before a request is bound which should result in the pending request
 * getting purged.
 */
TEST_F(Http1ConnPoolImplTest, ConnectFailure) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);
  EXPECT_NE(nullptr, handle);

  EXPECT_CALL(callbacks.pool_failure_, ready());
  EXPECT_CALL(*conn_pool_.test_clients_[0].connect_timer_, disableTimer());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  EXPECT_CALL(conn_pool_, onClientDestroy());
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_connect_fail_.value());
  EXPECT_EQ(1U, cluster_->stats_.upstream_rq_pending_failure_eject_.value());
}

/**
 * Tests that connection creation time is recorded correctly even in cases where
 * there are multiple pending connection creation attempts to the same upstream.
 */
TEST_F(Http1ConnPoolImplTest, MeasureConnectTime) {
  constexpr uint64_t sleep1_ms = 20;
  constexpr uint64_t sleep2_ms = 10;
  constexpr uint64_t sleep3_ms = 5;
  Event::SimulatedTimeSystem simulated_time;

  // Allow concurrent creation of 2 upstream connections.
  cluster_->resetResourceManager(2, 1024, 1024, 1, 1);

  InSequence s;

  // Start the first connect attempt.
  conn_pool_.expectClientCreate();
  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::Pending);

  // Move time forward and start the second connect attempt.
  simulated_time.sleep(std::chrono::milliseconds(sleep1_ms));
  conn_pool_.expectClientCreate();
  ActiveTestRequest r2(*this, 1, ActiveTestRequest::Type::Pending);

  // Move time forward, signal that the first connect completed and verify the time to connect.
  uint64_t upstream_cx_connect_ms1 = 0;
  simulated_time.sleep(std::chrono::milliseconds(sleep2_ms));
  EXPECT_CALL(*conn_pool_.test_clients_[0].connect_timer_, disableTimer());
  EXPECT_CALL(cluster_->stats_store_,
              deliverHistogramToSinks(Property(&Stats::Metric::name, "upstream_cx_connect_ms"), _))
      .WillOnce(SaveArg<1>(&upstream_cx_connect_ms1));
  r1.expectNewStream();
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);
  EXPECT_EQ(sleep1_ms + sleep2_ms, upstream_cx_connect_ms1);

  // Move time forward, signal that the second connect completed and verify the time to connect.
  uint64_t upstream_cx_connect_ms2 = 0;
  simulated_time.sleep(std::chrono::milliseconds(sleep3_ms));
  EXPECT_CALL(*conn_pool_.test_clients_[1].connect_timer_, disableTimer());
  EXPECT_CALL(cluster_->stats_store_,
              deliverHistogramToSinks(Property(&Stats::Metric::name, "upstream_cx_connect_ms"), _))
      .WillOnce(SaveArg<1>(&upstream_cx_connect_ms2));
  r2.expectNewStream();
  conn_pool_.test_clients_[1].connection_->raiseEvent(Network::ConnectionEvent::Connected);
  EXPECT_EQ(sleep2_ms + sleep3_ms, upstream_cx_connect_ms2);

  // Cleanup, cause the connections to go away.
  for (auto& test_client : conn_pool_.test_clients_) {
    EXPECT_CALL(
        cluster_->stats_store_,
        deliverHistogramToSinks(Property(&Stats::Metric::name, "upstream_cx_length_ms"), _));
    EXPECT_CALL(conn_pool_, onClientDestroy());
    test_client.connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  }
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Tests a connect timeout. Also test that we can add a new request during ejection processing.
 */
TEST_F(Http1ConnPoolImplTest, ConnectTimeout) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder1;
  ConnPoolCallbacks callbacks1;
  conn_pool_.expectClientCreate();
  EXPECT_NE(nullptr, conn_pool_.newStream(outer_decoder1, callbacks1));

  NiceMock<Http::MockStreamDecoder> outer_decoder2;
  ConnPoolCallbacks callbacks2;
  EXPECT_CALL(callbacks1.pool_failure_, ready()).WillOnce(Invoke([&]() -> void {
    conn_pool_.expectClientCreate();
    EXPECT_NE(nullptr, conn_pool_.newStream(outer_decoder2, callbacks2));
  }));

  conn_pool_.test_clients_[0].connect_timer_->invokeCallback();

  EXPECT_CALL(callbacks2.pool_failure_, ready());
  conn_pool_.test_clients_[1].connect_timer_->invokeCallback();

  EXPECT_CALL(conn_pool_, onClientDestroy()).Times(2);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(0U, cluster_->stats_.upstream_rq_total_.value());
  EXPECT_EQ(2U, cluster_->stats_.upstream_cx_connect_fail_.value());
  EXPECT_EQ(2U, cluster_->stats_.upstream_cx_connect_timeout_.value());
}

/**
 * Test cancelling before the request is bound to a connection.
 */
TEST_F(Http1ConnPoolImplTest, CancelBeforeBound) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);
  EXPECT_NE(nullptr, handle);

  handle->cancel();
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Cause the connection to go away.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Test an upstream disconnection while there is a bound request.
 */
TEST_F(Http1ConnPoolImplTest, DisconnectWhileBound) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);
  EXPECT_NE(nullptr, handle);

  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());

  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // We should get a reset callback when the connection disconnects.
  Http::MockStreamCallbacks stream_callbacks;
  EXPECT_CALL(stream_callbacks, onResetStream(StreamResetReason::ConnectionTermination, _));
  request_encoder.getStream().addCallbacks(stream_callbacks);

  // Kill the connection while it has an active request.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Test that we correctly handle reaching max connections.
 */
TEST_F(Http1ConnPoolImplTest, MaxConnections) {
  InSequence s;

  EXPECT_EQ(0U, cluster_->circuit_breakers_stats_.cx_open_.value());

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder1;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder1, callbacks);

  EXPECT_NE(nullptr, handle);

  // Request 2 should not kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder2;
  ConnPoolCallbacks callbacks2;
  handle = conn_pool_.newStream(outer_decoder2, callbacks2);
  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_overflow_.value());
  EXPECT_EQ(1U, cluster_->circuit_breakers_stats_.cx_open_.value());

  EXPECT_NE(nullptr, handle);

  // Connect event will bind to request 1.
  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());

  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Finishing request 1 will immediately bind to request 2.
  conn_pool_.expectEnableUpstreamReady();
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks2.pool_ready_, ready());

  callbacks.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);
  Http::HeaderMapPtr response_headers(new TestHeaderMapImpl{{":status", "200"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);

  conn_pool_.expectAndRunUpstreamReady();
  callbacks2.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);
  // N.B. clang_tidy insists that we use std::make_unique which can not infer std::initialize_list.
  response_headers = std::make_unique<TestHeaderMapImpl>(
      std::initializer_list<std::pair<std::string, std::string>>{{":status", "200"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);

  // Cause the connection to go away.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Test when upstream closes connection without 'connection: close' like
 * https://github.com/envoyproxy/envoy/pull/2715
 */
TEST_F(Http1ConnPoolImplTest, ConnectionCloseWithoutHeader) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder1;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder1, callbacks);

  EXPECT_NE(nullptr, handle);

  // Request 2 should not kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder2;
  ConnPoolCallbacks callbacks2;
  handle = conn_pool_.newStream(outer_decoder2, callbacks2);
  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_overflow_.value());

  EXPECT_NE(nullptr, handle);

  // Connect event will bind to request 1.
  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());

  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Finishing request 1 will schedule binding the connection to request 2.
  conn_pool_.expectEnableUpstreamReady();

  callbacks.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);
  Http::HeaderMapPtr response_headers(new TestHeaderMapImpl{{":status", "200"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);

  // Cause the connection to go away.
  conn_pool_.expectClientCreate();
  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();

  conn_pool_.expectAndRunUpstreamReady();

  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks2.pool_ready_, ready());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);

  callbacks2.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);
  // N.B. clang_tidy insists that we use std::make_unique which can not infer std::initialize_list.
  response_headers = std::make_unique<TestHeaderMapImpl>(
      std::initializer_list<std::pair<std::string, std::string>>{{":status", "200"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);

  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();
}

/**
 * Test when upstream sends us 'connection: close'
 */
TEST_F(Http1ConnPoolImplTest, ConnectionCloseHeader) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);

  EXPECT_NE(nullptr, handle);

  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());

  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);
  callbacks.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);

  // Response with 'connection: close' which should cause the connection to go away.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  Http::HeaderMapPtr response_headers(
      new TestHeaderMapImpl{{":status", "200"}, {"Connection", "Close"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(0U, cluster_->stats_.upstream_cx_destroy_with_active_rq_.value());
}

/**
 * Test when upstream sends us 'proxy-connection: close'
 */
TEST_F(Http1ConnPoolImplTest, ProxyConnectionCloseHeader) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);

  EXPECT_NE(nullptr, handle);

  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());

  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);
  callbacks.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);

  // Response with 'proxy-connection: close' which should cause the connection to go away.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  Http::HeaderMapPtr response_headers(
      new TestHeaderMapImpl{{":status", "200"}, {"Proxy-Connection", "Close"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(0U, cluster_->stats_.upstream_cx_destroy_with_active_rq_.value());
}

/**
 * Test when upstream is HTTP/1.0 and does not send 'connection: keep-alive'
 */
TEST_F(Http1ConnPoolImplTest, Http10NoConnectionKeepAlive) {
  InSequence s;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate(Protocol::Http10);
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);

  EXPECT_NE(nullptr, handle);

  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());

  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);
  callbacks.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);

  // Response without 'connection: keep-alive' which should cause the connection to go away.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  Http::HeaderMapPtr response_headers(
      new TestHeaderMapImpl{{":protocol", "HTTP/1.0"}, {":status", "200"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(0U, cluster_->stats_.upstream_cx_destroy_with_active_rq_.value());
}

/**
 * Test when we reach max requests per connection.
 */
TEST_F(Http1ConnPoolImplTest, MaxRequestsPerConnection) {
  InSequence s;

  cluster_->max_requests_per_connection_ = 1;

  // Request 1 should kick off a new connection.
  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);

  EXPECT_NE(nullptr, handle);

  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());

  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);
  callbacks.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);

  // Response with 'connection: close' which should cause the connection to go away.
  EXPECT_CALL(conn_pool_, onClientDestroy());
  Http::HeaderMapPtr response_headers(new TestHeaderMapImpl{{":status", "200"}});
  inner_decoder->decodeHeaders(std::move(response_headers), true);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(0U, cluster_->stats_.upstream_cx_destroy_with_active_rq_.value());
  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_max_requests_.value());
}

TEST_F(Http1ConnPoolImplTest, ConcurrentConnections) {
  cluster_->resetResourceManager(2, 1024, 1024, 1, 1);
  InSequence s;

  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::CreateConnection);
  r1.startRequest();

  ActiveTestRequest r2(*this, 1, ActiveTestRequest::Type::CreateConnection);
  r2.startRequest();

  ActiveTestRequest r3(*this, 0, ActiveTestRequest::Type::Pending);

  // Finish r1, which gets r3 going.
  conn_pool_.expectEnableUpstreamReady();
  r3.expectNewStream();

  r1.completeResponse(false);
  conn_pool_.expectAndRunUpstreamReady();
  r3.startRequest();
  EXPECT_EQ(3U, cluster_->stats_.upstream_rq_total_.value());

  r2.completeResponse(false);
  r3.completeResponse(false);

  // Disconnect both clients.
  EXPECT_CALL(conn_pool_, onClientDestroy()).Times(2);
  conn_pool_.test_clients_[1].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(2U, cluster_->stats_.upstream_cx_destroy_.value());
  EXPECT_EQ(2U, cluster_->stats_.upstream_cx_destroy_remote_.value());
}

TEST_F(Http1ConnPoolImplTest, DrainCallback) {
  InSequence s;
  ReadyWatcher drained;

  EXPECT_CALL(drained, ready());
  conn_pool_.addDrainedCallback([&]() -> void { drained.ready(); });

  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::CreateConnection);
  ActiveTestRequest r2(*this, 0, ActiveTestRequest::Type::Pending);
  r2.handle_->cancel();
  EXPECT_EQ(1U, cluster_->stats_.upstream_rq_total_.value());

  EXPECT_CALL(drained, ready());
  r1.startRequest();
  r1.completeResponse(false);

  EXPECT_CALL(conn_pool_, onClientDestroy());
  dispatcher_.clearDeferredDeleteList();
}

// Test draining a connection pool that has a pending connection.
TEST_F(Http1ConnPoolImplTest, DrainWhileConnecting) {
  InSequence s;
  ReadyWatcher drained;

  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);
  EXPECT_NE(nullptr, handle);

  conn_pool_.addDrainedCallback([&]() -> void { drained.ready(); });
  handle->cancel();
  EXPECT_CALL(*conn_pool_.test_clients_[0].connection_,
              close(Network::ConnectionCloseType::NoFlush));
  EXPECT_CALL(drained, ready());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);

  EXPECT_CALL(conn_pool_, onClientDestroy());
  dispatcher_.clearDeferredDeleteList();
}

TEST_F(Http1ConnPoolImplTest, RemoteCloseToCompleteResponse) {
  InSequence s;

  NiceMock<Http::MockStreamDecoder> outer_decoder;
  ConnPoolCallbacks callbacks;
  conn_pool_.expectClientCreate();
  Http::ConnectionPool::Cancellable* handle = conn_pool_.newStream(outer_decoder, callbacks);
  EXPECT_NE(nullptr, handle);

  NiceMock<Http::MockStreamEncoder> request_encoder;
  Http::StreamDecoder* inner_decoder;
  EXPECT_CALL(*conn_pool_.test_clients_[0].connect_timer_, disableTimer());
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, newStream(_))
      .WillOnce(DoAll(SaveArgAddress(&inner_decoder), ReturnRef(request_encoder)));
  EXPECT_CALL(callbacks.pool_ready_, ready());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::Connected);

  callbacks.outer_encoder_->encodeHeaders(TestHeaderMapImpl{}, true);

  inner_decoder->decodeHeaders(HeaderMapPtr{new HeaderMapImpl{}}, false);
  Buffer::OwnedImpl dummy_data("12345");
  inner_decoder->decodeData(dummy_data, false);

  Buffer::OwnedImpl empty_data;
  EXPECT_CALL(*conn_pool_.test_clients_[0].codec_, dispatch(BufferEqual(&empty_data)))
      .WillOnce(Invoke([&](Buffer::Instance& data) -> void {
        // Simulate the onResponseComplete call to decodeData since dispatch is mocked out.
        inner_decoder->decodeData(data, true);
      }));

  EXPECT_CALL(*conn_pool_.test_clients_[0].connection_,
              close(Network::ConnectionCloseType::NoFlush));
  EXPECT_CALL(conn_pool_, onClientDestroy());
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_destroy_.value());
  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_destroy_remote_.value());
}

TEST_F(Http1ConnPoolImplTest, NoActiveConnectionsByDefault) {
  EXPECT_FALSE(conn_pool_.hasActiveConnections());
}

TEST_F(Http1ConnPoolImplTest, ActiveRequestHasActiveConnectionsTrue) {
  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::CreateConnection);
  r1.startRequest();

  EXPECT_TRUE(conn_pool_.hasActiveConnections());

  // cleanup
  r1.completeResponse(false);
  conn_pool_.drainConnections();
  EXPECT_CALL(conn_pool_, onClientDestroy());
  dispatcher_.clearDeferredDeleteList();
}

TEST_F(Http1ConnPoolImplTest, ResponseCompletedConnectionReadyNoActiveConnections) {
  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::CreateConnection);
  r1.startRequest();
  r1.completeResponse(false);

  EXPECT_FALSE(conn_pool_.hasActiveConnections());

  conn_pool_.drainConnections();
  EXPECT_CALL(conn_pool_, onClientDestroy());
  dispatcher_.clearDeferredDeleteList();
}

TEST_F(Http1ConnPoolImplTest, PendingRequestIsConsideredActive) {
  conn_pool_.expectClientCreate();
  ActiveTestRequest r1(*this, 0, ActiveTestRequest::Type::Pending);

  EXPECT_TRUE(conn_pool_.hasActiveConnections());

  EXPECT_CALL(conn_pool_, onClientDestroy());
  r1.handle_->cancel();
  EXPECT_EQ(0U, cluster_->stats_.upstream_rq_total_.value());
  conn_pool_.drainConnections();
  conn_pool_.test_clients_[0].connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  dispatcher_.clearDeferredDeleteList();

  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_destroy_.value());
  EXPECT_EQ(1U, cluster_->stats_.upstream_cx_destroy_remote_.value());
}

} // namespace
} // namespace Http1
} // namespace Http
} // namespace Envoy
