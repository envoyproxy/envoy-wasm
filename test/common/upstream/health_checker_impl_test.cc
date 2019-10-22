#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "envoy/api/v2/core/health_check.pb.validate.h"

#include "common/buffer/buffer_impl.h"
#include "common/buffer/zero_copy_input_stream_impl.h"
#include "common/grpc/common.h"
#include "common/http/headers.h"
#include "common/json/json_loader.h"
#include "common/network/utility.h"
#include "common/protobuf/utility.h"
#include "common/upstream/health_checker_impl.h"
#include "common/upstream/upstream_impl.h"

#include "test/common/http/common.h"
#include "test/common/upstream/utility.h"
#include "test/mocks/access_log/mocks.h"
#include "test/mocks/api/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/protobuf/mocks.h"
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
using testing::InvokeWithoutArgs;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::SaveArg;

namespace Envoy {
namespace Upstream {
namespace {

envoy::api::v2::core::HealthCheck createGrpcHealthCheckConfig() {
  envoy::api::v2::core::HealthCheck health_check;
  health_check.mutable_timeout()->set_seconds(1);
  health_check.mutable_interval()->set_seconds(1);
  health_check.mutable_unhealthy_threshold()->set_value(2);
  health_check.mutable_healthy_threshold()->set_value(2);
  health_check.mutable_grpc_health_check();
  return health_check;
}

TEST(HealthCheckerFactoryTest, GrpcHealthCheckHTTP2NotConfiguredException) {
  NiceMock<Upstream::MockClusterMockPrioritySet> cluster;
  EXPECT_CALL(*cluster.info_, features()).WillRepeatedly(Return(0));

  Runtime::MockLoader runtime;
  Runtime::MockRandomGenerator random;
  Event::MockDispatcher dispatcher;
  AccessLog::MockAccessLogManager log_manager;
  NiceMock<ProtobufMessage::MockValidationVisitor> validation_visitor;
  Api::MockApi api;

  EXPECT_THROW_WITH_MESSAGE(
      HealthCheckerFactory::create(createGrpcHealthCheckConfig(), cluster, runtime, random,
                                   dispatcher, log_manager, validation_visitor, api),
      EnvoyException, "fake_cluster cluster must support HTTP/2 for gRPC healthchecking");
}

TEST(HealthCheckerFactoryTest, CreateGrpc) {

  NiceMock<Upstream::MockClusterMockPrioritySet> cluster;
  EXPECT_CALL(*cluster.info_, features())
      .WillRepeatedly(Return(Upstream::ClusterInfo::Features::HTTP2));

  Runtime::MockLoader runtime;
  Runtime::MockRandomGenerator random;
  Event::MockDispatcher dispatcher;
  AccessLog::MockAccessLogManager log_manager;
  NiceMock<ProtobufMessage::MockValidationVisitor> validation_visitor;
  Api::MockApi api;

  EXPECT_NE(nullptr, dynamic_cast<GrpcHealthCheckerImpl*>(
                         HealthCheckerFactory::create(createGrpcHealthCheckConfig(), cluster,
                                                      runtime, random, dispatcher, log_manager,
                                                      validation_visitor, api)
                             .get()));
}

class TestHttpHealthCheckerImpl : public HttpHealthCheckerImpl {
public:
  using HttpHealthCheckerImpl::HttpHealthCheckerImpl;

  Http::CodecClient* createCodecClient(Upstream::Host::CreateConnectionData& conn_data) override {
    return createCodecClient_(conn_data);
  };

  // HttpHealthCheckerImpl
  MOCK_METHOD1(createCodecClient_, Http::CodecClient*(Upstream::Host::CreateConnectionData&));

  Http::CodecClient::Type codecClientType() { return codec_client_type_; }
};

class HttpHealthCheckerImplTest : public testing::Test {
public:
  struct TestSession {
    Event::MockTimer* interval_timer_{};
    Event::MockTimer* timeout_timer_{};
    Http::MockClientConnection* codec_{};
    Stats::IsolatedStoreImpl stats_store_;
    Network::MockClientConnection* client_connection_{};
    NiceMock<Http::MockStreamEncoder> request_encoder_;
    Http::StreamDecoder* stream_response_callbacks_{};
  };

  using TestSessionPtr = std::unique_ptr<TestSession>;
  using HostWithHealthCheckMap =
      std::unordered_map<std::string, const envoy::api::v2::endpoint::Endpoint::HealthCheckConfig>;

  HttpHealthCheckerImplTest()
      : cluster_(new NiceMock<MockClusterMockPrioritySet>()),
        event_logger_(new MockHealthCheckEventLogger()) {}

  void setupNoServiceValidationHCWithHttp2() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
      codec_client_type: Http2
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupInitialJitter() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    initial_jitter: 5s
    interval_jitter_percent: 40
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupIntervalJitterPercent() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter_percent: 40
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupNoServiceValidationHC() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupNoServiceValidationHCOneUnhealthy() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 1
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupNoServiceValidationHCAlwaysLogFailure() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    always_log_health_check_failures: true
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupNoServiceValidationNoReuseConnectionHC() {
    std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    reuse_connection: false
    http_health_check:
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupHealthCheckIntervalOverridesHC() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    unhealthy_interval: 2s
    unhealthy_edge_interval: 3s
    healthy_edge_interval: 4s
    no_traffic_interval: 5s
    interval_jitter: 0s
    unhealthy_threshold: 3
    healthy_threshold: 3
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupServiceValidationHC() {
    std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupServiceValidationWithCustomHostValueHC(const std::string& host) {
    std::string yaml = fmt::format(R"EOF(
    timeout: 1s
    interval: 1s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
      host: {0}
    )EOF",
                                   host);

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  const envoy::api::v2::endpoint::Endpoint::HealthCheckConfig
  makeHealthCheckConfig(const uint32_t port_value) {
    envoy::api::v2::endpoint::Endpoint::HealthCheckConfig config;
    config.set_port_value(port_value);
    return config;
  }

  void appendTestHosts(std::shared_ptr<MockClusterMockPrioritySet> cluster,
                       const HostWithHealthCheckMap& hosts, const std::string& protocol = "tcp://",
                       const uint32_t priority = 0) {
    for (const auto& host : hosts) {
      cluster->prioritySet().getMockHostSet(priority)->hosts_.emplace_back(
          makeTestHost(cluster->info_, fmt::format("{}{}", protocol, host.first), host.second));
    }
  }

  void setupServiceValidationWithAdditionalHeaders() {
    std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
      host: "www.envoyproxy.io"
      request_headers_to_add:
        - header:
            key: x-envoy-ok
            value: ok
        - header:
            key: x-envoy-cool
            value: cool
        - header:
            key: x-envoy-awesome
            value: awesome
        # The following entry replaces the current user-agent.
        - header:
            key: user-agent
            value: CoolEnvoy/HC
          append: false
        - header:
            key: x-protocol
            value: "%PROTOCOL%"
        - header:
            key: x-upstream-metadata
            value: "%UPSTREAM_METADATA([\"namespace\", \"key\"])%"
        - header:
            key: x-downstream-remote-address-without-port
            value: "%DOWNSTREAM_REMOTE_ADDRESS_WITHOUT_PORT%"
        - header:
            key: x-downstream-local-address
            value: "%DOWNSTREAM_LOCAL_ADDRESS%"
        - header:
            key: x-downstream-local-address-without-port
            value: "%DOWNSTREAM_LOCAL_ADDRESS_WITHOUT_PORT%"
        - header:
            key: x-start-time
            value: "%START_TIME(%s.%9f)%"
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupServiceValidationWithoutUserAgent() {
    std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
      host: "www.envoyproxy.io"
      # The following entry removes the default "user-agent" header.
      request_headers_to_remove: ["user-agent"]
    )EOF";

    health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void expectSessionCreate(const HostWithHealthCheckMap& health_check_map) {
    // Expectations are in LIFO order.
    TestSessionPtr new_test_session(new TestSession());
    test_sessions_.emplace_back(std::move(new_test_session));
    TestSession& test_session = *test_sessions_.back();
    test_session.timeout_timer_ = new Event::MockTimer(&dispatcher_);
    test_session.interval_timer_ = new Event::MockTimer(&dispatcher_);
    expectClientCreate(test_sessions_.size() - 1, health_check_map);
  }

  void expectClientCreate(size_t index, const HostWithHealthCheckMap& health_check_map) {
    TestSession& test_session = *test_sessions_[index];
    test_session.codec_ = new NiceMock<Http::MockClientConnection>();
    test_session.client_connection_ = new NiceMock<Network::MockClientConnection>();
    connection_index_.push_back(index);
    codec_index_.push_back(index);

    EXPECT_CALL(dispatcher_, createClientConnection_(_, _, _, _))
        .Times(testing::AnyNumber())
        .WillRepeatedly(InvokeWithoutArgs([&]() -> Network::ClientConnection* {
          uint32_t index = connection_index_.front();
          connection_index_.pop_front();
          return test_sessions_[index]->client_connection_;
        }));
    EXPECT_CALL(*health_checker_, createCodecClient_(_))
        .WillRepeatedly(
            Invoke([&](Upstream::Host::CreateConnectionData& conn_data) -> Http::CodecClient* {
              if (!health_check_map.empty()) {
                const auto& health_check_config =
                    health_check_map.at(conn_data.host_description_->address()->asString());
                // To make sure health checker checks the correct port.
                EXPECT_EQ(health_check_config.port_value(),
                          conn_data.host_description_->healthCheckAddress()->ip()->port());
              }
              uint32_t index = codec_index_.front();
              codec_index_.pop_front();
              TestSession& test_session = *test_sessions_[index];
              std::shared_ptr<Upstream::MockClusterInfo> cluster{
                  new NiceMock<Upstream::MockClusterInfo>()};
              Event::MockDispatcher dispatcher_;
              return new CodecClientForTest(
                  Http::CodecClient::Type::HTTP1, std::move(conn_data.connection_),
                  test_session.codec_, nullptr,
                  Upstream::makeTestHost(cluster, "tcp://127.0.0.1:9000"), dispatcher_);
            }));
  }

  void expectStreamCreate(size_t index) {
    test_sessions_[index]->request_encoder_.stream_.callbacks_.clear();
    EXPECT_CALL(*test_sessions_[index]->codec_, newStream(_))
        .WillOnce(DoAll(SaveArgAddress(&test_sessions_[index]->stream_response_callbacks_),
                        ReturnRef(test_sessions_[index]->request_encoder_)));
  }

  void respond(size_t index, const std::string& code, bool conn_close, bool proxy_close = false,
               bool body = false, bool trailers = false,
               const absl::optional<std::string>& service_cluster = absl::optional<std::string>(),
               bool degraded = false) {
    std::unique_ptr<Http::TestHeaderMapImpl> response_headers(
        new Http::TestHeaderMapImpl{{":status", code}});

    if (degraded) {
      response_headers->insertEnvoyDegraded().value(1);
    }

    if (service_cluster) {
      response_headers->addCopy(Http::Headers::get().EnvoyUpstreamHealthCheckedCluster,
                                service_cluster.value());
    }
    if (conn_close) {
      response_headers->addCopy("connection", "close");
    }
    if (proxy_close) {
      response_headers->addCopy("proxy-connection", "close");
    }

    test_sessions_[index]->stream_response_callbacks_->decodeHeaders(std::move(response_headers),
                                                                     !body && !trailers);
    if (body) {
      Buffer::OwnedImpl response_data;
      test_sessions_[index]->stream_response_callbacks_->decodeData(response_data, !trailers);
    }

    if (trailers) {
      test_sessions_[index]->stream_response_callbacks_->decodeTrailers(
          Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{"some", "trailer"}}});
    }
  }

  void expectSessionCreate() { expectSessionCreate(health_checker_map_); }
  void expectClientCreate(size_t index) { expectClientCreate(index, health_checker_map_); }

  void expectSuccessStartFailedFailFirst(
      const absl::optional<std::string>& health_checked_cluster = absl::optional<std::string>()) {
    cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
        makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
    cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagSet(
        Host::HealthFlag::FAILED_ACTIVE_HC);
    expectSessionCreate();
    expectStreamCreate(0);
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
    health_checker_->start();

    // Test that failing first disables fast success.
    EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
    EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
    EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
    respond(0, "503", false, false, false, false, health_checked_cluster);
    EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
        Host::HealthFlag::FAILED_ACTIVE_HC));
    EXPECT_EQ(Host::Health::Unhealthy,
              cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
    expectStreamCreate(0);
    test_sessions_[0]->interval_timer_->invokeCallback();

    EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
    EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
    respond(0, "200", false, false, false, false, health_checked_cluster);
    EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
        Host::HealthFlag::FAILED_ACTIVE_HC));
    EXPECT_EQ(Host::Health::Unhealthy,
              cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
    expectStreamCreate(0);
    test_sessions_[0]->interval_timer_->invokeCallback();

    EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
    EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
    EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
    respond(0, "200", false, false, false, false, health_checked_cluster);
    EXPECT_EQ(Host::Health::Healthy,
              cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
  }

  MOCK_METHOD2(onHostStatus, void(HostSharedPtr host, HealthTransition changed_state));

  std::shared_ptr<MockClusterMockPrioritySet> cluster_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  std::vector<TestSessionPtr> test_sessions_;
  std::shared_ptr<TestHttpHealthCheckerImpl> health_checker_;
  NiceMock<Runtime::MockLoader> runtime_;
  NiceMock<Runtime::MockRandomGenerator> random_;
  MockHealthCheckEventLogger* event_logger_{};
  std::list<uint32_t> connection_index_{};
  std::list<uint32_t> codec_index_{};
  const HostWithHealthCheckMap health_checker_map_{};
};

TEST_F(HttpHealthCheckerImplTest, Success) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, Degraded) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed)).Times(2);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillRepeatedly(Return(45000));

  // We start off as healthy, and should go degraded after receiving the degraded health response.
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*event_logger_, logDegraded(_, _));
  respond(0, "200", false, false, true, false, {}, true);
  EXPECT_EQ(Host::Health::Degraded, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  // Then, after receiving a regular health check response we should go back to healthy.
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->interval_timer_->invokeCallback();
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*event_logger_, logNoLongerDegraded(_, _));
  respond(0, "200", false, false, true, false, {}, false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessIntervalJitter) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(testing::AnyNumber());

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  for (int i = 0; i < 50000; i += 239) {
    EXPECT_CALL(random_, random()).WillOnce(Return(i));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
    expectStreamCreate(0);
    test_sessions_[0]->interval_timer_->invokeCallback();
    // the jitter is 1000ms here
    EXPECT_CALL(*test_sessions_[0]->interval_timer_,
                enableTimer(std::chrono::milliseconds(5000 + i % 1000), _));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
    respond(0, "200", false, false, true, true);
  }
}

