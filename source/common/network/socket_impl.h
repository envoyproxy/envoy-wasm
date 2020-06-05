#pragma once

#include "envoy/network/socket.h"

#include "common/common/assert.h"

namespace Envoy {
namespace Network {

namespace SocketInterface {

/**
 * Low level api to create a socket in the underlying host stack. Does not create an
 * Envoy socket.
 * @param type type of socket requested
 * @param addr_type type of address used with the socket
 * @param version IP version if address type is IP
 * @return Socket file descriptor
 */
IoHandlePtr socket(Address::SocketType type, Address::Type addr_type, Address::IpVersion version);

/**
 * Low level api to create a socket in the underlying host stack. Does not create an
 * Envoy socket.
 * @param socket_type type of socket requested
 * @param addr address that is gleaned for address type and version if needed (@see createSocket)
 */
IoHandlePtr socket(Address::SocketType socket_type, const Address::InstanceConstSharedPtr addr);

/**
 * Returns true if the given family is supported on this machine.
 * @param domain the IP family.
 */
bool ipFamilySupported(int domain);

/**
 * Obtain an address from a bound file descriptor. Raises an EnvoyException on failure.
 * @param fd socket file descriptor
 * @return InstanceConstSharedPtr for bound address.
 */
Address::InstanceConstSharedPtr addressFromFd(os_fd_t fd);

/**
 * Obtain the address of the peer of the socket with the specified file descriptor.
 * Raises an EnvoyException on failure.
 * @param fd socket file descriptor
 * @return InstanceConstSharedPtr for peer address.
 */
Address::InstanceConstSharedPtr peerAddressFromFd(os_fd_t fd);

} // namespace SocketInterface

class SocketImpl : public virtual Socket {
public:
  SocketImpl(Address::SocketType type, Address::Type addr_type, Address::IpVersion version);
  SocketImpl(Address::SocketType socket_type, const Address::InstanceConstSharedPtr addr);

  // Network::Socket
  const Address::InstanceConstSharedPtr& localAddress() const override { return local_address_; }
  void setLocalAddress(const Address::InstanceConstSharedPtr& local_address) override {
    local_address_ = local_address;
  }

  IoHandle& ioHandle() override { return *io_handle_; }
  const IoHandle& ioHandle() const override { return *io_handle_; }
  void close() override {
    if (io_handle_->isOpen()) {
      io_handle_->close();
    }
  }
  bool isOpen() const override { return io_handle_->isOpen(); }
  void ensureOptions() {
    if (!options_) {
      options_ = std::make_shared<std::vector<OptionConstSharedPtr>>();
    }
  }
  void addOption(const OptionConstSharedPtr& option) override {
    ensureOptions();
    options_->emplace_back(std::move(option));
  }
  void addOptions(const OptionsSharedPtr& options) override {
    ensureOptions();
    Network::Socket::appendOptions(options_, options);
  }

  Api::SysCallIntResult bind(Network::Address::InstanceConstSharedPtr address) override;
  Api::SysCallIntResult listen(int backlog) override;
  Api::SysCallIntResult connect(const Address::InstanceConstSharedPtr addr) override;
  Api::SysCallIntResult setSocketOption(int level, int optname, const void* optval,
                                        socklen_t optlen) override;
  Api::SysCallIntResult getSocketOption(int level, int optname, void* optval,
                                        socklen_t* optlen) override;
  Api::SysCallIntResult setBlockingForTest(bool blocking) override;

  const OptionsSharedPtr& options() const override { return options_; }
  Address::SocketType socketType() const override { return sock_type_; }
  Address::Type addressType() const override { return addr_type_; }

protected:
  SocketImpl(IoHandlePtr&& io_handle, const Address::InstanceConstSharedPtr& local_address);

  const IoHandlePtr io_handle_;
  Address::InstanceConstSharedPtr local_address_;
  OptionsSharedPtr options_;
  Address::SocketType sock_type_;
  Address::Type addr_type_;
};

} // namespace Network
} // namespace Envoy