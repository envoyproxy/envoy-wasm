#include "extensions/filters/http/jwt_authn/filter.h"
#include "extensions/filters/http/well_known_names.h"

#include "test/extensions/filters/http/jwt_authn/mock.h"
#include "test/mocks/server/mocks.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::envoy::config::filter::http::jwt_authn::v2alpha::JwtAuthentication;
using ::google::jwt_verify::Status;

using testing::_;
using testing::Invoke;
using testing::Return;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace JwtAuthn {
namespace {

class MockMatcher : public Matcher {
public:
  MOCK_CONST_METHOD1(matches, bool(const Http::HeaderMap& headers));
};

class MockFilterConfig : public FilterConfig {
public:
  MockFilterConfig(
      const ::envoy::config::filter::http::jwt_authn::v2alpha::JwtAuthentication& proto_config,
      const std::string& stats_prefix, Server::Configuration::FactoryContext& context)
      : FilterConfig(proto_config, stats_prefix, context) {}
  MOCK_CONST_METHOD2(findVerifier, const Verifier*(const Http::HeaderMap& headers,
                                                   const StreamInfo::FilterState& filter_state));
};

class FilterTest : public testing::Test {
public:
  void SetUp() override {
    mock_config_ = ::std::make_shared<MockFilterConfig>(proto_config_, "", mock_context_);

    mock_verifier_ = std::make_unique<MockVerifier>();
    filter_ = std::make_unique<Filter>(mock_config_);
    filter_->setDecoderFilterCallbacks(filter_callbacks_);
  }

  void setupMockConfig() {
    EXPECT_CALL(*mock_config_.get(), findVerifier(_, _)).WillOnce(Return(mock_verifier_.get()));
  }

  JwtAuthentication proto_config_;
  NiceMock<Server::Configuration::MockFactoryContext> mock_context_;
  std::shared_ptr<MockFilterConfig> mock_config_;
  NiceMock<Http::MockStreamDecoderFilterCallbacks> filter_callbacks_;
  std::unique_ptr<Filter> filter_;
  std::unique_ptr<MockVerifier> mock_verifier_;
  NiceMock<MockVerifierCallbacks> verifier_callback_;
};

// This test verifies Verifier::Callback is called inline with OK status.
// All functions should return Continue.
TEST_F(FilterTest, InlineOK) {
  setupMockConfig();
  // A successful authentication completed inline: callback is called inside verify().
  EXPECT_CALL(*mock_verifier_, verify(_)).WillOnce(Invoke([](ContextSharedPtr context) {
    context->callback()->onComplete(Status::Ok);
  }));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(headers, false));
  Http::MetadataMap metadata_map{{"metadata", "metadata"}};
  EXPECT_EQ(Http::FilterMetadataStatus::Continue, filter_->decodeMetadata(metadata_map));
  EXPECT_EQ(1U, mock_config_->stats().allowed_.value());

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));
}

// This test verifies the setPayload call is handled correctly
TEST_F(FilterTest, TestSetPayloadCall) {
  setupMockConfig();
  ProtobufWkt::Struct payload;
  // A successful authentication completed inline: callback is called inside verify().
  EXPECT_CALL(*mock_verifier_, verify(_)).WillOnce(Invoke([&payload](ContextSharedPtr context) {
    context->callback()->setPayload(payload);
    context->callback()->onComplete(Status::Ok);
  }));

  EXPECT_CALL(filter_callbacks_.stream_info_, setDynamicMetadata(_, _))
      .WillOnce(Invoke([&payload](const std::string& ns, const ProtobufWkt::Struct& out_payload) {
        EXPECT_EQ(ns, HttpFilterNames::get().JwtAuthn);
        EXPECT_TRUE(TestUtility::protoEqual(out_payload, payload));
      }));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(headers, false));
  EXPECT_EQ(1U, mock_config_->stats().allowed_.value());

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));
}

// This test verifies Verifier::Callback is called inline with a failure(401 Unauthorized) status.
// All functions should return Continue except decodeHeaders(), it returns StopIteration.
TEST_F(FilterTest, InlineUnauthorizedFailure) {
  setupMockConfig();
  // A failed authentication completed inline: callback is called inside verify().

  EXPECT_CALL(filter_callbacks_, sendLocalReply(Http::Code::Unauthorized, _, _, _, _));
  EXPECT_CALL(*mock_verifier_, verify(_)).WillOnce(Invoke([](ContextSharedPtr context) {
    context->callback()->onComplete(Status::JwtBadFormat);
  }));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_->decodeHeaders(headers, false));
  EXPECT_EQ(1U, mock_config_->stats().denied_.value());

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));
  EXPECT_EQ("jwt_authn_access_denied", filter_callbacks_.details_);
}

