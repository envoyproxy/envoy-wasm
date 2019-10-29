#pragma once

#include <vector>

#include "envoy/buffer/buffer.h"
#include "envoy/common/pure.h"
#include "envoy/network/io_handle.h"
#include "envoy/ssl/connection.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Network {

class Connection;
enum class ConnectionEvent;

/**
 * Action that should occur on a connection after I/O.
 */
enum class PostIoAction {
  // Close the connection.
  Close,
  // Keep the connection open.
  KeepOpen
};

/**
 * Result of each I/O event.
 */
struct IoResult {
  PostIoAction action_;

  /**
   * Number of bytes processed by the I/O event.
   */
  uint64_t bytes_processed_;

  /**
   * True if an end-of-stream was read from a connection. This
   * can only be true for read operations.
   */
  bool end_stream_read_;
};

/**
 * Callbacks used by transport socket instances to communicate with connection.
 */
class TransportSocketCallbacks {
public:
  virtual ~TransportSocketCallbacks() = default;

  /**
   * @return reference to the IoHandle associated with the connection.
   */
  virtual IoHandle& ioHandle() PURE;

  /**
   * @return const reference to the IoHandle associated with the connection.
   */
  virtual const IoHandle& ioHandle() const PURE;

  /**
   * @return Network::Connection& the connection interface.
   */
  virtual Network::Connection& connection() PURE;

  /**
   * @return bool whether the read buffer should be drained. This is used to enforce yielding for
   *         configured read limits.
   */
  virtual bool shouldDrainReadBuffer() PURE;

  /**
   * Mark read buffer ready to read in the event loop. This is used when yielding following
   * shouldDrainReadBuffer().
   */
  virtual void setReadBufferReady() PURE;

  /**
   * Raise a connection event to the connection. This can be used by a secure socket (e.g. TLS)
   * to raise a connected event when handshake is done.
   * @param event supplies the connection event
   */
  virtual void raiseEvent(ConnectionEvent event) PURE;
};

/**
 * A transport socket that does actual read / write. It can also do some transformations on
 * the data (e.g. TLS).
 */
class TransportSocket {
public:
  virtual ~TransportSocket() = default;

  /**
   * Called by connection once to initialize the transport socket callbacks that the transport
   * socket should use.
   * @param callbacks supplies the callbacks instance.
   */
  virtual void setTransportSocketCallbacks(TransportSocketCallbacks& callbacks) PURE;

  /**
   * @return std::string the protocol to use as selected by network level negotiation. (E.g., ALPN).
   *         If network level negotiation is not supported by the connection or no protocol
   *         has been negotiated the empty string is returned.
   */
  virtual std::string protocol() const PURE;

  /**
   * @return std::string the last failure reason occurred on the transport socket. If no failure
   *         has been occurred the empty string is returned.
   */
  virtual absl::string_view failureReason() const PURE;

  /**
   * @return bool whether the socket can be flushed and closed.
   */
  virtual bool canFlushClose() PURE;

  /**
   * Closes the transport socket.
   * @param event supplies the connection event that is closing the socket.
   */
  virtual void closeSocket(Network::ConnectionEvent event) PURE;

  /**
   * @param buffer supplies the buffer to read to.
   * @return IoResult the result of the read action.
   */
  virtual IoResult doRead(Buffer::Instance& buffer) PURE;

  /**
   * @param buffer supplies the buffer to write from
   * @param end_stream supplies whether this is the end of the stream. If true and all
   *        data in buffer is written, the connection will be half-closed.
   * @return IoResult the result of the write action.
   */
  virtual IoResult doWrite(Buffer::Instance& buffer, bool end_stream) PURE;

  /**
   * Called when underlying transport is established.
   */
  virtual void onConnected() PURE;

  /**
   * @return the const SSL connection data if this is an SSL connection, or nullptr if it is not.
   */
  virtual Ssl::ConnectionInfoConstSharedPtr ssl() const PURE;
};

using TransportSocketPtr = std::unique_ptr<TransportSocket>;

/**
 * Options for creating transport sockets.
 */
class TransportSocketOptions {
public:
  virtual ~TransportSocketOptions() = default;

  /**
   * @return the const optional server name to set in the transport socket, for example SNI for
   *         SSL, regardless of the upstream cluster configuration. Filters that influence
   *         upstream connection selection, such as tcp_proxy, should take this option into account
   *         and should pass it through to the connection pool to ensure the correct endpoints are
   *         selected and the upstream connection is set up accordingly.
   */
  virtual const absl::optional<std::string>& serverNameOverride() const PURE;

  /**
   * @return the optional overridden SAN names to verify, if the transport socket supports SAN
   *         verification.
   */
  virtual const std::vector<std::string>& verifySubjectAltNameListOverride() const PURE;

  /**
   * @return the optional overridden application protocols.
   */
  virtual const std::vector<std::string>& applicationProtocolListOverride() const PURE;

  /**
   * @param vector of bytes to which the option should append hash key data that will be used
   *        to separate connections based on the option. Any data already in the key vector must
   *        not be modified.
   */
  virtual void hashKey(std::vector<uint8_t>& key) const PURE;
};

// TODO(mattklein123): Rename to TransportSocketOptionsConstSharedPtr in a dedicated follow up.
using TransportSocketOptionsSharedPtr = std::shared_ptr<const TransportSocketOptions>;

/**
 * A factory for creating transport socket. It will be associated to filter chains and clusters.
 */
class TransportSocketFactory {
public:
  virtual ~TransportSocketFactory() = default;

  /**
   * @return bool whether the transport socket implements secure transport.
   */
  virtual bool implementsSecureTransport() const PURE;

  /**
   * @param options for creating the transport socket
   * @return Network::TransportSocketPtr a transport socket to be passed to connection.
   */
  virtual TransportSocketPtr
  createTransportSocket(TransportSocketOptionsSharedPtr options) const PURE;
};

using TransportSocketFactoryPtr = std::unique_ptr<TransportSocketFactory>;

} // namespace Network
} // namespace Envoy
