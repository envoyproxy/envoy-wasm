#include "stress_test_upstream.h"

#include <future>

#include "envoy/http/codec.h"
#include "envoy/network/transport_socket.h"

#include "common/common/lock_guard.h"
#include "common/common/logger.h"
#include "common/grpc/codec.h"
#include "common/http/conn_manager_config.h"
#include "common/http/conn_manager_impl.h"
#include "common/http/exception.h"
#include "common/http/http1/codec_impl.h"
#include "common/http/http2/codec_impl.h"
#include "common/network/listen_socket_impl.h"
#include "common/network/raw_buffer_socket.h"

#include "server/connection_handler_impl.h"

#include "test/test_common/network_utility.h"
#include "test/test_common/utility.h"

#include "fmt/printf.h"

namespace Envoy {
namespace Stress {

static Http::LowerCaseString RequestId(std::string("x-request-id"));

class ServerStreamImpl : public ServerStream,
                         public Http::StreamDecoder,
                         public Http::StreamCallbacks,
                         Logger::Loggable<Logger::Id::testing> {
public:
  ServerStreamImpl(uint32_t id, ServerConnection& connection,
                   ServerRequestCallback& request_callback, Http::StreamEncoder& stream_encoder)
      : id_(id), connection_(connection), request_callback_(request_callback),
        stream_encoder_(stream_encoder) {}

  ~ServerStreamImpl() override {
    ENVOY_LOG(trace, "ServerStream({}:{}:{}) destroyed", connection_.name(), connection_.id(), id_);
  }

  ServerStreamImpl(const ServerStreamImpl&) = delete;

  ServerStreamImpl& operator=(const ServerStreamImpl&) = delete;

  //
  // ServerStream
  //

  void sendResponseHeaders(const Http::HeaderMap& response_headers,
                           const std::chrono::milliseconds delay) override {
    if (connection_.networkConnection().state() != Network::Connection::State::Open) {
      ENVOY_LOG(warn, "ServerStream({}:{}:{})'s underlying connection is not open!",
                connection_.name(), connection_.id(), id_);
      return;
    }

    if (delay <= std::chrono::milliseconds(0)) {
      ENVOY_LOG(debug, "ServerStream({}:{}:{}) sending response headers", connection_.name(),
                connection_.id(), id_);
      stream_encoder_.encodeHeaders(response_headers, true);
      return;
    }

    // Limitation: at most one response can be sent on a stream at a time.
    assert(nullptr == delay_timer_.get());
    if (delay_timer_.get()) {
      return;
    }

    response_headers_ = std::make_unique<Http::HeaderMapImpl>(response_headers);
    delay_timer_ = connection_.dispatcher().createTimer([this, delay]() {
      ENVOY_LOG(debug, "ServerStream({}:{}:{}) sending response headers after {} msec delay",
                connection_.name(), connection_.id(), id_, static_cast<long int>(delay.count()));
      stream_encoder_.encodeHeaders(*response_headers_, true);
      delay_timer_->disableTimer();
      delay_timer_ = nullptr;
      response_headers_ = nullptr;
    });
    delay_timer_->enableTimer(delay);
  }

  void sendGrpcResponse(Grpc::Status::GrpcStatus status, const ProtobufWkt::Message& message,
                        const std::chrono::milliseconds delay) override {
    // Limitation: at most one response can be sent on a stream at a time.
    assert(nullptr == delay_timer_.get());
    if (delay_timer_.get()) {
      return;
    }

    response_status_ = status;
    response_body_ = Grpc::Common::serializeToGrpcFrame(message);
    Event::TimerCb send_grpc_response = [this, delay]() {
      ENVOY_LOG(debug, "ServerStream({}:{}:{}) sending gRPC response after {} msec delay",
                connection_.name(), connection_.id(), id_, static_cast<long int>(delay.count()));
      stream_encoder_.encodeHeaders(Http::TestHeaderMapImpl{{":status", "200"}}, false);
      stream_encoder_.encodeData(*response_body_, false);
      stream_encoder_.encodeTrailers(Http::TestHeaderMapImpl{
          {"grpc-status", std::to_string(static_cast<uint32_t>(response_status_))}});
    };

    if (delay <= std::chrono::milliseconds(0)) {
      send_grpc_response();
      return;
    }

    delay_timer_ = connection_.dispatcher().createTimer([this, send_grpc_response]() {
      send_grpc_response();
      delay_timer_->disableTimer();
    });

    delay_timer_->enableTimer(delay);
  }

  //
  // Http::StreamDecoder
  //

  void decode100ContinueHeaders(Http::HeaderMapPtr&&) override {
    ENVOY_LOG(error, "ServerStream({}:{}:{}) got continue headers?!?!", connection_.name(),
              connection_.id(), id_);
  }

  /**
   * Called with decoded headers, optionally indicating end of stream.
   * @param headers supplies the decoded headers map that is moved into the
   * callee.
   * @param end_stream supplies whether this is a header only request/response.
   */
  void decodeHeaders(Http::HeaderMapPtr&& headers, bool end_stream) override {
    ENVOY_LOG(debug, "ServerStream({}:{}:{}) got request headers", connection_.name(),
              connection_.id(), id_);

    request_headers_ = std::move(headers);

    if (end_stream) {
      onEndStream();
      // stream is now destroyed
    }
  }

  void decodeData(Buffer::Instance&, bool end_stream) override {
    ENVOY_LOG(debug, "ServerStream({}:{}:{}) got request body data", connection_.name(),
              connection_.id(), id_);

    if (end_stream) {
      onEndStream();
      // stream is now destroyed
    }
  }

  void decodeTrailers(Http::HeaderMapPtr&&) override {
    ENVOY_LOG(trace, "ServerStream({}:{}:{}) got request trailers", connection_.name(),
              connection_.id(), id_);
    onEndStream();
    // stream is now destroyed
  }

  void decodeMetadata(Http::MetadataMapPtr&&) override {
    ENVOY_LOG(trace, "ServerStream({}:{}):{} got metadata", connection_.name(), connection_.id(),
              id_);
  }

  //
  // Http::StreamCallbacks
  //

  void onResetStream(Http::StreamResetReason reason, absl::string_view) override {
    switch (reason) {
    case Http::StreamResetReason::LocalReset:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) was locally reset", connection_.name(),
                connection_.id(), id_);
      break;
    case Http::StreamResetReason::LocalRefusedStreamReset:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) refused local stream reset", connection_.name(),
                connection_.id(), id_);
      break;
    case Http::StreamResetReason::RemoteReset:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) was remotely reset", connection_.name(),
                connection_.id(), id_);
      break;
    case Http::StreamResetReason::RemoteRefusedStreamReset:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) refused remote stream reset", connection_.name(),
                connection_.id(), id_);
      break;
    case Http::StreamResetReason::ConnectionFailure:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) reseet due to initial connection failure",
                connection_.name(), connection_.id(), id_);
      break;
    case Http::StreamResetReason::ConnectionTermination:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) reset due to underlying connection reset",
                connection_.name(), connection_.id(), id_);
      break;
    case Http::StreamResetReason::Overflow:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) reset due to resource overflow", connection_.name(),
                connection_.id(), id_);
      break;
    default:
      ENVOY_LOG(trace, "ServerStream({}:{}:{}) reset due to unknown reason", connection_.name(),
                connection_.id(), id_);
      break;
    }
  }

  void onAboveWriteBufferHighWatermark() override {
    ENVOY_LOG(trace, "ServerStream({}:{}:{}) above write buffer high watermark", connection_.name(),
              connection_.id(), id_);
  }

  void onBelowWriteBufferLowWatermark() override {
    ENVOY_LOG(trace, "ServerStream({}:{}:{}) below write buffer low watermark", connection_.name(),
              connection_.id(), id_);
  }

