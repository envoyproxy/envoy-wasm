#pragma once

#include "common/api/api_impl.h"
#include "common/common/thread.h"
#include "common/grpc/common.h"
#include "common/http/codec_client.h"
#include "common/network/connection_balancer_impl.h"
#include "common/network/listen_socket_impl.h"
#include "common/stats/isolated_store_impl.h"

#include "test/test_common/test_time.h"
#include "test/test_common/utility.h"

#include "stress_test_common.h"

namespace Envoy {
namespace Stress {

enum class ServerCloseReason {
  REMOTE_CLOSE, // Peer closed or connection was reset after it was
                // established.
  LOCAL_CLOSE   // This process decided to close the connection.
};

enum class ServerCallbackResult {
  CONTINUE, // Leave the connection open
  CLOSE     // Close the connection.
};

class ServerStream {
public:
  ServerStream() = default;

  virtual ~ServerStream() = default;

  ServerStream(const ServerStream&) = delete;
  void operator=(const ServerStream&) = delete;

  /**
   * Send a HTTP header-only response and close the stream.
   *
   * @param response_headers the response headers
   * @param delay delay in msec before sending the response. if 0 send
   * immediately
   */
  virtual void
  sendResponseHeaders(const Http::HeaderMap& response_headers,
                      const std::chrono::milliseconds delay = std::chrono::milliseconds(0)) PURE;

  /**
   * Send a gRPC response and close the stream
   *
   * @param status The gRPC status (carried in the HTTP response trailer)
   * @param response The gRPC response (carried in the HTTP response body)
   * @param delay delay in msec before sending the response. if 0 send
   * immediately
   */
  virtual void
  sendGrpcResponse(Grpc::Status::GrpcStatus status, const Protobuf::Message& response,
                   const std::chrono::milliseconds delay = std::chrono::milliseconds(0)) PURE;
};

typedef std::unique_ptr<ServerStream> ServerStreamPtr;
typedef std::shared_ptr<ServerStream> ServerStreamSharedPtr;

class ServerConnection;

// NB: references passed to any of these callbacks are owned by the caller and
// must not be used after the callback returns -- except for the request headers
// which may be moved into the caller.
typedef std::function<ServerCallbackResult(ServerConnection& server_connection)>
    ServerAcceptCallback;
typedef std::function<void(ServerConnection& connection, ServerCloseReason reason)>
    ServerCloseCallback;
typedef std::function<void(ServerConnection& connection, ServerStream& stream,
                           Http::HeaderMapPtr&& request_headers)>
    ServerRequestCallback;

class ServerConnection : public Network::ReadFilter,
                         public Network::ConnectionCallbacks,
                         public Http::ServerConnectionCallbacks,
                         Logger::Loggable<Logger::Id::testing> {
public:
  ServerConnection(const std::string& name, uint32_t id, ServerRequestCallback& request_callback,
                   ServerCloseCallback& close_callback, Network::Connection& network_connection,
                   Event::Dispatcher& dispatcher, Http::CodecClient::Type http_type,
                   Stats::Scope& scope);

  ~ServerConnection() override;
  ServerConnection(const ServerConnection&) = delete;
  ServerConnection& operator=(const ServerConnection&) = delete;

  const std::string& name() const;

  uint32_t id() const;

  Network::Connection& networkConnection();
  const Network::Connection& networkConnection() const;

  Http::ServerConnection& httpConnection();
  const Http::ServerConnection& httpConnection() const;

  Event::Dispatcher& dispatcher();

  /**
   * For internal use
   */
  void removeStream(uint32_t stream_id);

  //
  // Network::ReadFilter
  //

  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;

  Network::FilterStatus onNewConnection() override;

  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks&) override;

  //
  // Http::ConnectionCallbacks
  //

  void onGoAway() override;

  //
  // Http::ServerConnectionCallbacks
  //

  Http::StreamDecoder& newStream(Http::StreamEncoder& stream_encoder,
                                 bool is_internally_created = false) override;

  //
  // Network::ConnectionCallbacks
  //

  void onEvent(Network::ConnectionEvent event) override;

  void onAboveWriteBufferHighWatermark() override;

