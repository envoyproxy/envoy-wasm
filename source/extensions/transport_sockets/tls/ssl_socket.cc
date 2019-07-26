#include "extensions/transport_sockets/tls/ssl_socket.h"

#include "envoy/stats/scope.h"

#include "common/common/assert.h"
#include "common/common/empty_string.h"
#include "common/common/hex.h"
#include "common/http/headers.h"

#include "extensions/transport_sockets/tls/utility.h"

#include "absl/strings/str_replace.h"
#include "openssl/err.h"
#include "openssl/x509v3.h"

using Envoy::Network::PostIoAction;

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {

namespace {

constexpr absl::string_view NotReadyReason{"TLS error: Secret is not supplied by SDS"};

// This SslSocket will be used when SSL secret is not fetched from SDS server.
class NotReadySslSocket : public Network::TransportSocket {
public:
  // Network::TransportSocket
  void setTransportSocketCallbacks(Network::TransportSocketCallbacks&) override {}
  std::string protocol() const override { return EMPTY_STRING; }
  absl::string_view failureReason() const override { return NotReadyReason; }
  bool canFlushClose() override { return true; }
  void closeSocket(Network::ConnectionEvent) override {}
  Network::IoResult doRead(Buffer::Instance&) override { return {PostIoAction::Close, 0, false}; }
  Network::IoResult doWrite(Buffer::Instance&, bool) override {
    return {PostIoAction::Close, 0, false};
  }
  void onConnected() override {}
  const Ssl::ConnectionInfo* ssl() const override { return nullptr; }
};
} // namespace

SslSocket::SslSocket(Envoy::Ssl::ContextSharedPtr ctx, InitialState state,
                     const Network::TransportSocketOptionsSharedPtr& transport_socket_options)
    : transport_socket_options_(transport_socket_options),
      ctx_(std::dynamic_pointer_cast<ContextImpl>(ctx)),
      ssl_(ctx_->newSsl(transport_socket_options_.get())) {
  if (state == InitialState::Client) {
    SSL_set_connect_state(ssl_.get());
  } else {
    ASSERT(state == InitialState::Server);
    SSL_set_accept_state(ssl_.get());
  }
}

void SslSocket::setTransportSocketCallbacks(Network::TransportSocketCallbacks& callbacks) {
  ASSERT(!callbacks_);
  callbacks_ = &callbacks;

  BIO* bio = BIO_new_socket(callbacks_->ioHandle().fd(), 0);
  SSL_set_bio(ssl_.get(), bio, bio);
}

SslSocket::ReadResult SslSocket::sslReadIntoSlice(Buffer::RawSlice& slice) {
  ReadResult result;
  uint8_t* mem = static_cast<uint8_t*>(slice.mem_);
  size_t remaining = slice.len_;
  while (remaining > 0) {
    int rc = SSL_read(ssl_.get(), mem, remaining);
    ENVOY_CONN_LOG(trace, "ssl read returns: {}", callbacks_->connection(), rc);
    if (rc > 0) {
      ASSERT(static_cast<size_t>(rc) <= remaining);
      mem += rc;
      remaining -= rc;
      result.commit_slice_ = true;
    } else {
      result.error_ = absl::make_optional<int>(rc);
      break;
    }
  }

  if (result.commit_slice_) {
    slice.len_ -= remaining;
  }
  return result;
}

Network::IoResult SslSocket::doRead(Buffer::Instance& read_buffer) {
  if (!handshake_complete_) {
    PostIoAction action = doHandshake();
    if (action == PostIoAction::Close || !handshake_complete_) {
      // end_stream is false because either a hard error occurred (action == Close) or
      // the handshake isn't complete, so a half-close cannot occur yet.
      return {action, 0, false};
    }
  }

  bool keep_reading = true;
  bool end_stream = false;
  PostIoAction action = PostIoAction::KeepOpen;
  uint64_t bytes_read = 0;
  while (keep_reading) {
    // We use 2 slices here so that we can use the remainder of an existing buffer chain element
    // if there is extra space. 16K read is arbitrary and can be tuned later.
    Buffer::RawSlice slices[2];
    uint64_t slices_to_commit = 0;
    uint64_t num_slices = read_buffer.reserve(16384, slices, 2);
    for (uint64_t i = 0; i < num_slices; i++) {
      auto result = sslReadIntoSlice(slices[i]);
      if (result.commit_slice_) {
        slices_to_commit++;
        bytes_read += slices[i].len_;
      }
      if (result.error_.has_value()) {
        keep_reading = false;
        int err = SSL_get_error(ssl_.get(), result.error_.value());
        switch (err) {
        case SSL_ERROR_WANT_READ:
          break;
        case SSL_ERROR_ZERO_RETURN:
          end_stream = true;
          break;
        case SSL_ERROR_WANT_WRITE:
        // Renegotiation has started. We don't handle renegotiation so just fall through.
        default:
          drainErrorQueue();
          action = PostIoAction::Close;
          break;
        }

        break;
      }
    }

    if (slices_to_commit > 0) {
      read_buffer.commit(slices, slices_to_commit);
      if (callbacks_->shouldDrainReadBuffer()) {
        callbacks_->setReadBufferReady();
        keep_reading = false;
      }
    }
  }

  ENVOY_CONN_LOG(trace, "ssl read {} bytes into {} slices", callbacks_->connection(), bytes_read,
                 read_buffer.getRawSlices(nullptr, 0));

  return {action, bytes_read, end_stream};
}

PostIoAction SslSocket::doHandshake() {
  ASSERT(!handshake_complete_);
  int rc = SSL_do_handshake(ssl_.get());
  if (rc == 1) {
    ENVOY_CONN_LOG(debug, "handshake complete", callbacks_->connection());
    handshake_complete_ = true;
    ctx_->logHandshake(ssl_.get());
    callbacks_->raiseEvent(Network::ConnectionEvent::Connected);

    // It's possible that we closed during the handshake callback.
    return callbacks_->connection().state() == Network::Connection::State::Open
               ? PostIoAction::KeepOpen
               : PostIoAction::Close;
  } else {
    int err = SSL_get_error(ssl_.get(), rc);
    ENVOY_CONN_LOG(debug, "handshake error: {}", callbacks_->connection(), err);
    switch (err) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      return PostIoAction::KeepOpen;
    default:
      drainErrorQueue();
      return PostIoAction::Close;
    }
  }
}

