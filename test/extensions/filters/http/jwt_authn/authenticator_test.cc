#include "envoy/config/core/v3/http_uri.pb.h"
#include "envoy/extensions/filters/http/jwt_authn/v3/config.pb.h"

#include "common/http/message_impl.h"
#include "common/protobuf/utility.h"

#include "extensions/filters/http/common/jwks_fetcher.h"
#include "extensions/filters/http/jwt_authn/authenticator.h"
#include "extensions/filters/http/jwt_authn/filter_config.h"

#include "test/extensions/filters/http/common/mock.h"
#include "test/extensions/filters/http/jwt_authn/mock.h"
#include "test/extensions/filters/http/jwt_authn/test_common.h"
#include "test/mocks/server/factory_context.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

using envoy::extensions::filters::http::jwt_authn::v3::JwtAuthentication;
using Envoy::Extensions::HttpFilters::Common::JwksFetcher;
using Envoy::Extensions::HttpFilters::Common::JwksFetcherPtr;
using Envoy::Extensions::HttpFilters::Common::MockJwksFetcher;
using ::google::jwt_verify::Jwks;
using ::google::jwt_verify::Status;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace JwtAuthn {
namespace {

class AuthenticatorTest : public testing::Test {
public:
  void SetUp() override {
    TestUtility::loadFromYaml(ExampleConfig, proto_config_);
    createAuthenticator();
  }

  void createAuthenticator(
      ::google::jwt_verify::CheckAudience* check_audience = nullptr,
      const absl::optional<std::string>& provider = absl::make_optional<std::string>(ProviderName),
      bool allow_failed = false, bool allow_missing = false) {
    filter_config_ = FilterConfigImpl::create(proto_config_, "", mock_factory_ctx_);
    raw_fetcher_ = new MockJwksFetcher;
    fetcher_.reset(raw_fetcher_);
    auth_ = Authenticator::create(
        check_audience, provider, allow_failed, allow_missing,
        filter_config_->getCache().getJwksCache(), filter_config_->cm(),
        [this](Upstream::ClusterManager&) { return std::move(fetcher_); },
        filter_config_->timeSource());
    jwks_ = Jwks::createFrom(PublicKey, Jwks::JWKS);
    EXPECT_TRUE(jwks_->getStatus() == Status::Ok);
  }

  void expectVerifyStatus(Status expected_status, Http::RequestHeaderMap& headers) {
    std::function<void(const Status&)> on_complete_cb = [&expected_status](const Status& status) {
      ASSERT_EQ(status, expected_status);
    };
    auto set_payload_cb = [this](const std::string& name, const ProtobufWkt::Struct& payload) {
      out_name_ = name;
      out_payload_ = payload;
    };
    initTokenExtractor();
    auto tokens = extractor_->extract(headers);
    auth_->verify(headers, parent_span_, std::move(tokens), std::move(set_payload_cb),
                  std::move(on_complete_cb));
  }

  void initTokenExtractor() {
    JwtProviderList providers;
    for (const auto& it : proto_config_.providers()) {
      providers.emplace_back(&it.second);
    }
    extractor_ = Extractor::create(providers);
  }

  JwtAuthentication proto_config_;
  ExtractorConstPtr extractor_;
  std::shared_ptr<FilterConfigImpl> filter_config_;
  MockJwksFetcher* raw_fetcher_;
  JwksFetcherPtr fetcher_;
  AuthenticatorPtr auth_;
  ::google::jwt_verify::JwksPtr jwks_;
  NiceMock<Server::Configuration::MockFactoryContext> mock_factory_ctx_;
  std::string out_name_;
  ProtobufWkt::Struct out_payload_;
  NiceMock<Tracing::MockSpan> parent_span_;
};

// This test validates a good JWT authentication with a remote Jwks.
// It also verifies Jwks cache with 10 JWT authentications, but only one Jwks fetch.
TEST_F(AuthenticatorTest, TestOkJWTandCache) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  // Test OK pubkey and its cache
  for (int i = 0; i < 10; i++) {
    Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};

    expectVerifyStatus(Status::Ok, headers);

    EXPECT_EQ(headers.get_("sec-istio-auth-userinfo"), ExpectedPayloadValue);
    // Verify the token is removed.
    EXPECT_FALSE(headers.has(Http::CustomHeaders::get().Authorization));
  }
}