private:
  void onEndStream() {
    ENVOY_LOG(debug, "ServerStream({}:{}:{}) complete", connection_.name(), connection_.id(), id_);
    request_callback_(connection_, *this, std::move(request_headers_));

    connection_.removeStream(id_);
    // This stream is now destroyed
  }

  uint32_t id_;
  ServerConnection& connection_;
  Http::HeaderMapPtr request_headers_{nullptr};
  Http::HeaderMapPtr response_headers_{nullptr};
  Buffer::InstancePtr response_body_{nullptr};
  Grpc::Status::GrpcStatus response_status_{Grpc::Status::Ok};
  ServerRequestCallback& request_callback_;
  Http::StreamEncoder& stream_encoder_;
  Event::TimerPtr delay_timer_{nullptr};
};

ServerConnection::ServerConnection(const std::string& name, uint32_t id,
                                   ServerRequestCallback& request_callback,
                                   ServerCloseCallback& close_callback,
                                   Network::Connection& network_connection,
                                   Event::Dispatcher& dispatcher, Http::CodecClient::Type http_type,
                                   Stats::Scope& scope)
    : name_(name), id_(id), network_connection_(network_connection), dispatcher_(dispatcher),
      request_callback_(request_callback), close_callback_(close_callback) {
  constexpr uint32_t max_request_headers_kb = 2U;
  constexpr uint32_t max_request_headers_count = 100;

  switch (http_type) {
  case Http::CodecClient::Type::HTTP1:
    http_connection_ = std::make_unique<Http::Http1::ServerConnectionImpl>(
        network_connection, scope, *this, Http::Http1Settings(), max_request_headers_kb,
        max_request_headers_count);
    break;
  case Http::CodecClient::Type::HTTP2: {
    Http::Http2Settings settings;
    settings.allow_connect_ = true;
    settings.allow_metadata_ = true;
    http_connection_ = std::make_unique<Http::Http2::ServerConnectionImpl>(
        network_connection, *this, scope, settings, max_request_headers_kb,
        max_request_headers_count);
  } break;
  default:
    ENVOY_LOG(error,
              "ServerConnection({}:{}) doesn't support http type %d, "
              "defaulting to HTTP1",
              name_, id_, static_cast<int>(http_type) + 1);
    http_connection_ = std::make_unique<Http::Http1::ServerConnectionImpl>(
        network_connection, scope, *this, Http::Http1Settings(), max_request_headers_kb,
        max_request_headers_count);
    break;
  }
}