TEST_F(HttpHealthCheckerImplTest, InitialJitterNoTraffic) {
  setupInitialJitter();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(testing::AnyNumber());

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  for (int i = 0; i < 2; i += 1) {
    EXPECT_CALL(random_, random()).WillOnce(Return(i));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
    expectStreamCreate(0);
    test_sessions_[0]->interval_timer_->invokeCallback();
    // the jitter is 40% of 5000, so should be 2000
    EXPECT_CALL(*test_sessions_[0]->interval_timer_,
                enableTimer(std::chrono::milliseconds(5000 + i % 2000), _));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
    respond(0, "200", false, false, true, true);
  }
}

TEST_F(HttpHealthCheckerImplTest, SuccessIntervalJitterPercentNoTraffic) {
  setupIntervalJitterPercent();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(testing::AnyNumber());

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  for (int i = 0; i < 50000; i += 239) {
    EXPECT_CALL(random_, random()).WillOnce(Return(i));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
    expectStreamCreate(0);
    test_sessions_[0]->interval_timer_->invokeCallback();
    // the jitter is 40% of 5000, so should be 2000
    EXPECT_CALL(*test_sessions_[0]->interval_timer_,
                enableTimer(std::chrono::milliseconds(5000 + i % 2000), _));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
    respond(0, "200", false, false, true, true);
  }
}

TEST_F(HttpHealthCheckerImplTest, SuccessIntervalJitterPercent) {
  setupIntervalJitterPercent();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(testing::AnyNumber());

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  for (int i = 0; i < 50000; i += 239) {
    EXPECT_CALL(random_, random()).WillOnce(Return(i));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
    expectStreamCreate(0);
    test_sessions_[0]->interval_timer_->invokeCallback();
    // the jitter is 40% of 1000, so should be 400
    EXPECT_CALL(*test_sessions_[0]->interval_timer_,
                enableTimer(std::chrono::milliseconds(1000 + i % 400), _));
    EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
    respond(0, "200", false, false, true, true);
  }
}

TEST_F(HttpHealthCheckerImplTest, SuccessWithSpurious100Continue) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());

  std::unique_ptr<Http::TestHeaderMapImpl> continue_headers(
      new Http::TestHeaderMapImpl{{":status", "100"}});
  test_sessions_[0]->stream_response_callbacks_->decode100ContinueHeaders(
      std::move(continue_headers));

  respond(0, "200", false, false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessWithSpuriousMetadata) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());

  std::unique_ptr<Http::MetadataMap> metadata_map(new Http::MetadataMap());
  metadata_map->insert(std::make_pair<std::string, std::string>("key", "value"));
  test_sessions_[0]->stream_response_callbacks_->decodeMetadata(std::move(metadata_map));

  respond(0, "200", false, false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

// Test host check success with multiple hosts.
TEST_F(HttpHealthCheckerImplTest, SuccessWithMultipleHosts) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(2);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80"),
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:81")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectSessionCreate();
  expectStreamCreate(1);
  EXPECT_CALL(*test_sessions_[1]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _)).Times(2);
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .Times(2)
      .WillRepeatedly(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*test_sessions_[1]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[1]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true);
  respond(1, "200", false, false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[1]->health());
}

// Test host check success with multiple hosts across multiple priorities.
TEST_F(HttpHealthCheckerImplTest, SuccessWithMultipleHostSets) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(2);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->prioritySet().getMockHostSet(1)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:81")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectSessionCreate();
  expectStreamCreate(1);
  EXPECT_CALL(*test_sessions_[1]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _)).Times(2);
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .Times(2)
      .WillRepeatedly(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*test_sessions_[1]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[1]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true);
  respond(1, "200", false, false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(1)->hosts_[0]->health());
}

// Validate that runtime settings can't force a zero lengthy retry duration (and hence livelock).
TEST_F(HttpHealthCheckerImplTest, ZeroRetryInterval) {
  const std::string host = "fake_cluster";
  const std::string path = "/healthcheck";
  const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 1s
    interval_jitter_percent: 40
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

  health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                      dispatcher_, runtime_, random_,
                                                      HealthCheckEventLoggerPtr(event_logger_)));
  health_checker_->addHostCheckCompleteCb(
      [this](HostSharedPtr host, HealthTransition changed_state) -> void {
        onHostStatus(host, changed_state);
      });

  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeHeaders(_, true))
      .WillOnce(Invoke([&](const Http::HeaderMap& headers, bool) {
        EXPECT_EQ(headers.Host()->value().getStringView(), host);
        EXPECT_EQ(headers.Path()->value().getStringView(), path);
        EXPECT_EQ(headers.Scheme()->value().getStringView(),
                  Http::Headers::get().SchemeValues.Http);
      }));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _)).WillOnce(Return(0));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _)).WillOnce(Return(0));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("locations-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessServiceCheck) {
  const std::string host = "fake_cluster";
  const std::string path = "/healthcheck";
  setupServiceValidationHC();
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeHeaders(_, true))
      .WillOnce(Invoke([&](const Http::HeaderMap& headers, bool) {
        EXPECT_EQ(headers.Host()->value().getStringView(), host);
        EXPECT_EQ(headers.Path()->value().getStringView(), path);
        EXPECT_EQ(headers.Scheme()->value().getStringView(),
                  Http::Headers::get().SchemeValues.Http);
      }));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("locations-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessServiceCheckWithCustomHostValue) {
  const std::string host = "www.envoyproxy.io";
  const std::string path = "/healthcheck";
  setupServiceValidationWithCustomHostValueHC(host);
  // requires non-empty `service_name` in config.
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeHeaders(_, true))
      .WillOnce(Invoke([&](const Http::HeaderMap& headers, bool) {
        EXPECT_EQ(headers.Host()->value().getStringView(), host);
        EXPECT_EQ(headers.Path()->value().getStringView(), path);
      }));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("locations-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessServiceCheckWithAdditionalHeaders) {
  const Http::LowerCaseString header_ok("x-envoy-ok");
  const Http::LowerCaseString header_cool("x-envoy-cool");
  const Http::LowerCaseString header_awesome("x-envoy-awesome");
  const Http::LowerCaseString upstream_metadata("x-upstream-metadata");
  const Http::LowerCaseString protocol("x-protocol");
  const Http::LowerCaseString downstream_remote_address_without_port(
      "x-downstream-remote-address-without-port");
  const Http::LowerCaseString downstream_local_address("x-downstream-local-address");
  const Http::LowerCaseString downstream_local_address_without_port(
      "x-downstream-local-address-without-port");
  const Http::LowerCaseString start_time("x-start-time");

  const std::string value_ok = "ok";
  const std::string value_cool = "cool";
  const std::string value_awesome = "awesome";

  const std::string value_user_agent = "CoolEnvoy/HC";
  const std::string value_upstream_metadata = "value";
  const std::string value_protocol = "HTTP/1.1";
  const std::string value_downstream_remote_address_without_port = "127.0.0.1";
  const std::string value_downstream_local_address = "127.0.0.1:0";
  const std::string value_downstream_local_address_without_port = "127.0.0.1";

  setupServiceValidationWithAdditionalHeaders();
  // requires non-empty `service_name` in config.
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);
  auto metadata = TestUtility::parseYaml<envoy::api::v2::core::Metadata>(
      R"EOF(
        filter_metadata:
          namespace:
            key: value
      )EOF");

  std::string current_start_time;
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80", metadata)};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeHeaders(_, true))
      .WillRepeatedly(Invoke([&](const Http::HeaderMap& headers, bool) {
        EXPECT_EQ(headers.get(header_ok)->value().getStringView(), value_ok);
        EXPECT_EQ(headers.get(header_cool)->value().getStringView(), value_cool);
        EXPECT_EQ(headers.get(header_awesome)->value().getStringView(), value_awesome);

        EXPECT_EQ(headers.UserAgent()->value().getStringView(), value_user_agent);
        EXPECT_EQ(headers.get(upstream_metadata)->value().getStringView(), value_upstream_metadata);

        EXPECT_EQ(headers.get(protocol)->value().getStringView(), value_protocol);
        EXPECT_EQ(headers.get(downstream_remote_address_without_port)->value().getStringView(),
                  value_downstream_remote_address_without_port);
        EXPECT_EQ(headers.get(downstream_local_address)->value().getStringView(),
                  value_downstream_local_address);
        EXPECT_EQ(headers.get(downstream_local_address_without_port)->value().getStringView(),
                  value_downstream_local_address_without_port);

        EXPECT_NE(headers.get(start_time)->value().getStringView(), current_start_time);
        current_start_time = std::string(headers.get(start_time)->value().getStringView());
      }));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("locations-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();
}