  void onBelowWriteBufferLowWatermark() override;

private:
  std::string name_;
  uint32_t id_;
  Network::Connection& network_connection_;
  Http::ServerConnectionPtr http_connection_;
  Event::Dispatcher& dispatcher_;
  ServerRequestCallback& request_callback_;
  ServerCloseCallback& close_callback_;

  std::mutex streams_lock_;
  std::unordered_map<uint32_t, ServerStreamPtr> streams_;
  uint32_t stream_counter_{0U};
};

typedef std::unique_ptr<ServerConnection> ServerConnectionPtr;
typedef std::shared_ptr<ServerConnection> ServerConnectionSharedPtr;

class ServerFilterChain : public Network::FilterChain {
public:
  explicit ServerFilterChain(Network::TransportSocketFactory& transport_socket_factory);
  ServerFilterChain(const ServerFilterChain&) = delete;
  ServerFilterChain& operator=(const ServerFilterChain&) = delete;

  //
  // Network::FilterChain
  //

  const Network::TransportSocketFactory& transportSocketFactory() const override;

  const std::vector<Network::FilterFactoryCb>& networkFilterFactories() const override;

private:
  Network::TransportSocketFactory& transport_socket_factory_;
  std::vector<Network::FilterFactoryCb> network_filter_factories_;
};

/**
 * A convenience class for creating a listening socket bound to localhost
 */
class LocalListenSocket : public Network::TcpListenSocket {
public:
  /**
   * Create a listening socket bound to localhost.
   *
   * @param ip_version v4 or v6. v4 by default.
   * @param port the port. If 0, let the kernel allocate an available ephemeral
   * port. 0 by default.
   * @param options socket options. nullptr by default
   * @param bind_to_port if true immediately bind to the port, allocating one if
   * necessary. true by default.
   */
  explicit LocalListenSocket(
      Network::Address::IpVersion ip_version = Network::Address::IpVersion::v4, uint16_t port = 0,
      const Network::Socket::OptionsSharedPtr& options = nullptr, bool bind_to_port = true);

  LocalListenSocket(const LocalListenSocket&) = delete;
  void operator=(const LocalListenSocket&) = delete;
};

/**
 * A convenience class for passing callbacks to a Server. If no callbacks are
 * provided, default callbacks that track some simple metrics will be used. If
 * callbacks are provided, they will be wrapped with callbacks that maintain the
 * same simple set of metrics.
 */
class ServerCallbackHelper {
public:
  explicit ServerCallbackHelper(ServerRequestCallback&& request_callback = nullptr,
                                ServerAcceptCallback&& accept_callback = nullptr,
                                ServerCloseCallback&& close_callback = nullptr);
  ServerCallbackHelper(const ServerCallbackHelper&) = delete;
  ServerCallbackHelper& operator=(const ServerCallbackHelper&) = delete;
  virtual ~ServerCallbackHelper() = default;

  uint32_t connectionsAccepted() const;
  uint32_t requestsReceived() const;
  uint32_t localCloses() const;
  uint32_t remoteCloses() const;
  ServerAcceptCallback acceptCallback() const;
  ServerRequestCallback requestCallback() const;
  ServerCloseCallback closeCallback() const;

  /*
   * Wait until the server has accepted n connections and seen them closed (due
   * to error or client close)
   */
  void wait(uint32_t connections);

  /*
   * Wait until the server has seen a close for every connection it has
   * accepted.
   */
  void wait();

private:
  ServerAcceptCallback accept_callback_;
  ServerRequestCallback request_callback_;
  ServerCloseCallback close_callback_;