ServerConnection::~ServerConnection() {
  ENVOY_LOG(trace, "ServerConnection({}:{}) destroyed", name_, id_);
}

const std::string& ServerConnection::name() const { return name_; }

uint32_t ServerConnection::id() const { return id_; }

Network::Connection& ServerConnection::networkConnection() { return network_connection_; }

const Network::Connection& ServerConnection::networkConnection() const {
  return network_connection_;
}

Http::ServerConnection& ServerConnection::httpConnection() { return *http_connection_; }

const Http::ServerConnection& ServerConnection::httpConnection() const { return *http_connection_; }

Event::Dispatcher& ServerConnection::dispatcher() { return dispatcher_; }

Network::FilterStatus ServerConnection::onData(Buffer::Instance& data, bool end_stream) {
  ENVOY_LOG(trace, "ServerConnection({}:{}) got data", name_, id_);

  try {
    http_connection_->dispatch(data);
  } catch (const Http::CodecProtocolException& e) {
    ENVOY_LOG(error, "ServerConnection({}:{}) received the wrong protocol: {}", name_, id_,
              e.what());
    network_connection_.close(Network::ConnectionCloseType::NoFlush);
    return Network::FilterStatus::StopIteration;
  }

  if (end_stream) {
    ENVOY_LOG(error, "ServerConnection({}:{}) got end stream", name_, id_);
  }

  return Network::FilterStatus::StopIteration;
}

Network::FilterStatus ServerConnection::onNewConnection() {
  ENVOY_LOG(trace, "ServerConnection({}:{}) onNewConnection", name_, id_);
  return Network::FilterStatus::Continue;
}

void ServerConnection::initializeReadFilterCallbacks(Network::ReadFilterCallbacks&) {}

