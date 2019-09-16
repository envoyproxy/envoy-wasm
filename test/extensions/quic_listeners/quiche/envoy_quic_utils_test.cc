#include "extensions/quic_listeners/quiche/envoy_quic_utils.h"

#pragma GCC diagnostic push
// QUICHE allows unused parameters.
#pragma GCC diagnostic ignored "-Wunused-parameter"
// QUICHE uses offsetof().
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

#include "quiche/quic/test_tools/quic_test_utils.h"

#pragma GCC diagnostic pop

#include "test/mocks/api/mocks.h"
#include "test/test_common/threadsafe_singleton_injector.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Return;

namespace Envoy {
namespace Quic {

TEST(EnvoyQuicUtilsTest, ConversionBetweenQuicAddressAndEnvoyAddress) {
  // Mock out socket() system call to test both V4 and V6 address conversion.
  testing::NiceMock<Envoy::Api::MockOsSysCalls> os_sys_calls;
  TestThreadsafeSingletonInjector<Envoy::Api::OsSysCallsImpl> os_calls{&os_sys_calls};
  ON_CALL(os_sys_calls, socket(_, _, _)).WillByDefault(Return(Api::SysCallIntResult{1, 0}));
  ON_CALL(os_sys_calls, close(_)).WillByDefault(Return(Api::SysCallIntResult{0, 0}));

  quic::QuicSocketAddress quic_uninitialized_addr;
  EXPECT_EQ(nullptr, quicAddressToEnvoyAddressInstance(quic_uninitialized_addr));

  for (const std::string& ip_str : {"fd00:0:0:1::1", "1.2.3.4"}) {
    quic::QuicIpAddress quic_ip;
    quic_ip.FromString(ip_str);
    quic::QuicSocketAddress quic_addr(quic_ip, 12345);
    Network::Address::InstanceConstSharedPtr envoy_addr =
        quicAddressToEnvoyAddressInstance(quic_addr);
    EXPECT_EQ(quic_addr.ToString(), envoy_addr->asStringView());
    EXPECT_EQ(quic_addr, envoyAddressInstanceToQuicSocketAddress(envoy_addr));
  }
}

TEST(EnvoyQuicUtilsTest, HeadersConversion) {
  spdy::SpdyHeaderBlock headers_block;
  headers_block[":host"] = "www.google.com";
  headers_block[":path"] = "/index.hml";
  headers_block[":scheme"] = "https";
  Http::HeaderMapImplPtr envoy_headers = spdyHeaderBlockToEnvoyHeaders(headers_block);
  EXPECT_EQ(headers_block.size(), envoy_headers->size());
  EXPECT_EQ("www.google.com",
            envoy_headers->get(Http::LowerCaseString(":host"))->value().getStringView());
  EXPECT_EQ("/index.hml",
            envoy_headers->get(Http::LowerCaseString(":path"))->value().getStringView());
  EXPECT_EQ("https", envoy_headers->get(Http::LowerCaseString(":scheme"))->value().getStringView());

  quic::QuicHeaderList quic_headers = quic::test::AsHeaderList(headers_block);
  Http::HeaderMapImplPtr envoy_headers2 = quicHeadersToEnvoyHeaders(quic_headers);
  EXPECT_EQ(*envoy_headers, *envoy_headers2);
}

} // namespace Quic
} // namespace Envoy
