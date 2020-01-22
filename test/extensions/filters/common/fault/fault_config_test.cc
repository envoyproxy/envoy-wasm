#include "envoy/extensions/filters/common/fault/v3alpha/fault.pb.h"

#include "extensions/filters/common/fault/fault_config.h"

#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Fault {
namespace {

TEST(FaultConfigTest, FaultDelayHeaderConfig) {
  envoy::extensions::filters::common::fault::v3alpha::FaultDelay proto_config;
  proto_config.mutable_header_delay();
  FaultDelayConfig config(proto_config);

  // No header.
  EXPECT_EQ(absl::nullopt, config.duration(nullptr));

  // Header with bad data.
  Http::TestHeaderMapImpl bad_headers{{"x-envoy-fault-delay-request", "abc"}};
  EXPECT_EQ(absl::nullopt, config.duration(bad_headers.get(HeaderNames::get().DelayRequest)));

  // Valid header.
  Http::TestHeaderMapImpl good_headers{{"x-envoy-fault-delay-request", "123"}};
  EXPECT_EQ(std::chrono::milliseconds(123),
            config.duration(good_headers.get(HeaderNames::get().DelayRequest)).value());
}

TEST(FaultConfigTest, FaultRateLimitHeaderConfig) {
  envoy::extensions::filters::common::fault::v3alpha::FaultRateLimit proto_config;
  proto_config.mutable_header_limit();
  FaultRateLimitConfig config(proto_config);

  // No header.
  EXPECT_EQ(absl::nullopt, config.rateKbps(nullptr));

  // Header with bad data.
  Http::TestHeaderMapImpl bad_headers{{"x-envoy-fault-throughput-response", "abc"}};
  EXPECT_EQ(absl::nullopt, config.rateKbps(bad_headers.get(HeaderNames::get().ThroughputResponse)));

  // Header with zero.
  Http::TestHeaderMapImpl zero_headers{{"x-envoy-fault-throughput-response", "0"}};
  EXPECT_EQ(absl::nullopt,
            config.rateKbps(zero_headers.get(HeaderNames::get().ThroughputResponse)));

  // Valid header.
  Http::TestHeaderMapImpl good_headers{{"x-envoy-fault-throughput-response", "123"}};
  EXPECT_EQ(123UL,
            config.rateKbps(good_headers.get(HeaderNames::get().ThroughputResponse)).value());
}

} // namespace
} // namespace Fault
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
