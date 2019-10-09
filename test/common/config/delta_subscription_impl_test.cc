#include "common/buffer/zero_copy_input_stream_impl.h"

#include "test/common/config/delta_subscription_test_harness.h"

namespace Envoy {
namespace Config {
namespace {

class DeltaSubscriptionImplTest : public DeltaSubscriptionTestHarness, public testing::Test {
protected:
  DeltaSubscriptionImplTest() = default;

  // We need to destroy the subscription before the test's destruction, because the subscription's
  // destructor removes its watch from the NewGrpcMuxImpl, and that removal process involves
  // some things held by the test fixture.
  void TearDown() override { doSubscriptionTearDown(); }
};

TEST_F(DeltaSubscriptionImplTest, UpdateResourcesCausesRequest) {
  startSubscription({"name1", "name2", "name3"});
  expectSendMessage({"name4"}, {"name1", "name2"}, Grpc::Status::GrpcStatus::Ok, "", {});
  subscription_->updateResourceInterest({"name3", "name4"});
  expectSendMessage({"name1", "name2"}, {}, Grpc::Status::GrpcStatus::Ok, "", {});
  subscription_->updateResourceInterest({"name1", "name2", "name3", "name4"});
  expectSendMessage({}, {"name1", "name2"}, Grpc::Status::GrpcStatus::Ok, "", {});
  subscription_->updateResourceInterest({"name3", "name4"});
  expectSendMessage({"name1", "name2"}, {}, Grpc::Status::GrpcStatus::Ok, "", {});
  subscription_->updateResourceInterest({"name1", "name2", "name3", "name4"});
  expectSendMessage({}, {"name1", "name2", "name3"}, Grpc::Status::GrpcStatus::Ok, "", {});
  subscription_->updateResourceInterest({"name4"});
}

// Checks that after a pause(), no requests are sent until resume().
// Also demonstrates the collapsing of subscription interest updates into a single
// request. (This collapsing happens any time multiple updates arrive before a request
// can be sent, not just with pausing: rate limiting or a down gRPC stream would also do it).
TEST_F(DeltaSubscriptionImplTest, PauseHoldsRequest) {
  startSubscription({"name1", "name2", "name3"});
  subscription_->pause();

  expectSendMessage({"name4"}, {"name1", "name2"}, Grpc::Status::GrpcStatus::Ok, "", {});
  // If not for the pause, these updates would make the expectSendMessage fail due to too many
  // messages being sent.
  subscription_->updateResourceInterest({"name3", "name4"});
  subscription_->updateResourceInterest({"name1", "name2", "name3", "name4"});
  subscription_->updateResourceInterest({"name3", "name4"});
  subscription_->updateResourceInterest({"name1", "name2", "name3", "name4"});
  subscription_->updateResourceInterest({"name3", "name4"});

  subscription_->resume();
}

TEST_F(DeltaSubscriptionImplTest, ResponseCausesAck) {
  startSubscription({"name1"});
  deliverConfigUpdate({"name1"}, "someversion", true);
}

// Checks that after a pause(), no ACK requests are sent until resume(), but that after the
// resume, *all* ACKs that arrived during the pause are sent (in order).
TEST_F(DeltaSubscriptionImplTest, PauseQueuesAcks) {
  startSubscription({"name1", "name2", "name3"});
  subscription_->pause();
  // The server gives us our first version of resource name1.
  // subscription_ now wants to ACK name1 (but can't due to pause).
  {
    auto message = std::make_unique<envoy::api::v2::DeltaDiscoveryResponse>();
    auto* resource = message->mutable_resources()->Add();
    resource->set_name("name1");
    resource->set_version("version1A");
    const std::string nonce = std::to_string(HashUtil::xxHash64("version1A"));
    message->set_nonce(nonce);
    message->set_type_url(Config::TypeUrl::get().ClusterLoadAssignment);
    nonce_acks_required_.push(nonce);
    static_cast<NewGrpcMuxImpl*>(subscription_->getContextForTest().get())
        ->onDiscoveryResponse(std::move(message));
  }
  // The server gives us our first version of resource name2.
  // subscription_ now wants to ACK name1 and then name2 (but can't due to pause).
  {
    auto message = std::make_unique<envoy::api::v2::DeltaDiscoveryResponse>();
    auto* resource = message->mutable_resources()->Add();
    resource->set_name("name2");
    resource->set_version("version2A");
    const std::string nonce = std::to_string(HashUtil::xxHash64("version2A"));
    message->set_nonce(nonce);
    message->set_type_url(Config::TypeUrl::get().ClusterLoadAssignment);
    nonce_acks_required_.push(nonce);
    static_cast<NewGrpcMuxImpl*>(subscription_->getContextForTest().get())
        ->onDiscoveryResponse(std::move(message));
  }
  // The server gives us an updated version of resource name1.
  // subscription_ now wants to ACK name1A, then name2, then name1B (but can't due to pause).
  {
    auto message = std::make_unique<envoy::api::v2::DeltaDiscoveryResponse>();
    auto* resource = message->mutable_resources()->Add();
    resource->set_name("name1");
    resource->set_version("version1B");
    const std::string nonce = std::to_string(HashUtil::xxHash64("version1B"));
    message->set_nonce(nonce);
    message->set_type_url(Config::TypeUrl::get().ClusterLoadAssignment);
    nonce_acks_required_.push(nonce);
    static_cast<NewGrpcMuxImpl*>(subscription_->getContextForTest().get())
        ->onDiscoveryResponse(std::move(message));
  }
  // All ACK sendMessage()s will happen upon calling resume().
  EXPECT_CALL(async_stream_, sendMessageRaw_(_, _))
      .WillRepeatedly(Invoke([this](Buffer::InstancePtr& buffer, bool) {
        envoy::api::v2::DeltaDiscoveryRequest message;
        EXPECT_TRUE(Grpc::Common::parseBufferInstance(std::move(buffer), message));
        const std::string nonce = message.response_nonce();
        if (!nonce.empty()) {
          nonce_acks_sent_.push(nonce);
        }
      }));
  subscription_->resume();
  // DeltaSubscriptionTestHarness's dtor will check that all ACKs were sent with the correct nonces,
  // in the correct order.
}

TEST(DeltaSubscriptionImplFixturelessTest, NoGrpcStream) {
  Stats::IsolatedStoreImpl stats_store;
  SubscriptionStats stats(Utility::generateStats(stats_store));

  envoy::api::v2::core::Node node;
  node.set_id("fo0");
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  EXPECT_CALL(local_info, node()).WillRepeatedly(testing::ReturnRef(node));

  NiceMock<Event::MockDispatcher> dispatcher;
  NiceMock<Runtime::MockRandomGenerator> random;
  Envoy::Config::RateLimitSettings rate_limit_settings;
  NiceMock<Config::MockSubscriptionCallbacks<envoy::api::v2::ClusterLoadAssignment>> callbacks;
  auto* async_client = new Grpc::MockAsyncClient();

  const Protobuf::MethodDescriptor* method_descriptor =
      Protobuf::DescriptorPool::generated_pool()->FindMethodByName(
          "envoy.api.v2.EndpointDiscoveryService.StreamEndpoints");
  std::shared_ptr<NewGrpcMuxImpl> xds_context = std::make_shared<NewGrpcMuxImpl>(
      std::unique_ptr<Grpc::MockAsyncClient>(async_client), dispatcher, *method_descriptor, random,
      stats_store, rate_limit_settings, local_info);

  std::unique_ptr<DeltaSubscriptionImpl> subscription = std::make_unique<DeltaSubscriptionImpl>(
      xds_context, Config::TypeUrl::get().ClusterLoadAssignment, callbacks, stats,
      std::chrono::milliseconds(12345), false);

  EXPECT_CALL(*async_client, startRaw(_, _, _)).WillOnce(Return(nullptr));

  subscription->start({"name1"});
  subscription->updateResourceInterest({"name1", "name2"});
}

} // namespace
} // namespace Config
} // namespace Envoy
