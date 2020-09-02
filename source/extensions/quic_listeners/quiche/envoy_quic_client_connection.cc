#include "extensions/quic_listeners/quiche/envoy_quic_client_connection.h"

#include <memory>

#include "envoy/config/core/v3/base.pb.h"

#include "common/network/listen_socket_impl.h"
#include "common/network/socket_option_factory.h"
#include "common/network/udp_packet_writer_handler_impl.h"

#include "extensions/quic_listeners/quiche/envoy_quic_packet_writer.h"
#include "extensions/quic_listeners/quiche/envoy_quic_utils.h"
#include "extensions/transport_sockets/well_known_names.h"

namespace Envoy {
namespace Quic {

EnvoyQuicClientConnection::EnvoyQuicClientConnection(
    const quic::QuicConnectionId& server_connection_id,
    Network::Address::InstanceConstSharedPtr& initial_peer_address,
    quic::QuicConnectionHelperInterface& helper, quic::QuicAlarmFactory& alarm_factory,
    const quic::ParsedQuicVersionVector& supported_versions,
    Network::Address::InstanceConstSharedPtr local_addr, Event::Dispatcher& dispatcher,
    const Network::ConnectionSocket::OptionsSharedPtr& options)
    : EnvoyQuicClientConnection(server_connection_id, helper, alarm_factory, supported_versions,
                                dispatcher,
                                createConnectionSocket(initial_peer_address, local_addr, options)) {
}

EnvoyQuicClientConnection::EnvoyQuicClientConnection(
    const quic::QuicConnectionId& server_connection_id, quic::QuicConnectionHelperInterface& helper,
    quic::QuicAlarmFactory& alarm_factory, const quic::ParsedQuicVersionVector& supported_versions,
    Event::Dispatcher& dispatcher, Network::ConnectionSocketPtr&& connection_socket)
    : EnvoyQuicClientConnection(
          server_connection_id, helper, alarm_factory,
          new EnvoyQuicPacketWriter(
              std::make_unique<Network::UdpDefaultWriter>(connection_socket->ioHandle())),
          true, supported_versions, dispatcher, std::move(connection_socket)) {}

EnvoyQuicClientConnection::EnvoyQuicClientConnection(
    const quic::QuicConnectionId& server_connection_id, quic::QuicConnectionHelperInterface& helper,
    quic::QuicAlarmFactory& alarm_factory, quic::QuicPacketWriter* writer, bool owns_writer,
    const quic::ParsedQuicVersionVector& supported_versions, Event::Dispatcher& dispatcher,
    Network::ConnectionSocketPtr&& connection_socket)
    : EnvoyQuicConnection(
          server_connection_id,
          envoyIpAddressToQuicSocketAddress(connection_socket->remoteAddress()->ip()), helper,
          alarm_factory, writer, owns_writer, quic::Perspective::IS_CLIENT, supported_versions,
          std::move(connection_socket)),
      dispatcher_(dispatcher) {}

void EnvoyQuicClientConnection::processPacket(
    Network::Address::InstanceConstSharedPtr local_address,
    Network::Address::InstanceConstSharedPtr peer_address, Buffer::InstancePtr buffer,
    MonotonicTime receive_time) {
  if (!connected()) {
    return;
  }
  quic::QuicTime timestamp =
      quic::QuicTime::Zero() +
      quic::QuicTime::Delta::FromMicroseconds(
          std::chrono::duration_cast<std::chrono::microseconds>(receive_time.time_since_epoch())
              .count());
  ASSERT(buffer->getRawSlices().size() == 1);
  Buffer::RawSliceVector slices = buffer->getRawSlices(/*max_slices=*/1);
  quic::QuicReceivedPacket packet(reinterpret_cast<char*>(slices[0].mem_), slices[0].len_,
                                  timestamp, /*owns_buffer=*/false, /*ttl=*/0, /*ttl_valid=*/false,
                                  /*packet_headers=*/nullptr, /*headers_length=*/0,
                                  /*owns_header_buffer*/ false);
  ProcessUdpPacket(envoyIpAddressToQuicSocketAddress(local_address->ip()),
                   envoyIpAddressToQuicSocketAddress(peer_address->ip()), packet);
}

uint64_t EnvoyQuicClientConnection::maxPacketSize() const {
  // TODO(danzh) make this variable configurable to support jumbo frames.
  return Network::MAX_UDP_PACKET_SIZE;
}

void EnvoyQuicClientConnection::setUpConnectionSocket() {
  if (connectionSocket()->ioHandle().isOpen()) {
    file_event_ = connectionSocket()->ioHandle().createFileEvent(
        dispatcher_, [this](uint32_t events) -> void { onFileEvent(events); },
        Event::FileTriggerType::Edge, Event::FileReadyType::Read | Event::FileReadyType::Write);

    if (!Network::Socket::applyOptions(connectionSocket()->options(), *connectionSocket(),
                                       envoy::config::core::v3::SocketOption::STATE_LISTENING)) {
      ENVOY_CONN_LOG(error, "Fail to apply listening options", *this);
      connectionSocket()->close();
    }
  }
  if (!connectionSocket()->ioHandle().isOpen()) {
    CloseConnection(quic::QUIC_CONNECTION_CANCELLED, "Fail to set up connection socket.",
                    quic::ConnectionCloseBehavior::SILENT_CLOSE);
  }
}

void EnvoyQuicClientConnection::switchConnectionSocket(
    Network::ConnectionSocketPtr&& connection_socket) {
  auto writer = std::make_unique<EnvoyQuicPacketWriter>(
      std::make_unique<Network::UdpDefaultWriter>(connection_socket->ioHandle()));
  // Destroy the old file_event before closing the old socket. Otherwise the socket might be picked
  // up by another socket() call while file_event is still operating on it.
  file_event_.reset();
  // The old socket is closed in this call.
  setConnectionSocket(std::move(connection_socket));
  setUpConnectionSocket();
  SetQuicPacketWriter(writer.release(), true);
}

void EnvoyQuicClientConnection::onFileEvent(uint32_t events) {
  ENVOY_CONN_LOG(trace, "socket event: {}", *this, events);
  ASSERT(events & (Event::FileReadyType::Read | Event::FileReadyType::Write));

  if (events & Event::FileReadyType::Write) {
    OnCanWrite();
  }

  // It's possible for a write event callback to close the connection, in such case ignore read
  // event processing.
  if (connected() && (events & Event::FileReadyType::Read)) {
    Api::IoErrorPtr err = Network::Utility::readPacketsFromSocket(
        connectionSocket()->ioHandle(), *connectionSocket()->localAddress(), *this,
        dispatcher_.timeSource(), packets_dropped_);
    // TODO(danzh): Handle no error when we limit the number of packets read.
    if (err->getErrorCode() != Api::IoError::IoErrorCode::Again) {
      ENVOY_CONN_LOG(error, "recvmsg result {}: {}", *this, static_cast<int>(err->getErrorCode()),
                     err->getErrorDetails());
    }
  }
}

} // namespace Quic
} // namespace Envoy
