#include "extensions/filters/network/kafka/kafka_response_parser.h"

#include "test/extensions/filters/network/kafka/buffer_based_test.h"
#include "test/extensions/filters/network/kafka/serialization_utilities.h"
#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"

using testing::_;
using testing::Return;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Kafka {
namespace KafkaResponseParserTest {

const int32_t FAILED_DESERIALIZER_STEP = 13;

class KafkaResponseParserTest : public testing::Test, public BufferBasedTest {};

class MockResponseParserResolver : public ResponseParserResolver {
public:
  MockResponseParserResolver() = default;
  MOCK_CONST_METHOD1(createParser, ResponseParserSharedPtr(ResponseContextSharedPtr));
};

class MockParser : public ResponseParser {
public:
  ResponseParseResponse parse(absl::string_view&) override {
    throw new EnvoyException("should not be invoked");
  }
};

TEST_F(KafkaResponseParserTest, ResponseHeaderParserShouldExtractHeaderAndResolveNextParser) {
  // given
  const MockResponseParserResolver parser_resolver;
  const ResponseParserSharedPtr parser{new MockParser{}};
  EXPECT_CALL(parser_resolver, createParser(_)).WillOnce(Return(parser));

  ResponseContextSharedPtr context = std::make_shared<ResponseContext>(0, 0);
  ResponseHeaderParser testee{context, parser_resolver};

  const int32_t payload_length = 100;
  const int32_t correlation_id = 1234;
  uint32_t header_len = 0;
  header_len += putIntoBuffer(payload_length);
  header_len += putIntoBuffer(correlation_id); // Insert correlation id.

  const absl::string_view orig_data = putGarbageIntoBuffer();
  absl::string_view data = orig_data;

  // when
  const ResponseParseResponse result = testee.parse(data);

  // then
  ASSERT_EQ(result.hasData(), true);
  ASSERT_EQ(result.next_parser_, parser);
  ASSERT_EQ(result.message_, nullptr);
  ASSERT_EQ(result.failure_data_, nullptr);

  ASSERT_EQ(testee.contextForTest()->correlation_id_, correlation_id);
  ASSERT_EQ(testee.contextForTest()->remaining_response_size_,
            payload_length - sizeof(correlation_id));

  assertStringViewIncrement(data, orig_data, header_len);
}

TEST_F(KafkaResponseParserTest, ResponseDataParserShoulRethrowDeserializerExceptionsDuringFeeding) {
  // given

  // This deserializer throws during feeding.
  class ThrowingDeserializer : public Deserializer<int32_t> {
  public:
    uint32_t feed(absl::string_view&) override {
      // Move some pointers to simulate data consumption.
      throw EnvoyException("feed");
    };

    bool ready() const override { throw std::runtime_error("should not be invoked at all"); };

    int32_t get() const override { throw std::runtime_error("should not be invoked at all"); };
  };

  ResponseContextSharedPtr context = std::make_shared<ResponseContext>(0, 0);
  ResponseDataParser<int32_t, ThrowingDeserializer> testee{context};

  absl::string_view data = putGarbageIntoBuffer();

  // when
  bool caught = false;
  try {
    testee.parse(data);
  } catch (EnvoyException& e) {
    caught = true;
  }

  // then
  ASSERT_EQ(caught, true);
}

// This deserializer consumes FAILED_DESERIALIZER_STEP bytes and returns 0
class SomeBytesDeserializer : public Deserializer<int32_t> {
public:
  uint32_t feed(absl::string_view& data) override {
    data = {data.data() + FAILED_DESERIALIZER_STEP, data.size() - FAILED_DESERIALIZER_STEP};
    return FAILED_DESERIALIZER_STEP;
  };

  bool ready() const override { return true; };

  int32_t get() const override { return 0; };
};

TEST_F(KafkaResponseParserTest,
       ResponseDataParserShouldHandleDeserializerReturningReadyButLeavingData) {
  // given
  const int32_t message_size = 1024; // There are still 1024 bytes to read to complete the message.
  ResponseContextSharedPtr context = std::make_shared<ResponseContext>(0, 0);
  context->remaining_response_size_ = message_size;

  ResponseDataParser<int32_t, SomeBytesDeserializer> testee{context};

  const absl::string_view orig_data = putGarbageIntoBuffer();
  absl::string_view data = orig_data;

  // when
  const ResponseParseResponse result = testee.parse(data);

  // then
  ASSERT_EQ(result.hasData(), true);
  ASSERT_NE(std::dynamic_pointer_cast<SentinelResponseParser>(result.next_parser_), nullptr);
  ASSERT_EQ(result.message_, nullptr);
  ASSERT_EQ(result.failure_data_, nullptr);

  ASSERT_EQ(testee.contextForTest()->remaining_response_size_,
            message_size - FAILED_DESERIALIZER_STEP);

  assertStringViewIncrement(data, orig_data, FAILED_DESERIALIZER_STEP);
}

TEST_F(KafkaResponseParserTest, SentinelResponseParserShouldConsumeDataUntilEndOfMessage) {
  // given
  const int32_t response_len = 1000;
  ResponseContextSharedPtr context = std::make_shared<ResponseContext>(0, 0);
  context->remaining_response_size_ = response_len;
  SentinelResponseParser testee{context};

  const absl::string_view orig_data = putGarbageIntoBuffer(response_len * 2);
  absl::string_view data = orig_data;

  // when
  const ResponseParseResponse result = testee.parse(data);

  // then
  ASSERT_EQ(result.hasData(), true);
  ASSERT_EQ(result.next_parser_, nullptr);
  ASSERT_EQ(result.message_, nullptr);
  ASSERT_NE(std::dynamic_pointer_cast<ResponseMetadata>(result.failure_data_), nullptr);

  ASSERT_EQ(testee.contextForTest()->remaining_response_size_, 0);

  assertStringViewIncrement(data, orig_data, response_len);
}

} // namespace KafkaResponseParserTest
} // namespace Kafka
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