// This test verifies the Jwt is forwarded if "forward" flag is set.
TEST_F(AuthenticatorTest, TestForwardJwt) {
  // Config forward_jwt flag
  (*proto_config_.mutable_providers())[std::string(ProviderName)].set_forward(true);
  createAuthenticator();
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  // Test OK pubkey and its cache
  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};

  expectVerifyStatus(Status::Ok, headers);

  // Verify the token is NOT removed.
  EXPECT_TRUE(headers.has(Http::CustomHeaders::get().Authorization));

  // Payload not set by default
  EXPECT_EQ(out_name_, "");
}

// This test verifies the Jwt payload is set.
TEST_F(AuthenticatorTest, TestSetPayload) {
  // Config payload_in_metadata flag
  (*proto_config_.mutable_providers())[std::string(ProviderName)].set_payload_in_metadata(
      "my_payload");
  createAuthenticator();
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  // Test OK pubkey and its cache
  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};

  expectVerifyStatus(Status::Ok, headers);

  // Payload is set
  EXPECT_EQ(out_name_, "my_payload");

  ProtobufWkt::Struct expected_payload;
  TestUtility::loadFromJson(ExpectedPayloadJSON, expected_payload);
  EXPECT_TRUE(TestUtility::protoEqual(out_payload_, expected_payload));
}

// This test verifies the Jwt with non existing kid
TEST_F(AuthenticatorTest, TestJwtWithNonExistKid) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  // Test OK pubkey and its cache
  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(NonExistKidToken)}};

  expectVerifyStatus(Status::JwtVerificationFail, headers);
}

// This test verifies the Jwt without "iss" work
TEST_F(AuthenticatorTest, TestJwtWithoutIss) {
  jwks_ = Jwks::createFrom(ES256PublicKey, Jwks::JWKS);
  EXPECT_TRUE(jwks_->getStatus() == Status::Ok);

  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  // Test OK pubkey and its cache
  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(ES256WithoutIssToken)}};

  expectVerifyStatus(Status::Ok, headers);
}

// This test verifies if Jwt is missing, proper status is called.
TEST_F(AuthenticatorTest, TestMissedJWT) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  // Empty headers.
  Http::TestRequestHeaderMapImpl headers{};

  expectVerifyStatus(Status::JwtMissed, headers);
}

// Test multiple tokens; the one from query parameter is bad, verification should fail.
TEST_F(AuthenticatorTest, TestMultipleJWTOneBadFromQuery) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(1);

  // headers with multiple tokens: one good, one bad
  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(GoodToken)},
      {":path", "/foo?access_token=" + std::string(NonExistKidToken)},
  };

  expectVerifyStatus(Status::JwtVerificationFail, headers);
}

// Test multiple tokens; the one from header is bad, verification should fail.
TEST_F(AuthenticatorTest, TestMultipleJWTOneBadFromHeader) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(1);

  // headers with multiple tokens: one good, one bad
  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(NonExistKidToken)},
      {":path", "/foo?access_token=" + std::string(GoodToken)},
  };

  expectVerifyStatus(Status::JwtVerificationFail, headers);
}

// Test multiple tokens; all are good, verification is ok.
TEST_F(AuthenticatorTest, TestMultipleJWTAllGood) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(1);

  // headers with multiple tokens: all are good
  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(GoodToken)},
      {":path", "/foo?access_token=" + std::string(GoodToken)},
  };

  expectVerifyStatus(Status::Ok, headers);
}

// Test multiple tokens; one of them is bad and allow_failed, verification is ok.
TEST_F(AuthenticatorTest, TestMultipleJWTOneBadAllowFails) {
  createAuthenticator(nullptr, absl::make_optional<std::string>(ProviderName),
                      /*allow_failed=*/true, /*all_missing=*/false);
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(1);

  // headers with multiple tokens: one good, one bad
  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(GoodToken)},
      {":path", "/foo?access_token=" + std::string(NonExistKidToken)},
  };

  expectVerifyStatus(Status::Ok, headers);
}

// Test empty header and allow_missing, verification is ok.
TEST_F(AuthenticatorTest, TestAllowMissingWithEmptyHeader) {
  createAuthenticator(nullptr, absl::make_optional<std::string>(ProviderName),
                      /*allow_failed=*/false, /*all_missing=*/true);
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  // Empty headers
  Http::TestRequestHeaderMapImpl headers{};

  expectVerifyStatus(Status::Ok, headers);
}

// This test verifies if Jwt is invalid, JwtBadFormat status is returned.
TEST_F(AuthenticatorTest, TestInvalidJWT) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  std::string token = "invalidToken";
  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + token}};
  expectVerifyStatus(Status::JwtBadFormat, headers);
}

// This test verifies if Authorization header has invalid prefix, JwtMissed status is returned
TEST_F(AuthenticatorTest, TestInvalidPrefix) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer-invalid"}};
  expectVerifyStatus(Status::JwtMissed, headers);
}

