#include "common/common/assert.h"
#include "common/common/thread_impl.h"
#include "common/filesystem/filesystem_impl.h"

#include "exe/platform_impl.h"

// clang-format off
#include <winsock2.h>
// clang-format on

namespace Envoy {

PlatformImpl::PlatformImpl()
    : thread_factory_(std::make_unique<Thread::ThreadFactoryImplWin32>()),
      file_system_(std::make_unique<Filesystem::InstanceImplWin32>()) {
  const WORD wVersionRequested = MAKEWORD(2, 2);
  WSADATA wsaData;
  const int rc = ::WSAStartup(wVersionRequested, &wsaData);
  RELEASE_ASSERT(rc == 0, "WSAStartup failed with error");
}

PlatformImpl::~PlatformImpl() { ::WSACleanup(); }

} // namespace Envoy
