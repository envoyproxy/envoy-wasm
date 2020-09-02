#include "common/network/socket_interface_impl.h"

#include "envoy/common/exception.h"
#include "envoy/extensions/network/socket_interface/v3/default_socket_interface.pb.h"

#include "common/api/os_sys_calls_impl.h"
#include "common/common/utility.h"
#include "common/network/io_socket_handle_impl.h"

namespace Envoy {
namespace Network {

IoHandlePtr SocketInterfaceImpl::makeSocket(int socket_fd, bool socket_v6only) const {
  return std::make_unique<IoSocketHandleImpl>(socket_fd, socket_v6only);
}

IoHandlePtr SocketInterfaceImpl::socket(Socket::Type socket_type, Address::Type addr_type,
                                        Address::IpVersion version, bool socket_v6only) const {
#if defined(__APPLE__) || defined(WIN32)
  int flags = 0;
#else
  int flags = SOCK_NONBLOCK;
#endif

  if (socket_type == Socket::Type::Stream) {
    flags |= SOCK_STREAM;
  } else {
    flags |= SOCK_DGRAM;
  }

  int domain;
  if (addr_type == Address::Type::Ip) {
    if (version == Address::IpVersion::v6) {
      domain = AF_INET6;
    } else {
      ASSERT(version == Address::IpVersion::v4);
      domain = AF_INET;
    }
  } else {
    ASSERT(addr_type == Address::Type::Pipe);
    domain = AF_UNIX;
  }

  const Api::SysCallSocketResult result = Api::OsSysCallsSingleton::get().socket(domain, flags, 0);
  RELEASE_ASSERT(SOCKET_VALID(result.rc_),
                 fmt::format("socket(2) failed, got error: {}", errorDetails(result.errno_)));
  IoHandlePtr io_handle = makeSocket(result.rc_, socket_v6only);

#if defined(__APPLE__) || defined(WIN32)
  // Cannot set SOCK_NONBLOCK as a ::socket flag.
  const int rc = io_handle->setBlocking(false).rc_;
  RELEASE_ASSERT(!SOCKET_FAILURE(rc), "");
#endif

  return io_handle;
}

IoHandlePtr SocketInterfaceImpl::socket(Socket::Type socket_type,
                                        const Address::InstanceConstSharedPtr addr) const {
  Address::IpVersion ip_version = addr->ip() ? addr->ip()->version() : Address::IpVersion::v4;
  int v6only = 0;
  if (addr->type() == Address::Type::Ip && ip_version == Address::IpVersion::v6) {
    v6only = addr->ip()->ipv6()->v6only();
  }

  IoHandlePtr io_handle =
      SocketInterfaceImpl::socket(socket_type, addr->type(), ip_version, v6only);
  if (addr->type() == Address::Type::Ip && ip_version == Address::IpVersion::v6) {
    // Setting IPV6_V6ONLY restricts the IPv6 socket to IPv6 connections only.
    const Api::SysCallIntResult result = io_handle->setOption(
        IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6only), sizeof(v6only));
    RELEASE_ASSERT(!SOCKET_FAILURE(result.rc_), "");
  }
  return io_handle;
}

IoHandlePtr SocketInterfaceImpl::socket(os_fd_t fd) {
  return std::make_unique<IoSocketHandleImpl>(fd);
}

bool SocketInterfaceImpl::ipFamilySupported(int domain) {
  Api::OsSysCalls& os_sys_calls = Api::OsSysCallsSingleton::get();
  const Api::SysCallSocketResult result = os_sys_calls.socket(domain, SOCK_STREAM, 0);
  if (SOCKET_VALID(result.rc_)) {
    RELEASE_ASSERT(os_sys_calls.close(result.rc_).rc_ == 0,
                   fmt::format("Fail to close fd: response code {}", errorDetails(result.rc_)));
  }
  return SOCKET_VALID(result.rc_);
}

Server::BootstrapExtensionPtr
SocketInterfaceImpl::createBootstrapExtension(const Protobuf::Message&,
                                              Server::Configuration::ServerFactoryContext&) {
  return std::make_unique<SocketInterfaceExtension>(*this);
}

ProtobufTypes::MessagePtr SocketInterfaceImpl::createEmptyConfigProto() {
  return std::make_unique<
      envoy::extensions::network::socket_interface::v3::DefaultSocketInterface>();
}

REGISTER_FACTORY(SocketInterfaceImpl, Server::Configuration::BootstrapExtensionFactory);

static SocketInterfaceLoader* socket_interface_ =
    new SocketInterfaceLoader(std::make_unique<SocketInterfaceImpl>());

} // namespace Network
} // namespace Envoy
