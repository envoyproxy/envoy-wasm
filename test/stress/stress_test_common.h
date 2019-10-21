#pragma once

#include "envoy/network/address.h"

#include "common/http/codec_client.h"

namespace Envoy {
namespace Stress {

extern Network::Address::InstanceConstSharedPtr
loopbackAddress(Network::Address::IpVersion ip_version, uint32_t port);

extern Http::CodecClient::Type httpType(const std::string& str);

extern Network::Address::IpVersion ipVersion(const std::string& str);

} // namespace Stress
} // namespace Envoy
