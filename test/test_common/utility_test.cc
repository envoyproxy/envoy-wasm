#include "envoy/api/v2/discovery.pb.h"

#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {

TEST(HeaderMapEqualIgnoreOrder, ActuallyEqual) {
  Http::TestHeaderMapImpl lhs{{":method", "GET"}, {":path", "/"}, {":authority", "host"}};
  Http::TestHeaderMapImpl rhs{{":method", "GET"}, {":path", "/"}, {":authority", "host"}};
  EXPECT_TRUE(TestUtility::headerMapEqualIgnoreOrder(lhs, rhs));
  EXPECT_EQ(lhs, rhs);
}

TEST(HeaderMapEqualIgnoreOrder, IgnoreOrder) {
  Http::TestHeaderMapImpl lhs{{":method", "GET"}, {":authority", "host"}, {":path", "/"}};
  Http::TestHeaderMapImpl rhs{{":method", "GET"}, {":path", "/"}, {":authority", "host"}};
  EXPECT_TRUE(TestUtility::headerMapEqualIgnoreOrder(lhs, rhs));
  EXPECT_THAT(&lhs, HeaderMapEqualIgnoreOrder(&rhs));
  EXPECT_FALSE(lhs == rhs);
}

TEST(HeaderMapEqualIgnoreOrder, NotEqual) {
  Http::TestHeaderMapImpl lhs{{":method", "GET"}, {":authority", "host"}, {":authority", "host"}};
  Http::TestHeaderMapImpl rhs{{":method", "GET"}, {":authority", "host"}};
  EXPECT_FALSE(TestUtility::headerMapEqualIgnoreOrder(lhs, rhs));
}

TEST(ProtoEqIgnoreField, ActuallyEqual) {
  // Ignored field equal
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    lhs.set_response_nonce("nonce");
    rhs.set_response_nonce("nonce");
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_TRUE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
  // Ignored field not equal
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    lhs.set_response_nonce("nonce");
    rhs.set_response_nonce("noncense");
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_TRUE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
  // Ignored field not present
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_TRUE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
  // Ignored field only present in one
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    rhs.set_response_nonce("noncense");
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_TRUE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
}

TEST(ProtoEqIgnoreField, NotEqual) {
  // Ignored field equal
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    lhs.set_response_nonce("nonce");
    rhs.set_response_nonce("nonce");
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.Listener");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_FALSE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
  // Ignored field not equal
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    lhs.set_response_nonce("nonce");
    rhs.set_response_nonce("noncense");
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.Listener");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_FALSE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
  // Ignored field not present
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.Listener");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_FALSE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
  // Ignored field only present in one
  {
    envoy::api::v2::DeltaDiscoveryRequest lhs, rhs;
    rhs.set_response_nonce("noncense");
    lhs.set_type_url("type.googleapis.com/envoy.api.v2.Listener");
    rhs.set_type_url("type.googleapis.com/envoy.api.v2.ClusterLoadAssignment");
    EXPECT_FALSE(TestUtility::protoEqualIgnoringField(lhs, rhs, "response_nonce"));
  }
}

TEST(BuffersEqual, Aligned) {
  Buffer::OwnedImpl buffer1, buffer2;
  EXPECT_TRUE(TestUtility::buffersEqual(buffer1, buffer2));

  buffer1.appendSliceForTest("hello");
  EXPECT_FALSE(TestUtility::buffersEqual(buffer1, buffer2));
  buffer2.appendSliceForTest("hello");
  EXPECT_TRUE(TestUtility::buffersEqual(buffer1, buffer2));

  buffer1.appendSliceForTest(", world");
  EXPECT_FALSE(TestUtility::buffersEqual(buffer1, buffer2));
  buffer2.appendSliceForTest(", world");
  EXPECT_TRUE(TestUtility::buffersEqual(buffer1, buffer2));
}

TEST(BuffersEqual, NonAligned) {
  Buffer::OwnedImpl buffer1, buffer2;
  EXPECT_TRUE(TestUtility::buffersEqual(buffer1, buffer2));

  buffer1.appendSliceForTest("hello");
  EXPECT_FALSE(TestUtility::buffersEqual(buffer1, buffer2));
  buffer2.appendSliceForTest("hello");
  EXPECT_TRUE(TestUtility::buffersEqual(buffer1, buffer2));

  buffer1.appendSliceForTest(", ");
  buffer1.appendSliceForTest("world");
  EXPECT_FALSE(TestUtility::buffersEqual(buffer1, buffer2));
  buffer2.appendSliceForTest(", world");
  EXPECT_TRUE(TestUtility::buffersEqual(buffer1, buffer2));
}

} // namespace Envoy