Http::StreamDecoder& ServerConnection::newStream(Http::StreamEncoder& stream_encoder, bool) {
  ServerStreamImpl* raw = nullptr;
  uint32_t id = 0U;

  {
    std::lock_guard<std::mutex> guard(streams_lock_);

    id = stream_counter_++;
    auto stream = std::make_unique<ServerStreamImpl>(id, *this, request_callback_, stream_encoder);
    raw = stream.get();
    streams_[id] = std::move(stream);
  }

  ENVOY_LOG(debug, "ServerConnection({}:{}) received new Stream({}:{}:{})", name_, id_, name_, id_,
            id);

  return *raw;
}

void ServerConnection::removeStream(uint32_t stream_id) {
  unsigned long size = 0UL;

  {
    std::lock_guard<std::mutex> guard(streams_lock_);
    streams_.erase(stream_id);
    size = streams_.size();
  }

  if (0 == size) {
    ENVOY_LOG(debug, "ServerConnection({}:{}) is idle", name_, id_);
  }
}

void ServerConnection::onEvent(Network::ConnectionEvent event) {
  switch (event) {
  case Network::ConnectionEvent::RemoteClose:
    ENVOY_LOG(debug, "ServerConnection({}:{}) closed by peer or reset", name_, id_);
    close_callback_(*this, ServerCloseReason::REMOTE_CLOSE);
    return;
  case Network::ConnectionEvent::LocalClose:
    ENVOY_LOG(debug, "ServerConnection({}:{}) closed locally", name_, id_);
    close_callback_(*this, ServerCloseReason::LOCAL_CLOSE);
    return;
  default:
    ENVOY_LOG(error, "ServerConnection({}:{}) got unknown event", name_, id_);
  }
}

void ServerConnection::onAboveWriteBufferHighWatermark() {
  ENVOY_LOG(debug, "ServerConnection({}:{}) above write buffer high watermark", name_, id_);
  http_connection_->onUnderlyingConnectionAboveWriteBufferHighWatermark();
}

void ServerConnection::onBelowWriteBufferLowWatermark() {
  ENVOY_LOG(debug, "ServerConnection({}:{}) below write buffer low watermark", name_, id_);
  http_connection_->onUnderlyingConnectionBelowWriteBufferLowWatermark();
}

void ServerConnection::onGoAway() { ENVOY_LOG(warn, "ServerConnection({}) got go away", name_); }

ServerFilterChain::ServerFilterChain(Network::TransportSocketFactory& transport_socket_factory)
    : transport_socket_factory_(transport_socket_factory) {}

const Network::TransportSocketFactory& ServerFilterChain::transportSocketFactory() const {
  return transport_socket_factory_;
}

const std::vector<Network::FilterFactoryCb>& ServerFilterChain::networkFilterFactories() const {
  return network_filter_factories_;
}

LocalListenSocket::LocalListenSocket(Network::Address::IpVersion ip_version, uint16_t port,
                                     const Network::Socket::OptionsSharedPtr& options,
                                     bool bind_to_port)
    : NetworkListenSocket(loopbackAddress(ip_version, port), options, bind_to_port) {}

ServerCallbackHelper::ServerCallbackHelper(ServerRequestCallback&& request_callback,
                                           ServerAcceptCallback&& accept_callback,
                                           ServerCloseCallback&& close_callback) {
  if (request_callback) {
    request_callback_ = [this, request_callback = std::move(request_callback)](
                            ServerConnection& connection, ServerStream& stream,
                            Http::HeaderMapPtr&& request_headers) {
      ++requests_received_;
      request_callback(connection, stream, std::move(request_headers));
    };
  } else {
    request_callback_ = [this](ServerConnection&, ServerStream& stream, Http::HeaderMapPtr&&) {
      ++requests_received_;
      Http::TestHeaderMapImpl response{{":status", "200"}};
      stream.sendResponseHeaders(response);
    };
  }

  if (accept_callback) {
    accept_callback_ = [this, accept_callback = std::move(accept_callback)](
                           ServerConnection& connection) -> ServerCallbackResult {
      ++accepts_;
      return accept_callback(connection);
    };
  } else {
    accept_callback_ = [this](ServerConnection&) -> ServerCallbackResult {
      ++accepts_;
      return ServerCallbackResult::CONTINUE;
    };
  }

  if (close_callback) {
    close_callback_ = [this, close_callback = std::move(close_callback)](
                          ServerConnection& connection, ServerCloseReason reason) {
      absl::MutexLock lock(&mutex_);

      switch (reason) {
      case ServerCloseReason::REMOTE_CLOSE:
        ++remote_closes_;
        break;
      case ServerCloseReason::LOCAL_CLOSE:
        ++local_closes_;
        break;
      }

      close_callback(connection, reason);
    };
  } else {
    close_callback_ = [this](ServerConnection&, ServerCloseReason reason) {
      absl::MutexLock lock(&mutex_);

      switch (reason) {
      case ServerCloseReason::REMOTE_CLOSE:
        ++remote_closes_;
        break;
      case ServerCloseReason::LOCAL_CLOSE:
        ++local_closes_;
        break;
      }
    };
  }
}

