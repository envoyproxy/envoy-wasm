#include "common/network/transport_socket_options_impl.h"

#include "common/common/scalar_to_byte_vector.h"
#include "common/common/utility.h"

namespace Envoy {
namespace Network {
void TransportSocketOptionsImpl::hashKey(std::vector<uint8_t>& key) const {
  if (override_server_name_.has_value()) {
    pushScalarToByteVector(StringUtil::CaseInsensitiveHash()(override_server_name_.value()), key);
  }

  if (!override_alpn_list_.empty()) {
    for (const auto& protocol : override_alpn_list_) {
      pushScalarToByteVector(StringUtil::CaseInsensitiveHash()(protocol), key);
    }
  }
}
} // namespace Network
} // namespace Envoy
