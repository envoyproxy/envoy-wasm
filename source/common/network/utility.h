#pragma once

#include <cstdint>
#include <list>
#include <string>

#include "envoy/api/v2/core/address.pb.h"
#include "envoy/network/connection.h"

#include "absl/strings/string_view.h"

namespace Envoy {
namespace Network {

/**
 * Utility class to represent TCP/UDP port range
 */
class PortRange {
public:
  PortRange(uint32_t min, uint32_t max) : min_(min), max_(max) {}

  bool contains(uint32_t port) const { return (port >= min_ && port <= max_); }

private:
  const uint32_t min_;
  const uint32_t max_;
};

using PortRangeList = std::list<PortRange>;

/**
 * A callback interface used by readFromSocket() to pass packets read from
 * socket.
 */
class UdpPacketProcessor {
public:
  virtual ~UdpPacketProcessor() = default;

  /**
   * Consume the packet read out of the socket with the information from UDP
   * header.
   * @param local_address is the destination address in the UDP header.
   * @param peer_address is the source address in the UDP header.
   * @param buffer contains the packet read.
   * @param receive_time is the time when the packet is read.
   */
  virtual void processPacket(Address::InstanceConstSharedPtr local_address,
                             Address::InstanceConstSharedPtr peer_address,
                             Buffer::InstancePtr buffer, MonotonicTime receive_time) PURE;

  /**
   * The expected max size of the packet to be read. If it's smaller than
   * actually packets received, the payload will be truncated.
   */
  virtual uint64_t maxPacketSize() const PURE;
};

static const uint64_t MAX_UDP_PACKET_SIZE = 1500;

/**
 * Common network utility routines.
 */
class Utility {
public:
  static const std::string TCP_SCHEME;
  static const std::string UDP_SCHEME;
  static const std::string UNIX_SCHEME;

  /**
   * Resolve a URL.
   * @param url supplies the url to resolve.
   * @return Address::InstanceConstSharedPtr the resolved address.
   */
  static Address::InstanceConstSharedPtr resolveUrl(const std::string& url);

  /**
   * Match a URL to the TCP scheme
   * @param url supplies the URL to match.
   * @return bool true if the URL matches the TCP scheme, false otherwise.
   */
  static bool urlIsTcpScheme(const std::string& url);

  /**
   * Match a URL to the UDP scheme
   * @param url supplies the URL to match.
   * @return bool true if the URL matches the UDP scheme, false otherwise.
   */
  static bool urlIsUdpScheme(const std::string& url);

  /**
   * Match a URL to the Unix scheme
   * @param url supplies the Unix to match.
   * @return bool true if the URL matches the Unix scheme, false otherwise.
   */
  static bool urlIsUnixScheme(const std::string& url);

  /**
   * Parses the host from a TCP URL
   * @param the URL to parse host from
   * @return std::string the parsed host
   */
  static std::string hostFromTcpUrl(const std::string& url);

  /**
   * Parses the port from a TCP URL
   * @param the URL to parse port from
   * @return uint32_t the parsed port
   */
  static uint32_t portFromTcpUrl(const std::string& url);

  /**
   * Parses the host from a UDP URL
   * @param the URL to parse host from
   * @return std::string the parsed host
   */
  static std::string hostFromUdpUrl(const std::string& url);

  /**
   * Parses the port from a UDP URL
   * @param the URL to parse port from
   * @return uint32_t the parsed port
   */
  static uint32_t portFromUdpUrl(const std::string& url);

  /**
   * Parse an internet host address (IPv4 or IPv6) and create an Instance from it. The address must
   * not include a port number. Throws EnvoyException if unable to parse the address.
   * @param ip_address string to be parsed as an internet address.
   * @param port optional port to include in Instance created from ip_address, 0 by default.
   * @param v6only disable IPv4-IPv6 mapping for IPv6 addresses?
   * @return pointer to the Instance, or nullptr if unable to parse the address.
   */
  static Address::InstanceConstSharedPtr
  parseInternetAddress(const std::string& ip_address, uint16_t port = 0, bool v6only = true);

