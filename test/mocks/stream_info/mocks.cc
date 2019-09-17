#include "test/mocks/stream_info/mocks.h"

#include "common/network/address_impl.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Const;
using testing::Invoke;
using testing::ReturnPointee;
using testing::ReturnRef;

namespace Envoy {
namespace StreamInfo {

MockStreamInfo::MockStreamInfo()
    : downstream_local_address_(new Network::Address::Ipv4Instance("127.0.0.2")),
      downstream_direct_remote_address_(new Network::Address::Ipv4Instance("127.0.0.1")),
      downstream_remote_address_(new Network::Address::Ipv4Instance("127.0.0.1")) {
  ON_CALL(*this, upstreamHost()).WillByDefault(ReturnPointee(&host_));
  ON_CALL(*this, startTime()).WillByDefault(ReturnPointee(&start_time_));
  ON_CALL(*this, startTimeMonotonic()).WillByDefault(ReturnPointee(&start_time_monotonic_));
  ON_CALL(*this, lastDownstreamRxByteReceived())
      .WillByDefault(ReturnPointee(&last_downstream_rx_byte_received_));
  ON_CALL(*this, firstUpstreamTxByteSent())
      .WillByDefault(ReturnPointee(&first_upstream_tx_byte_sent_));
  ON_CALL(*this, lastUpstreamTxByteSent())
      .WillByDefault(ReturnPointee(&last_upstream_tx_byte_sent_));
  ON_CALL(*this, firstUpstreamRxByteReceived())
      .WillByDefault(ReturnPointee(&first_upstream_rx_byte_received_));
  ON_CALL(*this, lastUpstreamRxByteReceived())
      .WillByDefault(ReturnPointee(&last_upstream_rx_byte_received_));
  ON_CALL(*this, firstDownstreamTxByteSent())
      .WillByDefault(ReturnPointee(&first_downstream_tx_byte_sent_));
  ON_CALL(*this, lastDownstreamTxByteSent())
      .WillByDefault(ReturnPointee(&last_downstream_tx_byte_sent_));
  ON_CALL(*this, requestComplete()).WillByDefault(ReturnPointee(&end_time_));
  ON_CALL(*this, setUpstreamLocalAddress(_))
      .WillByDefault(
          Invoke([this](const Network::Address::InstanceConstSharedPtr& upstream_local_address) {
            upstream_local_address_ = upstream_local_address;
          }));
  ON_CALL(*this, upstreamLocalAddress()).WillByDefault(ReturnRef(upstream_local_address_));
  ON_CALL(*this, setDownstreamLocalAddress(_))
      .WillByDefault(
          Invoke([this](const Network::Address::InstanceConstSharedPtr& downstream_local_address) {
            downstream_local_address_ = downstream_local_address;
          }));
  ON_CALL(*this, downstreamLocalAddress()).WillByDefault(ReturnRef(downstream_local_address_));
  ON_CALL(*this, setDownstreamDirectRemoteAddress(_))
      .WillByDefault(Invoke(
          [this](const Network::Address::InstanceConstSharedPtr& downstream_direct_remote_address) {
            downstream_direct_remote_address_ = downstream_direct_remote_address;
          }));
  ON_CALL(*this, downstreamDirectRemoteAddress())
      .WillByDefault(ReturnRef(downstream_direct_remote_address_));
  ON_CALL(*this, setDownstreamRemoteAddress(_))
      .WillByDefault(
          Invoke([this](const Network::Address::InstanceConstSharedPtr& downstream_remote_address) {
            downstream_remote_address_ = downstream_remote_address;
          }));
  ON_CALL(*this, downstreamRemoteAddress()).WillByDefault(ReturnRef(downstream_remote_address_));
  ON_CALL(*this, setDownstreamSslConnection(_))
      .WillByDefault(Invoke(
          [this](const auto& connection_info) { downstream_connection_info_ = connection_info; }));
  ON_CALL(*this, setUpstreamSslConnection(_))
      .WillByDefault(Invoke(
          [this](const auto& connection_info) { upstream_connection_info_ = connection_info; }));
  ON_CALL(*this, downstreamSslConnection()).WillByDefault(Invoke([this]() {
    return downstream_connection_info_;
  }));
  ON_CALL(*this, upstreamSslConnection()).WillByDefault(Invoke([this]() {
    return upstream_connection_info_;
  }));
  ON_CALL(*this, protocol()).WillByDefault(ReturnPointee(&protocol_));
  ON_CALL(*this, responseCode()).WillByDefault(ReturnPointee(&response_code_));
  ON_CALL(*this, responseCodeDetails()).WillByDefault(ReturnPointee(&response_code_details_));
  ON_CALL(*this, addBytesReceived(_)).WillByDefault(Invoke([this](uint64_t bytes_received) {
    bytes_received_ += bytes_received;
  }));
  ON_CALL(*this, bytesReceived()).WillByDefault(ReturnPointee(&bytes_received_));
  ON_CALL(*this, addBytesSent(_)).WillByDefault(Invoke([this](uint64_t bytes_sent) {
    bytes_sent_ += bytes_sent;
  }));
  ON_CALL(*this, bytesSent()).WillByDefault(ReturnPointee(&bytes_sent_));
  ON_CALL(*this, dynamicMetadata()).WillByDefault(ReturnRef(metadata_));
  ON_CALL(Const(*this), dynamicMetadata()).WillByDefault(ReturnRef(metadata_));
  ON_CALL(*this, filterState()).WillByDefault(ReturnRef(filter_state_));
  ON_CALL(Const(*this), filterState()).WillByDefault(ReturnRef(filter_state_));
  ON_CALL(*this, setRequestedServerName(_))
      .WillByDefault(Invoke([this](const absl::string_view requested_server_name) {
        requested_server_name_ = std::string(requested_server_name);
      }));
  ON_CALL(*this, requestedServerName()).WillByDefault(ReturnRef(requested_server_name_));
  ON_CALL(*this, setRouteName(_)).WillByDefault(Invoke([this](const absl::string_view route_name) {
    route_name_ = std::string(route_name);
  }));
  ON_CALL(*this, getRouteName()).WillByDefault(ReturnRef(route_name_));
  ON_CALL(*this, upstreamTransportFailureReason())
      .WillByDefault(ReturnRef(upstream_transport_failure_reason_));
}

MockStreamInfo::~MockStreamInfo() = default;

} // namespace StreamInfo
} // namespace Envoy