void SslSocket::drainErrorQueue() {
  bool saw_error = false;
  bool saw_counted_error = false;
  while (uint64_t err = ERR_get_error()) {
    if (ERR_GET_LIB(err) == ERR_LIB_SSL) {
      if (ERR_GET_REASON(err) == SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE) {
        ctx_->stats().fail_verify_no_cert_.inc();
        saw_counted_error = true;
      } else if (ERR_GET_REASON(err) == SSL_R_CERTIFICATE_VERIFY_FAILED) {
        saw_counted_error = true;
      }
    }
    saw_error = true;

    if (failure_reason_.empty()) {
      failure_reason_ = "TLS error:";
    }
    failure_reason_.append(absl::StrCat(" ", err, ":", ERR_lib_error_string(err), ":",
                                        ERR_func_error_string(err), ":",
                                        ERR_reason_error_string(err)));
  }
  ENVOY_CONN_LOG(debug, "{}", callbacks_->connection(), failure_reason_);
  if (saw_error && !saw_counted_error) {
    ctx_->stats().connection_error_.inc();
  }
}

Network::IoResult SslSocket::doWrite(Buffer::Instance& write_buffer, bool end_stream) {
  ASSERT(!shutdown_sent_ || write_buffer.length() == 0);
  if (!handshake_complete_) {
    PostIoAction action = doHandshake();
    if (action == PostIoAction::Close || !handshake_complete_) {
      return {action, 0, false};
    }
  }

  uint64_t bytes_to_write;
  if (bytes_to_retry_) {
    bytes_to_write = bytes_to_retry_;
    bytes_to_retry_ = 0;
  } else {
    bytes_to_write = std::min(write_buffer.length(), static_cast<uint64_t>(16384));
  }

  uint64_t total_bytes_written = 0;
  while (bytes_to_write > 0) {
    // TODO(mattklein123): As it relates to our fairness efforts, we might want to limit the number
    // of iterations of this loop, either by pure iterations, bytes written, etc.

    // SSL_write() requires that if a previous call returns SSL_ERROR_WANT_WRITE, we need to call
    // it again with the same parameters. This is done by tracking last write size, but not write
    // data, since linearize() will return the same undrained data anyway.
    ASSERT(bytes_to_write <= write_buffer.length());
    int rc = SSL_write(ssl_.get(), write_buffer.linearize(bytes_to_write), bytes_to_write);
    ENVOY_CONN_LOG(trace, "ssl write returns: {}", callbacks_->connection(), rc);
    if (rc > 0) {
      ASSERT(rc == static_cast<int>(bytes_to_write));
      total_bytes_written += rc;
      write_buffer.drain(rc);
      bytes_to_write = std::min(write_buffer.length(), static_cast<uint64_t>(16384));
    } else {
      int err = SSL_get_error(ssl_.get(), rc);
      switch (err) {
      case SSL_ERROR_WANT_WRITE:
        bytes_to_retry_ = bytes_to_write;
        break;
      case SSL_ERROR_WANT_READ:
      // Renegotiation has started. We don't handle renegotiation so just fall through.
      default:
        drainErrorQueue();
        return {PostIoAction::Close, total_bytes_written, false};
      }

      break;
    }
  }

  if (write_buffer.length() == 0 && end_stream) {
    shutdownSsl();
  }

  return {PostIoAction::KeepOpen, total_bytes_written, false};
}

