#include "test/extensions/common/tap/common.h"

#include "envoy/data/tap/v3alpha/wrapper.pb.h"

namespace envoy {
namespace data {
namespace tap {
namespace v3alpha {

std::ostream& operator<<(std::ostream& os, const TraceWrapper& trace) {
  return os << Envoy::MessageUtil::getJsonStringFromMessage(trace, true, false);
}

} // namespace v3alpha
} // namespace tap
} // namespace data
} // namespace envoy

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Tap {

MockPerTapSinkHandleManager::MockPerTapSinkHandleManager() = default;
MockPerTapSinkHandleManager::~MockPerTapSinkHandleManager() = default;

MockMatcher::~MockMatcher() = default;

} // namespace Tap
} // namespace Common
} // namespace Extensions
} // namespace Envoy