  /**
   * Parse an internet host address (IPv4 or IPv6) AND port, and create an Instance from it. Throws
   * EnvoyException if unable to parse the address. This is needed when a shared pointer is needed
   * but only a raw instance is available.
   * @param Address::Ip& to be copied to the new instance.
   * @return pointer to the Instance.
   */
  static Address::InstanceConstSharedPtr copyInternetAddressAndPort(const Address::Ip& ip);

  /**
   * Create a new Instance from an internet host address (IPv4 or IPv6) and port.
   * @param ip_addr string to be parsed as an internet address and port. Examples:
   *        - "1.2.3.4:80"
   *        - "[1234:5678::9]:443"
   * @param v6only disable IPv4-IPv6 mapping for IPv6 addresses?
   * @return pointer to the Instance.
   */
  static Address::InstanceConstSharedPtr parseInternetAddressAndPort(const std::string& ip_address,
                                                                     bool v6only = true);

  /**
   * Get the local address of the first interface address that is of type
   * version and is not a loopback address. If no matches are found, return the
   * loopback address of type version.
   * @param the local address IP version.
   * @return the local IP address of the server
   */
  static Address::InstanceConstSharedPtr getLocalAddress(const Address::IpVersion version);

  /**
   * Determine whether this is a local connection.
   * @return bool the address is a local connection.
   */
  static bool isLocalConnection(const Network::ConnectionSocket& socket);

  /**
   * Determine whether this is an internal (RFC1918) address.
   * @return bool the address is an RFC1918 address.
   */
  static bool isInternalAddress(const Address::Instance& address);

  /**
   * Check if address is loopback address.
   * @param address IP address to check.
   * @return true if so, otherwise false
   */
  static bool isLoopbackAddress(const Address::Instance& address);

  /**
   * @return Address::InstanceConstSharedPtr an address that represents the canonical IPv4 loopback
   *         address (i.e. "127.0.0.1"). Note that the range "127.0.0.0/8" is all defined as the
   *         loopback range, but the address typically used (e.g. in tests) is "127.0.0.1".
   */
  static Address::InstanceConstSharedPtr getCanonicalIpv4LoopbackAddress();

  /**
   * @return Address::InstanceConstSharedPtr an address that represents the IPv6 loopback address
   *         (i.e. "::1").
   */
  static Address::InstanceConstSharedPtr getIpv6LoopbackAddress();

  /**
   * @return Address::InstanceConstSharedPtr an address that represents the IPv4 wildcard address
   *         (i.e. "0.0.0.0"). Used during binding to indicate that incoming connections to any
   *         local IPv4 address are to be accepted.
   */
  static Address::InstanceConstSharedPtr getIpv4AnyAddress();

  /**
   * @return Address::InstanceConstSharedPtr an address that represents the IPv6 wildcard address
   *         (i.e. "::"). Used during binding to indicate that incoming connections to any local
   *         IPv6 address are to be accepted.
   */
  static Address::InstanceConstSharedPtr getIpv6AnyAddress();

  /**
   * @return the IPv4 CIDR catch-all address (0.0.0.0/0).
   */
  static const std::string& getIpv4CidrCatchAllAddress();

  /**
   * @return the IPv6 CIDR catch-all address (::/0).
   */
  static const std::string& getIpv6CidrCatchAllAddress();

  /**
   * @param address IP address instance.
   * @param port to update.
   * @return Address::InstanceConstSharedPtr a new address instance with updated port.
   */
  static Address::InstanceConstSharedPtr getAddressWithPort(const Address::Instance& address,
                                                            uint32_t port);

  /**
   * Retrieve the original destination address from an accepted fd.
   * The address (IP and port) may be not local and the port may differ from
   * the listener port if the packets were redirected using iptables
   * @param fd is the descriptor returned by accept()
   * @return the original destination or nullptr if not available.
   */
  static Address::InstanceConstSharedPtr getOriginalDst(int fd);

