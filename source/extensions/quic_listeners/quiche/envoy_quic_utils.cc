#include "extensions/quic_listeners/quiche/envoy_quic_utils.h"

#include <sys/socket.h>

#include "common/network/socket_option_factory.h"

namespace Envoy {
namespace Quic {

// TODO(danzh): this is called on each write. Consider to return an address instance on the stack if
// the heap allocation is too expensive.
Network::Address::InstanceConstSharedPtr
quicAddressToEnvoyAddressInstance(const quic::QuicSocketAddress& quic_address) {
  return quic_address.IsInitialized()
             ? Network::Address::addressFromSockAddr(quic_address.generic_address(),
                                                     quic_address.host().address_family() ==
                                                             quic::IpAddressFamily::IP_V4
                                                         ? sizeof(sockaddr_in)
                                                         : sizeof(sockaddr_in6),
                                                     false)
             : nullptr;
}

quic::QuicSocketAddress envoyAddressInstanceToQuicSocketAddress(
    const Network::Address::InstanceConstSharedPtr& envoy_address) {
  ASSERT(envoy_address != nullptr && envoy_address->type() == Network::Address::Type::Ip);
  uint32_t port = envoy_address->ip()->port();
  sockaddr_storage ss;
  if (envoy_address->ip()->version() == Network::Address::IpVersion::v4) {
    auto ipv4_addr = reinterpret_cast<sockaddr_in*>(&ss);
    memset(ipv4_addr, 0, sizeof(sockaddr_in));
    ipv4_addr->sin_family = AF_INET;
    ipv4_addr->sin_port = htons(port);
    ipv4_addr->sin_addr.s_addr = envoy_address->ip()->ipv4()->address();
  } else {
    auto ipv6_addr = reinterpret_cast<sockaddr_in6*>(&ss);
    memset(ipv6_addr, 0, sizeof(sockaddr_in6));
    ipv6_addr->sin6_family = AF_INET6;
    ipv6_addr->sin6_port = htons(port);
    ASSERT(sizeof(ipv6_addr->sin6_addr.s6_addr) == 16u);
    *reinterpret_cast<absl::uint128*>(ipv6_addr->sin6_addr.s6_addr) =
        envoy_address->ip()->ipv6()->address();
  }
  return quic::QuicSocketAddress(ss);
}

Http::HeaderMapImplPtr quicHeadersToEnvoyHeaders(const quic::QuicHeaderList& header_list) {
  Http::HeaderMapImplPtr headers = std::make_unique<Http::HeaderMapImpl>();
  for (const auto& entry : header_list) {
    // TODO(danzh): Avoid copy by referencing entry as header_list is already validated by QUIC.
    headers->addCopy(Http::LowerCaseString(entry.first), entry.second);
  }
  return headers;
}

Http::HeaderMapImplPtr spdyHeaderBlockToEnvoyHeaders(const spdy::SpdyHeaderBlock& header_block) {
  Http::HeaderMapImplPtr headers = std::make_unique<Http::HeaderMapImpl>();
  for (auto entry : header_block) {
    // TODO(danzh): Avoid temporary strings and addCopy() with std::string_view.
    std::string key(entry.first);
    std::string value(entry.second);
    headers->addCopy(Http::LowerCaseString(key), value);
  }
  return headers;
}

spdy::SpdyHeaderBlock envoyHeadersToSpdyHeaderBlock(const Http::HeaderMap& headers) {
  spdy::SpdyHeaderBlock header_block;
  headers.iterate(
      [](const Http::HeaderEntry& header, void* context) -> Http::HeaderMap::Iterate {
        auto spdy_headers = static_cast<spdy::SpdyHeaderBlock*>(context);
        // The key-value pairs are copied.
        spdy_headers->insert({header.key().getStringView(), header.value().getStringView()});
        return Http::HeaderMap::Iterate::Continue;
      },
      &header_block);
  return header_block;
}

quic::QuicRstStreamErrorCode envoyResetReasonToQuicRstError(Http::StreamResetReason reason) {
  switch (reason) {
  case Http::StreamResetReason::LocalRefusedStreamReset:
    return quic::QUIC_REFUSED_STREAM;
  case Http::StreamResetReason::ConnectionFailure:
    return quic::QUIC_STREAM_CONNECTION_ERROR;
  case Http::StreamResetReason::LocalReset:
    return quic::QUIC_STREAM_CANCELLED;
  case Http::StreamResetReason::ConnectionTermination:
    return quic::QUIC_STREAM_NO_ERROR;
  default:
    return quic::QUIC_BAD_APPLICATION_PAYLOAD;
  }
}

Http::StreamResetReason quicRstErrorToEnvoyResetReason(quic::QuicRstStreamErrorCode rst_err) {
  switch (rst_err) {
  case quic::QUIC_REFUSED_STREAM:
    return Http::StreamResetReason::RemoteRefusedStreamReset;
  default:
    return Http::StreamResetReason::RemoteReset;
  }
}

Http::StreamResetReason quicErrorCodeToEnvoyResetReason(quic::QuicErrorCode error) {
  if (error == quic::QUIC_NO_ERROR) {
    return Http::StreamResetReason::ConnectionTermination;
  } else {
    return Http::StreamResetReason::ConnectionFailure;
  }
}

Network::ConnectionSocketPtr
createConnectionSocket(Network::Address::InstanceConstSharedPtr& peer_addr,
                       Network::Address::InstanceConstSharedPtr& local_addr,
                       const Network::ConnectionSocket::OptionsSharedPtr& options) {
  Network::IoHandlePtr io_handle = peer_addr->socket(Network::Address::SocketType::Datagram);
  auto connection_socket =
      std::make_unique<Network::ConnectionSocketImpl>(std::move(io_handle), local_addr, peer_addr);
  connection_socket->addOptions(Network::SocketOptionFactory::buildIpPacketInfoOptions());
  connection_socket->addOptions(Network::SocketOptionFactory::buildRxQueueOverFlowOptions());
  if (options != nullptr) {
    connection_socket->addOptions(options);
  }
  if (!Network::Socket::applyOptions(connection_socket->options(), *connection_socket,
                                     envoy::api::v2::core::SocketOption::STATE_PREBIND)) {
    connection_socket->close();
    ENVOY_LOG_MISC(error, "Fail to apply pre-bind options");
    return connection_socket;
  }
  local_addr->bind(connection_socket->ioHandle().fd());
  ASSERT(local_addr->ip());
  if (local_addr->ip()->port() == 0) {
    // Get ephemeral port number.
    local_addr = Network::Address::addressFromFd(connection_socket->ioHandle().fd());
  }
  if (!Network::Socket::applyOptions(connection_socket->options(), *connection_socket,
                                     envoy::api::v2::core::SocketOption::STATE_BOUND)) {
    ENVOY_LOG_MISC(error, "Fail to apply post-bind options");
    connection_socket->close();
  }
  return connection_socket;
}

} // namespace Quic
} // namespace Envoy
