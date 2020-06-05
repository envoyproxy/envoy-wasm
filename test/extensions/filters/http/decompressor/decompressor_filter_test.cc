#include "envoy/extensions/filters/http/decompressor/v3/decompressor.pb.h"

#include "common/http/headers.h"
#include "common/protobuf/utility.h"

#include "extensions/filters/http/decompressor/decompressor_filter.h"

#include "test/mocks/buffer/mocks.h"
#include "test/mocks/compression/decompressor/mocks.h"
#include "test/mocks/http/mocks.h"
#include "test/mocks/protobuf/mocks.h"
#include "test/mocks/runtime/mocks.h"
#include "test/mocks/stats/mocks.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

using testing::ByMove;
using testing::Return;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Decompressor {
namespace {

class DecompressorFilterTest : public testing::TestWithParam<bool> {
public:
  void SetUp() override {
    setUpFilter(R"EOF(
decompressor_library:
  typed_config:
    "@type": "type.googleapis.com/envoy.extensions.compression.gzip.decompressor.v3.Gzip"
)EOF");
  }

  void setUpFilter(std::string&& yaml) {
    envoy::extensions::filters::http::decompressor::v3::Decompressor decompressor;
    TestUtility::loadFromYaml(yaml, decompressor);
    auto decompressor_factory =
        std::make_unique<NiceMock<Compression::Decompressor::MockDecompressorFactory>>();
    decompressor_factory_ = decompressor_factory.get();
    config_ = std::make_shared<DecompressorFilterConfig>(decompressor, "test.", stats_, runtime_,
                                                         std::move(decompressor_factory));
    filter_ = std::make_unique<DecompressorFilter>(config_);
    filter_->setDecoderFilterCallbacks(decoder_callbacks_);
    filter_->setEncoderFilterCallbacks(encoder_callbacks_);
  }

  bool isRequestDirection() { return GetParam(); }

  std::unique_ptr<Http::RequestOrResponseHeaderMap> doHeaders(const Http::HeaderMap& headers,
                                                              const bool end_stream) {
    if (isRequestDirection()) {
      auto request_headers = Http::createHeaderMap<Http::TestRequestHeaderMapImpl>(headers);
      EXPECT_EQ(Http::FilterHeadersStatus::Continue,
                filter_->decodeHeaders(*request_headers, end_stream));
      return std::move(request_headers);
    } else {
      auto response_headers = Http::createHeaderMap<Http::TestResponseHeaderMapImpl>(headers);
      EXPECT_EQ(Http::FilterHeadersStatus::Continue,
                filter_->encodeHeaders(*response_headers, end_stream));
      return std::move(response_headers);
    }
  }

  void doData(Buffer::Instance& buffer, const bool end_stream) {
    if (isRequestDirection()) {
      EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(buffer, end_stream));
    } else {
      EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->encodeData(buffer, end_stream));
    }
  }

  void expectDecompression(Compression::Decompressor::MockDecompressor* decompressor_ptr) {
    EXPECT_CALL(*decompressor_ptr, decompress(_, _))
        .Times(2)
        .WillRepeatedly(
            Invoke([&](const Buffer::Instance& input_buffer, Buffer::Instance& output_buffer) {
              TestUtility::feedBufferWithRandomCharacters(output_buffer, 2 * input_buffer.length());
            }));
    Buffer::OwnedImpl buffer;
    TestUtility::feedBufferWithRandomCharacters(buffer, 10);
    EXPECT_EQ(10, buffer.length());
    doData(buffer, false /* end_stream */);
    EXPECT_EQ(20, buffer.length());
    doData(buffer, true /* end_stream */);
    EXPECT_EQ(40, buffer.length());
  }

  void expectNoDecompression() {
    Buffer::OwnedImpl buffer;
    TestUtility::feedBufferWithRandomCharacters(buffer, 10);
    EXPECT_EQ(10, buffer.length());
    doData(buffer, true /* end_stream */);
    EXPECT_EQ(10, buffer.length());
  }

  void decompressionActive(const Http::HeaderMap& headers_before_filter,
                           const absl::optional<std::string> expected_content_encoding,
                           const absl::optional<std::string> expected_accept_encoding = "mock") {
    // Keep the decompressor to set expectations about it
    auto decompressor = std::make_unique<Compression::Decompressor::MockDecompressor>();
    auto* decompressor_ptr = decompressor.get();
    EXPECT_CALL(*decompressor_factory_, createDecompressor())
        .WillOnce(Return(ByMove(std::move(decompressor))));

    std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
        doHeaders(headers_before_filter, false /* end_stream */);

    // The filter removes Content-Length
    EXPECT_EQ(nullptr, headers_after_filter->ContentLength());

    // The filter removes the decompressor's content encoding from the Content-Encoding header.
    if (expected_content_encoding.has_value()) {
      EXPECT_EQ(expected_content_encoding.value(),
                headers_after_filter->ContentEncoding()->value().getStringView());
    } else {
      EXPECT_EQ(nullptr, headers_after_filter->ContentEncoding());
    }

    // The filter adds the decompressor's content encoding to the Accept-Encoding header on the
    // request direction.
    const auto* accept_encoding =
        headers_after_filter->get(Http::LowerCaseString{"accept-encoding"});
    if (isRequestDirection() && expected_accept_encoding.has_value()) {
      EXPECT_EQ(expected_accept_encoding.value(), accept_encoding->value().getStringView());
    } else {
      EXPECT_EQ(nullptr, accept_encoding);
    }

    expectDecompression(decompressor_ptr);
  }

  Compression::Decompressor::MockDecompressorFactory* decompressor_factory_{};
  DecompressorFilterConfigSharedPtr config_;
  std::unique_ptr<DecompressorFilter> filter_;
  Stats::TestUtil::TestStore stats_;
  NiceMock<Runtime::MockLoader> runtime_;
  NiceMock<Http::MockStreamDecoderFilterCallbacks> decoder_callbacks_;
  NiceMock<Http::MockStreamEncoderFilterCallbacks> encoder_callbacks_;
};