  /**
   * Parses a string containing a comma-separated list of port numbers and/or
   * port ranges and appends the values to a caller-provided list of PortRange structures.
   * For example, the string "1-1024,2048-4096,12345" causes 3 PortRange structures
   * to be appended to the supplied list.
   * @param str is the string containing the port numbers and ranges
   * @param list is the list to append the new data structures to
   */
  static void parsePortRangeList(absl::string_view string, std::list<PortRange>& list);

  /**
   * Checks whether a given port number appears in at least one of the port ranges in a list
   * @param address supplies the IP address to compare.
   * @param list the list of port ranges in which the port may appear
   * @return whether the port appears in at least one of the ranges in the list
   */
  static bool portInRangeList(const Address::Instance& address, const std::list<PortRange>& list);

  /**
   * Converts IPv6 absl::uint128 in network byte order to host byte order.
   * @param address supplies the IPv6 address in network byte order.
   * @return the absl::uint128 IPv6 address in host byte order.
   */
  static absl::uint128 Ip6ntohl(const absl::uint128& address);

  /**
   * Converts IPv6 absl::uint128 in host byte order to network byte order.
   * @param address supplies the IPv6 address in host byte order.
   * @return the absl::uint128 IPv6 address in network byte order.
   */
  static absl::uint128 Ip6htonl(const absl::uint128& address);

  static Address::InstanceConstSharedPtr
  protobufAddressToAddress(const envoy::api::v2::core::Address& proto_address);

  /**
   * Copies the address instance into the protobuf representation of an address.
   * @param address is the address to be copied into the protobuf representation of this address.
   * @param proto_address is the protobuf address to which the address instance is copied into.
   */
  static void addressToProtobufAddress(const Address::Instance& address,
                                       envoy::api::v2::core::Address& proto_address);

  /**
   * Returns socket type corresponding to SocketAddress.protocol value of the
   * given address, or SocketType::Stream if the address is a pipe address.
   * @param proto_address the address protobuf
   * @return socket type
   */
  static Address::SocketType
  protobufAddressSocketType(const envoy::api::v2::core::Address& proto_address);

  /**
   * Send a packet via given UDP socket with specific source address.
   * @param socket is the UDP socket used to send.
   * @param slices points to the buffers containing the packet.
   * @param num_slices is the number of buffers.
   * @param local_ip is the source address to be used to send.
   * @param peer_address is the destination address to send to.
   */
  static Api::IoCallUint64Result writeToSocket(Network::Socket& socket, Buffer::RawSlice* slices,
                                               uint64_t num_slices, const Address::Ip* local_ip,
                                               const Address::Instance& peer_address);

  /**
   * Read a packet from given UDP socket and pass the packet to given
   * UdpPacketProcessor.
   * @param socket is the UDP socket to read from.
   * @param udp_packet_processor is the callback to receive the packet.
   * @param receive_time is the timestamp passed to udp_packet_processor for the
   * receive time of the packet.
   * @param packets_dropped is the output parameter for number of packets dropped in kernel. If the
   * caller is not interested in it, nullptr can be passed in.
   */
  static Api::IoCallUint64Result readFromSocket(Network::Socket& socket,
                                                UdpPacketProcessor& udp_packet_processor,
                                                MonotonicTime receive_time,
                                                uint32_t* packets_dropped);

private:
  static void throwWithMalformedIp(const std::string& ip_address);

  /**
   * Takes a number and flips the order in byte chunks. The last byte of the input will be the
   * first byte in the output. The second to last byte will be the second to first byte in the
   * output. Etc..
   * @param input supplies the input to have the bytes flipped.
   * @return the absl::uint128 of the input having the bytes flipped.
   */
  static absl::uint128 flipOrder(const absl::uint128& input);
};

} // namespace Network
} // namespace Envoy