uint32_t ServerCallbackHelper::connectionsAccepted() const { return accepts_; }

uint32_t ServerCallbackHelper::requestsReceived() const { return requests_received_; }

uint32_t ServerCallbackHelper::localCloses() const {
  absl::MutexLock lock(&mutex_);
  return local_closes_;
}

uint32_t ServerCallbackHelper::remoteCloses() const {
  absl::MutexLock lock(&mutex_);
  return remote_closes_;
}

ServerAcceptCallback ServerCallbackHelper::acceptCallback() const { return accept_callback_; }

ServerRequestCallback ServerCallbackHelper::requestCallback() const { return request_callback_; }

ServerCloseCallback ServerCallbackHelper::closeCallback() const { return close_callback_; }

void ServerCallbackHelper::wait(uint32_t connections_closed) {
  auto constraints = [connections_closed, this]() {
    return connections_closed <= local_closes_ + remote_closes_;
  };

  absl::MutexLock lock(&mutex_);
  mutex_.Await(absl::Condition(&constraints));
}

void ServerCallbackHelper::wait() {
  auto constraints = [this]() { return accepts_ <= local_closes_ + remote_closes_; };

  absl::MutexLock lock(&mutex_);
  mutex_.Await(absl::Condition(&constraints));
}

Server::Server(const std::string& name, Network::Socket& listening_socket,
               Network::TransportSocketFactory& transport_socket_factory,
               Http::CodecClient::Type http_type)
    : name_(name), stats_(), time_system_(),
      api_(Thread::threadFactoryForTest(), stats_, time_system_, Filesystem::fileSystemForTest()),
      dispatcher_(api_.allocateDispatcher()),
      connection_handler_(new Envoy::Server::ConnectionHandlerImpl(*dispatcher_, "stress_server")),
      thread_(nullptr), listening_socket_(listening_socket),
      server_filter_chain_(transport_socket_factory), http_type_(http_type) {}

Server::~Server() { stop(); }

void Server::start(ServerAcceptCallback&& accept_callback, ServerRequestCallback&& request_callback,
                   ServerCloseCallback&& close_callback) {
  accept_callback_ = std::move(accept_callback);
  request_callback_ = std::move(request_callback);
  close_callback_ = std::move(close_callback);
  std::promise<bool> promise;

  thread_ = api_.threadFactory().createThread([this, &promise]() {
    is_running = true;
    ENVOY_LOG(debug, "Server({}) started", name_.c_str());
    connection_handler_->addListener(*this);

    promise.set_value(true); // do not use promise again after this
    while (is_running) {
      dispatcher_->run(Event::Dispatcher::RunType::NonBlock);
    }

    ENVOY_LOG(debug, "Server({}) stopped", name_.c_str());

    connection_handler_.reset();
  });

  promise.get_future().get();
}

void Server::start(ServerCallbackHelper& helper) {
  start(helper.acceptCallback(), helper.requestCallback(), helper.closeCallback());
}

void Server::stop() {
  is_running = false;

  if (thread_) {
    thread_->join();
    thread_ = nullptr;
  }
}

void Server::stopAcceptingConnections() {
  ENVOY_LOG(debug, "Server({}) stopped accepting connections", name_);
  connection_handler_->disableListeners();
}

