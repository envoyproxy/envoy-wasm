#include "common/network/listen_socket_impl.h"

#include <sys/types.h>

#include <string>

#include "envoy/common/exception.h"
#include "envoy/common/platform.h"
#include "envoy/config/core/v3/base.pb.h"

#include "common/api/os_sys_calls_impl.h"
#include "common/common/assert.h"
#include "common/common/fmt.h"
#include "common/network/address_impl.h"
#include "common/network/utility.h"

namespace Envoy {
namespace Network {

Api::SysCallIntResult ListenSocketImpl::bind(Network::Address::InstanceConstSharedPtr address) {
  local_address_ = address;

  const Api::SysCallIntResult result = SocketImpl::bind(local_address_);
  if (SOCKET_FAILURE(result.rc_)) {
    close();
    throw SocketBindException(
        fmt::format("cannot bind '{}': {}", local_address_->asString(), strerror(result.errno_)),
        result.errno_);
  }
  // TODO(fcoras): should this be moved to SocketImpl::bind()?
  if (local_address_->type() == Address::Type::Ip && local_address_->ip()->port() == 0) {
    // If the port we bind is zero, then the OS will pick a free port for us (assuming there are
    // any), and we need to find out the port number that the OS picked.
    local_address_ = SocketInterface::addressFromFd(io_handle_->fd());
  }

  return {0, 0};
}

void ListenSocketImpl::setListenSocketOptions(const Network::Socket::OptionsSharedPtr& options) {
  if (!Network::Socket::applyOptions(options, *this,
                                     envoy::config::core::v3::SocketOption::STATE_PREBIND)) {
    throw EnvoyException("ListenSocket: Setting socket options failed");
  }
}

void ListenSocketImpl::setupSocket(const Network::Socket::OptionsSharedPtr& options,
                                   bool bind_to_port) {
  setListenSocketOptions(options);

  if (bind_to_port) {
    bind(local_address_);
  }
}

template <>
void NetworkListenSocket<
    NetworkSocketTrait<Address::SocketType::Stream>>::setPrebindSocketOptions() {

  int on = 1;
  auto& os_syscalls = Api::OsSysCallsSingleton::get();
  Api::SysCallIntResult status =
      os_syscalls.setsockopt(io_handle_->fd(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  RELEASE_ASSERT(status.rc_ != -1, "failed to set SO_REUSEADDR socket option");
}

template <>
void NetworkListenSocket<
    NetworkSocketTrait<Address::SocketType::Datagram>>::setPrebindSocketOptions() {}

UdsListenSocket::UdsListenSocket(const Address::InstanceConstSharedPtr& address)
    : ListenSocketImpl(SocketInterface::socket(Address::SocketType::Stream, address), address) {
  RELEASE_ASSERT(io_handle_->fd() != -1, "");
  bind(local_address_);
}

UdsListenSocket::UdsListenSocket(IoHandlePtr&& io_handle,
                                 const Address::InstanceConstSharedPtr& address)
    : ListenSocketImpl(std::move(io_handle), address) {}

} // namespace Network
} // namespace Envoy