  std::atomic<uint32_t> accepts_{0};
  std::atomic<uint32_t> requests_received_{0};
  uint32_t local_closes_{0};
  uint32_t remote_closes_{0};
  mutable absl::Mutex mutex_;
};

typedef std::unique_ptr<ServerCallbackHelper> ServerCallbackHelperPtr;
typedef std::shared_ptr<ServerCallbackHelper> ServerCallbackHelperSharedPtr;

class Server : public Network::FilterChainManager,
               public Network::FilterChainFactory,
               public Network::ListenerConfig,
               Logger::Loggable<Logger::Id::testing> {
public:
  Server(const std::string& name, Network::Socket& listening_socket,
         Network::TransportSocketFactory& transport_socket_factory,
         Http::CodecClient::Type http_type);
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;
  ~Server() override;

  void start(ServerAcceptCallback&& accept_callback, ServerRequestCallback&& request_callback,
             ServerCloseCallback&& close_callback);

  void start(ServerCallbackHelper& helper);

  void stop();

  void stopAcceptingConnections();

  void startAcceptingConnections();

  const Stats::Store& statsStore() const;

  void setPerConnectionBufferLimitBytes(uint32_t limit);

  //
  // Network::ListenerConfig
  //

  Network::FilterChainManager& filterChainManager() override;

  Network::FilterChainFactory& filterChainFactory() override;

  Network::Socket& socket() override;

  const Network::Socket& socket() const override;

  bool bindToPort() override;

  bool handOffRestoredDestinationConnections() const override;

  uint32_t perConnectionBufferLimitBytes() const override;

  std::chrono::milliseconds listenerFiltersTimeout() const override;

  bool continueOnListenerFiltersTimeout() const override;

  Stats::Scope& listenerScope() override;

  uint64_t listenerTag() const override;

  const std::string& name() const override;

  const Network::ActiveUdpListenerFactory* udpListenerFactory() override;

  Network::ConnectionBalancer& connectionBalancer() override { return connection_balancer_; }

  envoy::api::v2::core::TrafficDirection direction() const override {
    return envoy::api::v2::core::TrafficDirection::UNSPECIFIED;
  }

  //
  // Network::FilterChainManager
  //

  const Network::FilterChain* findFilterChain(const Network::ConnectionSocket&) const override;

  //
  // Network::FilterChainFactory
  //

  bool createNetworkFilterChain(Network::Connection& network_connection,
                                const std::vector<Network::FilterFactoryCb>&) override;

  bool createListenerFilterChain(Network::ListenerFilterManager&) override;

  bool createUdpListenerFilterChain(Network::UdpListenerFilterManager&,
                                    Network::UdpReadFilterCallbacks&) override;

private:
  std::string name_;
  Stats::IsolatedStoreImpl stats_;
  Event::TestRealTimeSystem time_system_;
  Api::Impl api_;
  Event::DispatcherPtr dispatcher_;
  Network::ConnectionHandlerPtr connection_handler_;
  Network::NopConnectionBalancerImpl connection_balancer_;
  Thread::ThreadPtr thread_;
  std::atomic<bool> is_running{false};

  ServerAcceptCallback accept_callback_{nullptr};
  ServerRequestCallback request_callback_{nullptr};
  ServerCloseCallback close_callback_{nullptr};

  //
  // Network::ListenerConfig
  //

  Network::Socket& listening_socket_;
  std::atomic<uint32_t> connection_buffer_limit_bytes_{0U};

  //
  // Network::FilterChainManager
  //

  ServerFilterChain server_filter_chain_;

  //
  // Network::FilterChainFactory
  //

  Http::CodecClient::Type http_type_;
  std::atomic<uint32_t> connection_counter_{0U};
};

typedef std::unique_ptr<Server> ServerPtr;
typedef std::shared_ptr<Server> ServerSharedPtr;

class ClusterHelper {
public:
  explicit ClusterHelper(const std::string& name);
  virtual ~ClusterHelper() = default;
  ClusterHelper(const ClusterHelper&) = delete;
  ClusterHelper& operator=(const ClusterHelper&) = delete;

  ClusterHelper& addServer(ServerCallbackHelperPtr&& server_callback);

  const std::vector<ServerCallbackHelperPtr>& servers() const;
  std::vector<ServerCallbackHelperPtr>& servers();

  inline const std::string& name() const { return name_; }

  uint32_t connectionsAccepted() const;
  uint32_t requestsReceived() const;
  uint32_t localCloses() const;
  uint32_t remoteCloses() const;

  void wait();

private:
  std::string name_;
  std::vector<ServerCallbackHelperPtr> server_callback_helpers_;
};

typedef std::unique_ptr<ClusterHelper> ClusterHelperPtr;
typedef std::shared_ptr<ClusterHelper> ClusterHelperSharedPtr;

} // namespace Stress
} // namespace Envoy
