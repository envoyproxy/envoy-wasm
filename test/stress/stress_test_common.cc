#include "stress_test_common.h"

#include "common/network/address_impl.h"

namespace Envoy {
namespace Stress {

Http::CodecClient::Type httpType(const std::string& str) {
  return 0 == str.compare("http1") ? Http::CodecClient::Type::HTTP1
                                   : Http::CodecClient::Type::HTTP2;
}

Network::Address::IpVersion ipVersion(const std::string& str) {
  return 0 == str.compare("IPv4") ? Network::Address::IpVersion::v4
                                  : Network::Address::IpVersion::v6;
}

Network::Address::InstanceConstSharedPtr loopbackAddress(Network::Address::IpVersion ip_version,
                                                         uint32_t port) {
  switch (ip_version) {
  case Network::Address::IpVersion::v6: {
    Network::Address::InstanceConstSharedPtr addr{new Network::Address::Ipv6Instance("::1", port)};
    return addr;
  }
  case Network::Address::IpVersion::v4:
  default: {
    Network::Address::InstanceConstSharedPtr addr{
        new Network::Address::Ipv4Instance("127.0.0.1", port)};
    return addr;
  }
  }
}

} // namespace Stress
} // namespace Envoy