// This test verifies when a JWT is non-expiring without audience specified, JwtAudienceNotAllowed
// is returned.
TEST_F(AuthenticatorTest, TestNonExpiringJWT) {
  EXPECT_CALL(mock_factory_ctx_.cluster_manager_, httpAsyncClientForCluster(_)).Times(0);

  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(NonExpiringToken)}};
  expectVerifyStatus(Status::JwtAudienceNotAllowed, headers);
}

// This test verifies when a JWT is expired, JwtExpired status is returned.
TEST_F(AuthenticatorTest, TestExpiredJWT) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(ExpiredToken)}};
  expectVerifyStatus(Status::JwtExpired, headers);
}

// This test verifies when a JWT is not yet valid, JwtNotYetValid status is returned.
TEST_F(AuthenticatorTest, TestNotYetValidJWT) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(NotYetValidToken)}};
  expectVerifyStatus(Status::JwtNotYetValid, headers);
}

// This test verifies when an inline JWKS is misconfigured, JwksNoValidKeys is returns
TEST_F(AuthenticatorTest, TestInvalidLocalJwks) {
  auto& provider = (*proto_config_.mutable_providers())[std::string(ProviderName)];
  provider.clear_remote_jwks();
  provider.mutable_local_jwks()->set_inline_string("invalid");
  createAuthenticator();

  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};
  expectVerifyStatus(Status::JwksNoValidKeys, headers);
}

// This test verifies when a JWT is with invalid audience, JwtAudienceNotAllowed is returned.
TEST_F(AuthenticatorTest, TestNonMatchAudJWT) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(InvalidAudToken)}};
  expectVerifyStatus(Status::JwtAudienceNotAllowed, headers);
}

// This test verifies when Jwt issuer is not configured, JwtUnknownIssuer is returned.
TEST_F(AuthenticatorTest, TestIssuerNotFound) {
  // Create a config with an other issuer.
  (*proto_config_.mutable_providers())[std::string(ProviderName)].set_issuer("other_issuer");
  createAuthenticator();

  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(0);

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};
  expectVerifyStatus(Status::JwtUnknownIssuer, headers);
}

// This test verifies that when Jwks fetching fails, JwksFetchFail status is returned.
TEST_F(AuthenticatorTest, TestPubkeyFetchFail) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                          JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksError(JwksFetcher::JwksReceiver::Failure::InvalidJwks);
      }));

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};
  expectVerifyStatus(Status::JwksFetchFail, headers);

  Http::ResponseMessagePtr response_message(new Http::ResponseMessageImpl(
      Http::ResponseHeaderMapPtr{new Http::TestResponseHeaderMapImpl{{":status", "401"}}}));
}

// This test verifies when a Jwks fetching is not completed yet, but onDestroy() is called,
// onComplete() callback should not be called, but internal request->cancel() should be called.
// Most importantly, no crash.
TEST_F(AuthenticatorTest, TestOnDestroy) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _)).Times(1);

  // Cancel is called once.
  EXPECT_CALL(*raw_fetcher_, cancel()).Times(1);

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};
  initTokenExtractor();
  auto tokens = extractor_->extract(headers);
  // callback should not be called.
  std::function<void(const Status&)> on_complete_cb = [](const Status&) { FAIL(); };
  auth_->verify(headers, parent_span_, std::move(tokens), nullptr, std::move(on_complete_cb));

  // Destroy the authenticating process.
  auth_->onDestroy();
}

// This test verifies if "forward_payload_header" is empty, payload is not forwarded.
TEST_F(AuthenticatorTest, TestNoForwardPayloadHeader) {
  // In this config, there is no forward_payload_header
  auto& provider0 = (*proto_config_.mutable_providers())[std::string(ProviderName)];
  provider0.clear_forward_payload_header();
  createAuthenticator();
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};
  expectVerifyStatus(Status::Ok, headers);

  // Test when forward_payload_header is not set, the output should NOT
  // contain the sec-istio-auth-userinfo header.
  EXPECT_FALSE(headers.has("sec-istio-auth-userinfo"));
}

