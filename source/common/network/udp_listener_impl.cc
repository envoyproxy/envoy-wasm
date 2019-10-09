#include "common/network/udp_listener_impl.h"

#include <sys/un.h>

#include <cerrno>
#include <csetjmp>
#include <cstring>

#include "envoy/buffer/buffer.h"
#include "envoy/common/exception.h"

#include "common/api/os_sys_calls_impl.h"
#include "common/common/assert.h"
#include "common/common/empty_string.h"
#include "common/common/fmt.h"
#include "common/common/stack_array.h"
#include "common/event/dispatcher_impl.h"
#include "common/network/address_impl.h"
#include "common/network/io_socket_error_impl.h"

#include "event2/listener.h"

#define ENVOY_UDP_LOG(LEVEL, FORMAT, ...)                                                          \
  ENVOY_LOG_TO_LOGGER(ENVOY_LOGGER(), LEVEL, "Listener at {} :" FORMAT,                            \
                      this->localAddress()->asString(), ##__VA_ARGS__)

namespace Envoy {
namespace Network {

UdpListenerImpl::UdpListenerImpl(Event::DispatcherImpl& dispatcher, Socket& socket,
                                 UdpListenerCallbacks& cb, TimeSource& time_source)
    : BaseListenerImpl(dispatcher, socket), cb_(cb), time_source_(time_source) {
  file_event_ = dispatcher_.createFileEvent(
      socket.ioHandle().fd(), [this](uint32_t events) -> void { onSocketEvent(events); },
      Event::FileTriggerType::Edge, Event::FileReadyType::Read | Event::FileReadyType::Write);

  ASSERT(file_event_);

  if (!Network::Socket::applyOptions(socket.options(), socket,
                                     envoy::api::v2::core::SocketOption::STATE_BOUND)) {
    throw CreateListenerException(fmt::format("cannot set post-bound socket option on socket: {}",
                                              socket.localAddress()->asString()));
  }
}

UdpListenerImpl::~UdpListenerImpl() {
  disable();
  file_event_.reset();
}

void UdpListenerImpl::disable() { file_event_->setEnabled(0); }

void UdpListenerImpl::enable() {
  file_event_->setEnabled(Event::FileReadyType::Read | Event::FileReadyType::Write);
}

void UdpListenerImpl::onSocketEvent(short flags) {
  ASSERT((flags & (Event::FileReadyType::Read | Event::FileReadyType::Write)));
  ENVOY_UDP_LOG(trace, "socket event: {}", flags);

  if (flags & Event::FileReadyType::Read) {
    handleReadCallback();
  }

  if (flags & Event::FileReadyType::Write) {
    handleWriteCallback();
  }
}

void UdpListenerImpl::handleReadCallback() {
  ENVOY_UDP_LOG(trace, "handleReadCallback");
  do {
    uint32_t old_packets_dropped = packets_dropped_;
    MonotonicTime receive_time = time_source_.monotonicTime();
    Api::IoCallUint64Result result =
        Utility::readFromSocket(socket_, *this, receive_time, &packets_dropped_);

    if (!result.ok()) {
      // No more to read or encountered a system error.
      if (result.err_->getErrorCode() != Api::IoError::IoErrorCode::Again) {
        ENVOY_UDP_LOG(error, "recvmsg result {}: {}", static_cast<int>(result.err_->getErrorCode()),
                      result.err_->getErrorDetails());
        cb_.onReceiveError(UdpListenerCallbacks::ErrorCode::SyscallError,
                           result.err_->getErrorCode());
      }
      // Stop reading.
      return;
    }

    if (result.rc_ == 0) {
      // TODO(conqerAtapple): Is zero length packet interesting? If so add stats
      // for it. Otherwise remove the warning log below.
      ENVOY_UDP_LOG(trace, "received 0-length packet");
    }

    if (packets_dropped_ != old_packets_dropped) {
      // The kernel tracks SO_RXQ_OVFL as a uint32 which can overflow to a smaller
      // value. So as long as this count differs from previously recorded value,
      // more packets are dropped by kernel.
      uint32_t delta = (packets_dropped_ > old_packets_dropped)
                           ? (packets_dropped_ - old_packets_dropped)
                           : (packets_dropped_ +
                              (std::numeric_limits<uint32_t>::max() - old_packets_dropped) + 1);
      // TODO(danzh) add stats for this.
      ENVOY_UDP_LOG(debug, "Kernel dropped {} more packets. Consider increase receive buffer size.",
                    delta);
    }
  } while (true);
}

void UdpListenerImpl::processPacket(Address::InstanceConstSharedPtr local_address,
                                    Address::InstanceConstSharedPtr peer_address,
                                    Buffer::InstancePtr buffer, MonotonicTime receive_time) {
  UdpRecvData recvData{std::move(local_address), std::move(peer_address), std::move(buffer),
                       receive_time};
  cb_.onData(recvData);
}

void UdpListenerImpl::handleWriteCallback() {
  ENVOY_UDP_LOG(trace, "handleWriteCallback");
  cb_.onWriteReady(socket_);
}

Event::Dispatcher& UdpListenerImpl::dispatcher() { return dispatcher_; }

const Address::InstanceConstSharedPtr& UdpListenerImpl::localAddress() const {
  return socket_.localAddress();
}

Api::IoCallUint64Result UdpListenerImpl::send(const UdpSendData& send_data) {
  ENVOY_UDP_LOG(trace, "send");
  Buffer::Instance& buffer = send_data.buffer_;
  uint64_t num_slices = buffer.getRawSlices(nullptr, 0);
  STACK_ARRAY(slices, Buffer::RawSlice, num_slices);
  buffer.getRawSlices(slices.begin(), num_slices);
  Api::IoCallUint64Result send_result = Utility::writeToSocket(
      socket_, slices.begin(), num_slices, send_data.local_ip_, send_data.peer_address_);

  // The send_result normalizes the rc_ value to 0 in error conditions.
  // The drain call is hence 'safe' in success and failure cases.
  buffer.drain(send_result.rc_);
  return send_result;
}

} // namespace Network
} // namespace Envoy