TEST_F(HttpHealthCheckerImplTest, SuccessServiceCheckWithoutUserAgent) {
  setupServiceValidationWithoutUserAgent();
  // requires non-empty `service_name` in config.
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);
  auto metadata = TestUtility::parseYaml<envoy::api::v2::core::Metadata>(
      R"EOF(
        filter_metadata:
          namespace:
            key: value
      )EOF");

  std::string current_start_time;
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80", metadata)};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeHeaders(_, true))
      .WillRepeatedly(Invoke(
          [&](const Http::HeaderMap& headers, bool) { EXPECT_EQ(headers.UserAgent(), nullptr); }));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("locations-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();
}

TEST_F(HttpHealthCheckerImplTest, ServiceDoesNotMatchFail) {
  setupServiceValidationHC();
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed)).Times(1);
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("api-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, ServiceNotPresentInResponseFail) {
  setupServiceValidationHC();
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed)).Times(1);
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true, false);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, ServiceCheckRuntimeOff) {
  setupServiceValidationHC();
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(false));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("api-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessStartFailedFailFirstServiceCheck) {
  setupNoServiceValidationHC();
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillRepeatedly(Return(true));
  absl::optional<std::string> health_checked_cluster("locations-production-iad");
  expectSuccessStartFailedFailFirst(health_checked_cluster);
}

TEST_F(HttpHealthCheckerImplTest, SuccessNoTraffic) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(5000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessStartFailedSuccessFirst) {
  setupNoServiceValidationHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagSet(
      Host::HealthFlag::FAILED_ACTIVE_HC);
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  // Test fast success immediately moves us to healthy.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed)).Times(1);
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, true));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _)).WillOnce(Return(500));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(500), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessStartFailedFailFirst) {
  setupNoServiceValidationHC();
  expectSuccessStartFailedFailFirst();
}

TEST_F(HttpHealthCheckerImplTest, SuccessStartFailedFailFirstLogError) {
  setupNoServiceValidationHCAlwaysLogFailure();
  expectSuccessStartFailedFailFirst();
}

// Verify that removal during a failure callback works.
TEST_F(HttpHealthCheckerImplTest, HttpFailRemoveHostInCallbackNoClose) {
  setupNoServiceValidationHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed))
      .WillOnce(Invoke([&](HostSharedPtr host, HealthTransition) {
        cluster_->prioritySet().getMockHostSet(0)->hosts_ = {};
        cluster_->prioritySet().runUpdateCallbacks(0, {}, {host});
      }));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _)).Times(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer()).Times(0);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  respond(0, "503", false);
}

// Verify that removal during a failure callback works with connection close.
TEST_F(HttpHealthCheckerImplTest, HttpFailRemoveHostInCallbackClose) {
  setupNoServiceValidationHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed))
      .WillOnce(Invoke([&](HostSharedPtr host, HealthTransition) {
        cluster_->prioritySet().getMockHostSet(0)->hosts_ = {};
        cluster_->prioritySet().runUpdateCallbacks(0, {}, {host});
      }));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _)).Times(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer()).Times(0);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  respond(0, "503", true);
}

TEST_F(HttpHealthCheckerImplTest, HttpFail) {
  setupNoServiceValidationHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  respond(0, "503", false);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::UNHEALTHY);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, HttpFailLogError) {
  setupNoServiceValidationHCAlwaysLogFailure();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  respond(0, "503", false);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::UNHEALTHY);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // logUnhealthy is called with first_check == false
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, false));
  respond(0, "503", false);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::UNHEALTHY);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, Disconnect) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending)).Times(1);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate(0);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(cluster_->prioritySet().getMockHostSet(0)->hosts_[0],
                                  HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, Timeout) {
  setupNoServiceValidationHCOneUnhealthy();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*test_sessions_[0]->client_connection_, close(_));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  test_sessions_[0]->timeout_timer_->invokeCallback();
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::TIMEOUT);
}

// Make sure that a timeout during a partial response works correctly.
TEST_F(HttpHealthCheckerImplTest, TimeoutThenSuccess) {
  setupNoServiceValidationHC();

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  // Do a response that is not complete but includes headers.
  std::unique_ptr<Http::TestHeaderMapImpl> response_headers(
      new Http::TestHeaderMapImpl{{":status", "200"}});
  test_sessions_[0]->stream_response_callbacks_->decodeHeaders(std::move(response_headers), false);

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*test_sessions_[0]->client_connection_, close(_));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate(0);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, TimeoutThenRemoteClose) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->client_connection_, close(_));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate(0);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::TIMEOUT);
}

TEST_F(HttpHealthCheckerImplTest, TimeoutAfterDisconnect) {
  setupNoServiceValidationHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _)).Times(2);
  health_checker_->start();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending)).Times(1);
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _)).Times(2);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  for (auto& session : test_sessions_) {
    session->client_connection_->close(Network::ConnectionCloseType::NoFlush);
  }

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed)).Times(1);
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());

  test_sessions_[0]->timeout_timer_->enableTimer(std::chrono::seconds(10), nullptr);
  test_sessions_[0]->timeout_timer_->invokeCallback();
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, DynamicAddAndRemove) {
  setupNoServiceValidationHC();
  health_checker_->start();

  expectSessionCreate();
  expectStreamCreate(0);
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  cluster_->prioritySet().getMockHostSet(0)->runCallbacks(
      {cluster_->prioritySet().getMockHostSet(0)->hosts_.back()}, {});

  HostVector removed{cluster_->prioritySet().getMockHostSet(0)->hosts_.back()};
  cluster_->prioritySet().getMockHostSet(0)->hosts_.clear();
  EXPECT_CALL(*test_sessions_[0]->client_connection_, close(_));
  cluster_->prioritySet().getMockHostSet(0)->runCallbacks({}, removed);
}

TEST_F(HttpHealthCheckerImplTest, ConnectionClose) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate(0);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  test_sessions_[0]->interval_timer_->invokeCallback();
}

TEST_F(HttpHealthCheckerImplTest, ProxyConnectionClose) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate(0);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  test_sessions_[0]->interval_timer_->invokeCallback();
}

TEST_F(HttpHealthCheckerImplTest, HealthCheckIntervals) {
  setupHealthCheckIntervalOverridesHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://128.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  // First check should respect no_traffic_interval setting.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(5000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  cluster_->info_->stats().upstream_cx_total_.inc();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Follow up successful checks should respect interval setting.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Follow up successful checks should respect interval setting.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // A logical failure is not considered a network failure, therefore the unhealthy threshold is
  // ignored and health state changes immediately. Since the threshold is ignored, next health
  // check respects "unhealthy_interval".
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "503", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent failing checks should respect unhealthy_interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "503", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent failing checks should respect unhealthy_interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "503", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // When transitioning to a successful state, checks should respect healthy_edge_interval. Health
  // state should be delayed pending healthy threshold.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // After the healthy threshold is reached, health state should change while checks should respect
  // the default interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent checks shouldn't change the state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // First failed check after a run o successful ones should respect unhealthy_edge_interval. A
  // timeout, being a network type failure, should respect unhealthy threshold before changing the
  // health state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(3000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a network timeout.
  expectClientCreate(0);
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(3000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a network timeout.
  expectClientCreate(0);
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent failing checks should respect unhealthy_interval. As the unhealthy threshold is
  // reached, health state should also change.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a network timeout.
  expectClientCreate(0);
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Remaining failing checks shouldn't change the state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a network timeout.
  expectClientCreate(0);
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // When transitioning to a successful state, checks should respect healthy_edge_interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // After the healthy threshold is reached, health state should change while checks should respect
  // the default interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent checks shouldn't change the state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
}

TEST_F(HttpHealthCheckerImplTest, RemoteCloseBetweenChecks) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(2);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);

  expectClientCreate(0);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

// Test that we close connections on a healthy check when reuse_connection is false.
TEST_F(HttpHealthCheckerImplTest, DontReuseConnectionBetweenChecks) {
  setupNoServiceValidationNoReuseConnectionHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(2);

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  // A new client is created because we close the connection ourselves.
  // See HttpHealthCheckerImplTest.RemoteCloseBetweenChecks for how this works when the remote end
  // closes the connection.
  expectClientCreate(0);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respond(0, "200", false);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, StreamReachesWatermarkDuringCheck) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());

  test_sessions_[0]->request_encoder_.stream_.runHighWatermarkCallbacks();
  test_sessions_[0]->request_encoder_.stream_.runLowWatermarkCallbacks();

  respond(0, "200", true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, ConnectionReachesWatermarkDuringCheck) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());

  test_sessions_[0]->client_connection_->runHighWatermarkCallbacks();
  test_sessions_[0]->client_connection_->runLowWatermarkCallbacks();

  respond(0, "200", true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(HttpHealthCheckerImplTest, SuccessServiceCheckWithAltPort) {
  const std::string host = "fake_cluster";
  const std::string path = "/healthcheck";
  setupServiceValidationHC();
  EXPECT_CALL(runtime_.snapshot_, featureEnabled("health_check.verify_cluster", 100))
      .WillOnce(Return(true));

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(1);

  // Prepares a host with its designated health check port.
  const HostWithHealthCheckMap hosts{{"127.0.0.1:80", makeHealthCheckConfig(8000)}};
  appendTestHosts(cluster_, hosts);
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate(hosts);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeHeaders(_, true))
      .WillOnce(Invoke([&](const Http::HeaderMap& headers, bool) {
        EXPECT_EQ(headers.Host()->value().getStringView(), host);
        EXPECT_EQ(headers.Path()->value().getStringView(), path);
      }));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .WillOnce(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  absl::optional<std::string> health_checked_cluster("locations-production-iad");
  respond(0, "200", false, false, true, false, health_checked_cluster);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

// Test host check success with multiple hosts by checking each host defined health check port.
TEST_F(HttpHealthCheckerImplTest, SuccessWithMultipleHostsAndAltPort) {
  setupNoServiceValidationHC();
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged)).Times(2);

  // Prepares a set of hosts along with its designated health check ports.
  const HostWithHealthCheckMap hosts = {{"127.0.0.1:80", makeHealthCheckConfig(8000)},
                                        {"127.0.0.1:81", makeHealthCheckConfig(8001)}};
  appendTestHosts(cluster_, hosts);
  cluster_->info_->stats().upstream_cx_total_.inc();
  cluster_->info_->stats().upstream_cx_total_.inc();
  expectSessionCreate(hosts);
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  expectSessionCreate(hosts);
  expectStreamCreate(1);
  EXPECT_CALL(*test_sessions_[1]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _)).Times(2);
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
      .Times(2)
      .WillRepeatedly(Return(45000));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  EXPECT_CALL(*test_sessions_[1]->interval_timer_,
              enableTimer(std::chrono::milliseconds(45000), _));
  EXPECT_CALL(*test_sessions_[1]->timeout_timer_, disableTimer());
  respond(0, "200", false, false, true);
  respond(1, "200", false, false, true);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[1]->health());
}

TEST_F(HttpHealthCheckerImplTest, Http2ClusterUseHttp2CodecClient) {
  setupNoServiceValidationHCWithHttp2();
  EXPECT_EQ(Http::CodecClient::Type::HTTP2, health_checker_->codecClientType());
}