// This test verifies that allow failed authenticator will verify all tokens.
TEST_F(AuthenticatorTest, TestAllowFailedMultipleTokens) {
  auto& provider = (*proto_config_.mutable_providers())[std::string(ProviderName)];
  std::vector<std::string> names = {"a", "b", "c"};
  for (const auto& it : names) {
    auto header = provider.add_from_headers();
    header->set_name(it);
    header->set_value_prefix("Bearer ");
  }

  createAuthenticator(nullptr, absl::nullopt, /*allow_failed=*/true);
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  Http::TestRequestHeaderMapImpl headers1{
      {"a", "Bearer " + std::string(ExpiredToken)},
      {"b", "Bearer " + std::string(GoodToken)},
      {"c", "Bearer " + std::string(InvalidAudToken)},
      {":path", "/"},
  };
  expectVerifyStatus(Status::Ok, headers1);

  EXPECT_TRUE(headers1.has("a"));
  EXPECT_FALSE(headers1.has("b"));
  EXPECT_TRUE(headers1.has("c"));

  Http::TestRequestHeaderMapImpl headers2{
      {"a", "Bearer " + std::string(GoodToken)},
      {"b", "Bearer " + std::string(GoodToken)},
      {"c", "Bearer " + std::string(GoodToken)},
      {":path", "/"},
  };
  expectVerifyStatus(Status::Ok, headers2);

  EXPECT_FALSE(headers2.has("a"));
  EXPECT_FALSE(headers2.has("b"));
  EXPECT_FALSE(headers2.has("c"));
}

// This test verifies that allow failed authenticator will verify all tokens.
TEST_F(AuthenticatorTest, TestAllowFailedMultipleIssuers) {
  auto& provider = (*proto_config_.mutable_providers())["other_provider"];
  provider.set_issuer("https://other.com");
  provider.add_audiences("other_service");
  auto& uri = *provider.mutable_remote_jwks()->mutable_http_uri();
  uri.set_uri("https://pubkey_server/pubkey_path");
  uri.set_cluster("pubkey_cluster");
  auto header = provider.add_from_headers();
  header->set_name("expired-auth");
  header->set_value_prefix("Bearer ");
  header = provider.add_from_headers();
  header->set_name("other-auth");
  header->set_value_prefix("Bearer ");

  createAuthenticator(nullptr, absl::nullopt, /*allow_failed=*/true);
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .Times(2)
      .WillRepeatedly(Invoke([](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                                JwksFetcher::JwksReceiver& receiver) {
        ::google::jwt_verify::JwksPtr jwks = Jwks::createFrom(PublicKey, Jwks::JWKS);
        EXPECT_TRUE(jwks->getStatus() == Status::Ok);
        receiver.onJwksSuccess(std::move(jwks));
      }));

  Http::TestRequestHeaderMapImpl headers{
      {"Authorization", "Bearer " + std::string(GoodToken)},
      {"expired-auth", "Bearer " + std::string(ExpiredToken)},
      {"other-auth", "Bearer " + std::string(OtherGoodToken)},
      {":path", "/"},
  };
  expectVerifyStatus(Status::Ok, headers);

  EXPECT_FALSE(headers.has("Authorization"));
  EXPECT_TRUE(headers.has("expired-auth"));
  EXPECT_FALSE(headers.has("other-auth"));
}

// Test checks that supplying a CheckAudience to auth will override the one in JwksCache.
TEST_F(AuthenticatorTest, TestCustomCheckAudience) {
  auto check_audience = std::make_unique<::google::jwt_verify::CheckAudience>(
      std::vector<std::string>{"invalid_service"});
  createAuthenticator(check_audience.get());
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([this](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                              JwksFetcher::JwksReceiver& receiver) {
        receiver.onJwksSuccess(std::move(jwks_));
      }));

  Http::TestRequestHeaderMapImpl headers1{
      {"Authorization", "Bearer " + std::string(InvalidAudToken)}};
  expectVerifyStatus(Status::Ok, headers1);

  Http::TestRequestHeaderMapImpl headers2{{"Authorization", "Bearer " + std::string(GoodToken)}};
  expectVerifyStatus(Status::JwtAudienceNotAllowed, headers2);
}

// This test verifies that when invalid JWKS is fetched, an JWKS error status is returned.
TEST_F(AuthenticatorTest, TestInvalidPubkeyKey) {
  EXPECT_CALL(*raw_fetcher_, fetch(_, _, _))
      .WillOnce(Invoke([](const envoy::config::core::v3::HttpUri&, Tracing::Span&,
                          JwksFetcher::JwksReceiver& receiver) {
        auto jwks = Jwks::createFrom(PublicKey, Jwks::PEM);
        receiver.onJwksSuccess(std::move(jwks));
      }));

  Http::TestRequestHeaderMapImpl headers{{"Authorization", "Bearer " + std::string(GoodToken)}};
  expectVerifyStatus(Status::JwksPemBadBase64, headers);
}

} // namespace
} // namespace JwtAuthn
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