void SslSocket::onConnected() { ASSERT(!handshake_complete_); }

void SslSocket::shutdownSsl() {
  ASSERT(handshake_complete_);
  if (!shutdown_sent_ && callbacks_->connection().state() != Network::Connection::State::Closed) {
    int rc = SSL_shutdown(ssl_.get());
    ENVOY_CONN_LOG(debug, "SSL shutdown: rc={}", callbacks_->connection(), rc);
    drainErrorQueue();
    shutdown_sent_ = true;
  }
}

bool SslSocket::peerCertificatePresented() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  return cert != nullptr;
}

std::vector<std::string> SslSocket::uriSanLocalCertificate() const {
  // The cert object is not owned.
  X509* cert = SSL_get_certificate(ssl_.get());
  if (!cert) {
    return {};
  }
  return Utility::getSubjectAltNames(*cert, GEN_URI);
}

std::vector<std::string> SslSocket::dnsSansLocalCertificate() const {
  X509* cert = SSL_get_certificate(ssl_.get());
  if (!cert) {
    return {};
  }
  return Utility::getSubjectAltNames(*cert, GEN_DNS);
}

const std::string& SslSocket::sha256PeerCertificateDigest() const {
  if (!cached_sha_256_peer_certificate_digest_.empty()) {
    return cached_sha_256_peer_certificate_digest_;
  }
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    ASSERT(cached_sha_256_peer_certificate_digest_.empty());
    return cached_sha_256_peer_certificate_digest_;
  }

  std::vector<uint8_t> computed_hash(SHA256_DIGEST_LENGTH);
  unsigned int n;
  X509_digest(cert.get(), EVP_sha256(), computed_hash.data(), &n);
  RELEASE_ASSERT(n == computed_hash.size(), "");
  cached_sha_256_peer_certificate_digest_ = Hex::encode(computed_hash);
  return cached_sha_256_peer_certificate_digest_;
}

const std::string& SslSocket::urlEncodedPemEncodedPeerCertificate() const {
  if (!cached_url_encoded_pem_encoded_peer_certificate_.empty()) {
    return cached_url_encoded_pem_encoded_peer_certificate_;
  }
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    ASSERT(cached_url_encoded_pem_encoded_peer_certificate_.empty());
    return cached_url_encoded_pem_encoded_peer_certificate_;
  }

  bssl::UniquePtr<BIO> buf(BIO_new(BIO_s_mem()));
  RELEASE_ASSERT(buf != nullptr, "");
  RELEASE_ASSERT(PEM_write_bio_X509(buf.get(), cert.get()) == 1, "");
  const uint8_t* output;
  size_t length;
  RELEASE_ASSERT(BIO_mem_contents(buf.get(), &output, &length) == 1, "");
  absl::string_view pem(reinterpret_cast<const char*>(output), length);
  cached_url_encoded_pem_encoded_peer_certificate_ = absl::StrReplaceAll(
      pem, {{"\n", "%0A"}, {" ", "%20"}, {"+", "%2B"}, {"/", "%2F"}, {"=", "%3D"}});
  return cached_url_encoded_pem_encoded_peer_certificate_;
}

