#pragma once

#include <cstdint>
#include <string>

#include "envoy/network/connection.h"
#include "envoy/network/transport_socket.h"
#include "envoy/secret/secret_callbacks.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "common/common/logger.h"

#include "extensions/transport_sockets/tls/context_impl.h"
#include "extensions/transport_sockets/tls/utility.h"

#include "absl/synchronization/mutex.h"
#include "absl/types/optional.h"
#include "openssl/ssl.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {

// clang-format off
#define ALL_SSL_SOCKET_FACTORY_STATS(COUNTER)                                 \
  COUNTER(ssl_context_update_by_sds)                                          \
  COUNTER(upstream_context_secrets_not_ready)                                 \
  COUNTER(downstream_context_secrets_not_ready)
// clang-format on

/**
 * Wrapper struct for SSL socket factory stats. @see stats_macros.h
 */
struct SslSocketFactoryStats {
  ALL_SSL_SOCKET_FACTORY_STATS(GENERATE_COUNTER_STRUCT)
};

enum class InitialState { Client, Server };

class SslSocket : public Network::TransportSocket,
                  public Envoy::Ssl::ConnectionInfo,
                  protected Logger::Loggable<Logger::Id::connection> {
public:
  SslSocket(Envoy::Ssl::ContextSharedPtr ctx, InitialState state,
            const Network::TransportSocketOptionsSharedPtr& transport_socket_options);

  // Ssl::ConnectionInfo
  bool peerCertificatePresented() const override;
  std::vector<std::string> uriSanLocalCertificate() const override;
  const std::string& sha256PeerCertificateDigest() const override;
  std::string serialNumberPeerCertificate() const override;
  std::string issuerPeerCertificate() const override;
  std::string subjectPeerCertificate() const override;
  std::string subjectLocalCertificate() const override;
  std::vector<std::string> uriSanPeerCertificate() const override;
  const std::string& urlEncodedPemEncodedPeerCertificate() const override;
  const std::string& urlEncodedPemEncodedPeerCertificateChain() const override;
  std::vector<std::string> dnsSansPeerCertificate() const override;
  std::vector<std::string> dnsSansLocalCertificate() const override;
  absl::optional<SystemTime> validFromPeerCertificate() const override;
  absl::optional<SystemTime> expirationPeerCertificate() const override;
  std::string sessionId() const override;
  uint16_t ciphersuiteId() const override;
  std::string ciphersuiteString() const override;
  std::string tlsVersion() const override;

  // Network::TransportSocket
  void setTransportSocketCallbacks(Network::TransportSocketCallbacks& callbacks) override;
  std::string protocol() const override;
  absl::string_view failureReason() const override;
  bool canFlushClose() override { return handshake_complete_; }
  void closeSocket(Network::ConnectionEvent close_type) override;
  Network::IoResult doRead(Buffer::Instance& read_buffer) override;
  Network::IoResult doWrite(Buffer::Instance& write_buffer, bool end_stream) override;
  void onConnected() override;
  const Ssl::ConnectionInfo* ssl() const override { return this; }

  SSL* rawSslForTest() const { return ssl_.get(); }

private:
  struct ReadResult {
    bool commit_slice_{};
    absl::optional<int> error_;
  };
  ReadResult sslReadIntoSlice(Buffer::RawSlice& slice);

  Network::PostIoAction doHandshake();
  void drainErrorQueue();
  void shutdownSsl();

  const Network::TransportSocketOptionsSharedPtr transport_socket_options_;
  Network::TransportSocketCallbacks* callbacks_{};
  ContextImplSharedPtr ctx_;
  bssl::UniquePtr<SSL> ssl_;
  bool handshake_complete_{};
  bool shutdown_sent_{};
  uint64_t bytes_to_retry_{};
  std::string failure_reason_;
  mutable std::string cached_sha_256_peer_certificate_digest_;
  mutable std::string cached_url_encoded_pem_encoded_peer_certificate_;
  mutable std::string cached_url_encoded_pem_encoded_peer_cert_chain_;
};

class ClientSslSocketFactory : public Network::TransportSocketFactory,
                               public Secret::SecretCallbacks,
                               Logger::Loggable<Logger::Id::config> {
public:
  ClientSslSocketFactory(Envoy::Ssl::ClientContextConfigPtr config,
                         Envoy::Ssl::ContextManager& manager, Stats::Scope& stats_scope);

  Network::TransportSocketPtr
  createTransportSocket(Network::TransportSocketOptionsSharedPtr options) const override;
  bool implementsSecureTransport() const override;

  // Secret::SecretCallbacks
  void onAddOrUpdateSecret() override;

private:
  Envoy::Ssl::ContextManager& manager_;
  Stats::Scope& stats_scope_;
  SslSocketFactoryStats stats_;
  Envoy::Ssl::ClientContextConfigPtr config_;
  mutable absl::Mutex ssl_ctx_mu_;
  Envoy::Ssl::ClientContextSharedPtr ssl_ctx_ GUARDED_BY(ssl_ctx_mu_);
};

class ServerSslSocketFactory : public Network::TransportSocketFactory,
                               public Secret::SecretCallbacks,
                               Logger::Loggable<Logger::Id::config> {
public:
  ServerSslSocketFactory(Envoy::Ssl::ServerContextConfigPtr config,
                         Envoy::Ssl::ContextManager& manager, Stats::Scope& stats_scope,
                         const std::vector<std::string>& server_names);

  Network::TransportSocketPtr
  createTransportSocket(Network::TransportSocketOptionsSharedPtr options) const override;
  bool implementsSecureTransport() const override;

  // Secret::SecretCallbacks
  void onAddOrUpdateSecret() override;

private:
  Ssl::ContextManager& manager_;
  Stats::Scope& stats_scope_;
  SslSocketFactoryStats stats_;
  Envoy::Ssl::ServerContextConfigPtr config_;
  const std::vector<std::string> server_names_;
  mutable absl::Mutex ssl_ctx_mu_;
  Envoy::Ssl::ServerContextSharedPtr ssl_ctx_ GUARDED_BY(ssl_ctx_mu_);
};

} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
