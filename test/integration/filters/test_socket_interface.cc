#include "test/integration/filters/test_socket_interface.h"

#include <algorithm>

#include "envoy/common/exception.h"
#include "envoy/extensions/network/socket_interface/v3/default_socket_interface.pb.h"
#include "envoy/network/socket.h"

#include "common/api/os_sys_calls_impl.h"
#include "common/common/utility.h"
#include "common/network/address_impl.h"

namespace Envoy {
namespace Network {

Api::IoCallUint64Result TestIoSocketHandle::writev(const Buffer::RawSlice* slices,
                                                   uint64_t num_slice) {
  if (writev_override_) {
    auto result = writev_override_(this, slices, num_slice);
    if (result.has_value()) {
      return std::move(result).value();
    }
  }
  return IoSocketHandleImpl::writev(slices, num_slice);
}

IoHandlePtr TestIoSocketHandle::accept(struct sockaddr* addr, socklen_t* addrlen) {
  auto result = Api::OsSysCallsSingleton::get().accept(fd_, addr, addrlen);
  if (SOCKET_INVALID(result.rc_)) {
    return nullptr;
  }

  return std::make_unique<TestIoSocketHandle>(writev_override_, result.rc_, socket_v6only_);
}

IoHandlePtr TestSocketInterface::makeSocket(int socket_fd, bool socket_v6only) const {
  return std::make_unique<TestIoSocketHandle>(writev_override_proc_, socket_fd, socket_v6only);
}

} // namespace Network
} // namespace Envoy