class TestProdHttpHealthChecker : public ProdHttpHealthCheckerImpl {
public:
  using ProdHttpHealthCheckerImpl::ProdHttpHealthCheckerImpl;

  std::unique_ptr<Http::CodecClient>
  createCodecClientForTest(std::unique_ptr<Network::ClientConnection>&& connection) {
    Upstream::Host::CreateConnectionData data;
    data.connection_ = std::move(connection);
    data.host_description_ = std::make_shared<NiceMock<Upstream::MockHostDescription>>();
    return std::unique_ptr<Http::CodecClient>(createCodecClient(data));
  }
};

class ProdHttpHealthCheckerTest : public HttpHealthCheckerImplTest {
public:
  void setupNoServiceValidationHCWithHttp2() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
      codec_client_type: Http2
    )EOF";

    health_checker_.reset(new TestProdHttpHealthChecker(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupNoServiceValidationHC() {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";

    health_checker_.reset(new TestProdHttpHealthChecker(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                        dispatcher_, runtime_, random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  std::unique_ptr<Network::MockClientConnection> connection_ =
      std::make_unique<NiceMock<Network::MockClientConnection>>();
  std::shared_ptr<TestProdHttpHealthChecker> health_checker_;
};

TEST_F(ProdHttpHealthCheckerTest, ProdHttpHealthCheckerH1HealthChecking) {
  setupNoServiceValidationHC();
  EXPECT_EQ(Http::CodecClient::Type::HTTP1,
            health_checker_->createCodecClientForTest(std::move(connection_))->type());
}

TEST_F(HttpHealthCheckerImplTest, DEPRECATED_FEATURE_TEST(Http1CodecClient)) {
  const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
      use_http2: false
    )EOF";

  health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                      dispatcher_, runtime_, random_,
                                                      HealthCheckEventLoggerPtr(event_logger_)));
  health_checker_->addHostCheckCompleteCb(
      [this](HostSharedPtr host, HealthTransition changed_state) -> void {
        onHostStatus(host, changed_state);
      });
  EXPECT_EQ(Http::CodecClient::Type::HTTP1, health_checker_->codecClientType());
}

TEST_F(HttpHealthCheckerImplTest, DEPRECATED_FEATURE_TEST(Http2CodecClient)) {
  const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 5s
    interval_jitter: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    http_health_check:
      service_name: locations
      path: /healthcheck
      use_http2: true
    )EOF";

  health_checker_.reset(new TestHttpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                      dispatcher_, runtime_, random_,
                                                      HealthCheckEventLoggerPtr(event_logger_)));
  health_checker_->addHostCheckCompleteCb(
      [this](HostSharedPtr host, HealthTransition changed_state) -> void {
        onHostStatus(host, changed_state);
      });
  EXPECT_EQ(Http::CodecClient::Type::HTTP2, health_checker_->codecClientType());
}

TEST_F(ProdHttpHealthCheckerTest, ProdHttpHealthCheckerH2HealthChecking) {
  setupNoServiceValidationHCWithHttp2();
  EXPECT_EQ(Http::CodecClient::Type::HTTP2,
            health_checker_->createCodecClientForTest(std::move(connection_))->type());
}

TEST(HttpStatusChecker, Default) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthcheck
  )EOF";

  HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
      parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200);

  EXPECT_TRUE(http_status_checker.inRange(200));
  EXPECT_FALSE(http_status_checker.inRange(204));
}

TEST(HttpStatusChecker, Single100) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthcheck
    expected_statuses:
      - start: 100
        end: 101
  )EOF";

  HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
      parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200);

  EXPECT_FALSE(http_status_checker.inRange(200));

  EXPECT_FALSE(http_status_checker.inRange(99));
  EXPECT_TRUE(http_status_checker.inRange(100));
  EXPECT_FALSE(http_status_checker.inRange(101));
}

TEST(HttpStatusChecker, Single599) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthcheck
    expected_statuses:
      - start: 599
        end: 600
  )EOF";

  HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
      parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200);

  EXPECT_FALSE(http_status_checker.inRange(200));

  EXPECT_FALSE(http_status_checker.inRange(598));
  EXPECT_TRUE(http_status_checker.inRange(599));
  EXPECT_FALSE(http_status_checker.inRange(600));
}

TEST(HttpStatusChecker, Ranges_204_304) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthcheck
    expected_statuses:
      - start: 204
        end: 205
      - start: 304
        end: 305
  )EOF";

  HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
      parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200);

  EXPECT_FALSE(http_status_checker.inRange(200));

  EXPECT_FALSE(http_status_checker.inRange(203));
  EXPECT_TRUE(http_status_checker.inRange(204));
  EXPECT_FALSE(http_status_checker.inRange(205));
  EXPECT_FALSE(http_status_checker.inRange(303));
  EXPECT_TRUE(http_status_checker.inRange(304));
  EXPECT_FALSE(http_status_checker.inRange(305));
}

TEST(HttpStatusChecker, Below100) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthcheck
    expected_statuses:
      - start: 99
        end: 100
  )EOF";

  EXPECT_THROW_WITH_MESSAGE(
      HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
          parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200),
      EnvoyException, "Invalid http status range: expecting start >= 100, but found start=99");
}

TEST(HttpStatusChecker, Above599) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthchecka
    expected_statuses:
      - start: 600
        end: 601
  )EOF";

  EXPECT_THROW_WITH_MESSAGE(
      HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
          parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200),
      EnvoyException, "Invalid http status range: expecting end <= 600, but found end=601");
}

TEST(HttpStatusChecker, InvalidRange) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthchecka
    expected_statuses:
      - start: 200
        end: 200
  )EOF";

  EXPECT_THROW_WITH_MESSAGE(
      HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
          parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200),
      EnvoyException,
      "Invalid http status range: expecting start < end, but found start=200 and end=200");
}

TEST(HttpStatusChecker, InvalidRange2) {
  const std::string yaml = R"EOF(
  http_health_check:
    service_name: locations
    path: /healthchecka
    expected_statuses:
      - start: 201
        end: 200
  )EOF";

  EXPECT_THROW_WITH_MESSAGE(
      HttpHealthCheckerImpl::HttpStatusChecker http_status_checker(
          parseHealthCheckFromV2Yaml(yaml).http_health_check().expected_statuses(), 200),
      EnvoyException,
      "Invalid http status range: expecting start < end, but found start=201 and end=200");
}

TEST(TcpHealthCheckMatcher, loadJsonBytes) {
  {
    Protobuf::RepeatedPtrField<envoy::api::v2::core::HealthCheck::Payload> repeated_payload;
    repeated_payload.Add()->set_text("39000000");
    repeated_payload.Add()->set_text("EEEEEEEE");

    TcpHealthCheckMatcher::MatchSegments segments =
        TcpHealthCheckMatcher::loadProtoBytes(repeated_payload);
    EXPECT_EQ(2U, segments.size());
  }

  {
    Protobuf::RepeatedPtrField<envoy::api::v2::core::HealthCheck::Payload> repeated_payload;
    repeated_payload.Add()->set_text("4");

    EXPECT_THROW(TcpHealthCheckMatcher::loadProtoBytes(repeated_payload), EnvoyException);
  }

  {
    Protobuf::RepeatedPtrField<envoy::api::v2::core::HealthCheck::Payload> repeated_payload;
    repeated_payload.Add()->set_text("gg");

    EXPECT_THROW(TcpHealthCheckMatcher::loadProtoBytes(repeated_payload), EnvoyException);
  }
}

static void add_uint8(Buffer::Instance& buffer, uint8_t addend) {
  buffer.add(&addend, sizeof(addend));
}

TEST(TcpHealthCheckMatcher, match) {
  Protobuf::RepeatedPtrField<envoy::api::v2::core::HealthCheck::Payload> repeated_payload;
  repeated_payload.Add()->set_text("01");
  repeated_payload.Add()->set_text("02");

  TcpHealthCheckMatcher::MatchSegments segments =
      TcpHealthCheckMatcher::loadProtoBytes(repeated_payload);

  Buffer::OwnedImpl buffer;
  EXPECT_FALSE(TcpHealthCheckMatcher::match(segments, buffer));
  add_uint8(buffer, 1);
  EXPECT_FALSE(TcpHealthCheckMatcher::match(segments, buffer));
  add_uint8(buffer, 2);
  EXPECT_TRUE(TcpHealthCheckMatcher::match(segments, buffer));

  buffer.drain(2);
  add_uint8(buffer, 1);
  add_uint8(buffer, 3);
  add_uint8(buffer, 2);
  EXPECT_TRUE(TcpHealthCheckMatcher::match(segments, buffer));

  buffer.drain(3);
  add_uint8(buffer, 0);
  add_uint8(buffer, 3);
  add_uint8(buffer, 1);
  add_uint8(buffer, 2);
  EXPECT_TRUE(TcpHealthCheckMatcher::match(segments, buffer));
}

class TcpHealthCheckerImplTest : public testing::Test {
public:
  TcpHealthCheckerImplTest()
      : cluster_(new NiceMock<MockClusterMockPrioritySet>()),
        event_logger_(new MockHealthCheckEventLogger()) {}

  void setupData(unsigned int unhealthy_threshold = 2) {
    std::ostringstream yaml;
    yaml << R"EOF(
    timeout: 1s
    interval: 1s
    unhealthy_threshold: )EOF"
         << unhealthy_threshold << R"EOF(
    healthy_threshold: 2
    tcp_health_check:
      send:
        text: "01"
      receive:
      - text: "02"
    )EOF";

    health_checker_.reset(
        new TcpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml.str()), dispatcher_,
                                 runtime_, random_, HealthCheckEventLoggerPtr(event_logger_)));
  }

  void setupNoData() {
    std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    tcp_health_check: {}
    )EOF";

    health_checker_.reset(new TcpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                   dispatcher_, runtime_, random_,
                                                   HealthCheckEventLoggerPtr(event_logger_)));
  }

  void setupDataDontReuseConnection() {
    std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    unhealthy_threshold: 2
    healthy_threshold: 2
    reuse_connection: false
    tcp_health_check:
      send:
        text: "01"
      receive:
      - text: "02"
    )EOF";

    health_checker_.reset(new TcpHealthCheckerImpl(*cluster_, parseHealthCheckFromV2Yaml(yaml),
                                                   dispatcher_, runtime_, random_,
                                                   HealthCheckEventLoggerPtr(event_logger_)));
  }

  void expectSessionCreate() {
    interval_timer_ = new Event::MockTimer(&dispatcher_);
    timeout_timer_ = new Event::MockTimer(&dispatcher_);
  }

  void expectClientCreate() {
    connection_ = new NiceMock<Network::MockClientConnection>();
    EXPECT_CALL(dispatcher_, createClientConnection_(_, _, _, _)).WillOnce(Return(connection_));
    EXPECT_CALL(*connection_, addReadFilter(_)).WillOnce(SaveArg<0>(&read_filter_));
  }

  std::shared_ptr<MockClusterMockPrioritySet> cluster_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  std::shared_ptr<TcpHealthCheckerImpl> health_checker_;
  MockHealthCheckEventLogger* event_logger_{};
  Network::MockClientConnection* connection_{};
  Event::MockTimer* timeout_timer_{};
  Event::MockTimer* interval_timer_{};
  Network::ReadFilterSharedPtr read_filter_;
  NiceMock<Runtime::MockLoader> runtime_;
  NiceMock<Runtime::MockRandomGenerator> random_;
};