INSTANTIATE_TEST_SUITE_P(IsRequestDirection, DecompressorFilterTest,
                         ::testing::Values(true, false));

TEST_P(DecompressorFilterTest, DecompressionActive) {
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock"},
                                                {"content-length", "256"}};
  decompressionActive(headers_before_filter, absl::nullopt /* expected_content_encoding */);
}

TEST_P(DecompressorFilterTest, DecompressionActiveContentEncodingSpacing) {
  // Additional spacing should still match.
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", " mock "},
                                                {"content-length", "256"}};
  decompressionActive(headers_before_filter, absl::nullopt /* expected_content_encoding */);
}

TEST_P(DecompressorFilterTest, DecompressionActiveContentEncodingCasing) {
  // Different casing should still match.
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "MOCK"},
                                                {"content-length", "256"}};
  decompressionActive(headers_before_filter, absl::nullopt /* expected_content_encoding */);
}

TEST_P(DecompressorFilterTest, DecompressionActiveMultipleEncodings) {
  // If the first encoding in the Content-Encoding header is the configured value, the filter should
  // still be active.
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock, br"},
                                                {"content-length", "256"}};
  decompressionActive(headers_before_filter, "br");
}

TEST_P(DecompressorFilterTest, DecompressionActiveMultipleEncodings2) {
  // If the first encoding in the Content-Encoding header is the configured value, the filter should
  // still be active.
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock, br , gzip "},
                                                {"content-length", "256"}};
  decompressionActive(headers_before_filter, "br , gzip");
}

TEST_P(DecompressorFilterTest, DisableAdvertiseAcceptEncoding) {
  setUpFilter(R"EOF(
decompressor_library:
  typed_config:
    "@type": "type.googleapis.com/envoy.extensions.compression.gzip.decompressor.v3.Gzip"
request_direction_config:
  advertise_accept_encoding: false
)EOF");

  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock"},
                                                {"content-length", "256"}};
  decompressionActive(headers_before_filter, absl::nullopt /* expected_content_encoding*/,
                      absl::nullopt /* expected_accept_encoding */);
}

TEST_P(DecompressorFilterTest, ExplicitlyEnableAdvertiseAcceptEncoding) {
  setUpFilter(R"EOF(
decompressor_library:
  typed_config:
    "@type": "type.googleapis.com/envoy.extensions.compression.gzip.decompressor.v3.Gzip"
request_direction_config:
  advertise_accept_encoding: true
)EOF");

  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock"},
                                                {"content-length", "256"}};
  if (isRequestDirection()) {
    // Also test that the filter appends to an already existing header.
    headers_before_filter.addCopy("accept-encoding", "br");
  }
  decompressionActive(headers_before_filter, absl::nullopt /* expected_content_encoding*/,
                      "br,mock" /* expected_accept_encoding */);
}