const std::string& SslSocket::urlEncodedPemEncodedPeerCertificateChain() const {
  if (!cached_url_encoded_pem_encoded_peer_cert_chain_.empty()) {
    return cached_url_encoded_pem_encoded_peer_cert_chain_;
  }

  STACK_OF(X509)* cert_chain = SSL_get_peer_full_cert_chain(ssl_.get());
  if (cert_chain == nullptr) {
    ASSERT(cached_url_encoded_pem_encoded_peer_cert_chain_.empty());
    return cached_url_encoded_pem_encoded_peer_cert_chain_;
  }

  for (uint64_t i = 0; i < sk_X509_num(cert_chain); i++) {
    X509* cert = sk_X509_value(cert_chain, i);

    bssl::UniquePtr<BIO> buf(BIO_new(BIO_s_mem()));
    RELEASE_ASSERT(buf != nullptr, "");
    RELEASE_ASSERT(PEM_write_bio_X509(buf.get(), cert) == 1, "");
    const uint8_t* output;
    size_t length;
    RELEASE_ASSERT(BIO_mem_contents(buf.get(), &output, &length) == 1, "");

    absl::string_view pem(reinterpret_cast<const char*>(output), length);
    cached_url_encoded_pem_encoded_peer_cert_chain_ = absl::StrCat(
        cached_url_encoded_pem_encoded_peer_cert_chain_,
        absl::StrReplaceAll(
            pem, {{"\n", "%0A"}, {" ", "%20"}, {"+", "%2B"}, {"/", "%2F"}, {"=", "%3D"}}));
  }
  return cached_url_encoded_pem_encoded_peer_cert_chain_;
}

std::vector<std::string> SslSocket::uriSanPeerCertificate() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    return {};
  }
  return Utility::getSubjectAltNames(*cert, GEN_URI);
}

std::vector<std::string> SslSocket::dnsSansPeerCertificate() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    return {};
  }
  return Utility::getSubjectAltNames(*cert, GEN_DNS);
}

void SslSocket::closeSocket(Network::ConnectionEvent) {
  // Attempt to send a shutdown before closing the socket. It's possible this won't go out if
  // there is no room on the socket. We can extend the state machine to handle this at some point
  // if needed.
  if (handshake_complete_) {
    shutdownSsl();
  }
}

std::string SslSocket::protocol() const {
  const unsigned char* proto;
  unsigned int proto_len;
  SSL_get0_alpn_selected(ssl_.get(), &proto, &proto_len);
  return std::string(reinterpret_cast<const char*>(proto), proto_len);
}

uint16_t SslSocket::ciphersuiteId() const {
  const SSL_CIPHER* cipher = SSL_get_current_cipher(ssl_.get());
  if (cipher == nullptr) {
    return 0xffff;
  }

  // From the OpenSSL docs:
  //    SSL_CIPHER_get_id returns |cipher|'s id. It may be cast to a |uint16_t| to
  //    get the cipher suite value.
  return static_cast<uint16_t>(SSL_CIPHER_get_id(cipher));
}

std::string SslSocket::ciphersuiteString() const {
  const SSL_CIPHER* cipher = SSL_get_current_cipher(ssl_.get());
  if (cipher == nullptr) {
    return std::string();
  }

  return std::string(SSL_CIPHER_get_name(cipher));
}

std::string SslSocket::tlsVersion() const { return std::string(SSL_get_version(ssl_.get())); }

absl::string_view SslSocket::failureReason() const { return failure_reason_; }

std::string SslSocket::serialNumberPeerCertificate() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    return "";
  }
  return Utility::getSerialNumberFromCertificate(*cert.get());
}

std::string SslSocket::issuerPeerCertificate() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    return "";
  }
  return Utility::getIssuerFromCertificate(*cert);
}

std::string SslSocket::subjectPeerCertificate() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    return "";
  }
  return Utility::getSubjectFromCertificate(*cert);
}

std::string SslSocket::subjectLocalCertificate() const {
  X509* cert = SSL_get_certificate(ssl_.get());
  if (!cert) {
    return "";
  }
  return Utility::getSubjectFromCertificate(*cert);
}

absl::optional<SystemTime> SslSocket::validFromPeerCertificate() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    return absl::nullopt;
  }
  return Utility::getValidFrom(*cert);
}

absl::optional<SystemTime> SslSocket::expirationPeerCertificate() const {
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl_.get()));
  if (!cert) {
    return absl::nullopt;
  }
  return Utility::getExpirationTime(*cert);
}

std::string SslSocket::sessionId() const {
  SSL_SESSION* session = SSL_get_session(ssl_.get());
  if (session == nullptr) {
    return "";
  }

  unsigned int session_id_length = 0;
  const uint8_t* session_id = SSL_SESSION_get_id(session, &session_id_length);
  return Hex::encode(session_id, session_id_length);
}

