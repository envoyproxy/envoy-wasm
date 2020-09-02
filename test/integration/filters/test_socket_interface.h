#pragma once

#include <functional>

#include "envoy/network/address.h"
#include "envoy/network/socket.h"

#include "common/network/io_socket_handle_impl.h"
#include "common/network/socket_interface_impl.h"

#include "absl/types/optional.h"

/**
 * TestSocketInterface allows overriding the behavior of the IoHandle interface.
 */
namespace Envoy {
namespace Network {

class TestIoSocketHandle : public IoSocketHandleImpl {
public:
  using WritevOverrideType = absl::optional<Api::IoCallUint64Result>(TestIoSocketHandle* io_handle,
                                                                     const Buffer::RawSlice* slices,
                                                                     uint64_t num_slice);
  using WritevOverrideProc = std::function<WritevOverrideType>;

  TestIoSocketHandle(WritevOverrideProc writev_override_proc, os_fd_t fd = INVALID_SOCKET,
                     bool socket_v6only = false)
      : IoSocketHandleImpl(fd, socket_v6only), writev_override_(writev_override_proc) {}

private:
  IoHandlePtr accept(struct sockaddr* addr, socklen_t* addrlen) override;
  Api::IoCallUint64Result writev(const Buffer::RawSlice* slices, uint64_t num_slice) override;

  const WritevOverrideProc writev_override_;
};

/**
 * TestSocketInterface allows overriding of the behavior of the IoHandle interface of
 * accepted sockets.
 * Most integration tests have deterministic order in which Envoy accepts connections.
 * For example a test with one client connection will result in two accepted sockets. First
 * is for the client<->Envoy connection and the second is for the Envoy<->upstream connection.
 */

class TestSocketInterface : public SocketInterfaceImpl {
public:
  /**
   * Override the behavior of the IoSocketHandleImpl::writev() method.
   * The supplied callback is invoked with the arguments of the writev method and the index
   * of the accepted socket.
   * Returning absl::nullopt from the callback continues normal execution of the
   * IoSocketHandleImpl::writev() method. Returning a Api::IoCallUint64Result from callback skips
   * the IoSocketHandleImpl::writev() with the returned result value.
   */
  TestSocketInterface(TestIoSocketHandle::WritevOverrideProc writev)
      : writev_override_proc_(writev) {}

private:
  // SocketInterfaceImpl
  IoHandlePtr makeSocket(int socket_fd, bool socket_v6only) const override;

  const TestIoSocketHandle::WritevOverrideProc writev_override_proc_;
};

} // namespace Network
} // namespace Envoy