// This test verifies Verifier::Callback is called inline with a failure(403 Forbidden) status.
// All functions should return Continue except decodeHeaders(), it returns StopIteration.
TEST_F(FilterTest, InlineForbiddenFailure) {
  setupMockConfig();
  // A failed authentication completed inline: callback is called inside verify().

  EXPECT_CALL(filter_callbacks_, sendLocalReply(Http::Code::Forbidden, _, _, _, _));
  EXPECT_CALL(*mock_verifier_, verify(_)).WillOnce(Invoke([](ContextSharedPtr context) {
    context->callback()->onComplete(Status::JwtAudienceNotAllowed);
  }));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_->decodeHeaders(headers, false));
  EXPECT_EQ(1U, mock_config_->stats().denied_.value());

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));
  EXPECT_EQ("jwt_authn_access_denied", filter_callbacks_.details_);
}

// This test verifies Verifier::Callback is called with OK status after verify().
TEST_F(FilterTest, OutBoundOK) {
  setupMockConfig();
  Verifier::Callbacks* m_cb;
  // callback is saved, not called right
  EXPECT_CALL(*mock_verifier_, verify(_)).WillOnce(Invoke([&m_cb](ContextSharedPtr context) {
    m_cb = context->callback();
  }));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_->decodeHeaders(headers, false));

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndWatermark, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::StopIteration, filter_->decodeTrailers(headers));

  // Callback is called now with OK status.
  m_cb->onComplete(Status::Ok);

  EXPECT_EQ(1U, mock_config_->stats().allowed_.value());
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));
}

// This test verifies Verifier::Callback is called with a failure(401 Unauthorized) after verify()
// returns any NonOK status except JwtAudienceNotAllowed.
TEST_F(FilterTest, OutBoundUnauthorizedFailure) {
  setupMockConfig();
  Verifier::Callbacks* m_cb;
  // callback is saved, not called right
  EXPECT_CALL(*mock_verifier_, verify(_)).WillOnce(Invoke([&m_cb](ContextSharedPtr context) {
    m_cb = context->callback();
  }));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_->decodeHeaders(headers, false));

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndWatermark, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::StopIteration, filter_->decodeTrailers(headers));

  // Callback is called now with a failure status.
  EXPECT_CALL(filter_callbacks_, sendLocalReply(Http::Code::Unauthorized, _, _, _, _));
  m_cb->onComplete(Status::JwtBadFormat);

  EXPECT_EQ(1U, mock_config_->stats().denied_.value());
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));

  // Should be OK to call the onComplete() again.
  m_cb->onComplete(Status::JwtBadFormat);
}

// This test verifies Verifier::Callback is called with a failure(403 Forbidden) after verify()
// returns JwtAudienceNotAllowed.
TEST_F(FilterTest, OutBoundForbiddenFailure) {
  setupMockConfig();
  Verifier::Callbacks* m_cb;
  // callback is saved, not called right
  EXPECT_CALL(*mock_verifier_, verify(_)).WillOnce(Invoke([&m_cb](ContextSharedPtr context) {
    m_cb = context->callback();
  }));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_->decodeHeaders(headers, false));

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndWatermark, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::StopIteration, filter_->decodeTrailers(headers));

  // Callback is called now with a failure status.
  EXPECT_CALL(filter_callbacks_, sendLocalReply(Http::Code::Forbidden, _, _, _, _));
  m_cb->onComplete(Status::JwtAudienceNotAllowed);

  EXPECT_EQ(1U, mock_config_->stats().denied_.value());
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));

  // Should be OK to call the onComplete() again.
  m_cb->onComplete(Status::JwtAudienceNotAllowed);
}

// Test verifies that if no route matched requirement, then request is allowed.
TEST_F(FilterTest, TestNoRouteMatched) {
  EXPECT_CALL(*mock_config_.get(), findVerifier(_, _)).WillOnce(Return(nullptr));

  auto headers = Http::TestHeaderMapImpl{};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(headers, false));
  EXPECT_EQ(1U, mock_config_->stats().allowed_.value());

  Buffer::OwnedImpl data("");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, false));
  EXPECT_EQ(Http::FilterTrailersStatus::Continue, filter_->decodeTrailers(headers));
}

} // namespace
} // namespace JwtAuthn
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