TEST_F(TcpHealthCheckerImplTest, Success) {
  InSequence s;

  setupData();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  connection_->runHighWatermarkCallbacks();
  connection_->runLowWatermarkCallbacks();
  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  Buffer::OwnedImpl response;
  add_uint8(response, 2);
  read_filter_->onData(response, false);
}

// Tests that a successful healthcheck will disconnect the client when reuse_connection is false.
TEST_F(TcpHealthCheckerImplTest, DataWithoutReusingConnection) {
  InSequence s;

  setupDataDontReuseConnection();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(1);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Expected execution flow when a healthcheck is successful and reuse_connection is false.
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*connection_, close(Network::ConnectionCloseType::NoFlush)).Times(1);

  Buffer::OwnedImpl response;
  add_uint8(response, 2);
  read_filter_->onData(response, false);

  // These are the expected metric results after testing.
  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.failure").value());
}

// Tests an unsuccessful healthcheck, where the endpoint sends wrong data
TEST_F(TcpHealthCheckerImplTest, WrongData) {
  InSequence s;

  setupDataDontReuseConnection();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(1);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Not the expected response
  Buffer::OwnedImpl response;
  add_uint8(response, 3);
  read_filter_->onData(response, false);

  // These are the expected metric results after testing.
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  // TODO(lilika): This should indicate a failure
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.failure").value());
  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::UNHEALTHY);
}

TEST_F(TcpHealthCheckerImplTest, TimeoutThenRemoteClose) {
  InSequence s;

  setupData();
  health_checker_->start();

  expectSessionCreate();
  expectClientCreate();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));

  cluster_->prioritySet().getMockHostSet(0)->runCallbacks(
      {cluster_->prioritySet().getMockHostSet(0)->hosts_.back()}, {});

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  Buffer::OwnedImpl response;
  add_uint8(response, 1);
  read_filter_->onData(response, false);

  EXPECT_CALL(*connection_, close(_));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  timeout_timer_->invokeCallback();
  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::TIMEOUT);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  interval_timer_->invokeCallback();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  interval_timer_->invokeCallback();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  HostVector removed{cluster_->prioritySet().getMockHostSet(0)->hosts_.back()};
  cluster_->prioritySet().getMockHostSet(0)->hosts_.clear();
  EXPECT_CALL(*connection_, close(_));
  cluster_->prioritySet().getMockHostSet(0)->runCallbacks({}, removed);
}

TEST_F(TcpHealthCheckerImplTest, Timeout) {
  InSequence s;

  setupData(1);
  health_checker_->start();

  expectSessionCreate();
  expectClientCreate();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));

  cluster_->prioritySet().getMockHostSet(0)->runCallbacks(
      {cluster_->prioritySet().getMockHostSet(0)->hosts_.back()}, {});

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  Buffer::OwnedImpl response;
  add_uint8(response, 1);
  read_filter_->onData(response, false);

  EXPECT_CALL(*connection_, close(_));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  timeout_timer_->invokeCallback();
  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::TIMEOUT);
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

TEST_F(TcpHealthCheckerImplTest, DoubleTimeout) {
  InSequence s;

  setupData();
  health_checker_->start();

  expectSessionCreate();
  expectClientCreate();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));

  cluster_->prioritySet().getMockHostSet(0)->runCallbacks(
      {cluster_->prioritySet().getMockHostSet(0)->hosts_.back()}, {});

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  Buffer::OwnedImpl response;
  add_uint8(response, 1);
  read_filter_->onData(response, false);

  EXPECT_CALL(*connection_, close(_));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  timeout_timer_->invokeCallback();
  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::TIMEOUT);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  interval_timer_->invokeCallback();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  EXPECT_CALL(*connection_, close(_));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  timeout_timer_->invokeCallback();
  EXPECT_EQ(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->getActiveHealthFailureType(),
            Host::ActiveHealthFailureType::TIMEOUT);
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  interval_timer_->invokeCallback();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  HostVector removed{cluster_->prioritySet().getMockHostSet(0)->hosts_.back()};
  cluster_->prioritySet().getMockHostSet(0)->hosts_.clear();
  EXPECT_CALL(*connection_, close(_));
  cluster_->prioritySet().getMockHostSet(0)->runCallbacks({}, removed);
}

// Tests that when reuse_connection is false timeouts execute normally.
TEST_F(TcpHealthCheckerImplTest, TimeoutWithoutReusingConnection) {
  InSequence s;

  setupDataDontReuseConnection();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(1);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Expected flow when a healthcheck is successful and reuse_connection is false.
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  EXPECT_CALL(*connection_, close(Network::ConnectionCloseType::NoFlush)).Times(1);

  Buffer::OwnedImpl response;
  add_uint8(response, 2);
  read_filter_->onData(response, false);

  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.failure").value());

  // The healthcheck will run again.
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  interval_timer_->invokeCallback();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Expected flow when a healthcheck times out.
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  // The healthcheck is not yet at the unhealthy threshold.
  EXPECT_FALSE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  // The healthcheck metric results after first timeout block.
  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.failure").value());

  // The healthcheck will run again, it should be failing after this attempt.
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  interval_timer_->invokeCallback();

  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  // Expected flow when a healthcheck times out.
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  // The healthcheck metric results after the second timeout block.
  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(2UL, cluster_->info_->stats_store_.counter("health_check.failure").value());
}

TEST_F(TcpHealthCheckerImplTest, NoData) {
  InSequence s;

  setupNoData();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(0);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  EXPECT_CALL(*connection_, close(_));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  connection_->raiseEvent(Network::ConnectionEvent::Connected);

  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(0);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  interval_timer_->invokeCallback();
}

TEST_F(TcpHealthCheckerImplTest, PassiveFailure) {
  InSequence s;

  setupNoData();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(0);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  health_checker_->start();

  // Do multiple passive failures. This will not reset the active HC timers.
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthChecker().setUnhealthy();
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthChecker().setUnhealthy();
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  // A single success should not bring us back to healthy.
  EXPECT_CALL(*connection_, close(_));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));
  connection_->raiseEvent(Network::ConnectionEvent::Connected);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());

  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.attempt").value());
  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(2UL, cluster_->info_->stats_store_.counter("health_check.failure").value());
  EXPECT_EQ(2UL, cluster_->info_->stats_store_.counter("health_check.passive_failure").value());
}

TEST_F(TcpHealthCheckerImplTest, PassiveFailureCrossThreadRemoveHostRace) {
  InSequence s;

  setupNoData();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(0);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  // Do a passive failure. This will not reset the active HC timers.
  Event::PostCb post_cb;
  EXPECT_CALL(dispatcher_, post(_)).WillOnce(SaveArg<0>(&post_cb));
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthChecker().setUnhealthy();

  // Remove before the cross thread event comes in.
  EXPECT_CALL(*connection_, close(_));
  HostVector old_hosts = std::move(cluster_->prioritySet().getMockHostSet(0)->hosts_);
  cluster_->prioritySet().getMockHostSet(0)->runCallbacks({}, old_hosts);
  post_cb();

  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.attempt").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.failure").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.passive_failure").value());
}

TEST_F(TcpHealthCheckerImplTest, PassiveFailureCrossThreadRemoveClusterRace) {
  InSequence s;

  setupNoData();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _)).Times(0);
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  // Do a passive failure. This will not reset the active HC timers.
  Event::PostCb post_cb;
  EXPECT_CALL(dispatcher_, post(_)).WillOnce(SaveArg<0>(&post_cb));
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthChecker().setUnhealthy();

  // Remove before the cross thread event comes in.
  EXPECT_CALL(*connection_, close(_));
  health_checker_.reset();
  post_cb();

  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.attempt").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.failure").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.passive_failure").value());
}

TEST_F(TcpHealthCheckerImplTest, ConnectionLocalFailure) {
  InSequence s;

  setupData();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();
  expectClientCreate();
  EXPECT_CALL(*connection_, write(_, _));
  EXPECT_CALL(*timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  // Expect the LocalClose to be handled as a health check failure
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*timeout_timer_, disableTimer());
  EXPECT_CALL(*interval_timer_, enableTimer(_, _));

  // Raise a LocalClose that is not triggered by the health monitor itself.
  // e.g. a failure to setsockopt().
  connection_->raiseEvent(Network::ConnectionEvent::LocalClose);

  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.attempt").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.success").value());
  EXPECT_EQ(1UL, cluster_->info_->stats_store_.counter("health_check.failure").value());
  EXPECT_EQ(0UL, cluster_->info_->stats_store_.counter("health_check.passive_failure").value());
}

class TestGrpcHealthCheckerImpl : public GrpcHealthCheckerImpl {
public:
  using GrpcHealthCheckerImpl::GrpcHealthCheckerImpl;

  Http::CodecClientPtr createCodecClient(Upstream::Host::CreateConnectionData& conn_data) override {
    auto codec_client = createCodecClient_(conn_data);
    return Http::CodecClientPtr(codec_client);
  };

  // GrpcHealthCheckerImpl
  MOCK_METHOD1(createCodecClient_, Http::CodecClient*(Upstream::Host::CreateConnectionData&));
};

class GrpcHealthCheckerImplTestBase {
public:
  struct TestSession {
    TestSession() = default;

    Event::MockTimer* interval_timer_{};
    Event::MockTimer* timeout_timer_{};
    Http::MockClientConnection* codec_{};
    Stats::IsolatedStoreImpl stats_store_;
    Network::MockClientConnection* client_connection_{};
    NiceMock<Http::MockStreamEncoder> request_encoder_;
    Http::StreamDecoder* stream_response_callbacks_{};
    CodecClientForTest* codec_client_{};
  };

  using TestSessionPtr = std::unique_ptr<TestSession>;

  struct ResponseSpec {
    struct ChunkSpec {
      bool valid;
      std::vector<uint8_t> data;
    };
    static ChunkSpec invalidChunk() {
      ChunkSpec spec;
      spec.valid = false;
      return spec;
    }
    static ChunkSpec invalidPayload(uint8_t flags, bool valid_message) {
      ChunkSpec spec;
      spec.valid = true;
      spec.data = serializeResponse(grpc::health::v1::HealthCheckResponse::SERVING);
      spec.data[0] = flags;
      if (!valid_message) {
        const size_t kGrpcHeaderSize = 5;
        for (size_t i = kGrpcHeaderSize; i < spec.data.size(); i++) {
          // Fill payload with some random data.
          spec.data[i] = i % 256;
        }
      }
      return spec;
    }
    static ChunkSpec validChunk(grpc::health::v1::HealthCheckResponse::ServingStatus status) {
      ChunkSpec spec;
      spec.valid = true;
      spec.data = serializeResponse(status);
      return spec;
    }

    static ChunkSpec servingResponse() {
      return validChunk(grpc::health::v1::HealthCheckResponse::SERVING);
    }

    static ChunkSpec notServingResponse() {
      return validChunk(grpc::health::v1::HealthCheckResponse::NOT_SERVING);
    }

    static std::vector<uint8_t>
    serializeResponse(grpc::health::v1::HealthCheckResponse::ServingStatus status) {
      grpc::health::v1::HealthCheckResponse response;
      response.set_status(status);
      const auto data = Grpc::Common::serializeToGrpcFrame(response);
      auto ret = std::vector<uint8_t>(data->length(), 0);
      data->copyOut(0, data->length(), &ret[0]);
      return ret;
    }

    std::vector<std::pair<std::string, std::string>> response_headers;
    std::vector<ChunkSpec> body_chunks;
    std::vector<std::pair<std::string, std::string>> trailers;
  };

