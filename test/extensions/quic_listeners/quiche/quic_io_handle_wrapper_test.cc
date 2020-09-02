#include <cstddef>
#include <memory>

#include "envoy/common/platform.h"

#include "common/network/address_impl.h"

#include "extensions/quic_listeners/quiche/quic_io_handle_wrapper.h"

#include "test/mocks/api/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/test_common/threadsafe_singleton_injector.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Return;

namespace Envoy {
namespace Quic {

class QuicIoHandleWrapperTest : public testing::Test {
public:
  QuicIoHandleWrapperTest() : wrapper_(std::make_unique<QuicIoHandleWrapper>(socket_.ioHandle())) {
    EXPECT_TRUE(wrapper_->isOpen());
    EXPECT_FALSE(socket_.ioHandle().isOpen());
  }
  ~QuicIoHandleWrapperTest() override = default;

protected:
  testing::NiceMock<Network::MockListenSocket> socket_;
  std::unique_ptr<QuicIoHandleWrapper> wrapper_;
  testing::StrictMock<Envoy::Api::MockOsSysCalls> os_sys_calls_;
  TestThreadsafeSingletonInjector<Envoy::Api::OsSysCallsImpl> os_calls_{&os_sys_calls_};
};

TEST_F(QuicIoHandleWrapperTest, Close) {
  EXPECT_TRUE(wrapper_->close().ok());
  EXPECT_FALSE(wrapper_->isOpen());
}

TEST_F(QuicIoHandleWrapperTest, DelegateIoHandleCalls) {
  // TODO(fcoras): seems we could do without the fd in the tests lower. Can we remove it?
  os_fd_t fd = socket_.ioHandle().fdDoNotUse();
  char data[5];
  Buffer::RawSlice slice{data, 5};
  EXPECT_CALL(os_sys_calls_, readv(fd, _, 1)).WillOnce(Return(Api::SysCallSizeResult{5u, 0}));
  wrapper_->readv(5, &slice, 1);

  EXPECT_CALL(os_sys_calls_, writev(fd, _, 1)).WillOnce(Return(Api::SysCallSizeResult{5u, 0}));
  wrapper_->writev(&slice, 1);

  EXPECT_CALL(os_sys_calls_, socket(AF_INET6, SOCK_STREAM, 0))
      .WillRepeatedly(Return(Api::SysCallSocketResult{1, 0}));
  EXPECT_CALL(os_sys_calls_, close(1)).WillRepeatedly(Return(Api::SysCallIntResult{0, 0}));

  Network::Address::InstanceConstSharedPtr addr(new Network::Address::Ipv4Instance(12345));
  EXPECT_CALL(os_sys_calls_, sendmsg(fd, _, 0)).WillOnce(Return(Api::SysCallSizeResult{5u, 0}));
  wrapper_->sendmsg(&slice, 1, 0, /*self_ip=*/nullptr, *addr);

  EXPECT_CALL(os_sys_calls_, getsockname(_, _, _)).WillOnce(Return(Api::SysCallIntResult{0, 0}));
  wrapper_->domain();

  EXPECT_CALL(os_sys_calls_, getsockname(_, _, _))
      .WillOnce(Invoke([](os_fd_t, sockaddr* addr, socklen_t* addrlen) -> Api::SysCallIntResult {
        addr->sa_family = AF_INET6;
        *addrlen = sizeof(sockaddr_in6);
        return Api::SysCallIntResult{0, 0};
      }));
  addr = wrapper_->localAddress();

  EXPECT_CALL(os_sys_calls_, getpeername(_, _, _))
      .WillOnce(Invoke([](os_fd_t, sockaddr* addr, socklen_t* addrlen) -> Api::SysCallIntResult {
        addr->sa_family = AF_INET6;
        *addrlen = sizeof(sockaddr_in6);
        return Api::SysCallIntResult{0, 0};
      }));
  addr = wrapper_->peerAddress();

  Network::IoHandle::RecvMsgOutput output(1, nullptr);
  EXPECT_CALL(os_sys_calls_, recvmsg(fd, _, 0)).WillOnce(Invoke([](os_fd_t, msghdr* msg, int) {
    sockaddr_storage ss;
    auto ipv6_addr = reinterpret_cast<sockaddr_in6*>(&ss);
    memset(ipv6_addr, 0, sizeof(sockaddr_in6));
    ipv6_addr->sin6_family = AF_INET6;
    ipv6_addr->sin6_addr = in6addr_loopback;
    ipv6_addr->sin6_port = htons(54321);
    *reinterpret_cast<sockaddr_in6*>(msg->msg_name) = *ipv6_addr;
    msg->msg_namelen = sizeof(sockaddr_in6);
    msg->msg_controllen = 0;
    return Api::SysCallSizeResult{5u, 0};
  }));
  wrapper_->recvmsg(&slice, 1, /*self_port=*/12345, output);

  size_t num_packet_per_call = 1u;
  Network::IoHandle::RecvMsgOutput output2(num_packet_per_call, nullptr);
  RawSliceArrays slices(num_packet_per_call,
                        absl::FixedArray<Buffer::RawSlice>({Buffer::RawSlice{data, 5}}));
  EXPECT_CALL(os_sys_calls_, recvmmsg(fd, _, num_packet_per_call, _, nullptr))
      .WillOnce(Invoke([](os_fd_t, struct mmsghdr*, unsigned int, int, struct timespec*) {
        return Api::SysCallIntResult{1u, 0};
      }));
  wrapper_->recvmmsg(slices, /*self_port=*/12345, output2);

  EXPECT_TRUE(wrapper_->close().ok());

  // Following calls shouldn't be delegated.
  wrapper_->readv(5, &slice, 1);
  wrapper_->writev(&slice, 1);
  wrapper_->sendmsg(&slice, 1, 0, /*self_ip=*/nullptr, *addr);
  EXPECT_DEBUG_DEATH(wrapper_->recvmsg(&slice, 1, /*self_port=*/12345, output),
                     "recvmmsg is called after close");
  EXPECT_DEBUG_DEATH(wrapper_->recvmmsg(slices, /*self_port=*/12345, output2),
                     "recvmmsg is called after close");

  EXPECT_CALL(os_sys_calls_, supportsUdpGro());
  wrapper_->supportsUdpGro();

  EXPECT_CALL(os_sys_calls_, supportsMmsg());
  wrapper_->supportsMmsg();
}

} // namespace Quic
} // namespace Envoy
