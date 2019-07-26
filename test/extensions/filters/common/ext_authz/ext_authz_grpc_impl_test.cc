#include "envoy/api/v2/core/base.pb.h"

#include "common/grpc/common.h"
#include "common/http/headers.h"
#include "common/protobuf/protobuf.h"

#include "extensions/filters/common/ext_authz/ext_authz_grpc_impl.h"

#include "test/extensions/filters/common/ext_authz/mocks.h"
#include "test/extensions/filters/common/ext_authz/test_common.h"
#include "test/mocks/grpc/mocks.h"
#include "test/mocks/upstream/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::Invoke;
using testing::Ref;
using testing::Return;
using testing::Values;
using testing::WhenDynamicCastTo;

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace ExtAuthz {

constexpr char V2[] = "envoy.service.auth.v2.Authorization";
constexpr char V2Alpha[] = "envoy.service.auth.v2alpha.Authorization";

class ExtAuthzGrpcClientTest : public testing::TestWithParam<bool> {
public:
  ExtAuthzGrpcClientTest() : async_client_(new Grpc::MockAsyncClient()), timeout_(10) {}

  void initialize(bool use_alpha) {
    use_alpha_ = use_alpha;
    client_ = std::make_unique<GrpcClientImpl>(Grpc::RawAsyncClientPtr{async_client_}, timeout_,
                                               use_alpha_);
  }

  void expectCallSend(envoy::service::auth::v2::CheckRequest& request) {
    EXPECT_CALL(*async_client_,
                sendRaw(_, _, Grpc::ProtoBufferEq(request), Ref(*(client_.get())), _, _))
        .WillOnce(Invoke(
            [this](
                absl::string_view service_full_name, absl::string_view method_name,
                Buffer::InstancePtr&&, Grpc::RawAsyncRequestCallbacks&, Tracing::Span&,
                const absl::optional<std::chrono::milliseconds>& timeout) -> Grpc::AsyncRequest* {
              EXPECT_EQ(use_alpha_ ? V2Alpha : V2, service_full_name);
              EXPECT_EQ("Check", method_name);
              EXPECT_EQ(timeout_->count(), timeout->count());
              return &async_request_;
            }));
  }

  Grpc::MockAsyncClient* async_client_;
  absl::optional<std::chrono::milliseconds> timeout_;
  Grpc::MockAsyncRequest async_request_;
  std::unique_ptr<GrpcClientImpl> client_;
  MockRequestCallbacks request_callbacks_;
  Tracing::MockSpan span_;
  bool use_alpha_{};
};

INSTANTIATE_TEST_SUITE_P(Parameterized, ExtAuthzGrpcClientTest, Values(true, false));

// Test the client when an ok response is received.
TEST_P(ExtAuthzGrpcClientTest, AuthorizationOk) {
  initialize(GetParam());

  auto check_response = std::make_unique<envoy::service::auth::v2::CheckResponse>();
  auto status = check_response->mutable_status();
  status->set_code(Grpc::Status::GrpcStatus::Ok);
  auto authz_response = Response{};
  authz_response.status = CheckStatus::OK;

  envoy::service::auth::v2::CheckRequest request;
  expectCallSend(request);
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  Http::HeaderMapImpl headers;
  client_->onCreateInitialMetadata(headers);

  EXPECT_CALL(span_, setTag(Eq("ext_authz_status"), Eq("ext_authz_ok")));
  EXPECT_CALL(request_callbacks_, onComplete_(WhenDynamicCastTo<ResponsePtr&>(
                                      AuthzResponseNoAttributes(authz_response))));
  client_->onSuccess(std::move(check_response), span_);
}

// Test the client when an ok response is received.
TEST_P(ExtAuthzGrpcClientTest, AuthorizationOkWithAllAtributes) {
  initialize(GetParam());

  const std::string empty_body{};
  const auto expected_headers = TestCommon::makeHeaderValueOption({{"foo", "bar", false}});
  auto check_response = TestCommon::makeCheckResponse(
      Grpc::Status::GrpcStatus::Ok, envoy::type::StatusCode::OK, empty_body, expected_headers);
  auto authz_response =
      TestCommon::makeAuthzResponse(CheckStatus::OK, Http::Code::OK, empty_body, expected_headers);

  envoy::service::auth::v2::CheckRequest request;
  expectCallSend(request);
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  Http::HeaderMapImpl headers;
  client_->onCreateInitialMetadata(headers);

  EXPECT_CALL(span_, setTag(Eq("ext_authz_status"), Eq("ext_authz_ok")));
  EXPECT_CALL(request_callbacks_,
              onComplete_(WhenDynamicCastTo<ResponsePtr&>(AuthzOkResponse(authz_response))));
  client_->onSuccess(std::move(check_response), span_);
}

// Test the client when a denied response is received.
TEST_P(ExtAuthzGrpcClientTest, AuthorizationDenied) {
  initialize(GetParam());

  auto check_response = std::make_unique<envoy::service::auth::v2::CheckResponse>();
  auto status = check_response->mutable_status();
  status->set_code(Grpc::Status::GrpcStatus::PermissionDenied);
  auto authz_response = Response{};
  authz_response.status = CheckStatus::Denied;

  envoy::service::auth::v2::CheckRequest request;
  expectCallSend(request);
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  Http::HeaderMapImpl headers;
  client_->onCreateInitialMetadata(headers);
  EXPECT_EQ(nullptr, headers.RequestId());
  EXPECT_CALL(span_, setTag(Eq("ext_authz_status"), Eq("ext_authz_unauthorized")));
  EXPECT_CALL(request_callbacks_, onComplete_(WhenDynamicCastTo<ResponsePtr&>(
                                      AuthzResponseNoAttributes(authz_response))));

  client_->onSuccess(std::move(check_response), span_);
}

// Test the client when a gRPC status code unknown is received from the authorization server.
TEST_P(ExtAuthzGrpcClientTest, AuthorizationDeniedGrpcUnknownStatus) {
  initialize(GetParam());

  auto check_response = std::make_unique<envoy::service::auth::v2::CheckResponse>();
  auto status = check_response->mutable_status();
  status->set_code(Grpc::Status::GrpcStatus::Unknown);
  auto authz_response = Response{};
  authz_response.status = CheckStatus::Denied;

  envoy::service::auth::v2::CheckRequest request;
  expectCallSend(request);
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  Http::HeaderMapImpl headers;
  client_->onCreateInitialMetadata(headers);
  EXPECT_EQ(nullptr, headers.RequestId());
  EXPECT_CALL(span_, setTag(Eq("ext_authz_status"), Eq("ext_authz_unauthorized")));
  EXPECT_CALL(request_callbacks_, onComplete_(WhenDynamicCastTo<ResponsePtr&>(
                                      AuthzResponseNoAttributes(authz_response))));

  client_->onSuccess(std::move(check_response), span_);
}

// Test the client when a denied response with additional HTTP attributes is received.
TEST_P(ExtAuthzGrpcClientTest, AuthorizationDeniedWithAllAttributes) {
  initialize(GetParam());

  const std::string expected_body{"test"};
  const auto expected_headers =
      TestCommon::makeHeaderValueOption({{"foo", "bar", false}, {"foobar", "bar", true}});
  auto check_response = TestCommon::makeCheckResponse(Grpc::Status::GrpcStatus::PermissionDenied,
                                                      envoy::type::StatusCode::Unauthorized,
                                                      expected_body, expected_headers);
  auto authz_response = TestCommon::makeAuthzResponse(CheckStatus::Denied, Http::Code::Unauthorized,
                                                      expected_body, expected_headers);

  envoy::service::auth::v2::CheckRequest request;
  expectCallSend(request);
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  Http::HeaderMapImpl headers;
  client_->onCreateInitialMetadata(headers);
  EXPECT_EQ(nullptr, headers.RequestId());
  EXPECT_CALL(span_, setTag(Eq("ext_authz_status"), Eq("ext_authz_unauthorized")));
  EXPECT_CALL(request_callbacks_,
              onComplete_(WhenDynamicCastTo<ResponsePtr&>(AuthzDeniedResponse(authz_response))));

  client_->onSuccess(std::move(check_response), span_);
}

// Test the client when an unknown error occurs.
TEST_P(ExtAuthzGrpcClientTest, UnknownError) {
  initialize(GetParam());

  envoy::service::auth::v2::CheckRequest request;
  expectCallSend(request);
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  EXPECT_CALL(request_callbacks_,
              onComplete_(WhenDynamicCastTo<ResponsePtr&>(AuthzErrorResponse(CheckStatus::Error))));
  client_->onFailure(Grpc::Status::Unknown, "", span_);
}

// Test the client when the request is canceled.
TEST_P(ExtAuthzGrpcClientTest, CancelledAuthorizationRequest) {
  initialize(GetParam());

  envoy::service::auth::v2::CheckRequest request;
  EXPECT_CALL(*async_client_, sendRaw(_, _, _, _, _, _)).WillOnce(Return(&async_request_));
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  EXPECT_CALL(async_request_, cancel());
  client_->cancel();
}

// Test the client when the request times out.
TEST_P(ExtAuthzGrpcClientTest, AuthorizationRequestTimeout) {
  initialize(GetParam());

  envoy::service::auth::v2::CheckRequest request;
  expectCallSend(request);
  client_->check(request_callbacks_, request, Tracing::NullSpan::instance());

  EXPECT_CALL(request_callbacks_,
              onComplete_(WhenDynamicCastTo<ResponsePtr&>(AuthzErrorResponse(CheckStatus::Error))));
  client_->onFailure(Grpc::Status::DeadlineExceeded, "", span_);
}

} // namespace ExtAuthz
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