  GrpcHealthCheckerImplTestBase()
      : cluster_(new NiceMock<MockClusterMockPrioritySet>()),
        event_logger_(new MockHealthCheckEventLogger()) {
    EXPECT_CALL(*cluster_->info_, features())
        .WillRepeatedly(Return(Upstream::ClusterInfo::Features::HTTP2));
  }

  void setupHC() {
    const auto config = createGrpcHealthCheckConfig();
    health_checker_.reset(new TestGrpcHealthCheckerImpl(*cluster_, config, dispatcher_, runtime_,
                                                        random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupHCWithUnhealthyThreshold(int value) {
    auto config = createGrpcHealthCheckConfig();
    config.mutable_unhealthy_threshold()->set_value(value);
    health_checker_.reset(new TestGrpcHealthCheckerImpl(*cluster_, config, dispatcher_, runtime_,
                                                        random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupServiceNameHC(const absl::optional<std::string>& authority) {
    auto config = createGrpcHealthCheckConfig();
    config.mutable_grpc_health_check()->set_service_name("service");
    if (authority.has_value()) {
      config.mutable_grpc_health_check()->set_authority(authority.value());
    }
    health_checker_.reset(new TestGrpcHealthCheckerImpl(*cluster_, config, dispatcher_, runtime_,
                                                        random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupNoReuseConnectionHC() {
    auto config = createGrpcHealthCheckConfig();
    config.mutable_reuse_connection()->set_value(false);
    health_checker_.reset(new TestGrpcHealthCheckerImpl(*cluster_, config, dispatcher_, runtime_,
                                                        random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void setupHealthCheckIntervalOverridesHC() {
    auto config = createGrpcHealthCheckConfig();
    config.mutable_interval()->set_seconds(1);
    config.mutable_unhealthy_interval()->set_seconds(2);
    config.mutable_unhealthy_edge_interval()->set_seconds(3);
    config.mutable_healthy_edge_interval()->set_seconds(4);
    config.mutable_no_traffic_interval()->set_seconds(5);
    config.mutable_interval_jitter()->set_seconds(0);
    config.mutable_unhealthy_threshold()->set_value(3);
    config.mutable_healthy_threshold()->set_value(3);
    health_checker_.reset(new TestGrpcHealthCheckerImpl(*cluster_, config, dispatcher_, runtime_,
                                                        random_,
                                                        HealthCheckEventLoggerPtr(event_logger_)));
    health_checker_->addHostCheckCompleteCb(
        [this](HostSharedPtr host, HealthTransition changed_state) -> void {
          onHostStatus(host, changed_state);
        });
  }

  void expectSessionCreate() {
    // Expectations are in LIFO order.
    TestSessionPtr new_test_session(new TestSession());
    test_sessions_.emplace_back(std::move(new_test_session));
    TestSession& test_session = *test_sessions_.back();
    test_session.timeout_timer_ = new Event::MockTimer(&dispatcher_);
    test_session.interval_timer_ = new Event::MockTimer(&dispatcher_);
    expectClientCreate(test_sessions_.size() - 1);
  }

  void expectClientCreate(size_t index) {
    TestSession& test_session = *test_sessions_[index];
    test_session.codec_ = new NiceMock<Http::MockClientConnection>();
    test_session.client_connection_ = new NiceMock<Network::MockClientConnection>();
    connection_index_.push_back(index);
    codec_index_.push_back(index);

    EXPECT_CALL(dispatcher_, createClientConnection_(_, _, _, _))
        .Times(testing::AnyNumber())
        .WillRepeatedly(InvokeWithoutArgs([&]() -> Network::ClientConnection* {
          uint32_t index = connection_index_.front();
          connection_index_.pop_front();
          return test_sessions_[index]->client_connection_;
        }));

    EXPECT_CALL(*health_checker_, createCodecClient_(_))
        .WillRepeatedly(
            Invoke([&](Upstream::Host::CreateConnectionData& conn_data) -> Http::CodecClient* {
              uint32_t index = codec_index_.front();
              codec_index_.pop_front();
              TestSession& test_session = *test_sessions_[index];
              std::shared_ptr<Upstream::MockClusterInfo> cluster{
                  new NiceMock<Upstream::MockClusterInfo>()};
              Event::MockDispatcher dispatcher_;

              test_session.codec_client_ = new CodecClientForTest(
                  Http::CodecClient::Type::HTTP1, std::move(conn_data.connection_),
                  test_session.codec_, nullptr,
                  Upstream::makeTestHost(cluster, "tcp://127.0.0.1:9000"), dispatcher_);
              return test_session.codec_client_;
            }));
  }

  void expectStreamCreate(size_t index) {
    test_sessions_[index]->request_encoder_.stream_.callbacks_.clear();
    EXPECT_CALL(*test_sessions_[index]->codec_, newStream(_))
        .WillOnce(DoAll(SaveArgAddress(&test_sessions_[index]->stream_response_callbacks_),
                        ReturnRef(test_sessions_[index]->request_encoder_)));
  }

  // Starts healthchecker and sets up timer expectations, leaving up future specification of
  // healthcheck response for the caller. Useful when there is only one healthcheck attempt
  // performed during test case (but possibly on many hosts).
  void expectHealthchecks(HealthTransition host_changed_state, size_t num_healthchecks) {
    for (size_t i = 0; i < num_healthchecks; i++) {
      cluster_->info_->stats().upstream_cx_total_.inc();
      expectSessionCreate();
      expectHealthcheckStart(i);
    }
    health_checker_->start();

    EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _))
        .Times(num_healthchecks);
    EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
        .Times(num_healthchecks)
        .WillRepeatedly(Return(45000));
    for (size_t i = 0; i < num_healthchecks; i++) {
      expectHealthcheckStop(i, 45000);
    }
    EXPECT_CALL(*this, onHostStatus(_, host_changed_state)).Times(num_healthchecks);
  }

  void expectSingleHealthcheck(HealthTransition host_changed_state) {
    cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
        makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
    expectHealthchecks(host_changed_state, 1);
  }

  // Hides timer/stream-related boilerplate of healthcheck start.
  void expectHealthcheckStart(size_t index) {
    expectStreamCreate(index);
    EXPECT_CALL(*test_sessions_[index]->timeout_timer_, enableTimer(_, _));
  }

  // Hides timer-related boilerplate of healthcheck stop.
  void expectHealthcheckStop(size_t index, int interval_ms = 0) {
    if (interval_ms > 0) {
      EXPECT_CALL(*test_sessions_[index]->interval_timer_,
                  enableTimer(std::chrono::milliseconds(interval_ms), _));
    } else {
      EXPECT_CALL(*test_sessions_[index]->interval_timer_, enableTimer(_, _));
    }
    EXPECT_CALL(*test_sessions_[index]->timeout_timer_, disableTimer());
  }

  // Hides host status checking boilerplate when only single host is used in test.
  void expectHostHealthy(bool healthy) {
    const auto host = cluster_->prioritySet().getMockHostSet(0)->hosts_[0];
    if (!healthy) {
      EXPECT_TRUE(host->healthFlagGet(Host::HealthFlag::FAILED_ACTIVE_HC));
      EXPECT_EQ(Host::Health::Unhealthy, host->health());
    } else {
      EXPECT_EQ(Host::Health::Healthy, host->health());
    }
  }

  void respondServiceStatus(size_t index,
                            grpc::health::v1::HealthCheckResponse::ServingStatus status) {
    respondResponseSpec(index,
                        ResponseSpec{{{":status", "200"}, {"content-type", "application/grpc"}},
                                     {ResponseSpec::validChunk(status)},
                                     {{"grpc-status", "0"}}});
  }

  void respondResponseSpec(size_t index, ResponseSpec&& spec) {
    const bool trailers_empty = spec.trailers.empty();
    const bool end_stream_on_headers = spec.body_chunks.empty() && trailers_empty;
    auto response_headers = std::make_unique<Http::TestHeaderMapImpl>();
    for (const auto& header : spec.response_headers) {
      response_headers->addCopy(header.first, header.second);
    }
    test_sessions_[index]->stream_response_callbacks_->decodeHeaders(std::move(response_headers),
                                                                     end_stream_on_headers);
    for (size_t i = 0; i < spec.body_chunks.size(); i++) {
      const bool end_stream = i == spec.body_chunks.size() - 1 && trailers_empty;
      const auto& chunk = spec.body_chunks[i];
      if (chunk.valid) {
        const auto data = std::make_unique<Buffer::OwnedImpl>(chunk.data.data(), chunk.data.size());
        test_sessions_[index]->stream_response_callbacks_->decodeData(*data, end_stream);
      } else {
        Buffer::OwnedImpl incorrect_data("incorrect");
        test_sessions_[index]->stream_response_callbacks_->decodeData(incorrect_data, end_stream);
      }
    }
    if (!trailers_empty) {
      auto trailers = std::make_unique<Http::TestHeaderMapImpl>();
      for (const auto& header : spec.trailers) {
        trailers->addCopy(header.first, header.second);
      }
      test_sessions_[index]->stream_response_callbacks_->decodeTrailers(std::move(trailers));
    }
  }

  void testSingleHostSuccess(const absl::optional<std::string>& authority) {
    std::string expected_host = cluster_->info_->name();
    if (authority.has_value()) {
      expected_host = authority.value();
    }

    setupServiceNameHC(authority);

    cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
        makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
    cluster_->info_->stats().upstream_cx_total_.inc();

    expectSessionCreate();
    expectHealthcheckStart(0);

    EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeHeaders(_, false))
        .WillOnce(Invoke([&](const Http::HeaderMap& headers, bool) {
          EXPECT_EQ(Http::Headers::get().ContentTypeValues.Grpc,
                    headers.ContentType()->value().getStringView());
          EXPECT_EQ(std::string("/grpc.health.v1.Health/Check"),
                    headers.Path()->value().getStringView());
          EXPECT_EQ(Http::Headers::get().SchemeValues.Http,
                    headers.Scheme()->value().getStringView());
          EXPECT_NE(nullptr, headers.Method());
          EXPECT_EQ(expected_host, headers.Host()->value().getStringView());
        }));
    EXPECT_CALL(test_sessions_[0]->request_encoder_, encodeData(_, true))
        .WillOnce(Invoke([&](Buffer::Instance& data, bool) {
          std::vector<Grpc::Frame> decoded_frames;
          Grpc::Decoder decoder;
          ASSERT_TRUE(decoder.decode(data, decoded_frames));
          ASSERT_EQ(1U, decoded_frames.size());
          auto& frame = decoded_frames[0];
          Buffer::ZeroCopyInputStreamImpl stream(std::move(frame.data_));
          grpc::health::v1::HealthCheckRequest request;
          ASSERT_TRUE(request.ParseFromZeroCopyStream(&stream));
          EXPECT_EQ("service", request.service());
        }));
    health_checker_->start();

    EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _));
    EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _))
        .WillOnce(Return(45000));
    expectHealthcheckStop(0, 45000);

    // Host state should not be changed (remains healthy).
    EXPECT_CALL(*this, onHostStatus(cluster_->prioritySet().getMockHostSet(0)->hosts_[0],
                                    HealthTransition::Unchanged));
    respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
    expectHostHealthy(true);
  }

  MOCK_METHOD2(onHostStatus, void(HostSharedPtr host, HealthTransition changed_state));

  std::shared_ptr<MockClusterMockPrioritySet> cluster_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  std::vector<TestSessionPtr> test_sessions_;
  std::shared_ptr<TestGrpcHealthCheckerImpl> health_checker_;
  NiceMock<Runtime::MockLoader> runtime_;
  NiceMock<Runtime::MockRandomGenerator> random_;
  MockHealthCheckEventLogger* event_logger_{};
  std::list<uint32_t> connection_index_{};
  std::list<uint32_t> codec_index_{};
};

class GrpcHealthCheckerImplTest : public testing::Test, public GrpcHealthCheckerImplTestBase {};

// Test single host check success.
TEST_F(GrpcHealthCheckerImplTest, Success) { testSingleHostSuccess(absl::nullopt); }

// Test single host check success with custom authority.
TEST_F(GrpcHealthCheckerImplTest, SuccessWithCustomAuthority) {
  const std::string authority = "www.envoyproxy.io";
  testSingleHostSuccess(authority);
}

// Test host check success when gRPC response payload is split between several incoming data chunks.
TEST_F(GrpcHealthCheckerImplTest, SuccessResponseSplitBetweenChunks) {
  setupServiceNameHC(absl::nullopt);
  expectSingleHealthcheck(HealthTransition::Unchanged);

  auto response_headers = std::make_unique<Http::TestHeaderMapImpl>(
      std::initializer_list<std::pair<std::string, std::string>>{
          {":status", "200"},
          {"content-type", "application/grpc"},
      });
  test_sessions_[0]->stream_response_callbacks_->decodeHeaders(std::move(response_headers), false);

  grpc::health::v1::HealthCheckResponse response;
  response.set_status(grpc::health::v1::HealthCheckResponse::SERVING);
  auto data = Grpc::Common::serializeToGrpcFrame(response);

  const char* raw_data = static_cast<char*>(data->linearize(data->length()));
  const uint64_t chunk_size = data->length() / 5;
  for (uint64_t offset = 0; offset < data->length(); offset += chunk_size) {
    const uint64_t effective_size = std::min(chunk_size, data->length() - offset);
    const auto chunk = std::make_unique<Buffer::OwnedImpl>(raw_data + offset, effective_size);
    test_sessions_[0]->stream_response_callbacks_->decodeData(*chunk, false);
  }

  auto trailers = std::make_unique<Http::TestHeaderMapImpl>(
      std::initializer_list<std::pair<std::string, std::string>>{{"grpc-status", "0"}});
  test_sessions_[0]->stream_response_callbacks_->decodeTrailers(std::move(trailers));

  expectHostHealthy(true);
}

// Test host check success with multiple hosts.
TEST_F(GrpcHealthCheckerImplTest, SuccessWithMultipleHosts) {
  setupHC();

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80"),
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:81")};

  expectHealthchecks(HealthTransition::Unchanged, 2);

  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  respondServiceStatus(1, grpc::health::v1::HealthCheckResponse::SERVING);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[1]->health());
}

// Test host check success with multiple hosts across multiple priorities.
TEST_F(GrpcHealthCheckerImplTest, SuccessWithMultipleHostSets) {
  setupHC();

  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->prioritySet().getMockHostSet(1)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:81")};

  expectHealthchecks(HealthTransition::Unchanged, 2);

  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  respondServiceStatus(1, grpc::health::v1::HealthCheckResponse::SERVING);
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
  EXPECT_EQ(Host::Health::Healthy, cluster_->prioritySet().getMockHostSet(1)->hosts_[0]->health());
}

// Test stream-level watermarks does not interfere with health check.
TEST_F(GrpcHealthCheckerImplTest, StreamReachesWatermarkDuringCheck) {
  setupHC();
  expectSingleHealthcheck(HealthTransition::Unchanged);

  test_sessions_[0]->request_encoder_.stream_.runHighWatermarkCallbacks();
  test_sessions_[0]->request_encoder_.stream_.runLowWatermarkCallbacks();

  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

// Test connection-level watermarks does not interfere with health check.
TEST_F(GrpcHealthCheckerImplTest, ConnectionReachesWatermarkDuringCheck) {
  setupHC();
  expectSingleHealthcheck(HealthTransition::Unchanged);

  test_sessions_[0]->client_connection_->runHighWatermarkCallbacks();
  test_sessions_[0]->client_connection_->runLowWatermarkCallbacks();

  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

// Test health check on host without traffic sets larger unconfigurable interval for the next check.
TEST_F(GrpcHealthCheckerImplTest, SuccessNoTraffic) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};

  expectSessionCreate();
  expectHealthcheckStart(0);
  health_checker_->start();

  // Default healthcheck interval for hosts without traffic is 60 seconds.
  expectHealthcheckStop(0, 60000);
  // Host state should not be changed (remains healthy).
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

// Test first successful check immediately makes failed host available (without 2nd probe).
TEST_F(GrpcHealthCheckerImplTest, SuccessStartFailedSuccessFirst) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagSet(
      Host::HealthFlag::FAILED_ACTIVE_HC);
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagSet(
      Host::HealthFlag::PENDING_ACTIVE_HC);

  expectSessionCreate();
  expectHealthcheckStart(0);
  health_checker_->start();

  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.max_interval", _)).WillOnce(Return(500));
  EXPECT_CALL(runtime_.snapshot_, getInteger("health_check.min_interval", _));
  expectHealthcheckStop(0, 500);
  // Fast success immediately moves us to healthy.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, true));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
  EXPECT_FALSE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::PENDING_ACTIVE_HC));
}

// Test host recovery after first failed check requires several successful checks.
TEST_F(GrpcHealthCheckerImplTest, SuccessStartFailedFailFirst) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagSet(
      Host::HealthFlag::FAILED_ACTIVE_HC);
  cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagSet(
      Host::HealthFlag::PENDING_ACTIVE_HC);

  expectSessionCreate();
  expectHealthcheckStart(0);
  health_checker_->start();

  // Failing first disables fast success.
  expectHealthcheckStop(0);
  // Host was unhealthy from the start, but we expect a state change due to the pending active hc
  // flag changing.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::NOT_SERVING);
  expectHostHealthy(false);
  EXPECT_FALSE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::PENDING_ACTIVE_HC));

  // Next successful healthcheck does not move host int healthy state (because we configured
  // healthchecker this way).
  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  // Host still unhealthy, need yet another healthcheck.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(false);

  // 2nd successful healthcheck renders host healthy.
  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