TEST_P(DecompressorFilterTest, DecompressionDisabled) {
  setUpFilter(R"EOF(
decompressor_library:
  typed_config:
    "@type": "type.googleapis.com/envoy.extensions.compression.gzip.decompressor.v3.Gzip"
request_direction_config:
  common_config:
    enabled:
      default_value: false
      runtime_key: does_not_exist
response_direction_config:
  common_config:
    enabled:
      default_value: false
      runtime_key: does_not_exist
)EOF");

  EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock"},
                                                {"content-length", "256"}};
  std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
      doHeaders(headers_before_filter, false /* end_stream */);
  TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);

  expectNoDecompression();
}

TEST_P(DecompressorFilterTest, RequestDecompressionDisabled) {
  setUpFilter(R"EOF(
decompressor_library:
  typed_config:
    "@type": "type.googleapis.com/envoy.extensions.compression.gzip.decompressor.v3.Gzip"
request_direction_config:
  common_config:
    enabled:
      default_value: false
      runtime_key: does_not_exist
)EOF");

  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock"},
                                                {"content-length", "256"}};

  if (isRequestDirection()) {
    EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
    std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
        doHeaders(headers_before_filter, false /* end_stream */);
    TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);
    expectNoDecompression();
  } else {
    decompressionActive(headers_before_filter, absl::nullopt /* expected_content_encoding*/,
                        "mock" /* expected_accept_encoding */);
  }
}

TEST_P(DecompressorFilterTest, ResponseDecompressionDisabled) {
  setUpFilter(R"EOF(
decompressor_library:
  typed_config:
    "@type": "type.googleapis.com/envoy.extensions.compression.gzip.decompressor.v3.Gzip"
response_direction_config:
  common_config:
    enabled:
      default_value: false
      runtime_key: does_not_exist
)EOF");

  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "mock"},
                                                {"content-length", "256"}};

  if (isRequestDirection()) {
    // Accept-Encoding is not advertised in the request headers when response decompression is
    // disabled.
    decompressionActive(headers_before_filter, absl::nullopt /* expected_content_encoding*/,
                        absl::nullopt /* expected_accept_encoding */);
  } else {
    EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
    std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
        doHeaders(headers_before_filter, false /* end_stream */);
    TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);
    expectNoDecompression();
  }
}

TEST_P(DecompressorFilterTest, NoDecompressionHeadersOnly) {
  EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
  Http::TestHeaderMapImpl headers_before_filter;
  std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
      doHeaders(headers_before_filter, true /* end_stream */);
  TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);
}

TEST_P(DecompressorFilterTest, NoDecompressionContentEncodingDoesNotMatch) {
  EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "not-matching"},
                                                {"content-length", "256"}};
  std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
      doHeaders(headers_before_filter, false /* end_stream */);
  TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);

  expectNoDecompression();
}

TEST_P(DecompressorFilterTest, NoDecompressionContentEncodingNotCurrent) {
  EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
  // The decompressor's content scheme is not the first value in the comma-delimited list in the
  // Content-Encoding header. Therefore, compression will not occur.
  Http::TestHeaderMapImpl headers_before_filter{{"content-encoding", "gzip,mock"},
                                                {"content-length", "256"}};
  std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
      doHeaders(headers_before_filter, false /* end_stream */);
  TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);

  expectNoDecompression();
}

TEST_P(DecompressorFilterTest, NoResponseDecompressionNoTransformPresent) {
  EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
  Http::TestHeaderMapImpl headers_before_filter{
      {"cache-control", Http::Headers::get().CacheControlValues.NoTransform},
      {"content-encoding", "mock"},
      {"content-length", "256"}};
  std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
      doHeaders(headers_before_filter, false /* end_stream */);
  TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);

  expectNoDecompression();
}

TEST_P(DecompressorFilterTest, NoResponseDecompressionNoTransformPresentInList) {
  EXPECT_CALL(*decompressor_factory_, createDecompressor()).Times(0);
  Http::TestHeaderMapImpl headers_before_filter{
      {"cache-control", fmt::format("{}, {}", Http::Headers::get().CacheControlValues.NoCache,
                                    Http::Headers::get().CacheControlValues.NoTransform)},
      {"content-encoding", "mock"},
      {"content-length", "256"}};
  std::unique_ptr<Http::RequestOrResponseHeaderMap> headers_after_filter =
      doHeaders(headers_before_filter, false /* end_stream */);
  TestUtility::headerMapEqualIgnoreOrder(headers_before_filter, *headers_after_filter);

  expectNoDecompression();
}

} // namespace
} // namespace Decompressor
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