namespace {
SslSocketFactoryStats generateStats(const std::string& prefix, Stats::Scope& store) {
  return {
      ALL_SSL_SOCKET_FACTORY_STATS(POOL_COUNTER_PREFIX(store, prefix + "_ssl_socket_factory."))};
}
} // namespace

ClientSslSocketFactory::ClientSslSocketFactory(Envoy::Ssl::ClientContextConfigPtr config,
                                               Envoy::Ssl::ContextManager& manager,
                                               Stats::Scope& stats_scope)
    : manager_(manager), stats_scope_(stats_scope), stats_(generateStats("client", stats_scope)),
      config_(std::move(config)),
      ssl_ctx_(manager_.createSslClientContext(stats_scope_, *config_)) {
  config_->setSecretUpdateCallback([this]() { onAddOrUpdateSecret(); });
}

Network::TransportSocketPtr ClientSslSocketFactory::createTransportSocket(
    Network::TransportSocketOptionsSharedPtr transport_socket_options) const {
  // onAddOrUpdateSecret() could be invoked in the middle of checking the existence of ssl_ctx and
  // creating SslSocket using ssl_ctx. Capture ssl_ctx_ into a local variable so that we check and
  // use the same ssl_ctx to create SslSocket.
  Envoy::Ssl::ClientContextSharedPtr ssl_ctx;
  {
    absl::ReaderMutexLock l(&ssl_ctx_mu_);
    ssl_ctx = ssl_ctx_;
  }
  if (ssl_ctx) {
    return std::make_unique<SslSocket>(std::move(ssl_ctx), InitialState::Client,
                                       transport_socket_options);
  } else {
    ENVOY_LOG(debug, "Create NotReadySslSocket");
    stats_.upstream_context_secrets_not_ready_.inc();
    return std::make_unique<NotReadySslSocket>();
  }
}

bool ClientSslSocketFactory::implementsSecureTransport() const { return true; }

void ClientSslSocketFactory::onAddOrUpdateSecret() {
  ENVOY_LOG(debug, "Secret is updated.");
  {
    absl::WriterMutexLock l(&ssl_ctx_mu_);
    ssl_ctx_ = manager_.createSslClientContext(stats_scope_, *config_);
  }
  stats_.ssl_context_update_by_sds_.inc();
}

ServerSslSocketFactory::ServerSslSocketFactory(Envoy::Ssl::ServerContextConfigPtr config,
                                               Envoy::Ssl::ContextManager& manager,
                                               Stats::Scope& stats_scope,
                                               const std::vector<std::string>& server_names)
    : manager_(manager), stats_scope_(stats_scope), stats_(generateStats("server", stats_scope)),
      config_(std::move(config)), server_names_(server_names),
      ssl_ctx_(manager_.createSslServerContext(stats_scope_, *config_, server_names_)) {
  config_->setSecretUpdateCallback([this]() { onAddOrUpdateSecret(); });
}

Network::TransportSocketPtr
ServerSslSocketFactory::createTransportSocket(Network::TransportSocketOptionsSharedPtr) const {
  // onAddOrUpdateSecret() could be invoked in the middle of checking the existence of ssl_ctx and
  // creating SslSocket using ssl_ctx. Capture ssl_ctx_ into a local variable so that we check and
  // use the same ssl_ctx to create SslSocket.
  Envoy::Ssl::ServerContextSharedPtr ssl_ctx;
  {
    absl::ReaderMutexLock l(&ssl_ctx_mu_);
    ssl_ctx = ssl_ctx_;
  }
  if (ssl_ctx) {
    return std::make_unique<SslSocket>(std::move(ssl_ctx), InitialState::Server, nullptr);
  } else {
    ENVOY_LOG(debug, "Create NotReadySslSocket");
    stats_.downstream_context_secrets_not_ready_.inc();
    return std::make_unique<NotReadySslSocket>();
  }
}

bool ServerSslSocketFactory::implementsSecureTransport() const { return true; }

void ServerSslSocketFactory::onAddOrUpdateSecret() {
  ENVOY_LOG(debug, "Secret is updated.");
  {
    absl::WriterMutexLock l(&ssl_ctx_mu_);
    ssl_ctx_ = manager_.createSslServerContext(stats_scope_, *config_, server_names_);
  }
  stats_.ssl_context_update_by_sds_.inc();
}

} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