// Test host recovery after explicit check failure requires several successful checks.
TEST_F(GrpcHealthCheckerImplTest, GrpcHealthFail) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};

  expectSessionCreate();
  expectHealthcheckStart(0);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  health_checker_->start();

  // Explicit healthcheck failure immediately renders host unhealthy.
  expectHealthcheckStop(0);
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::NOT_SERVING);
  expectHostHealthy(false);

  // Next, we need 2 successful checks for host to become available again.
  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  // Host still considered unhealthy.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(false);

  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  // Host should has become healthy.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

// Test disconnects produce network-type failures which does not lead to immediate unhealthy state.
TEST_F(GrpcHealthCheckerImplTest, Disconnect) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};

  expectSessionCreate();
  expectHealthcheckStart(0);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  health_checker_->start();

  expectHealthcheckStop(0);
  // Network-type healthcheck failure should make host unhealthy only after 2nd event in a row.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  expectHostHealthy(true);

  expectClientCreate(0);
  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  // Now, host should be unhealthy.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  expectHostHealthy(false);
}

TEST_F(GrpcHealthCheckerImplTest, Timeout) {
  setupHCWithUnhealthyThreshold(1);
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();

  expectHealthcheckStart(0);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  health_checker_->start();

  expectHealthcheckStop(0);
  // Unhealthy threshold is 1 so first timeout causes unhealthy
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  test_sessions_[0]->timeout_timer_->invokeCallback();
  expectHostHealthy(false);
}

// Test timeouts produce network-type failures which does not lead to immediate unhealthy state.
TEST_F(GrpcHealthCheckerImplTest, DoubleTimeout) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  expectSessionCreate();

  expectHealthcheckStart(0);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  health_checker_->start();

  expectHealthcheckStop(0);
  // Timeouts are considered network failures and make host unhealthy also after 2nd event.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  test_sessions_[0]->timeout_timer_->invokeCallback();
  expectHostHealthy(true);

  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  // Close connection. Timeouts and connection closes counts together.
  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);
  expectHostHealthy(false);
}

// Test adding and removal of hosts starts and closes healthcheck sessions.
TEST_F(GrpcHealthCheckerImplTest, DynamicAddAndRemove) {
  setupHC();
  health_checker_->start();

  expectSessionCreate();
  expectStreamCreate(0);
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  cluster_->prioritySet().getMockHostSet(0)->runCallbacks(
      {cluster_->prioritySet().getMockHostSet(0)->hosts_.back()}, {});

  HostVector removed{cluster_->prioritySet().getMockHostSet(0)->hosts_.back()};
  cluster_->prioritySet().getMockHostSet(0)->hosts_.clear();
  EXPECT_CALL(*test_sessions_[0]->client_connection_, close(_));
  cluster_->prioritySet().getMockHostSet(0)->runCallbacks({}, removed);
}

TEST_F(GrpcHealthCheckerImplTest, HealthCheckIntervals) {
  setupHealthCheckIntervalOverridesHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://128.0.0.1:80")};
  expectSessionCreate();
  expectStreamCreate(0);
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  health_checker_->start();

  // First check should respect no_traffic_interval setting.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(5000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  cluster_->info_->stats().upstream_cx_total_.inc();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Follow up successful checks should respect interval setting.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Follow up successful checks should respect interval setting.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // A logical failure is not considered a network failure, therefore the unhealthy threshold is
  // ignored and health state changes immediately. Since the threshold is ignored, next health
  // check respects "unhealthy_interval".
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::NOT_SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent failing checks should respect unhealthy_interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::NOT_SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent failing checks should respect unhealthy_interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::NOT_SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // When transitioning to a successful state, checks should respect healthy_edge_interval. Health
  // state should be delayed pending healthy threshold.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // After the healthy threshold is reached, health state should change while checks should respect
  // the default interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent checks shouldn't change the state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // First failed check after a run o successful ones should respect unhealthy_edge_interval. A
  // timeout, being a network type failure, should respect unhealthy threshold before changing the
  // health state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(3000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(3000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent failing checks should respect unhealthy_interval. As the unhealthy threshold is
  // reached, health state should also change.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Remaining failing checks shouldn't change the state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(2000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  test_sessions_[0]->timeout_timer_->invokeCallback();

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // When transitioning to a successful state, checks should respect healthy_edge_interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::ChangePending));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(4000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // After the healthy threshold is reached, health state should change while checks should respect
  // the default interval.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Changed));
  EXPECT_CALL(*event_logger_, logAddHealthy(_, _, false));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);

  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, enableTimer(_, _));
  // Needed after a response is sent.
  expectStreamCreate(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  // Subsequent checks shouldn't change the state.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  EXPECT_CALL(*test_sessions_[0]->interval_timer_, enableTimer(std::chrono::milliseconds(1000), _));
  EXPECT_CALL(*test_sessions_[0]->timeout_timer_, disableTimer());
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
}

// Test connection close between checks affects nothing.
TEST_F(GrpcHealthCheckerImplTest, RemoteCloseBetweenChecks) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};

  expectSessionCreate();
  expectHealthcheckStart(0);
  health_checker_->start();

  expectHealthcheckStop(0);
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);

  // Connection closed between checks - nothing happens, just re-create client.
  test_sessions_[0]->client_connection_->raiseEvent(Network::ConnectionEvent::RemoteClose);

  expectClientCreate(0);
  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  // Test host state haven't changed.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

