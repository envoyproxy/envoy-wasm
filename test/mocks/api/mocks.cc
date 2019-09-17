#include "mocks.h"

#include "common/common/assert.h"
#include "common/common/lock_guard.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace Envoy {
namespace Api {

MockApi::MockApi() {
  ON_CALL(*this, fileSystem()).WillByDefault(ReturnRef(file_system_));
  ON_CALL(*this, rootScope()).WillByDefault(ReturnRef(stats_store_));
}

MockApi::~MockApi() = default;

Event::DispatcherPtr MockApi::allocateDispatcher() {
  return Event::DispatcherPtr{allocateDispatcher_(time_system_)};
}
Event::DispatcherPtr MockApi::allocateDispatcher(Buffer::WatermarkFactoryPtr&& watermark_factory) {
  return Event::DispatcherPtr{allocateDispatcher_(std::move(watermark_factory), time_system_)};
}

MockOsSysCalls::MockOsSysCalls() {
  ON_CALL(*this, close(_)).WillByDefault(Invoke([](int fd) {
    const int rc = ::close(fd);
    return SysCallIntResult{rc, errno};
  }));
}

MockOsSysCalls::~MockOsSysCalls() = default;

SysCallIntResult MockOsSysCalls::setsockopt(int sockfd, int level, int optname, const void* optval,
                                            socklen_t optlen) {
  ASSERT(optlen == sizeof(int));

  // Allow mocking system call failure.
  if (setsockopt_(sockfd, level, optname, optval, optlen) != 0) {
    return SysCallIntResult{-1, 0};
  }

  boolsockopts_[SockOptKey(sockfd, level, optname)] = !!*reinterpret_cast<const int*>(optval);
  return SysCallIntResult{0, 0};
};

SysCallIntResult MockOsSysCalls::getsockopt(int sockfd, int level, int optname, void* optval,
                                            socklen_t* optlen) {
  ASSERT(*optlen == sizeof(int));
  int val = 0;
  const auto& it = boolsockopts_.find(SockOptKey(sockfd, level, optname));
  if (it != boolsockopts_.end()) {
    val = it->second;
  }
  // Allow mocking system call failure.
  if (getsockopt_(sockfd, level, optname, optval, optlen) != 0) {
    return {-1, 0};
  }
  *reinterpret_cast<int*>(optval) = val;
  return {0, 0};
}

} // namespace Api
} // namespace Envoy
