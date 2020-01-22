#include "extensions/filters/common/fault/fault_config.h"

#include "envoy/extensions/filters/common/fault/v3alpha/fault.pb.h"

#include "common/protobuf/utility.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Fault {

FaultDelayConfig::FaultDelayConfig(
    const envoy::extensions::filters::common::fault::v3alpha::FaultDelay& delay_config)
    : percentage_(delay_config.percentage()) {
  switch (delay_config.fault_delay_secifier_case()) {
  case envoy::extensions::filters::common::fault::v3alpha::FaultDelay::FaultDelaySecifierCase::
      kFixedDelay:
    provider_ = std::make_unique<FixedDelayProvider>(
        std::chrono::milliseconds(PROTOBUF_GET_MS_REQUIRED(delay_config, fixed_delay)));
    break;
  case envoy::extensions::filters::common::fault::v3alpha::FaultDelay::FaultDelaySecifierCase::
      kHeaderDelay:
    provider_ = std::make_unique<HeaderDelayProvider>();
    break;
  case envoy::extensions::filters::common::fault::v3alpha::FaultDelay::FaultDelaySecifierCase::
      FAULT_DELAY_SECIFIER_NOT_SET:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

absl::optional<std::chrono::milliseconds>
FaultDelayConfig::HeaderDelayProvider::duration(const Http::HeaderEntry* header) const {
  if (header == nullptr) {
    return absl::nullopt;
  }

  uint64_t value;
  if (!absl::SimpleAtoi(header->value().getStringView(), &value)) {
    return absl::nullopt;
  }

  return std::chrono::milliseconds(value);
}

FaultRateLimitConfig::FaultRateLimitConfig(
    const envoy::extensions::filters::common::fault::v3alpha::FaultRateLimit& rate_limit_config)
    : percentage_(rate_limit_config.percentage()) {
  switch (rate_limit_config.limit_type_case()) {
  case envoy::extensions::filters::common::fault::v3alpha::FaultRateLimit::LimitTypeCase::
      kFixedLimit:
    provider_ =
        std::make_unique<FixedRateLimitProvider>(rate_limit_config.fixed_limit().limit_kbps());
    break;
  case envoy::extensions::filters::common::fault::v3alpha::FaultRateLimit::LimitTypeCase::
      kHeaderLimit:
    provider_ = std::make_unique<HeaderRateLimitProvider>();
    break;
  case envoy::extensions::filters::common::fault::v3alpha::FaultRateLimit::LimitTypeCase::
      LIMIT_TYPE_NOT_SET:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

absl::optional<uint64_t>
FaultRateLimitConfig::HeaderRateLimitProvider::rateKbps(const Http::HeaderEntry* header) const {
  if (header == nullptr) {
    return absl::nullopt;
  }

  uint64_t value;
  if (!absl::SimpleAtoi(header->value().getStringView(), &value)) {
    return absl::nullopt;
  }

  if (value == 0) {
    return absl::nullopt;
  }

  return value;
}

} // namespace Fault
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
