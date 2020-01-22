#pragma once

#include "envoy/config/rbac/v3alpha/rbac.pb.h"

#include "extensions/filters/common/rbac/engine_impl.h"

#include "gmock/gmock.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace RBAC {

class MockEngine : public RoleBasedAccessControlEngineImpl {
public:
  MockEngine(const envoy::config::rbac::v3alpha::RBAC& rules)
      : RoleBasedAccessControlEngineImpl(rules){};

  MOCK_CONST_METHOD4(allowed,
                     bool(const Envoy::Network::Connection&, const Envoy::Http::HeaderMap&,
                          const StreamInfo::StreamInfo&, std::string* effective_policy_id));

  MOCK_CONST_METHOD3(allowed, bool(const Envoy::Network::Connection&, const StreamInfo::StreamInfo&,
                                   std::string* effective_policy_id));
};

} // namespace RBAC
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
