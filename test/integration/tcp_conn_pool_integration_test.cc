#include <list>

#include "envoy/config/bootstrap/v2/bootstrap.pb.h"
#include "envoy/server/filter_config.h"

#include "test/integration/integration.h"
#include "test/integration/utility.h"
#include "test/server/utility.h"
#include "test/test_common/registry.h"
#include "test/test_common/utility.h"

namespace Envoy {
namespace {

std::string tcp_conn_pool_config;

// Trivial Filter that obtains connections from a TCP connection pool each time onData is called
// and sends the data to the resulting upstream. The upstream's response is sent directly to
// the downstream.
class TestFilter : public Network::ReadFilter {
public:
  TestFilter(Upstream::ClusterManager& cluster_manager) : cluster_manager_(cluster_manager) {}

  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override {
    UNREFERENCED_PARAMETER(end_stream);

    Tcp::ConnectionPool::Instance* pool = cluster_manager_.tcpConnPoolForCluster(
        "cluster_0", Upstream::ResourcePriority::Default, nullptr);
    ASSERT(pool != nullptr);

    requests_.emplace_back(*this, data);
    pool->newConnection(requests_.back());

    ASSERT(data.length() == 0);
    return Network::FilterStatus::StopIteration;
  }
  Network::FilterStatus onNewConnection() override { return Network::FilterStatus::Continue; }
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override {
    read_callbacks_ = &callbacks;
  }

private:
  class Request : public Tcp::ConnectionPool::Callbacks,
                  public Tcp::ConnectionPool::UpstreamCallbacks {
  public:
    Request(TestFilter& parent, Buffer::Instance& data) : parent_(parent) { data_.move(data); }

    // Tcp::ConnectionPool::Callbacks
    void onPoolFailure(Tcp::ConnectionPool::PoolFailureReason,
                       Upstream::HostDescriptionConstSharedPtr) override {
      ASSERT(false);
    }

    void onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn,
                     Upstream::HostDescriptionConstSharedPtr) override {
      upstream_ = std::move(conn);

      upstream_->addUpstreamCallbacks(*this);
      upstream_->connection().write(data_, false);
    }

    // Tcp::ConnectionPool::UpstreamCallbacks
    void onUpstreamData(Buffer::Instance& data, bool end_stream) override {
      UNREFERENCED_PARAMETER(end_stream);

      Network::Connection& downstream = parent_.read_callbacks_->connection();
      downstream.write(data, false);

      upstream_.reset();
    }
    void onEvent(Network::ConnectionEvent) override {}
    void onAboveWriteBufferHighWatermark() override {}
    void onBelowWriteBufferLowWatermark() override {}

    TestFilter& parent_;
    Buffer::OwnedImpl data_;
    Tcp::ConnectionPool::ConnectionDataPtr upstream_;
  };

  Upstream::ClusterManager& cluster_manager_;
  Network::ReadFilterCallbacks* read_callbacks_{};
  std::list<Request> requests_;
};

class TestFilterConfigFactory : public Server::Configuration::NamedNetworkFilterConfigFactory {
public:
  // NamedNetworkFilterConfigFactory
  Network::FilterFactoryCb
  createFilterFactory(const Json::Object&,
                      Server::Configuration::FactoryContext& context) override {
    return [&context](Network::FilterManager& filter_manager) -> void {
      filter_manager.addReadFilter(std::make_shared<TestFilter>(context.clusterManager()));
    };
  }

  Network::FilterFactoryCb
  createFilterFactoryFromProto(const Protobuf::Message&,
                               Server::Configuration::FactoryContext& context) override {
    return [&context](Network::FilterManager& filter_manager) -> void {
      filter_manager.addReadFilter(std::make_shared<TestFilter>(context.clusterManager()));
    };
  }

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return ProtobufTypes::MessagePtr{new Envoy::ProtobufWkt::Empty()};
  }

  std::string name() override { CONSTRUCT_ON_FIRST_USE(std::string, "envoy.test.router"); }
  bool isTerminalFilter() override { return true; }
};

} // namespace

class TcpConnPoolIntegrationTest : public testing::TestWithParam<Network::Address::IpVersion>,
                                   public BaseIntegrationTest {
public:
  TcpConnPoolIntegrationTest()
      : BaseIntegrationTest(GetParam(), tcp_conn_pool_config), filter_resolver_(config_factory_) {}

  // Called once by the gtest framework before any tests are run.
  static void SetUpTestSuite() {
    tcp_conn_pool_config = ConfigHelper::BASE_CONFIG + R"EOF(
    filter_chains:
      - filters:
        - name: envoy.test.router
          config:
      )EOF";
  }

  // Initializer for individual tests.
  void SetUp() override { BaseIntegrationTest::initialize(); }

  // Destructor for individual tests.
  void TearDown() override {
    test_server_.reset();
    fake_upstreams_.clear();
  }

private:
  TestFilterConfigFactory config_factory_;
  Registry::InjectFactory<Server::Configuration::NamedNetworkFilterConfigFactory> filter_resolver_;
};

INSTANTIATE_TEST_SUITE_P(IpVersions, TcpConnPoolIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(TcpConnPoolIntegrationTest, SingleRequest) {
  std::string request("request");
  std::string response("response");

  IntegrationTcpClientPtr tcp_client = makeTcpConnection(lookupPort("listener_0"));
  tcp_client->write(request);

  FakeRawConnectionPtr fake_upstream_connection;
  ASSERT_TRUE(fake_upstreams_[0]->waitForRawConnection(fake_upstream_connection));
  ASSERT_TRUE(fake_upstream_connection->waitForData(request.size()));
  ASSERT_TRUE(fake_upstream_connection->write(response));

  tcp_client->waitForData(response);
  tcp_client->close();
}

TEST_P(TcpConnPoolIntegrationTest, MultipleRequests) {
  std::string request1("request1");
  std::string request2("request2");
  std::string response1("response1");
  std::string response2("response2");

  IntegrationTcpClientPtr tcp_client = makeTcpConnection(lookupPort("listener_0"));

  // send request 1
  tcp_client->write(request1);
  FakeRawConnectionPtr fake_upstream_connection1;
  ASSERT_TRUE(fake_upstreams_[0]->waitForRawConnection(fake_upstream_connection1));
  std::string data;
  ASSERT_TRUE(fake_upstream_connection1->waitForData(request1.size(), &data));
  EXPECT_EQ(request1, data);

  // send request 2
  tcp_client->write(request2);
  FakeRawConnectionPtr fake_upstream_connection2;
  ASSERT_TRUE(fake_upstreams_[0]->waitForRawConnection(fake_upstream_connection2));
  ASSERT_TRUE(fake_upstream_connection2->waitForData(request2.size(), &data));
  EXPECT_EQ(request2, data);

  // send response 2
  ASSERT_TRUE(fake_upstream_connection2->write(response2));
  tcp_client->waitForData(response2);

  // send response 1
  ASSERT_TRUE(fake_upstream_connection1->write(response1));
  tcp_client->waitForData(response1, false);

  tcp_client->close();
}

} // namespace Envoy