// Test that we close connections on a healthy check when reuse_connection is false.
TEST_F(GrpcHealthCheckerImplTest, DontReuseConnectionBetweenChecks) {
  setupNoReuseConnectionHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};

  expectSessionCreate();
  expectHealthcheckStart(0);
  health_checker_->start();

  expectHealthcheckStop(0);
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);

  // A new client is created because we close the connection ourselves.
  // See GrpcHealthCheckerImplTest.RemoteCloseBetweenChecks for how this works when the remote end
  // closes the connection.
  expectClientCreate(0);
  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  // Test host state haven't changed.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

// Test UNKNOWN health status is considered unhealthy.
TEST_F(GrpcHealthCheckerImplTest, GrpcFailUnknown) {
  setupHC();
  expectSingleHealthcheck(HealthTransition::Changed);
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));

  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::UNKNOWN);
  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

// Test receiving GOAWAY is interpreted as connection close event.
TEST_F(GrpcHealthCheckerImplTest, GoAwayProbeInProgress) {
  // FailureType::Network will be issued, it will render host unhealthy only if unhealthy_threshold
  // is reached.
  setupHCWithUnhealthyThreshold(1);
  expectSingleHealthcheck(HealthTransition::Changed);
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));

  test_sessions_[0]->codec_client_->raiseGoAway();

  EXPECT_TRUE(cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->healthFlagGet(
      Host::HealthFlag::FAILED_ACTIVE_HC));
  EXPECT_EQ(Host::Health::Unhealthy,
            cluster_->prioritySet().getMockHostSet(0)->hosts_[0]->health());
}

// Test receiving GOAWAY between checks affects nothing.
TEST_F(GrpcHealthCheckerImplTest, GoAwayBetweenChecks) {
  setupHC();
  cluster_->prioritySet().getMockHostSet(0)->hosts_ = {
      makeTestHost(cluster_->info_, "tcp://127.0.0.1:80")};

  expectSessionCreate();
  expectHealthcheckStart(0);
  health_checker_->start();

  expectHealthcheckStop(0);
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);

  // GOAWAY between checks should go unnoticed.
  test_sessions_[0]->codec_client_->raiseGoAway();

  expectClientCreate(0);
  expectHealthcheckStart(0);
  test_sessions_[0]->interval_timer_->invokeCallback();

  expectHealthcheckStop(0);
  // Test host state haven't changed.
  EXPECT_CALL(*this, onHostStatus(_, HealthTransition::Unchanged));
  respondServiceStatus(0, grpc::health::v1::HealthCheckResponse::SERVING);
  expectHostHealthy(true);
}

class BadResponseGrpcHealthCheckerImplTest
    : public testing::TestWithParam<GrpcHealthCheckerImplTest::ResponseSpec>,
      public GrpcHealthCheckerImplTestBase {};

INSTANTIATE_TEST_SUITE_P(
    BadResponse, BadResponseGrpcHealthCheckerImplTest,
    testing::ValuesIn(std::vector<GrpcHealthCheckerImplTest::ResponseSpec>{
        // Non-200 response.
        {
            {{":status", "500"}},
            {},
            {},
        },
        // Non-200 response with gRPC status.
        {
            {{":status", "500"}, {"grpc-status", "2"}},
            {},
            {},
        },
        // Missing content-type.
        {
            {{":status", "200"}},
            {},
            {},
        },
        // End stream on response headers.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {},
            {},
        },
        // Non-OK gRPC status in headers.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}, {"grpc-status", "2"}},
            {},
            {},
        },
        // Non-OK gRPC status
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::servingResponse()},
            {{"grpc-status", "2"}},
        },
        // Missing body.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}, {"grpc-status", "0"}},
            {},
            {},
        },
        // Compressed body.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::invalidPayload(Grpc::GRPC_FH_COMPRESSED,
                                                                     true)},
            {},
        },
        // Invalid proto message.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::invalidPayload(Grpc::GRPC_FH_DEFAULT, false)},
            {},
        },
        // Duplicate response.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::servingResponse(),
             GrpcHealthCheckerImplTest::ResponseSpec::servingResponse()},
            {},
        },
        // Invalid response.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::invalidChunk()},
            {},
        },
        // No trailers.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::servingResponse()},
            {},
        },
        // No gRPC status in trailer.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::servingResponse()},
            {{"some-header", "1"}},
        },
        // Invalid gRPC status.
        {
            {{":status", "200"}, {"content-type", "application/grpc"}},
            {GrpcHealthCheckerImplTest::ResponseSpec::servingResponse()},
            {{"grpc-status", "invalid"}},
        },
    }));

// Test different cases of invalid gRPC response makes host unhealthy.
TEST_P(BadResponseGrpcHealthCheckerImplTest, GrpcBadResponse) {
  setupHC();
  expectSingleHealthcheck(HealthTransition::Changed);
  EXPECT_CALL(*event_logger_, logUnhealthy(_, _, _, true));
  EXPECT_CALL(*event_logger_, logEjectUnhealthy(_, _, _));

  ResponseSpec spec = GetParam();
  respondResponseSpec(0, std::move(spec));
  expectHostHealthy(false);
}

TEST(Printer, HealthStatePrinter) {
  std::ostringstream healthy;
  healthy << HealthState::Healthy;
  EXPECT_EQ("Healthy", healthy.str());

  std::ostringstream unhealthy;
  unhealthy << HealthState::Unhealthy;
  EXPECT_EQ("Unhealthy", unhealthy.str());
}

TEST(Printer, HealthTransitionPrinter) {
  std::ostringstream changed;
  changed << HealthTransition::Changed;
  EXPECT_EQ("Changed", changed.str());

  std::ostringstream unchanged;
  unchanged << HealthTransition::Unchanged;
  EXPECT_EQ("Unchanged", unchanged.str());
}

TEST(HealthCheckEventLoggerImplTest, All) {
  AccessLog::MockAccessLogManager log_manager;
  std::shared_ptr<AccessLog::MockAccessLogFile> file(new AccessLog::MockAccessLogFile());
  EXPECT_CALL(log_manager, createAccessLog("foo")).WillOnce(Return(file));

  std::shared_ptr<MockHostDescription> host(new NiceMock<MockHostDescription>());
  NiceMock<MockClusterInfo> cluster;
  ON_CALL(*host, cluster()).WillByDefault(ReturnRef(cluster));

  Event::SimulatedTimeSystem time_system;
  // This is rendered as "2009-02-13T23:31:31.234Z".a
  time_system.setSystemTime(std::chrono::milliseconds(1234567891234));

  HealthCheckEventLoggerImpl event_logger(log_manager, time_system, "foo");

  EXPECT_CALL(*file, write(absl::string_view{
                         "{\"health_checker_type\":\"HTTP\",\"host\":{\"socket_address\":{"
                         "\"protocol\":\"TCP\",\"address\":\"10.0.0.1\",\"resolver_name\":\"\","
                         "\"ipv4_compat\":false,\"port_value\":443}},\"cluster_name\":\"fake_"
                         "cluster\",\"eject_unhealthy_event\":{\"failure_type\":\"ACTIVE\"},"
                         "\"timestamp\":\"2009-02-13T23:31:31.234Z\"}\n"}));
  event_logger.logEjectUnhealthy(envoy::data::core::v2alpha::HealthCheckerType::HTTP, host,
                                 envoy::data::core::v2alpha::HealthCheckFailureType::ACTIVE);

  EXPECT_CALL(*file, write(absl::string_view{
                         "{\"health_checker_type\":\"HTTP\",\"host\":{\"socket_address\":{"
                         "\"protocol\":\"TCP\",\"address\":\"10.0.0.1\",\"resolver_name\":\"\","
                         "\"ipv4_compat\":false,\"port_value\":443}},\"cluster_name\":\"fake_"
                         "cluster\",\"add_healthy_event\":{\"first_check\":false},\"timestamp\":"
                         "\"2009-02-13T23:31:31.234Z\"}\n"}));
  event_logger.logAddHealthy(envoy::data::core::v2alpha::HealthCheckerType::HTTP, host, false);

  EXPECT_CALL(*file, write(absl::string_view{
                         "{\"health_checker_type\":\"HTTP\",\"host\":{\"socket_address\":{"
                         "\"protocol\":\"TCP\",\"address\":\"10.0.0.1\",\"resolver_name\":\"\","
                         "\"ipv4_compat\":false,\"port_value\":443}},\"cluster_name\":\"fake_"
                         "cluster\",\"health_check_failure_event\":{\"failure_type\":\"ACTIVE\","
                         "\"first_check\":false},"
                         "\"timestamp\":\"2009-02-13T23:31:31.234Z\"}\n"}));
  event_logger.logUnhealthy(envoy::data::core::v2alpha::HealthCheckerType::HTTP, host,
                            envoy::data::core::v2alpha::HealthCheckFailureType::ACTIVE, false);

  EXPECT_CALL(*file, write(absl::string_view{
                         "{\"health_checker_type\":\"HTTP\",\"host\":{\"socket_address\":{"
                         "\"protocol\":\"TCP\",\"address\":\"10.0.0.1\",\"resolver_name\":\"\","
                         "\"ipv4_compat\":false,\"port_value\":443}},\"cluster_name\":\"fake_"
                         "cluster\",\"degraded_healthy_host\":{},"
                         "\"timestamp\":\"2009-02-13T23:31:31.234Z\"}\n"}));
  event_logger.logDegraded(envoy::data::core::v2alpha::HealthCheckerType::HTTP, host);

  EXPECT_CALL(*file, write(absl::string_view{
                         "{\"health_checker_type\":\"HTTP\",\"host\":{\"socket_address\":{"
                         "\"protocol\":\"TCP\",\"address\":\"10.0.0.1\",\"resolver_name\":\"\","
                         "\"ipv4_compat\":false,\"port_value\":443}},\"cluster_name\":\"fake_"
                         "cluster\",\"no_longer_degraded_host\":{},"
                         "\"timestamp\":\"2009-02-13T23:31:31.234Z\"}\n"}));
  event_logger.logNoLongerDegraded(envoy::data::core::v2alpha::HealthCheckerType::HTTP, host);
}

// Validate that the proto constraints don't allow zero length edge durations.
TEST(HealthCheckProto, Validation) {
  {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    no_traffic_interval: 0s
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";
    EXPECT_THROW_WITH_REGEX(TestUtility::validate(parseHealthCheckFromV2Yaml(yaml)), EnvoyException,
                            "Proto constraint validation failed.*value must be greater than.*");
  }
  {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    unhealthy_interval: 0s
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";
    EXPECT_THROW_WITH_REGEX(TestUtility::validate(parseHealthCheckFromV2Yaml(yaml)), EnvoyException,
                            "Proto constraint validation failed.*value must be greater than.*");
  }
  {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    unhealthy_edge_interval: 0s
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";
    EXPECT_THROW_WITH_REGEX(TestUtility::validate(parseHealthCheckFromV2Yaml(yaml)), EnvoyException,
                            "Proto constraint validation failed.*value must be greater than.*");
  }
  {
    const std::string yaml = R"EOF(
    timeout: 1s
    interval: 1s
    healthy_edge_interval: 0s
    http_health_check:
      service_name: locations
      path: /healthcheck
    )EOF";
    EXPECT_THROW_WITH_REGEX(TestUtility::validate(parseHealthCheckFromV2Yaml(yaml)), EnvoyException,
                            "Proto constraint validation failed.*value must be greater than.*");
  }
}

} // namespace
} // namespace Upstream
} // namespace Envoy