void Server::startAcceptingConnections() {
  ENVOY_LOG(debug, "Server({}) started accepting connections", name_);
  connection_handler_->enableListeners();
}

const Stats::Store& Server::statsStore() const { return stats_; }

void Server::setPerConnectionBufferLimitBytes(uint32_t limit) {
  connection_buffer_limit_bytes_ = limit;
}

//
// Network::ListenerConfig
//

Network::FilterChainManager& Server::filterChainManager() { return *this; }

Network::FilterChainFactory& Server::filterChainFactory() { return *this; }

Network::Socket& Server::socket() { return listening_socket_; }

const Network::Socket& Server::socket() const { return listening_socket_; }

bool Server::bindToPort() { return true; }

bool Server::handOffRestoredDestinationConnections() const { return false; }

uint32_t Server::perConnectionBufferLimitBytes() const { return connection_buffer_limit_bytes_; }

std::chrono::milliseconds Server::listenerFiltersTimeout() const {
  return std::chrono::milliseconds(0);
}

bool Server::continueOnListenerFiltersTimeout() const { return false; }

Stats::Scope& Server::listenerScope() { return stats_; }

uint64_t Server::listenerTag() const { return 0; }

const std::string& Server::name() const { return name_; }

const Network::ActiveUdpListenerFactory* Server::udpListenerFactory() { return nullptr; }

const Network::FilterChain* Server::findFilterChain(const Network::ConnectionSocket&) const {
  return &server_filter_chain_;
}

bool Server::createNetworkFilterChain(Network::Connection& network_connection,
                                      const std::vector<Network::FilterFactoryCb>&) {
  uint32_t id = connection_counter_++;
  ENVOY_LOG(debug, "Server({}) accepted new Connection({}:{})", name_, name_, id);

  ServerConnectionSharedPtr connection =
      std::make_shared<ServerConnection>(name_, id, request_callback_, close_callback_,
                                         network_connection, *dispatcher_, http_type_, stats_);
  network_connection.addReadFilter(connection);
  network_connection.addConnectionCallbacks(*connection);

  return !(ServerCallbackResult::CLOSE == accept_callback_(*connection));
}

bool Server::createListenerFilterChain(Network::ListenerFilterManager&) { return true; }

bool Server::createUdpListenerFilterChain(Network::UdpListenerFilterManager&,
                                          Network::UdpReadFilterCallbacks&) {
  return true;
}

ClusterHelper::ClusterHelper(const std::string& name) : name_{name} {}

ClusterHelper& ClusterHelper::addServer(ServerCallbackHelperPtr&& server_callback) {
  server_callback_helpers_.push_back(std::move(server_callback));
  return *this;
}

const std::vector<ServerCallbackHelperPtr>& ClusterHelper::servers() const {
  return server_callback_helpers_;
}

std::vector<ServerCallbackHelperPtr>& ClusterHelper::servers() { return server_callback_helpers_; }

uint32_t ClusterHelper::connectionsAccepted() const {
  uint32_t total = 0U;

  for (const auto& server_callback_helper : server_callback_helpers_) {
    total += server_callback_helper->connectionsAccepted();
  }

  return total;
}

uint32_t ClusterHelper::requestsReceived() const {
  uint32_t total = 0U;

  for (const auto& server_callback_helper : server_callback_helpers_) {
    total += server_callback_helper->requestsReceived();
  }

  return total;
}

uint32_t ClusterHelper::localCloses() const {
  uint32_t total = 0U;

  for (const auto& server_callback_helper : server_callback_helpers_) {
    total += server_callback_helper->localCloses();
  }

  return total;
}

uint32_t ClusterHelper::remoteCloses() const {
  uint32_t total = 0U;

  for (const auto& server_callback_helper : server_callback_helpers_) {
    total += server_callback_helper->remoteCloses();
  }

  return total;
}

void ClusterHelper::wait() {
  for (auto& server_callback_helper : server_callback_helpers_) {
    server_callback_helper->wait();
  }
}

} // namespace Stress
} // namespace Envoy
