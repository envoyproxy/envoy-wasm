#include "envoy/extensions/filters/http/grpc_json_transcoder/v3alpha/transcoder.pb.h"
#include "envoy/extensions/filters/http/grpc_json_transcoder/v3alpha/transcoder.pb.validate.h"

#include "extensions/filters/http/grpc_json_transcoder/config.h"

#include "test/mocks/server/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace GrpcJsonTranscoder {
namespace {

TEST(GrpcJsonTranscoderFilterConfigTest, ValidateFail) {
  NiceMock<Server::Configuration::MockFactoryContext> context;
  EXPECT_THROW(
      GrpcJsonTranscoderFilterConfig().createFilterFactoryFromProto(
          envoy::extensions::filters::http::grpc_json_transcoder::v3alpha::GrpcJsonTranscoder(),
          "stats", context),
      ProtoValidationException);
}

} // namespace
} // namespace GrpcJsonTranscoder
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
