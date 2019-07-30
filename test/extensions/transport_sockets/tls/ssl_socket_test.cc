#include <cstdint>
#include <memory>
#include <string>

#include "envoy/network/transport_socket.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/empty_string.h"
#include "common/event/dispatcher_impl.h"
#include "common/json/json_loader.h"
#include "common/network/address_impl.h"
#include "common/network/listen_socket_impl.h"
#include "common/network/transport_socket_options_impl.h"
#include "common/network/utility.h"

#include "extensions/filters/listener/tls_inspector/tls_inspector.h"
#include "extensions/transport_sockets/tls/context_config_impl.h"
#include "extensions/transport_sockets/tls/context_impl.h"
#include "extensions/transport_sockets/tls/ssl_socket.h"

#include "test/extensions/transport_sockets/tls/ssl_certs_test.h"
#include "test/extensions/transport_sockets/tls/test_data/no_san_cert_info.h"
#include "test/extensions/transport_sockets/tls/test_data/password_protected_cert_info.h"
#include "test/extensions/transport_sockets/tls/test_data/san_dns2_cert_info.h"
#include "test/extensions/transport_sockets/tls/test_data/san_dns_cert_info.h"
#include "test/extensions/transport_sockets/tls/test_data/san_uri_cert_info.h"
#include "test/extensions/transport_sockets/tls/test_data/selfsigned_ecdsa_p256_cert_info.h"
#include "test/mocks/buffer/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/secret/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/mocks/stats/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/utility.h"

#include "absl/strings/str_replace.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openssl/ssl.h"

using testing::_;
using testing::ContainsRegex;
using testing::DoAll;
using testing::InSequence;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {
namespace {

/**
 * A base class to hold the options for testUtil() and testUtilV2().
 */
class TestUtilOptionsBase {
public:
  const std::vector<std::string>& expectedClientCertUri() const {
    return expected_client_cert_uri_;
  }
  const std::string& expectedServerStats() const { return expected_server_stats_; }
  bool expectSuccess() const { return expect_success_; }
  Network::Address::IpVersion version() const { return version_; }

protected:
  TestUtilOptionsBase(bool expect_success, Network::Address::IpVersion version)
      : expect_success_(expect_success), version_(version) {}

  void setExpectedClientCertUri(const std::string& expected_client_cert_uri) {
    expected_client_cert_uri_ = {expected_client_cert_uri};
  }

  void setExpectedServerStats(const std::string& expected_server_stats) {
    expected_server_stats_ = expected_server_stats;
  }

private:
  const bool expect_success_;
  const Network::Address::IpVersion version_;

  std::string expected_server_stats_;
  std::vector<std::string> expected_client_cert_uri_;
};

/**
 * A class to hold the options for testUtil().
 */
class TestUtilOptions : public TestUtilOptionsBase {
public:
  TestUtilOptions(const std::string& client_ctx_yaml, const std::string& server_ctx_yaml,
                  bool expect_success, Network::Address::IpVersion version)
      : TestUtilOptionsBase(expect_success, version), client_ctx_yaml_(client_ctx_yaml),
        server_ctx_yaml_(server_ctx_yaml), expect_no_cert_(false), expect_no_cert_chain_(false) {
    if (expect_success) {
      setExpectedServerStats("ssl.handshake");
    } else {
      setExpectedServerStats("ssl.fail_verify_error");
    }
  }

  const std::string& clientCtxYaml() const { return client_ctx_yaml_; }
  const std::string& serverCtxYaml() const { return server_ctx_yaml_; }

  TestUtilOptions& setExpectedServerStats(const std::string& expected_server_stats) {
    TestUtilOptionsBase::setExpectedServerStats(expected_server_stats);
    return *this;
  }

  bool expectNoCert() const { return expect_no_cert_; }

  TestUtilOptions& setExpectNoCert() {
    expect_no_cert_ = true;
    return *this;
  }

  bool expectNoCertChain() const { return expect_no_cert_chain_; }

  TestUtilOptions& setExpectNoCertChain() {
    expect_no_cert_chain_ = true;
    return *this;
  }

  TestUtilOptions& setExpectedClientCertUri(const std::string& expected_client_cert_uri) {
    TestUtilOptionsBase::setExpectedClientCertUri(expected_client_cert_uri);
    return *this;
  }

  TestUtilOptions& setExpectedDigest(const std::string& expected_digest) {
    expected_digest_ = expected_digest;
    return *this;
  }

  const std::string& expectedDigest() const { return expected_digest_; }

  TestUtilOptions& setExpectedLocalUri(const std::string& expected_local_uri) {
    expected_local_uri_ = {expected_local_uri};
    return *this;
  }

  const std::vector<std::string>& expectedLocalUri() const { return expected_local_uri_; }

  TestUtilOptions& setExpectedSerialNumber(const std::string& expected_serial_number) {
    expected_serial_number_ = expected_serial_number;
    return *this;
  }

  const std::string& expectedSerialNumber() const { return expected_serial_number_; }

  TestUtilOptions& setExpectedPeerIssuer(const std::string& expected_peer_issuer) {
    expected_peer_issuer_ = expected_peer_issuer;
    return *this;
  }

  const std::string& expectedPeerIssuer() const { return expected_peer_issuer_; }

  TestUtilOptions& setExpectedPeerSubject(const std::string& expected_peer_subject) {
    expected_peer_subject_ = expected_peer_subject;
    return *this;
  }

  const std::string& expectedPeerSubject() const { return expected_peer_subject_; }

  TestUtilOptions& setExpectedLocalSubject(const std::string& expected_local_subject) {
    expected_local_subject_ = expected_local_subject;
    return *this;
  }

  const std::string& expectedLocalSubject() const { return expected_local_subject_; }

  TestUtilOptions& setExpectedPeerCert(const std::string& expected_peer_cert) {
    expected_peer_cert_ = expected_peer_cert;
    return *this;
  }

  const std::string& expectedPeerCert() const { return expected_peer_cert_; }

  TestUtilOptions& setExpectedPeerCertChain(const std::string& expected_peer_cert_chain) {
    expected_peer_cert_chain_ = expected_peer_cert_chain;
    return *this;
  }

  const std::string& expectedPeerCertChain() const { return expected_peer_cert_chain_; }

  TestUtilOptions& setExpectedValidFromTimePeerCert(const std::string& expected_valid_from) {
    expected_valid_from_peer_cert_ = expected_valid_from;
    return *this;
  }

  const std::string& expectedValidFromTimePeerCert() const {
    return expected_valid_from_peer_cert_;
  }

  TestUtilOptions& setExpectedExpirationTimePeerCert(const std::string& expected_expiration) {
    expected_expiration_peer_cert_ = expected_expiration;
    return *this;
  }

  const std::string& expectedExpirationTimePeerCert() const {
    return expected_expiration_peer_cert_;
  }

private:
  const std::string client_ctx_yaml_;
  const std::string server_ctx_yaml_;

  bool expect_no_cert_;
  bool expect_no_cert_chain_;
  std::string expected_digest_;
  std::vector<std::string> expected_local_uri_;
  std::string expected_serial_number_;
  std::string expected_peer_issuer_;
  std::string expected_peer_subject_;
  std::string expected_local_subject_;
  std::string expected_peer_cert_;
  std::string expected_peer_cert_chain_;
  std::string expected_valid_from_peer_cert_;
  std::string expected_expiration_peer_cert_;
};

void testUtil(const TestUtilOptions& options) {
  Event::SimulatedTimeSystem time_system;

  Stats::IsolatedStoreImpl server_stats_store;
  Api::ApiPtr server_api = Api::createApiForTest(server_stats_store, time_system);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      server_factory_context;
  ON_CALL(server_factory_context, api()).WillByDefault(ReturnRef(*server_api));

  envoy::api::v2::auth::DownstreamTlsContext server_tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(options.serverCtxYaml()),
                            server_tls_context);
  auto server_cfg =
      std::make_unique<ServerContextConfigImpl>(server_tls_context, server_factory_context);
  ContextManagerImpl manager(*time_system);
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, std::vector<std::string>{});

  Event::DispatcherPtr dispatcher = server_api->allocateDispatcher();
  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(options.version()),
                                  nullptr, true);
  Network::MockListenerCallbacks callbacks;
  Network::MockConnectionHandler connection_handler;
  Network::ListenerPtr listener = dispatcher->createListener(socket, callbacks, true, false);

  envoy::api::v2::auth::UpstreamTlsContext client_tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(options.clientCtxYaml()),
                            client_tls_context);

  Stats::IsolatedStoreImpl client_stats_store;
  Api::ApiPtr client_api = Api::createApiForTest(client_stats_store, time_system);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      client_factory_context;
  ON_CALL(client_factory_context, api()).WillByDefault(ReturnRef(*client_api));

  auto client_cfg =
      std::make_unique<ClientContextConfigImpl>(client_tls_context, client_factory_context);
  ClientSslSocketFactory client_ssl_socket_factory(std::move(client_cfg), manager,
                                                   client_stats_store);
  Network::ClientConnectionPtr client_connection = dispatcher->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      client_ssl_socket_factory.createTransportSocket(nullptr), nullptr);
  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  Network::MockConnectionCallbacks client_connection_callbacks;
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  client_connection->connect();

  size_t connect_count = 0;
  auto connect_second_time = [&]() {
    if (++connect_count == 2) {
      if (!options.expectedDigest().empty()) {
        // Assert twice to ensure a cached value is returned and still valid.
        EXPECT_EQ(options.expectedDigest(),
                  server_connection->ssl()->sha256PeerCertificateDigest());
        EXPECT_EQ(options.expectedDigest(),
                  server_connection->ssl()->sha256PeerCertificateDigest());
      }
      EXPECT_EQ(options.expectedClientCertUri(), server_connection->ssl()->uriSanPeerCertificate());
      if (!options.expectedLocalUri().empty()) {
        EXPECT_EQ(options.expectedLocalUri(), server_connection->ssl()->uriSanLocalCertificate());
      }
      EXPECT_EQ(options.expectedSerialNumber(),
                server_connection->ssl()->serialNumberPeerCertificate());
      if (!options.expectedPeerIssuer().empty()) {
        EXPECT_EQ(options.expectedPeerIssuer(), server_connection->ssl()->issuerPeerCertificate());
      }
      if (!options.expectedPeerSubject().empty()) {
        EXPECT_EQ(options.expectedPeerSubject(),
                  server_connection->ssl()->subjectPeerCertificate());
      }
      if (!options.expectedLocalSubject().empty()) {
        EXPECT_EQ(options.expectedLocalSubject(),
                  server_connection->ssl()->subjectLocalCertificate());
      }
      if (!options.expectedPeerCert().empty()) {
        std::string urlencoded = absl::StrReplaceAll(
            options.expectedPeerCert(),
            {{"\n", "%0A"}, {" ", "%20"}, {"+", "%2B"}, {"/", "%2F"}, {"=", "%3D"}});
        // Assert twice to ensure a cached value is returned and still valid.
        EXPECT_EQ(urlencoded, server_connection->ssl()->urlEncodedPemEncodedPeerCertificate());
        EXPECT_EQ(urlencoded, server_connection->ssl()->urlEncodedPemEncodedPeerCertificate());
      }
      if (!options.expectedPeerCertChain().empty()) {
        std::string cert_chain = absl::StrReplaceAll(
            options.expectedPeerCertChain(),
            {{"\n", "%0A"}, {" ", "%20"}, {"+", "%2B"}, {"/", "%2F"}, {"=", "%3D"}});
        // Assert twice to ensure a cached value is returned and still valid.
        EXPECT_EQ(cert_chain, server_connection->ssl()->urlEncodedPemEncodedPeerCertificateChain());
        EXPECT_EQ(cert_chain, server_connection->ssl()->urlEncodedPemEncodedPeerCertificateChain());
      }
      if (!options.expectedValidFromTimePeerCert().empty()) {
        const std::string formatted = TestUtility::formatTime(
            server_connection->ssl()->validFromPeerCertificate().value(), "%b %e %H:%M:%S %Y GMT");
        EXPECT_EQ(options.expectedValidFromTimePeerCert(), formatted);
      }
      if (!options.expectedExpirationTimePeerCert().empty()) {
        const std::string formatted = TestUtility::formatTime(
            server_connection->ssl()->expirationPeerCertificate().value(), "%b %e %H:%M:%S %Y GMT");
        EXPECT_EQ(options.expectedExpirationTimePeerCert(), formatted);
      }
      if (options.expectNoCert()) {
        EXPECT_FALSE(server_connection->ssl()->peerCertificatePresented());
        EXPECT_FALSE(server_connection->ssl()->validFromPeerCertificate().has_value());
        EXPECT_FALSE(server_connection->ssl()->expirationPeerCertificate().has_value());
        EXPECT_EQ(EMPTY_STRING, server_connection->ssl()->sha256PeerCertificateDigest());
        EXPECT_EQ(EMPTY_STRING, server_connection->ssl()->urlEncodedPemEncodedPeerCertificate());
        EXPECT_EQ(EMPTY_STRING, server_connection->ssl()->subjectPeerCertificate());
        EXPECT_EQ(std::vector<std::string>{}, server_connection->ssl()->dnsSansPeerCertificate());
      }
      if (options.expectNoCertChain()) {
        EXPECT_EQ(EMPTY_STRING,
                  server_connection->ssl()->urlEncodedPemEncodedPeerCertificateChain());
      }
      // By default, the session is not created with session resumption. The
      // client should see a session ID but the server should not.
      EXPECT_EQ(EMPTY_STRING, server_connection->ssl()->sessionId());
      EXPECT_NE(EMPTY_STRING, client_connection->ssl()->sessionId());

      server_connection->close(Network::ConnectionCloseType::NoFlush);
      client_connection->close(Network::ConnectionCloseType::NoFlush);
      dispatcher->exit();
    }
  };

  size_t close_count = 0;
  auto close_second_time = [&close_count, &dispatcher]() {
    if (++close_count == 2) {
      dispatcher->exit();
    }
  };

  if (options.expectSuccess()) {
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
  } else {
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));
  }

  dispatcher->run(Event::Dispatcher::RunType::Block);

  if (!options.expectedServerStats().empty()) {
    EXPECT_EQ(1UL, server_stats_store.counter(options.expectedServerStats()).value());
  }
}

/**
 * A class to hold the options for testUtilV2().
 */
class TestUtilOptionsV2 : public TestUtilOptionsBase {
public:
  TestUtilOptionsV2(const envoy::api::v2::Listener& listener,
                    const envoy::api::v2::auth::UpstreamTlsContext& client_ctx_proto,
                    bool expect_success, Network::Address::IpVersion version)
      : TestUtilOptionsBase(expect_success, version), listener_(listener),
        client_ctx_proto_(client_ctx_proto), transport_socket_options_(nullptr) {
    if (expect_success) {
      setExpectedServerStats("ssl.handshake").setExpectedClientStats("ssl.handshake");
    } else {
      setExpectedServerStats("ssl.fail_verify_error")
          .setExpectedClientStats("ssl.connection_error");
    }
  }

  const envoy::api::v2::Listener& listener() const { return listener_; }
  const envoy::api::v2::auth::UpstreamTlsContext& clientCtxProto() const {
    return client_ctx_proto_;
  }
  const std::string& expectedClientStats() const { return expected_client_stats_; }

  TestUtilOptionsV2& setExpectedServerStats(const std::string& expected_server_stats) {
    TestUtilOptionsBase::setExpectedServerStats(expected_server_stats);
    return *this;
  }

  TestUtilOptionsV2& setExpectedClientCertUri(const std::string& expected_client_cert_uri) {
    TestUtilOptionsBase::setExpectedClientCertUri(expected_client_cert_uri);
    return *this;
  }

  TestUtilOptionsV2& setExpectedClientStats(const std::string& expected_client_stats) {
    expected_client_stats_ = expected_client_stats;
    return *this;
  }

  TestUtilOptionsV2& setClientSession(const std::string& client_session) {
    client_session_ = client_session;
    return *this;
  }

  const std::string& clientSession() const { return client_session_; }

  TestUtilOptionsV2& setExpectedProtocolVersion(const std::string& expected_protocol_version) {
    expected_protocol_version_ = expected_protocol_version;
    return *this;
  }

  const std::string& expectedProtocolVersion() const { return expected_protocol_version_; }

  TestUtilOptionsV2& setExpectedCiphersuite(const std::string& expected_cipher_suite) {
    expected_cipher_suite_ = expected_cipher_suite;
    return *this;
  }

  const std::string& expectedCiphersuite() const { return expected_cipher_suite_; }

  TestUtilOptionsV2& setExpectedServerCertDigest(const std::string& expected_server_cert_digest) {
    expected_server_cert_digest_ = expected_server_cert_digest;
    return *this;
  }

  const std::string& expectedServerCertDigest() const { return expected_server_cert_digest_; }

  TestUtilOptionsV2&
  setExpectedRequestedServerName(const std::string& expected_requested_server_name) {
    expected_requested_server_name_ = expected_requested_server_name;
    return *this;
  }

  const std::string& expectedRequestedServerName() const { return expected_requested_server_name_; }

  TestUtilOptionsV2& setExpectedALPNProtocol(const std::string& expected_alpn_protocol) {
    expected_alpn_protocol_ = expected_alpn_protocol;
    return *this;
  }

  const std::string& expectedALPNProtocol() const { return expected_alpn_protocol_; }

  TestUtilOptionsV2&
  setTransportSocketOptions(Network::TransportSocketOptionsSharedPtr transport_socket_options) {
    transport_socket_options_ = transport_socket_options;
    return *this;
  }

  Network::TransportSocketOptionsSharedPtr transportSocketOptions() const {
    return transport_socket_options_;
  }

  TestUtilOptionsV2& setExpectedTransportFailureReasonContains(
      const std::string& expected_transport_failure_reason_contains) {
    expected_transport_failure_reason_contains_ = expected_transport_failure_reason_contains;
    return *this;
  }

  const std::string& expectedTransportFailureReasonContains() const {
    return expected_transport_failure_reason_contains_;
  }

private:
  const envoy::api::v2::Listener& listener_;
  const envoy::api::v2::auth::UpstreamTlsContext& client_ctx_proto_;
  std::string expected_client_stats_;

  std::string client_session_;
  std::string expected_cipher_suite_;
  std::string expected_protocol_version_;
  std::string expected_server_cert_digest_;
  std::string expected_requested_server_name_;
  std::string expected_alpn_protocol_;
  Network::TransportSocketOptionsSharedPtr transport_socket_options_;
  std::string expected_transport_failure_reason_contains_;
};

const std::string testUtilV2(const TestUtilOptionsV2& options) {
  Event::SimulatedTimeSystem time_system;
  ContextManagerImpl manager(*time_system);
  std::string new_session = EMPTY_STRING;

  // SNI-based selection logic isn't happening in SslSocket anymore.
  ASSERT(options.listener().filter_chains().size() == 1);
  const auto& filter_chain = options.listener().filter_chains(0);
  std::vector<std::string> server_names(filter_chain.filter_chain_match().server_names().begin(),
                                        filter_chain.filter_chain_match().server_names().end());
  Stats::IsolatedStoreImpl server_stats_store;
  Api::ApiPtr server_api = Api::createApiForTest(server_stats_store, time_system);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      server_factory_context;
  ON_CALL(server_factory_context, api()).WillByDefault(ReturnRef(*server_api));

  auto server_cfg =
      std::make_unique<ServerContextConfigImpl>(filter_chain.tls_context(), server_factory_context);
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, server_names);

  Event::DispatcherPtr dispatcher(server_api->allocateDispatcher());
  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(options.version()),
                                  nullptr, true);
  NiceMock<Network::MockListenerCallbacks> callbacks;
  Network::MockConnectionHandler connection_handler;
  Network::ListenerPtr listener = dispatcher->createListener(socket, callbacks, true, false);

  Stats::IsolatedStoreImpl client_stats_store;
  Api::ApiPtr client_api = Api::createApiForTest(client_stats_store, time_system);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      client_factory_context;
  ON_CALL(client_factory_context, api()).WillByDefault(ReturnRef(*client_api));

  auto client_cfg =
      std::make_unique<ClientContextConfigImpl>(options.clientCtxProto(), client_factory_context);
  ClientSslSocketFactory client_ssl_socket_factory(std::move(client_cfg), manager,
                                                   client_stats_store);
  Network::ClientConnectionPtr client_connection = dispatcher->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      client_ssl_socket_factory.createTransportSocket(options.transportSocketOptions()), nullptr);

  if (!options.clientSession().empty()) {
    const SslSocket* ssl_socket = dynamic_cast<const SslSocket*>(client_connection->ssl());
    SSL* client_ssl_socket = ssl_socket->rawSslForTest();
    SSL_CTX* client_ssl_context = SSL_get_SSL_CTX(client_ssl_socket);
    SSL_SESSION* client_ssl_session =
        SSL_SESSION_from_bytes(reinterpret_cast<const uint8_t*>(options.clientSession().data()),
                               options.clientSession().size(), client_ssl_context);
    int rc = SSL_set_session(client_ssl_socket, client_ssl_session);
    ASSERT(rc == 1);
    SSL_SESSION_free(client_ssl_session);
  }

  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        std::string sni = options.transportSocketOptions() != nullptr &&
                                  options.transportSocketOptions()->serverNameOverride().has_value()
                              ? options.transportSocketOptions()->serverNameOverride().value()
                              : options.clientCtxProto().sni();
        socket->setRequestedServerName(sni);
        Network::ConnectionPtr new_connection = dispatcher->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  Network::MockConnectionCallbacks client_connection_callbacks;
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  client_connection->connect();

  size_t connect_count = 0;
  auto connect_second_time = [&]() {
    if (++connect_count == 2) {
      if (!options.expectedServerCertDigest().empty()) {
        EXPECT_EQ(options.expectedServerCertDigest(),
                  client_connection->ssl()->sha256PeerCertificateDigest());
      }
      if (!options.expectedALPNProtocol().empty()) {
        EXPECT_EQ(options.expectedALPNProtocol(), client_connection->nextProtocol());
      }
      EXPECT_EQ(options.expectedClientCertUri(), server_connection->ssl()->uriSanPeerCertificate());
      const SslSocket* ssl_socket = dynamic_cast<const SslSocket*>(client_connection->ssl());
      SSL* client_ssl_socket = ssl_socket->rawSslForTest();
      if (!options.expectedProtocolVersion().empty()) {
        EXPECT_EQ(options.expectedProtocolVersion(), client_connection->ssl()->tlsVersion());
      }
      if (!options.expectedCiphersuite().empty()) {
        EXPECT_EQ(options.expectedCiphersuite(), client_connection->ssl()->ciphersuiteString());
        const SSL_CIPHER* cipher =
            SSL_get_cipher_by_value(client_connection->ssl()->ciphersuiteId());
        EXPECT_NE(nullptr, cipher);
        EXPECT_EQ(options.expectedCiphersuite(), SSL_CIPHER_get_name(cipher));
      }

      absl::optional<std::string> server_ssl_requested_server_name;
      const SslSocket* server_ssl_socket = dynamic_cast<const SslSocket*>(server_connection->ssl());
      SSL* server_ssl = server_ssl_socket->rawSslForTest();
      auto requested_server_name = SSL_get_servername(server_ssl, TLSEXT_NAMETYPE_host_name);
      if (requested_server_name != nullptr) {
        server_ssl_requested_server_name = std::string(requested_server_name);
      }

      if (!options.expectedRequestedServerName().empty()) {
        EXPECT_TRUE(server_ssl_requested_server_name.has_value());
        EXPECT_EQ(options.expectedRequestedServerName(), server_ssl_requested_server_name.value());
      } else {
        EXPECT_FALSE(server_ssl_requested_server_name.has_value());
      }

      SSL_SESSION* client_ssl_session = SSL_get_session(client_ssl_socket);
      EXPECT_TRUE(SSL_SESSION_is_resumable(client_ssl_session));
      uint8_t* session_data;
      size_t session_len;
      int rc = SSL_SESSION_to_bytes(client_ssl_session, &session_data, &session_len);
      ASSERT(rc == 1);
      new_session = std::string(reinterpret_cast<char*>(session_data), session_len);
      OPENSSL_free(session_data);
      server_connection->close(Network::ConnectionCloseType::NoFlush);
      client_connection->close(Network::ConnectionCloseType::NoFlush);
      dispatcher->exit();
    }
  };

  size_t close_count = 0;
  auto close_second_time = [&close_count, &dispatcher]() {
    if (++close_count == 2) {
      dispatcher->exit();
    }
  };

  if (options.expectSuccess()) {
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void {
          EXPECT_EQ(options.expectedRequestedServerName(),
                    server_connection->requestedServerName());
          connect_second_time();
        }));
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
  } else {
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));
  }

  dispatcher->run(Event::Dispatcher::RunType::Block);

  if (!options.expectedServerStats().empty()) {
    EXPECT_EQ(1UL, server_stats_store.counter(options.expectedServerStats()).value());
  }

  if (!options.expectedClientStats().empty()) {
    EXPECT_EQ(1UL, client_stats_store.counter(options.expectedClientStats()).value());
  }

  if (options.expectSuccess()) {
    EXPECT_EQ("", client_connection->transportFailureReason());
    EXPECT_EQ("", server_connection->transportFailureReason());
  } else {
    EXPECT_THAT(std::string(client_connection->transportFailureReason()),
                ContainsRegex(options.expectedTransportFailureReasonContains()));
    EXPECT_NE("", server_connection->transportFailureReason());
  }

  return new_session;
}

// Configure the listener with unittest{cert,key}.pem and ca_cert.pem.
// Configure the client with expired_san_uri_{cert,key}.pem
void configureServerAndExpiredClientCertificate(envoy::api::v2::Listener& listener,
                                                envoy::api::v2::auth::UpstreamTlsContext& client) {
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(
      TestEnvironment::substitute("{{ test_tmpdir }}/unittestcert.pem"));
  server_cert->mutable_private_key()->set_filename(
      TestEnvironment::substitute("{{ test_tmpdir }}/unittestkey.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));

  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/expired_san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/expired_san_uri_key.pem"));
}

} // namespace

class SslSocketTest : public SslCertsTest,
                      public testing::WithParamInterface<Network::Address::IpVersion> {
protected:
  SslSocketTest() : dispatcher_(api_->allocateDispatcher()) {}

  void testClientSessionResumption(const std::string& server_ctx_yaml,
                                   const std::string& client_ctx_yaml, bool expect_reuse,
                                   const Network::Address::IpVersion version);

  Event::DispatcherPtr dispatcher_;
};

INSTANTIATE_TEST_SUITE_P(IpVersions, SslSocketTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(SslSocketTest, GetCertDigest) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedDigest(TEST_NO_SAN_CERT_HASH)
               .setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL));
}

TEST_P(SslSocketTest, GetCertDigestInline) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();

  // From test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem.
  server_cert->mutable_certificate_chain()->set_inline_bytes(
      TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem")));

  // From test/extensions/transport_sockets/tls/test_data/san_dns_key.pem.
  server_cert->mutable_private_key()->set_inline_bytes(
      TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem")));

  // From test/extensions/transport_sockets/tls/test_data/ca_certificates.pem.
  filter_chain->mutable_tls_context()
      ->mutable_common_tls_context()
      ->mutable_validation_context()
      ->mutable_trusted_ca()
      ->set_inline_bytes(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir "
          "}}/test/extensions/transport_sockets/tls/test_data/ca_certificates.pem")));

  envoy::api::v2::auth::UpstreamTlsContext client_ctx;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client_ctx.mutable_common_tls_context()->add_tls_certificates();

  // From test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem.
  client_cert->mutable_certificate_chain()->set_inline_bytes(
      TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem")));

  // From test/extensions/transport_sockets/tls/test_data/san_uri_key.pem.
  client_cert->mutable_private_key()->set_inline_bytes(
      TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem")));

  TestUtilOptionsV2 test_options(listener, client_ctx, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_SAN_DNS_CERT_HASH));
}

TEST_P(SslSocketTest, GetCertDigestServerCertWithIntermediateCA) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns3_chain.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns3_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedDigest(TEST_NO_SAN_CERT_HASH)
               .setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL));
}

TEST_P(SslSocketTest, GetCertDigestServerCertWithoutCommonName) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_only_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_only_dns_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedDigest(TEST_NO_SAN_CERT_HASH)
               .setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL));
}

TEST_P(SslSocketTest, GetUriWithUriSan) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
      verify_subject_alt_name: "spiffe://lyft.com/test-team"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
               .setExpectedSerialNumber(TEST_SAN_URI_CERT_SERIAL));
}

// Verify that IP SANs work with an IPv4 address specified in the validation context.
TEST_P(SslSocketTest, Ipv4San) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/config/integration/certs/upstreamcacert.pem"
      verify_subject_alt_name: "127.0.0.1"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/config/integration/certs/upstreamlocalhostcert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/config/integration/certs/upstreamlocalhostkey.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options);
}

// Verify that IP SANs work with an IPv6 address specified in the validation context.
TEST_P(SslSocketTest, Ipv6San) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/config/integration/certs/upstreamcacert.pem"
      verify_subject_alt_name: "::1"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/config/integration/certs/upstreamlocalhostcert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/config/integration/certs/upstreamlocalhostkey.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options);
}

TEST_P(SslSocketTest, GetNoUriWithDnsSan) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
)EOF";

  // The SAN field only has DNS, expect "" for uriSanPeerCertificate().
  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedSerialNumber(TEST_SAN_DNS_CERT_SERIAL));
}

TEST_P(SslSocketTest, NoCert) {
  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.no_certificate")
               .setExpectNoCert()
               .setExpectNoCertChain());
}

// Prefer ECDSA certificate when multiple RSA certificates are present and the
// client is RSA/ECDSA capable. We validate TLSv1.2 only here, since we validate
// the e2e behavior on TLSv1.2/1.3 in ssl_integration_test.
TEST_P(SslSocketTest, MultiCertPreferEcdsa) {
  const std::string client_ctx_yaml = absl::StrCat(R"EOF(
    common_tls_context:
      tls_params:
        tls_minimum_protocol_version: TLSv1_2
        tls_maximum_protocol_version: TLSv1_2
        cipher_suites:
        - ECDHE-ECDSA-AES128-GCM-SHA256
        - ECDHE-RSA-AES128-GCM-SHA256
      validation_context:
        verify_certificate_hash: )EOF",
                                                   TEST_SELFSIGNED_ECDSA_P256_CERT_HASH);

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
    - certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem"
    - certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_ecdsa_p256_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_ecdsa_p256_key.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options);
}

TEST_P(SslSocketTest, GetUriWithLocalUriSan) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedLocalUri("spiffe://lyft.com/test-team")
               .setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL));
}

TEST_P(SslSocketTest, GetSubjectsWithBothCerts) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
  require_client_certificate: true
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL)
               .setExpectedPeerIssuer(
                   "CN=Test CA,OU=Lyft Engineering,O=Lyft,L=San Francisco,ST=California,C=US")
               .setExpectedPeerSubject(
                   "CN=Test Server,OU=Lyft Engineering,O=Lyft,L=San Francisco,ST=California,C=US")
               .setExpectedLocalSubject(
                   "CN=Test Server,OU=Lyft Engineering,O=Lyft,L=San Francisco,ST=California,C=US"));
}

TEST_P(SslSocketTest, GetPeerCert) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
  require_client_certificate: true
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  std::string expected_peer_cert =
      TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"));
  testUtil(test_options.setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL)
               .setExpectedPeerIssuer(
                   "CN=Test CA,OU=Lyft Engineering,O=Lyft,L=San Francisco,ST=California,C=US")
               .setExpectedPeerSubject(
                   "CN=Test Server,OU=Lyft Engineering,O=Lyft,L=San Francisco,ST=California,C=US")
               .setExpectedLocalSubject(
                   "CN=Test Server,OU=Lyft Engineering,O=Lyft,L=San Francisco,ST=California,C=US")
               .setExpectedPeerCert(expected_peer_cert));
}

TEST_P(SslSocketTest, GetPeerCertChain) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_chain.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
  require_client_certificate: true
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  std::string expected_peer_cert_chain =
      TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir "
          "}}/test/extensions/transport_sockets/tls/test_data/no_san_chain.pem"));
  testUtil(test_options.setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL)
               .setExpectedPeerCertChain(expected_peer_cert_chain));
}

TEST_P(SslSocketTest, GetIssueExpireTimesPeerCert) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
  require_client_certificate: true
)EOF";
  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedSerialNumber(TEST_NO_SAN_CERT_SERIAL)
               .setExpectedValidFromTimePeerCert(TEST_NO_SAN_CERT_NOT_BEFORE)
               .setExpectedExpirationTimePeerCert(TEST_NO_SAN_CERT_NOT_AFTER));
}

TEST_P(SslSocketTest, FailedClientAuthCaVerificationNoClientCert) {
  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
  require_client_certificate: true
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_no_cert"));
}

TEST_P(SslSocketTest, FailedClientAuthCaVerification) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_error"));
}

TEST_P(SslSocketTest, FailedClientAuthSanVerificationNoClientCert) {
  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
      verify_subject_alt_name: "example.com"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_no_cert"));
}

TEST_P(SslSocketTest, FailedClientAuthSanVerification) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
      verify_subject_alt_name: "example.com"
)EOF";

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_san"));
}

// By default, expired certificates are not permitted.
TEST_P(SslSocketTest, FailedClientCertificateDefaultExpirationVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::auth::UpstreamTlsContext client;

  configureServerAndExpiredClientCertificate(listener, client);

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_EXPIRED"));
}

// Expired certificates will not be accepted when explicitly disallowed via
// allow_expired_certificate.
TEST_P(SslSocketTest, FailedClientCertificateExpirationVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::auth::UpstreamTlsContext client;

  configureServerAndExpiredClientCertificate(listener, client);

  listener.mutable_filter_chains(0)
      ->mutable_tls_context()
      ->mutable_common_tls_context()
      ->mutable_validation_context()
      ->set_allow_expired_certificate(false);

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_EXPIRED"));
}

// Expired certificates will be accepted when explicitly allowed via allow_expired_certificate.
TEST_P(SslSocketTest, ClientCertificateExpirationAllowedVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::auth::UpstreamTlsContext client;

  configureServerAndExpiredClientCertificate(listener, client);

  listener.mutable_filter_chains(0)
      ->mutable_tls_context()
      ->mutable_common_tls_context()
      ->mutable_validation_context()
      ->set_allow_expired_certificate(true);

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_EXPIRED"));
}

// Allow expired certificates, but add a certificate hash requirement so it still fails.
TEST_P(SslSocketTest, FailedClientCertAllowExpiredBadHashVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::auth::UpstreamTlsContext client;

  configureServerAndExpiredClientCertificate(listener, client);

  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      listener.mutable_filter_chains(0)
          ->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();

  server_validation_ctx->set_allow_expired_certificate(true);
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_cert_hash")
                 .setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_EXPIRED"));
}

// Allow expired certificates, but use the wrong CA so it should fail still.
TEST_P(SslSocketTest, FailedClientCertAllowServerExpiredWrongCAVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::auth::UpstreamTlsContext client;

  configureServerAndExpiredClientCertificate(listener, client);

  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      listener.mutable_filter_chains(0)
          ->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();

  server_validation_ctx->set_allow_expired_certificate(true);

  // This fake CA was not used to sign the client's certificate.
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/fake_ca_cert.pem"));

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedTransportFailureReasonContains("TLSV1_ALERT_UNKNOWN_CA"));
}

TEST_P(SslSocketTest, ClientCertificateHashVerification) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
)EOF";

  const std::string server_ctx_yaml = absl::StrCat(R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
      verify_certificate_hash: ")EOF",
                                                   TEST_SAN_URI_CERT_HASH, "\"");

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
               .setExpectedSerialNumber(TEST_SAN_URI_CERT_SERIAL));
}

TEST_P(SslSocketTest, ClientCertificateHashVerificationNoCA) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
)EOF";

  const std::string server_ctx_yaml = absl::StrCat(R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      verify_certificate_hash: ")EOF",
                                                   TEST_SAN_URI_CERT_HASH, "\"");

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, true, GetParam());
  testUtil(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
               .setExpectedSerialNumber(TEST_SAN_URI_CERT_SERIAL));
}

TEST_P(SslSocketTest, ClientCertificateHashListVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_hash(TEST_SAN_URI_CERT_HASH);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_SAN_DNS_CERT_HASH));

  // Works even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, ClientCertificateHashListVerificationNoCA) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_hash(TEST_SAN_URI_CERT_HASH);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_SAN_DNS_CERT_HASH));

  // Works even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateHashVerificationNoClientCertificate) {
  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  const std::string server_ctx_yaml = absl::StrCat(R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
      verify_certificate_hash: ")EOF",
                                                   TEST_SAN_URI_CERT_HASH, "\"");

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_no_cert"));
}

TEST_P(SslSocketTest, FailedClientCertificateHashVerificationNoCANoClientCertificate) {
  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  const std::string server_ctx_yaml = absl::StrCat(R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      verify_certificate_hash: ")EOF",
                                                   TEST_SAN_URI_CERT_HASH, "\"");

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_no_cert"));
}

TEST_P(SslSocketTest, FailedClientCertificateHashVerificationWrongClientCertificate) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = absl::StrCat(R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
      verify_certificate_hash: ")EOF",
                                                   TEST_SAN_URI_CERT_HASH, "\"");

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_cert_hash"));
}

TEST_P(SslSocketTest, FailedClientCertificateHashVerificationNoCAWrongClientCertificate) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  const std::string server_ctx_yaml = absl::StrCat(R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      verify_certificate_hash: ")EOF",
                                                   TEST_SAN_URI_CERT_HASH, "\"");

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_cert_hash"));
}

TEST_P(SslSocketTest, FailedClientCertificateHashVerificationWrongCA) {
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"
)EOF";

  const std::string server_ctx_yaml = absl::StrCat(R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/fake_ca_cert.pem"
      verify_certificate_hash: ")EOF",
                                                   TEST_SAN_URI_CERT_HASH, "\"");

  TestUtilOptions test_options(client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(test_options.setExpectedServerStats("ssl.fail_verify_error"));
}

TEST_P(SslSocketTest, CertificatesWithPassword) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/password_protected_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/password_protected_key.pem"));
  server_cert->mutable_password()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/password_protected_password.txt"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_hash(TEST_PASSWORD_PROTECTED_CERT_HASH);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/password_protected_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/password_protected_key.pem"));
  client_cert->mutable_password()->set_inline_string(
      TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
          "{{ test_rundir "
          "}}/test/extensions/transport_sockets/tls/test_data/password_protected_password.txt")));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_PASSWORD_PROTECTED_CERT_HASH));
  testUtilV2(test_options);

  // Works even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, ClientCertificateSpkiVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_SAN_DNS_CERT_HASH));

  // Works even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, ClientCertificateSpkiVerificationNoCA) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_SAN_DNS_CERT_HASH));

  // Works even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateSpkiVerificationNoClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_no_cert")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_HANDSHAKE_FAILURE"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateSpkiVerificationNoCANoClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_no_cert")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_HANDSHAKE_FAILURE"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateSpkiVerificationWrongClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_cert_hash")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_UNKNOWN"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateSpkiVerificationNoCAWrongClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_cert_hash")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_UNKNOWN"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateSpkiVerificationWrongCA) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/fake_ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedTransportFailureReasonContains("TLSV1_ALERT_UNKNOWN_CA"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, ClientCertificateHashAndSpkiVerification) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_SAN_DNS_CERT_HASH));

  // Works even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, ClientCertificateHashAndSpkiVerificationNoCA) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_DNS_CERT_SPKI);
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team")
                 .setExpectedServerCertDigest(TEST_SAN_DNS_CERT_HASH));

  // Works even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateHashAndSpkiVerificationNoClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_no_cert")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_HANDSHAKE_FAILURE"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateHashAndSpkiVerificationNoCANoClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_no_cert")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_HANDSHAKE_FAILURE"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateHashAndSpkiVerificationWrongClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_cert_hash")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_UNKNOWN"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateHashAndSpkiVerificationNoCAWrongClientCertificate) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedServerStats("ssl.fail_verify_cert_hash")
                 .setExpectedTransportFailureReasonContains("SSLV3_ALERT_CERTIFICATE_UNKNOWN"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

TEST_P(SslSocketTest, FailedClientCertificateHashAndSpkiVerificationWrongCA) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/fake_ca_cert.pem"));
  server_validation_ctx->add_verify_certificate_hash(
      "0000000000000000000000000000000000000000000000000000000000000000");
  server_validation_ctx->add_verify_certificate_spki(TEST_SAN_URI_CERT_SPKI);

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  TestUtilOptionsV2 test_options(listener, client, false, GetParam());
  testUtilV2(test_options.setExpectedTransportFailureReasonContains("TLSV1_ALERT_UNKNOWN_CA"));

  // Fails even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
}

// Make sure that we do not flush code and do an immediate close if we have not completed the
// handshake.
TEST_P(SslSocketTest, FlushCloseDuringHandshake) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_certificates.pem"
)EOF";

  envoy::api::v2::auth::DownstreamTlsContext tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml), tls_context);
  auto server_cfg = std::make_unique<ServerContextConfigImpl>(tls_context, factory_context_);
  ContextManagerImpl manager(time_system_);
  Stats::IsolatedStoreImpl server_stats_store;
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, std::vector<std::string>{});

  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr,
                                  true);
  Network::MockListenerCallbacks callbacks;
  Network::MockConnectionHandler connection_handler;
  Network::ListenerPtr listener = dispatcher_->createListener(socket, callbacks, true, false);

  Network::ClientConnectionPtr client_connection = dispatcher_->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      Network::Test::createRawBufferSocket(), nullptr);
  client_connection->connect();
  Network::MockConnectionCallbacks client_connection_callbacks;
  client_connection->addConnectionCallbacks(client_connection_callbacks);

  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
        Buffer::OwnedImpl data("hello");
        server_connection->write(data, false);
        server_connection->close(Network::ConnectionCloseType::FlushWrite);
      }));

  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
}

// Test that half-close is sent and received correctly
TEST_P(SslSocketTest, HalfClose) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_certificates.pem"
)EOF";

  envoy::api::v2::auth::DownstreamTlsContext server_tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml), server_tls_context);
  auto server_cfg = std::make_unique<ServerContextConfigImpl>(server_tls_context, factory_context_);
  ContextManagerImpl manager(time_system_);
  Stats::IsolatedStoreImpl server_stats_store;
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, std::vector<std::string>{});

  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr,
                                  true);
  Network::MockListenerCallbacks listener_callbacks;
  Network::MockConnectionHandler connection_handler;
  Network::ListenerPtr listener =
      dispatcher_->createListener(socket, listener_callbacks, true, false);
  std::shared_ptr<Network::MockReadFilter> server_read_filter(new Network::MockReadFilter());
  std::shared_ptr<Network::MockReadFilter> client_read_filter(new Network::MockReadFilter());

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  envoy::api::v2::auth::UpstreamTlsContext tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(client_ctx_yaml), tls_context);
  auto client_cfg = std::make_unique<ClientContextConfigImpl>(tls_context, factory_context_);
  Stats::IsolatedStoreImpl client_stats_store;
  ClientSslSocketFactory client_ssl_socket_factory(std::move(client_cfg), manager,
                                                   client_stats_store);
  Network::ClientConnectionPtr client_connection = dispatcher_->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      client_ssl_socket_factory.createTransportSocket(nullptr), nullptr);
  client_connection->enableHalfClose(true);
  client_connection->addReadFilter(client_read_filter);
  client_connection->connect();
  Network::MockConnectionCallbacks client_connection_callbacks;
  client_connection->addConnectionCallbacks(client_connection_callbacks);

  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(listener_callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        listener_callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(listener_callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->enableHalfClose(true);
        server_connection->addReadFilter(server_read_filter);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
        Buffer::OwnedImpl data("hello");
        server_connection->write(data, true);
      }));

  EXPECT_CALL(*server_read_filter, onNewConnection())
      .WillOnce(Return(Network::FilterStatus::Continue));
  EXPECT_CALL(*client_read_filter, onNewConnection())
      .WillOnce(Return(Network::FilterStatus::Continue));
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected));
  EXPECT_CALL(*client_read_filter, onData(BufferStringEqual("hello"), true))
      .WillOnce(Invoke([&](Buffer::Instance&, bool) -> Network::FilterStatus {
        Buffer::OwnedImpl buffer("world");
        client_connection->write(buffer, true);
        return Network::FilterStatus::Continue;
      }));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
  EXPECT_CALL(*server_read_filter, onData(BufferStringEqual("world"), true));
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);
}

TEST_P(SslSocketTest, ClientAuthMultipleCAs) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_certificates.pem"
)EOF";

  envoy::api::v2::auth::DownstreamTlsContext server_tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml), server_tls_context);
  auto server_cfg = std::make_unique<ServerContextConfigImpl>(server_tls_context, factory_context_);
  ContextManagerImpl manager(time_system_);
  Stats::IsolatedStoreImpl server_stats_store;
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, std::vector<std::string>{});

  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr,
                                  true);
  Network::MockListenerCallbacks callbacks;
  Network::MockConnectionHandler connection_handler;
  Network::ListenerPtr listener = dispatcher_->createListener(socket, callbacks, true, false);

  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  envoy::api::v2::auth::UpstreamTlsContext tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(client_ctx_yaml), tls_context);
  auto client_cfg = std::make_unique<ClientContextConfigImpl>(tls_context, factory_context_);
  Stats::IsolatedStoreImpl client_stats_store;
  ClientSslSocketFactory ssl_socket_factory(std::move(client_cfg), manager, client_stats_store);
  Network::ClientConnectionPtr client_connection = dispatcher_->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      ssl_socket_factory.createTransportSocket(nullptr), nullptr);

  // Verify that server sent list with 2 acceptable client certificate CA names.
  const SslSocket* ssl_socket = dynamic_cast<const SslSocket*>(client_connection->ssl());
  SSL_set_cert_cb(
      ssl_socket->rawSslForTest(),
      [](SSL* ssl, void*) -> int {
        STACK_OF(X509_NAME)* list = SSL_get_client_CA_list(ssl);
        EXPECT_NE(nullptr, list);
        EXPECT_EQ(2U, sk_X509_NAME_num(list));
        return 1;
      },
      nullptr);

  client_connection->connect();

  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void {
        server_connection->close(Network::ConnectionCloseType::NoFlush);
        client_connection->close(Network::ConnectionCloseType::NoFlush);
        dispatcher_->exit();
      }));
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));

  dispatcher_->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(1UL, server_stats_store.counter("ssl.handshake").value());
}

namespace {

// Test connecting with a client to server1, then trying to reuse the session on server2
void testTicketSessionResumption(const std::string& server_ctx_yaml1,
                                 const std::vector<std::string>& server_names1,
                                 const std::string& server_ctx_yaml2,
                                 const std::vector<std::string>& server_names2,
                                 const std::string& client_ctx_yaml, bool expect_reuse,
                                 const Network::Address::IpVersion ip_version) {
  Event::SimulatedTimeSystem time_system;
  ContextManagerImpl manager(*time_system);

  Stats::IsolatedStoreImpl server_stats_store;
  Api::ApiPtr server_api = Api::createApiForTest(server_stats_store, time_system);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      server_factory_context;
  ON_CALL(server_factory_context, api()).WillByDefault(ReturnRef(*server_api));

  envoy::api::v2::auth::DownstreamTlsContext server_tls_context1;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml1), server_tls_context1);
  auto server_cfg1 =
      std::make_unique<ServerContextConfigImpl>(server_tls_context1, server_factory_context);

  envoy::api::v2::auth::DownstreamTlsContext server_tls_context2;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml2), server_tls_context2);
  auto server_cfg2 =
      std::make_unique<ServerContextConfigImpl>(server_tls_context2, server_factory_context);
  ServerSslSocketFactory server_ssl_socket_factory1(std::move(server_cfg1), manager,
                                                    server_stats_store, server_names1);
  ServerSslSocketFactory server_ssl_socket_factory2(std::move(server_cfg2), manager,
                                                    server_stats_store, server_names2);

  Network::TcpListenSocket socket1(Network::Test::getCanonicalLoopbackAddress(ip_version), nullptr,
                                   true);
  Network::TcpListenSocket socket2(Network::Test::getCanonicalLoopbackAddress(ip_version), nullptr,
                                   true);
  NiceMock<Network::MockListenerCallbacks> callbacks;
  Network::MockConnectionHandler connection_handler;
  Event::DispatcherPtr dispatcher(server_api->allocateDispatcher());
  Network::ListenerPtr listener1 = dispatcher->createListener(socket1, callbacks, true, false);
  Network::ListenerPtr listener2 = dispatcher->createListener(socket2, callbacks, true, false);

  envoy::api::v2::auth::UpstreamTlsContext client_tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(client_ctx_yaml), client_tls_context);

  Stats::IsolatedStoreImpl client_stats_store;
  Api::ApiPtr client_api = Api::createApiForTest(client_stats_store, time_system);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      client_factory_context;
  ON_CALL(client_factory_context, api()).WillByDefault(ReturnRef(*client_api));

  auto client_cfg =
      std::make_unique<ClientContextConfigImpl>(client_tls_context, client_factory_context);
  ClientSslSocketFactory ssl_socket_factory(std::move(client_cfg), manager, client_stats_store);
  Network::ClientConnectionPtr client_connection = dispatcher->createClientConnection(
      socket1.localAddress(), Network::Address::InstanceConstSharedPtr(),
      ssl_socket_factory.createTransportSocket(nullptr), nullptr);

  Network::MockConnectionCallbacks client_connection_callbacks;
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  client_connection->connect();

  SSL_SESSION* ssl_session = nullptr;
  Network::ConnectionPtr server_connection;
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillRepeatedly(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::TransportSocketFactory& tsf = socket->localAddress() == socket1.localAddress()
                                                   ? server_ssl_socket_factory1
                                                   : server_ssl_socket_factory2;
        Network::ConnectionPtr new_connection = dispatcher->createServerConnection(
            std::move(socket), tsf.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke(
          [&](Network::ConnectionPtr& conn) -> void { server_connection = std::move(conn); }));

  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void {
        const SslSocket* ssl_socket = dynamic_cast<const SslSocket*>(client_connection->ssl());
        ssl_session = SSL_get1_session(ssl_socket->rawSslForTest());
        EXPECT_TRUE(SSL_SESSION_is_resumable(ssl_session));
        client_connection->close(Network::ConnectionCloseType::NoFlush);
        server_connection->close(Network::ConnectionCloseType::NoFlush);
        dispatcher->exit();
      }));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));

  dispatcher->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(0UL, server_stats_store.counter("ssl.session_reused").value());
  EXPECT_EQ(0UL, client_stats_store.counter("ssl.session_reused").value());

  client_connection = dispatcher->createClientConnection(
      socket2.localAddress(), Network::Address::InstanceConstSharedPtr(),
      ssl_socket_factory.createTransportSocket(nullptr), nullptr);
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  const SslSocket* ssl_socket = dynamic_cast<const SslSocket*>(client_connection->ssl());
  SSL_set_session(ssl_socket->rawSslForTest(), ssl_session);
  SSL_SESSION_free(ssl_session);

  client_connection->connect();

  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  // Different tests have different order of whether client or server gets Connected event
  // first, so always wait until both have happened.
  size_t connect_count = 0;
  auto connect_second_time = [&connect_count, &dispatcher, &server_connection, &client_connection,
                              expect_reuse]() {
    connect_count++;
    if (connect_count == 2) {
      if (expect_reuse) {
        EXPECT_NE(EMPTY_STRING, server_connection->ssl()->sessionId());
        EXPECT_EQ(server_connection->ssl()->sessionId(), client_connection->ssl()->sessionId());
      } else {
        EXPECT_EQ(EMPTY_STRING, server_connection->ssl()->sessionId());
      }
      client_connection->close(Network::ConnectionCloseType::NoFlush);
      server_connection->close(Network::ConnectionCloseType::NoFlush);
      dispatcher->exit();
    }
  };

  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));

  dispatcher->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(expect_reuse ? 1UL : 0UL, server_stats_store.counter("ssl.session_reused").value());
  EXPECT_EQ(expect_reuse ? 1UL : 0UL, client_stats_store.counter("ssl.session_reused").value());
}

} // namespace

TEST_P(SslSocketTest, TicketSessionResumption) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  testTicketSessionResumption(server_ctx_yaml, {}, server_ctx_yaml, {}, client_ctx_yaml, true,
                              GetParam());
}

TEST_P(SslSocketTest, TicketSessionResumptionWithClientCA) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  testTicketSessionResumption(server_ctx_yaml, {}, server_ctx_yaml, {}, client_ctx_yaml, true,
                              GetParam());
}

TEST_P(SslSocketTest, TicketSessionResumptionRotateKey) {
  const std::string server_ctx_yaml1 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string server_ctx_yaml2 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_b"
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  testTicketSessionResumption(server_ctx_yaml1, {}, server_ctx_yaml2, {}, client_ctx_yaml, true,
                              GetParam());
}

TEST_P(SslSocketTest, TicketSessionResumptionWrongKey) {
  const std::string server_ctx_yaml1 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string server_ctx_yaml2 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_b"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  testTicketSessionResumption(server_ctx_yaml1, {}, server_ctx_yaml2, {}, client_ctx_yaml, false,
                              GetParam());
}

// Sessions cannot be resumed even though the server certificates are the same,
// because of the different SNI requirements.
TEST_P(SslSocketTest, TicketSessionResumptionDifferentServerNames) {
  const std::string server_ctx_yaml1 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string server_ctx_yaml2 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  std::vector<std::string> server_names1 = {"server1.example.com"};

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  testTicketSessionResumption(server_ctx_yaml1, server_names1, server_ctx_yaml2, {},
                              client_ctx_yaml, false, GetParam());
}

// Sessions can be resumed because the server certificates are different but the CN/SANs and
// issuer are identical
TEST_P(SslSocketTest, TicketSessionResumptionDifferentServerCert) {
  const std::string server_ctx_yaml1 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string server_ctx_yaml2 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns2_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns2_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  testTicketSessionResumption(server_ctx_yaml1, {}, server_ctx_yaml2, {}, client_ctx_yaml, true,
                              GetParam());
}

// Sessions cannot be resumed because the server certificates are different, CN/SANs are identical,
// but the issuer is different.
TEST_P(SslSocketTest, TicketSessionResumptionDifferentServerCertIntermediateCA) {
  const std::string server_ctx_yaml1 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string server_ctx_yaml2 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns3_chain.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns3_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  testTicketSessionResumption(server_ctx_yaml1, {}, server_ctx_yaml2, {}, client_ctx_yaml, false,
                              GetParam());
}

// Sessions cannot be resumed because the server certificates are different and the SANs
// are not identical
TEST_P(SslSocketTest, TicketSessionResumptionDifferentServerCertDifferentSAN) {
  const std::string server_ctx_yaml1 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string server_ctx_yaml2 = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_multiple_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_multiple_dns_key.pem"
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
    common_tls_context:
  )EOF";

  testTicketSessionResumption(server_ctx_yaml1, {}, server_ctx_yaml2, {}, client_ctx_yaml, false,
                              GetParam());
}

// Test that if two listeners use the same cert and session ticket key, but
// different client CA, that sessions cannot be resumed.
TEST_P(SslSocketTest, ClientAuthCrossListenerSessionResumption) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
  require_client_certificate: true
)EOF";

  const std::string server2_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/fake_ca_cert.pem"
  require_client_certificate: true
  session_ticket_keys:
    keys:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a"
)EOF";

  envoy::api::v2::auth::DownstreamTlsContext tls_context1;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml), tls_context1);
  auto server_cfg = std::make_unique<ServerContextConfigImpl>(tls_context1, factory_context_);
  envoy::api::v2::auth::DownstreamTlsContext tls_context2;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server2_ctx_yaml), tls_context2);
  auto server2_cfg = std::make_unique<ServerContextConfigImpl>(tls_context2, factory_context_);
  ContextManagerImpl manager(time_system_);
  Stats::IsolatedStoreImpl server_stats_store;
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, std::vector<std::string>{});
  ServerSslSocketFactory server2_ssl_socket_factory(std::move(server2_cfg), manager,
                                                    server_stats_store, std::vector<std::string>{});

  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr,
                                  true);
  Network::TcpListenSocket socket2(Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr,
                                   true);
  Network::MockListenerCallbacks callbacks;
  Network::MockConnectionHandler connection_handler;
  Network::ListenerPtr listener = dispatcher_->createListener(socket, callbacks, true, false);
  Network::ListenerPtr listener2 = dispatcher_->createListener(socket2, callbacks, true, false);
  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  envoy::api::v2::auth::UpstreamTlsContext tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(client_ctx_yaml), tls_context);

  auto client_cfg = std::make_unique<ClientContextConfigImpl>(tls_context, factory_context_);
  Stats::IsolatedStoreImpl client_stats_store;
  ClientSslSocketFactory ssl_socket_factory(std::move(client_cfg), manager, client_stats_store);
  Network::ClientConnectionPtr client_connection = dispatcher_->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      ssl_socket_factory.createTransportSocket(nullptr), nullptr);

  Network::MockConnectionCallbacks client_connection_callbacks;
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  client_connection->connect();

  SSL_SESSION* ssl_session = nullptr;
  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillRepeatedly(Invoke([&](Network::ConnectionSocketPtr& accepted_socket, bool) -> void {
        Network::TransportSocketFactory& tsf =
            accepted_socket->localAddress() == socket.localAddress() ? server_ssl_socket_factory
                                                                     : server2_ssl_socket_factory;

        Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
            std::move(accepted_socket), tsf.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected));
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void {
        const SslSocket* ssl_socket = dynamic_cast<const SslSocket*>(client_connection->ssl());
        ssl_session = SSL_get1_session(ssl_socket->rawSslForTest());
        EXPECT_TRUE(SSL_SESSION_is_resumable(ssl_session));
        server_connection->close(Network::ConnectionCloseType::NoFlush);
        client_connection->close(Network::ConnectionCloseType::NoFlush);
        dispatcher_->exit();
      }));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose));

  dispatcher_->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(1UL, server_stats_store.counter("ssl.handshake").value());
  EXPECT_EQ(1UL, client_stats_store.counter("ssl.handshake").value());

  client_connection = dispatcher_->createClientConnection(
      socket2.localAddress(), Network::Address::InstanceConstSharedPtr(),
      ssl_socket_factory.createTransportSocket(nullptr), nullptr);
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  const SslSocket* ssl_socket = dynamic_cast<const SslSocket*>(client_connection->ssl());
  SSL_set_session(ssl_socket->rawSslForTest(), ssl_session);
  SSL_SESSION_free(ssl_session);

  client_connection->connect();

  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(1UL, client_stats_store.counter("ssl.connection_error").value());
  EXPECT_EQ(0UL, server_stats_store.counter("ssl.session_reused").value());
  EXPECT_EQ(0UL, client_stats_store.counter("ssl.session_reused").value());
}

void SslSocketTest::testClientSessionResumption(const std::string& server_ctx_yaml,
                                                const std::string& client_ctx_yaml,
                                                bool expect_reuse,
                                                const Network::Address::IpVersion version) {
  InSequence s;

  ContextManagerImpl manager(time_system_);

  Stats::IsolatedStoreImpl server_stats_store;
  Api::ApiPtr server_api = Api::createApiForTest(server_stats_store, time_system_);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      server_factory_context;
  ON_CALL(server_factory_context, api()).WillByDefault(ReturnRef(*server_api));

  envoy::api::v2::auth::DownstreamTlsContext server_ctx_proto;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml), server_ctx_proto);
  auto server_cfg =
      std::make_unique<ServerContextConfigImpl>(server_ctx_proto, server_factory_context);
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, std::vector<std::string>{});

  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(version), nullptr,
                                  true);
  NiceMock<Network::MockListenerCallbacks> callbacks;
  Network::MockConnectionHandler connection_handler;
  Api::ApiPtr api = Api::createApiForTest(server_stats_store, time_system_);
  Event::DispatcherPtr dispatcher(server_api->allocateDispatcher());
  Network::ListenerPtr listener = dispatcher->createListener(socket, callbacks, true, false);

  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;

  envoy::api::v2::auth::UpstreamTlsContext client_ctx_proto;
  TestUtility::loadFromYaml(TestEnvironment::substitute(client_ctx_yaml), client_ctx_proto);

  Stats::IsolatedStoreImpl client_stats_store;
  Api::ApiPtr client_api = Api::createApiForTest(client_stats_store, time_system_);
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext>
      client_factory_context;
  ON_CALL(client_factory_context, api()).WillByDefault(ReturnRef(*client_api));

  auto client_cfg =
      std::make_unique<ClientContextConfigImpl>(client_ctx_proto, client_factory_context);
  ClientSslSocketFactory client_ssl_socket_factory(std::move(client_cfg), manager,
                                                   client_stats_store);
  Network::ClientConnectionPtr client_connection = dispatcher->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      client_ssl_socket_factory.createTransportSocket(nullptr), nullptr);

  Network::MockConnectionCallbacks client_connection_callbacks;
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  client_connection->connect();

  size_t connect_count = 0;
  auto connect_second_time = [&connect_count, &server_connection]() {
    if (++connect_count == 2) {
      server_connection->close(Network::ConnectionCloseType::NoFlush);
    }
  };

  size_t close_count = 0;
  auto close_second_time = [&close_count, &dispatcher]() {
    if (++close_count == 2) {
      dispatcher->exit();
    }
  };

  // WillRepeatedly doesn't work with InSequence.
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  const bool expect_tls13 =
      client_ctx_proto.common_tls_context().tls_params().tls_maximum_protocol_version() ==
          envoy::api::v2::auth::TlsParameters::TLSv1_3 &&
      server_ctx_proto.common_tls_context().tls_params().tls_maximum_protocol_version() ==
          envoy::api::v2::auth::TlsParameters::TLSv1_3;

  // The order of "Connected" events depends on the version of the TLS protocol (1.3 or older).
  if (expect_tls13) {
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
  } else {
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
  }
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));

  dispatcher->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(0UL, server_stats_store.counter("ssl.session_reused").value());
  EXPECT_EQ(0UL, client_stats_store.counter("ssl.session_reused").value());

  connect_count = 0;
  close_count = 0;

  client_connection = dispatcher->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      client_ssl_socket_factory.createTransportSocket(nullptr), nullptr);
  client_connection->addConnectionCallbacks(client_connection_callbacks);
  client_connection->connect();

  // WillRepeatedly doesn't work with InSequence.
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  // The order of "Connected" events depends on the version of the TLS protocol (1.3 or older),
  // and whether or not the session was successfully resumed.
  if (expect_tls13 || expect_reuse) {
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
  } else {
    EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
    EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { connect_second_time(); }));
  }
  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::LocalClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));
  EXPECT_CALL(client_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { close_second_time(); }));

  dispatcher->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(expect_reuse ? 1UL : 0UL, server_stats_store.counter("ssl.session_reused").value());
  EXPECT_EQ(expect_reuse ? 1UL : 0UL, client_stats_store.counter("ssl.session_reused").value());
}

// Test client session resumption using default settings (should be enabled).
TEST_P(SslSocketTest, ClientSessionResumptionDefault) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
)EOF";

  testClientSessionResumption(server_ctx_yaml, client_ctx_yaml, true, GetParam());
}

// Make sure client session resumption is not happening with TLS 1.0-1.2 when it's disabled.
TEST_P(SslSocketTest, ClientSessionResumptionDisabledTls12) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_params:
      tls_minimum_protocol_version: TLSv1_0
      tls_maximum_protocol_version: TLSv1_2
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
  max_session_keys: 0
)EOF";

  testClientSessionResumption(server_ctx_yaml, client_ctx_yaml, false, GetParam());
}

// Test client session resumption with TLS 1.0-1.2.
TEST_P(SslSocketTest, ClientSessionResumptionEnabledTls12) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_params:
      tls_minimum_protocol_version: TLSv1_0
      tls_maximum_protocol_version: TLSv1_2
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_params:
      tls_minimum_protocol_version: TLSv1_0
      tls_maximum_protocol_version: TLSv1_2
  max_session_keys: 2
)EOF";

  testClientSessionResumption(server_ctx_yaml, client_ctx_yaml, true, GetParam());
}

// Make sure client session resumption is not happening with TLS 1.3 when it's disabled.
TEST_P(SslSocketTest, ClientSessionResumptionDisabledTls13) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_params:
      tls_minimum_protocol_version: TLSv1_3
      tls_maximum_protocol_version: TLSv1_3
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_params:
      tls_minimum_protocol_version: TLSv1_3
      tls_maximum_protocol_version: TLSv1_3
  max_session_keys: 0
)EOF";

  testClientSessionResumption(server_ctx_yaml, client_ctx_yaml, false, GetParam());
}

// Test client session resumption with TLS 1.3 (it's different than in older versions of TLS).
TEST_P(SslSocketTest, ClientSessionResumptionEnabledTls13) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_params:
      tls_minimum_protocol_version: TLSv1_3
      tls_maximum_protocol_version: TLSv1_3
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
)EOF";

  const std::string client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_params:
      tls_minimum_protocol_version: TLSv1_3
      tls_maximum_protocol_version: TLSv1_3
  max_session_keys: 2
)EOF";

  testClientSessionResumption(server_ctx_yaml, client_ctx_yaml, true, GetParam());
}

TEST_P(SslSocketTest, SslError) {
  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/fake_ca_cert.pem"
      verify_certificate_hash: "7B:0C:3F:0D:97:0E:FC:16:70:11:7A:0C:35:75:54:6B:17:AB:CF:20:D8:AA:A0:ED:87:08:0F:FB:60:4C:40:77"
)EOF";

  envoy::api::v2::auth::DownstreamTlsContext tls_context;
  TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml), tls_context);
  auto server_cfg = std::make_unique<ServerContextConfigImpl>(tls_context, factory_context_);
  ContextManagerImpl manager(time_system_);
  Stats::IsolatedStoreImpl server_stats_store;
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager,
                                                   server_stats_store, std::vector<std::string>{});

  Network::TcpListenSocket socket(Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr,
                                  true);
  Network::MockListenerCallbacks callbacks;
  Network::MockConnectionHandler connection_handler;
  Network::ListenerPtr listener = dispatcher_->createListener(socket, callbacks, true, false);

  Network::ClientConnectionPtr client_connection = dispatcher_->createClientConnection(
      socket.localAddress(), Network::Address::InstanceConstSharedPtr(),
      Network::Test::createRawBufferSocket(), nullptr);
  client_connection->connect();
  Buffer::OwnedImpl bad_data("bad_handshake_data");
  client_connection->write(bad_data, false);

  Network::ConnectionPtr server_connection;
  Network::MockConnectionCallbacks server_connection_callbacks;
  EXPECT_CALL(callbacks, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
            std::move(socket), server_ssl_socket_factory.createTransportSocket(nullptr));
        callbacks.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(callbacks, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection = std::move(conn);
        server_connection->addConnectionCallbacks(server_connection_callbacks);
      }));

  EXPECT_CALL(server_connection_callbacks, onEvent(Network::ConnectionEvent::RemoteClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void {
        client_connection->close(Network::ConnectionCloseType::NoFlush);
        dispatcher_->exit();
      }));

  dispatcher_->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(1UL, server_stats_store.counter("ssl.connection_error").value());
}

static TestUtilOptionsV2
createProtocolTestOptions(const envoy::api::v2::Listener& listener,
                          const envoy::api::v2::auth::UpstreamTlsContext& client_ctx,
                          Network::Address::IpVersion version, std::string protocol) {
  std::string stats = "ssl.versions." + protocol;
  TestUtilOptionsV2 options(listener, client_ctx, true, version);
  options.setExpectedServerStats(stats).setExpectedClientStats(stats);
  return options.setExpectedProtocolVersion(protocol);
}

TEST_P(SslSocketTest, ProtocolVersions) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::TlsParameters* server_params =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->mutable_tls_params();

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsParameters* client_params =
      client.mutable_common_tls_context()->mutable_tls_params();

  // Connection using defaults (client & server) succeeds, negotiating TLSv1.2.
  TestUtilOptionsV2 tls_v1_2_test_options =
      createProtocolTestOptions(listener, client, GetParam(), "TLSv1.2");
  testUtilV2(tls_v1_2_test_options);

  // Connection using defaults (client & server) succeeds, negotiating TLSv1.2,
  // even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(tls_v1_2_test_options);
  client.set_allow_renegotiation(false);

  // Connection using TLSv1.0 (client) and defaults (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  TestUtilOptionsV2 tls_v1_test_options =
      createProtocolTestOptions(listener, client, GetParam(), "TLSv1");
  testUtilV2(tls_v1_test_options);
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.1 (client) and defaults (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_1);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_1);
  TestUtilOptionsV2 tls_v1_1_test_options =
      createProtocolTestOptions(listener, client, GetParam(), "TLSv1.1");
  testUtilV2(tls_v1_1_test_options);
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.2 (client) and defaults (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_2);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_2);
  testUtilV2(tls_v1_2_test_options);
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.3 (client) and defaults (server) succeeds (non-FIPS) or fails (FIPS).
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  TestUtilOptionsV2 tls_v1_3_test_options =
      createProtocolTestOptions(listener, client, GetParam(), "TLSv1.3");
  TestUtilOptionsV2 error_test_options(listener, client, false, GetParam());
  error_test_options.setExpectedServerStats("ssl.connection_error")
      .setExpectedTransportFailureReasonContains("TLSV1_ALERT_PROTOCOL_VERSION");
#ifndef BORINGSSL_FIPS
  testUtilV2(tls_v1_3_test_options);
#else // BoringSSL FIPS
  testUtilV2(error_test_options);
#endif
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.0-1.3 (client) and defaults (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
#ifndef BORINGSSL_FIPS
  testUtilV2(tls_v1_3_test_options);
#else // BoringSSL FIPS
  testUtilV2(tls_v1_2_test_options);
#endif
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.0 (client) and TLSv1.0-1.3 (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  testUtilV2(tls_v1_test_options);
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.3 (client) and TLSv1.0-1.3 (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  testUtilV2(tls_v1_3_test_options);
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using defaults (client) and TLSv1.0 (server) succeeds.
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  testUtilV2(tls_v1_test_options);
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using defaults (client) and TLSv1.1 (server) succeeds.
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_1);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_1);
  testUtilV2(tls_v1_1_test_options);
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using defaults (client) and TLSv1.2 (server) succeeds.
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_2);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_2);
  testUtilV2(tls_v1_2_test_options);
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using defaults (client) and TLSv1.3 (server) fails.
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  testUtilV2(error_test_options);
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using defaults (client) and TLSv1.0-1.3 (server) succeeds.
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  testUtilV2(tls_v1_2_test_options);
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.0-TLSv1.3 (client) and TLSv1.0 (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  testUtilV2(tls_v1_test_options);
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();

  // Connection using TLSv1.0-TLSv1.3 (client) and TLSv1.3 (server) succeeds.
  client_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_0);
  client_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  server_params->set_tls_minimum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  server_params->set_tls_maximum_protocol_version(envoy::api::v2::auth::TlsParameters::TLSv1_3);
  testUtilV2(tls_v1_3_test_options);
  client_params->clear_tls_minimum_protocol_version();
  client_params->clear_tls_maximum_protocol_version();
  server_params->clear_tls_minimum_protocol_version();
  server_params->clear_tls_maximum_protocol_version();
}

TEST_P(SslSocketTest, ALPN) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::CommonTlsContext* server_ctx =
      filter_chain->mutable_tls_context()->mutable_common_tls_context();

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::CommonTlsContext* client_ctx = client.mutable_common_tls_context();

  // Connection using defaults (client & server) succeeds, no ALPN is negotiated.
  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options);

  // Connection using defaults (client & server) succeeds, no ALPN is negotiated,
  // even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
  client.set_allow_renegotiation(false);

  // Client connects without ALPN to a server with "test" ALPN, no ALPN is negotiated.
  server_ctx->add_alpn_protocols("test");
  testUtilV2(test_options);
  server_ctx->clear_alpn_protocols();

  // Client connects with "test" ALPN to a server without ALPN, no ALPN is negotiated.
  client_ctx->add_alpn_protocols("test");
  testUtilV2(test_options);

  client_ctx->clear_alpn_protocols();

  // Client connects with "test" ALPN to a server with "test" ALPN, "test" ALPN is negotiated.
  client_ctx->add_alpn_protocols("test");
  server_ctx->add_alpn_protocols("test");
  test_options.setExpectedALPNProtocol("test");
  testUtilV2(test_options);
  test_options.setExpectedALPNProtocol("");
  client_ctx->clear_alpn_protocols();
  server_ctx->clear_alpn_protocols();

  // Client connects with "test" ALPN to a server with "test" ALPN, "test" ALPN is negotiated,
  // even with client renegotiation.
  client.set_allow_renegotiation(true);
  client_ctx->add_alpn_protocols("test");
  server_ctx->add_alpn_protocols("test");
  test_options.setExpectedALPNProtocol("test");
  testUtilV2(test_options);
  test_options.setExpectedALPNProtocol("");
  client.set_allow_renegotiation(false);
  client_ctx->clear_alpn_protocols();
  server_ctx->clear_alpn_protocols();

  // Client connects with "test" ALPN to a server with "test2" ALPN, no ALPN is negotiated.
  client_ctx->add_alpn_protocols("test");
  server_ctx->add_alpn_protocols("test2");
  testUtilV2(test_options);
  client_ctx->clear_alpn_protocols();
  server_ctx->clear_alpn_protocols();
}

TEST_P(SslSocketTest, CipherSuites) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::TlsParameters* server_params =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->mutable_tls_params();

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsParameters* client_params =
      client.mutable_common_tls_context()->mutable_tls_params();

  // Connection using defaults (client & server) succeeds.
  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options);

  // Connection using defaults (client & server) succeeds, even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
  client.set_allow_renegotiation(false);

  // Client connects with one of the supported cipher suites, connection succeeds.
  std::string common_cipher_suite = "ECDHE-RSA-CHACHA20-POLY1305";
  client_params->add_cipher_suites(common_cipher_suite);
  server_params->add_cipher_suites(common_cipher_suite);
  server_params->add_cipher_suites("ECDHE-RSA-AES128-GCM-SHA256");
  TestUtilOptionsV2 cipher_test_options(listener, client, true, GetParam());
  cipher_test_options.setExpectedCiphersuite(common_cipher_suite);
  std::string stats = "ssl.ciphers." + common_cipher_suite;
  cipher_test_options.setExpectedServerStats(stats).setExpectedClientStats(stats);
  testUtilV2(cipher_test_options);
  client_params->clear_cipher_suites();
  server_params->clear_cipher_suites();

  // Client connects with unsupported cipher suite, connection fails.
  client_params->add_cipher_suites("ECDHE-RSA-AES128-GCM-SHA256");
  server_params->add_cipher_suites("ECDHE-RSA-CHACHA20-POLY1305");
  TestUtilOptionsV2 error_test_options(listener, client, false, GetParam());
  error_test_options.setExpectedServerStats("ssl.connection_error");
  testUtilV2(error_test_options);
  client_params->clear_cipher_suites();
  server_params->clear_cipher_suites();

  // Verify that ECDHE-RSA-CHACHA20-POLY1305 is not offered by default in FIPS builds.
  client_params->add_cipher_suites(common_cipher_suite);
#ifdef BORINGSSL_FIPS
  testUtilV2(error_test_options);
#else
  testUtilV2(cipher_test_options);
#endif
  client_params->clear_cipher_suites();
}

TEST_P(SslSocketTest, EcdhCurves) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));
  envoy::api::v2::auth::TlsParameters* server_params =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->mutable_tls_params();

  envoy::api::v2::auth::UpstreamTlsContext client;
  envoy::api::v2::auth::TlsParameters* client_params =
      client.mutable_common_tls_context()->mutable_tls_params();

  // Connection using defaults (client & server) succeeds.
  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options);

  // Connection using defaults (client & server) succeeds, even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(test_options);
  client.set_allow_renegotiation(false);

  // Client connects with one of the supported ECDH curves, connection succeeds.
  client_params->add_ecdh_curves("X25519");
  server_params->add_ecdh_curves("X25519");
  server_params->add_ecdh_curves("P-256");
  server_params->add_cipher_suites("ECDHE-RSA-AES128-GCM-SHA256");
  TestUtilOptionsV2 ecdh_curves_test_options(listener, client, true, GetParam());
  std::string stats = "ssl.curves.X25519";
  ecdh_curves_test_options.setExpectedServerStats(stats).setExpectedClientStats(stats);
  testUtilV2(ecdh_curves_test_options);
  client_params->clear_ecdh_curves();
  server_params->clear_ecdh_curves();
  server_params->clear_cipher_suites();

  // Client connects with unsupported ECDH curve, connection fails.
  client_params->add_ecdh_curves("X25519");
  server_params->add_ecdh_curves("P-256");
  server_params->add_cipher_suites("ECDHE-RSA-AES128-GCM-SHA256");

  TestUtilOptionsV2 error_test_options(listener, client, false, GetParam());
  error_test_options.setExpectedServerStats("ssl.connection_error");
  testUtilV2(error_test_options);

  client_params->clear_ecdh_curves();
  server_params->clear_ecdh_curves();
  server_params->clear_cipher_suites();

  // Verify that X25519 is not offered by default in FIPS builds.
  client_params->add_ecdh_curves("X25519");
  server_params->add_cipher_suites("ECDHE-RSA-AES128-GCM-SHA256");
#ifdef BORINGSSL_FIPS
  testUtilV2(error_test_options);
#else
  testUtilV2(ecdh_curves_test_options);
#endif
  client_params->clear_ecdh_curves();
  server_params->clear_cipher_suites();
}

TEST_P(SslSocketTest, SignatureAlgorithms) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::CertificateValidationContext* server_validation_ctx =
      filter_chain->mutable_tls_context()
          ->mutable_common_tls_context()
          ->mutable_validation_context();
  server_validation_ctx->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  // Server ECDSA certificate.
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/selfsigned_ecdsa_p256_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir "
      "}}/test/extensions/transport_sockets/tls/test_data/selfsigned_ecdsa_p256_key.pem"));

  envoy::api::v2::auth::UpstreamTlsContext client;
  // Client RSA certificate.
  envoy::api::v2::auth::TlsCertificate* client_cert =
      client.mutable_common_tls_context()->add_tls_certificates();
  client_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem"));
  client_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem"));

  // Connection using defaults (client & server) succeeds.
  TestUtilOptionsV2 algorithm_test_options(listener, client, true, GetParam());
  algorithm_test_options.setExpectedClientCertUri("spiffe://lyft.com/test-team");
  testUtilV2(algorithm_test_options);

  // Connection using defaults (client & server) succeeds, even with client renegotiation.
  client.set_allow_renegotiation(true);
  testUtilV2(algorithm_test_options);
  client.set_allow_renegotiation(false);
}

TEST_P(SslSocketTest, RevokedCertificate) {

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
      crl:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.crl"
)EOF";

  // This should fail, since the certificate has been revoked.
  const std::string revoked_client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
)EOF";

  TestUtilOptions revoked_test_options(revoked_client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(revoked_test_options.setExpectedServerStats("ssl.fail_verify_error"));

  // This should succeed, since the cert isn't revoked.
  const std::string successful_client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns2_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns2_key.pem"
)EOF";

  TestUtilOptions successful_test_options(successful_client_ctx_yaml, server_ctx_yaml, true,
                                          GetParam());
  testUtil(successful_test_options.setExpectedSerialNumber(TEST_SAN_DNS2_CERT_SERIAL));
}

TEST_P(SslSocketTest, RevokedCertificateCRLInTrustedCA) {

  const std::string server_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert_with_crl.pem"
)EOF";

  // This should fail, since the certificate has been revoked.
  const std::string revoked_client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"
)EOF";

  TestUtilOptions revoked_test_options(revoked_client_ctx_yaml, server_ctx_yaml, false, GetParam());
  testUtil(revoked_test_options.setExpectedServerStats("ssl.fail_verify_error"));

  // This should succeed, since the cert isn't revoked.
  const std::string successful_client_ctx_yaml = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns2_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns2_key.pem"
)EOF";
  TestUtilOptions successful_test_options(successful_client_ctx_yaml, server_ctx_yaml, true,
                                          GetParam());
  testUtil(successful_test_options.setExpectedSerialNumber(TEST_SAN_DNS2_CERT_SERIAL));
}

TEST_P(SslSocketTest, GetRequestedServerName) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));

  envoy::api::v2::auth::UpstreamTlsContext client;
  client.set_sni("lyft.com");

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedRequestedServerName("lyft.com"));
}

TEST_P(SslSocketTest, OverrideRequestedServerName) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));

  envoy::api::v2::auth::UpstreamTlsContext client;
  client.set_sni("lyft.com");

  Network::TransportSocketOptionsSharedPtr transport_socket_options(
      new Network::TransportSocketOptionsImpl("example.com"));

  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedRequestedServerName("example.com")
                 .setTransportSocketOptions(transport_socket_options));
}

TEST_P(SslSocketTest, OverrideRequestedServerNameWithoutSniInUpstreamTlsContext) {
  envoy::api::v2::Listener listener;
  envoy::api::v2::listener::FilterChain* filter_chain = listener.add_filter_chains();
  envoy::api::v2::auth::TlsCertificate* server_cert =
      filter_chain->mutable_tls_context()->mutable_common_tls_context()->add_tls_certificates();
  server_cert->mutable_certificate_chain()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem"));
  server_cert->mutable_private_key()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem"));

  envoy::api::v2::auth::UpstreamTlsContext client;

  Network::TransportSocketOptionsSharedPtr transport_socket_options(
      new Network::TransportSocketOptionsImpl("example.com"));
  TestUtilOptionsV2 test_options(listener, client, true, GetParam());
  testUtilV2(test_options.setExpectedRequestedServerName("example.com")
                 .setTransportSocketOptions(transport_socket_options));
}

// Validate that if downstream secrets are not yet downloaded from SDS server, Envoy creates
// NotReadySslSocket object to handle downstream connection.
TEST_P(SslSocketTest, DownstreamNotReadySslSocket) {
  Stats::IsolatedStoreImpl stats_store;
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext> factory_context;
  NiceMock<Init::MockManager> init_manager;
  NiceMock<Event::MockDispatcher> dispatcher;
  EXPECT_CALL(factory_context, dispatcher()).WillOnce(ReturnRef(dispatcher));
  EXPECT_CALL(factory_context, localInfo()).WillOnce(ReturnRef(local_info));
  EXPECT_CALL(factory_context, stats()).WillOnce(ReturnRef(stats_store));
  EXPECT_CALL(factory_context, initManager()).WillRepeatedly(Return(&init_manager));

  envoy::api::v2::auth::DownstreamTlsContext tls_context;
  auto sds_secret_configs =
      tls_context.mutable_common_tls_context()->mutable_tls_certificate_sds_secret_configs()->Add();
  sds_secret_configs->set_name("abc.com");
  sds_secret_configs->mutable_sds_config();
  auto server_cfg = std::make_unique<ServerContextConfigImpl>(tls_context, factory_context);
  EXPECT_TRUE(server_cfg->tlsCertificates().empty());
  EXPECT_FALSE(server_cfg->isReady());

  ContextManagerImpl manager(time_system_);
  ServerSslSocketFactory server_ssl_socket_factory(std::move(server_cfg), manager, stats_store,
                                                   std::vector<std::string>{});
  auto transport_socket = server_ssl_socket_factory.createTransportSocket(nullptr);
  EXPECT_EQ(EMPTY_STRING, transport_socket->protocol());
  EXPECT_EQ(nullptr, transport_socket->ssl());
  Buffer::OwnedImpl buffer;
  Network::IoResult result = transport_socket->doRead(buffer);
  EXPECT_EQ(Network::PostIoAction::Close, result.action_);
  result = transport_socket->doWrite(buffer, true);
  EXPECT_EQ(Network::PostIoAction::Close, result.action_);
  EXPECT_EQ("TLS error: Secret is not supplied by SDS", transport_socket->failureReason());
}

// Validate that if upstream secrets are not yet downloaded from SDS server, Envoy creates
// NotReadySslSocket object to handle upstream connection.
TEST_P(SslSocketTest, UpstreamNotReadySslSocket) {
  Stats::IsolatedStoreImpl stats_store;
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  testing::NiceMock<Server::Configuration::MockTransportSocketFactoryContext> factory_context;
  NiceMock<Init::MockManager> init_manager;
  NiceMock<Event::MockDispatcher> dispatcher;
  EXPECT_CALL(factory_context, localInfo()).WillOnce(ReturnRef(local_info));
  EXPECT_CALL(factory_context, stats()).WillOnce(ReturnRef(stats_store));
  EXPECT_CALL(factory_context, initManager()).WillRepeatedly(Return(&init_manager));
  EXPECT_CALL(factory_context, dispatcher()).WillOnce(ReturnRef(dispatcher));

  envoy::api::v2::auth::UpstreamTlsContext tls_context;
  auto sds_secret_configs =
      tls_context.mutable_common_tls_context()->mutable_tls_certificate_sds_secret_configs()->Add();
  sds_secret_configs->set_name("abc.com");
  sds_secret_configs->mutable_sds_config();
  auto client_cfg = std::make_unique<ClientContextConfigImpl>(tls_context, factory_context);
  EXPECT_TRUE(client_cfg->tlsCertificates().empty());
  EXPECT_FALSE(client_cfg->isReady());

  ContextManagerImpl manager(time_system_);
  ClientSslSocketFactory client_ssl_socket_factory(std::move(client_cfg), manager, stats_store);
  auto transport_socket = client_ssl_socket_factory.createTransportSocket(nullptr);
  EXPECT_EQ(EMPTY_STRING, transport_socket->protocol());
  EXPECT_EQ(nullptr, transport_socket->ssl());
  Buffer::OwnedImpl buffer;
  Network::IoResult result = transport_socket->doRead(buffer);
  EXPECT_EQ(Network::PostIoAction::Close, result.action_);
  result = transport_socket->doWrite(buffer, true);
  EXPECT_EQ(Network::PostIoAction::Close, result.action_);
  EXPECT_EQ("TLS error: Secret is not supplied by SDS", transport_socket->failureReason());
}

class SslReadBufferLimitTest : public SslSocketTest {
protected:
  void initialize() {
    TestUtility::loadFromYaml(TestEnvironment::substitute(server_ctx_yaml_),
                              downstream_tls_context_);
    auto server_cfg =
        std::make_unique<ServerContextConfigImpl>(downstream_tls_context_, factory_context_);
    manager_ = std::make_unique<ContextManagerImpl>(time_system_);
    server_ssl_socket_factory_ = std::make_unique<ServerSslSocketFactory>(
        std::move(server_cfg), *manager_, server_stats_store_, std::vector<std::string>{});

    listener_ = dispatcher_->createListener(socket_, listener_callbacks_, true, false);

    TestUtility::loadFromYaml(TestEnvironment::substitute(client_ctx_yaml_), upstream_tls_context_);
    auto client_cfg =
        std::make_unique<ClientContextConfigImpl>(upstream_tls_context_, factory_context_);

    client_ssl_socket_factory_ = std::make_unique<ClientSslSocketFactory>(
        std::move(client_cfg), *manager_, client_stats_store_);
    auto transport_socket = client_ssl_socket_factory_->createTransportSocket(nullptr);
    client_transport_socket_ = transport_socket.get();
    client_connection_ = dispatcher_->createClientConnection(
        socket_.localAddress(), source_address_, std::move(transport_socket), nullptr);
    client_connection_->addConnectionCallbacks(client_callbacks_);
    client_connection_->connect();
    read_filter_.reset(new Network::MockReadFilter());
  }

  void readBufferLimitTest(uint32_t read_buffer_limit, uint32_t expected_chunk_size,
                           uint32_t write_size, uint32_t num_writes, bool reserve_write_space) {
    initialize();

    EXPECT_CALL(listener_callbacks_, onAccept_(_, _))
        .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
          Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
              std::move(socket), server_ssl_socket_factory_->createTransportSocket(nullptr));
          new_connection->setBufferLimits(read_buffer_limit);
          listener_callbacks_.onNewConnection(std::move(new_connection));
        }));
    EXPECT_CALL(listener_callbacks_, onNewConnection_(_))
        .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
          server_connection_ = std::move(conn);
          server_connection_->addConnectionCallbacks(server_callbacks_);
          server_connection_->addReadFilter(read_filter_);
          EXPECT_EQ("", server_connection_->nextProtocol());
          EXPECT_EQ(read_buffer_limit, server_connection_->bufferLimit());
        }));

    EXPECT_CALL(client_callbacks_, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));
    dispatcher_->run(Event::Dispatcher::RunType::Block);

    uint32_t filter_seen = 0;

    EXPECT_CALL(*read_filter_, onNewConnection());
    EXPECT_CALL(*read_filter_, onData(_, _))
        .WillRepeatedly(Invoke([&](Buffer::Instance& data, bool) -> Network::FilterStatus {
          EXPECT_GE(expected_chunk_size, data.length());
          filter_seen += data.length();
          data.drain(data.length());
          if (filter_seen == (write_size * num_writes)) {
            server_connection_->close(Network::ConnectionCloseType::FlushWrite);
          }
          return Network::FilterStatus::StopIteration;
        }));

    EXPECT_CALL(client_callbacks_, onEvent(Network::ConnectionEvent::RemoteClose))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void {
          EXPECT_EQ((write_size * num_writes), filter_seen);
          dispatcher_->exit();
        }));

    for (uint32_t i = 0; i < num_writes; i++) {
      Buffer::OwnedImpl data(std::string(write_size, 'a'));

      // Incredibly contrived way of making sure that the write buffer has an empty chain in it.
      if (reserve_write_space) {
        Buffer::RawSlice iovecs[2];
        EXPECT_EQ(2UL, data.reserve(16384, iovecs, 2));
        iovecs[0].len_ = 0;
        iovecs[1].len_ = 0;
        data.commit(iovecs, 2);
      }

      client_connection_->write(data, false);
    }

    dispatcher_->run(Event::Dispatcher::RunType::Block);

    EXPECT_EQ(0UL, server_stats_store_.counter("ssl.connection_error").value());
    EXPECT_EQ(0UL, client_stats_store_.counter("ssl.connection_error").value());
  }

  void singleWriteTest(uint32_t read_buffer_limit, uint32_t bytes_to_write) {
    MockWatermarkBuffer* client_write_buffer = nullptr;
    MockBufferFactory* factory = new StrictMock<MockBufferFactory>;
    dispatcher_ = api_->allocateDispatcher(Buffer::WatermarkFactoryPtr{factory});

    // By default, expect 4 buffers to be created - the client and server read and write buffers.
    EXPECT_CALL(*factory, create_(_, _))
        .Times(2)
        .WillOnce(Invoke([&](std::function<void()> below_low,
                             std::function<void()> above_high) -> Buffer::Instance* {
          client_write_buffer = new MockWatermarkBuffer(below_low, above_high);
          return client_write_buffer;
        }))
        .WillRepeatedly(Invoke([](std::function<void()> below_low,
                                  std::function<void()> above_high) -> Buffer::Instance* {
          return new Buffer::WatermarkBuffer(below_low, above_high);
        }));

    initialize();

    EXPECT_CALL(client_callbacks_, onEvent(Network::ConnectionEvent::Connected))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));

    EXPECT_CALL(listener_callbacks_, onAccept_(_, _))
        .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
          Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
              std::move(socket), server_ssl_socket_factory_->createTransportSocket(nullptr));
          new_connection->setBufferLimits(read_buffer_limit);
          listener_callbacks_.onNewConnection(std::move(new_connection));
        }));
    EXPECT_CALL(listener_callbacks_, onNewConnection_(_))
        .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
          server_connection_ = std::move(conn);
          server_connection_->addConnectionCallbacks(server_callbacks_);
          server_connection_->addReadFilter(read_filter_);
          EXPECT_EQ("", server_connection_->nextProtocol());
          EXPECT_EQ(read_buffer_limit, server_connection_->bufferLimit());
        }));

    dispatcher_->run(Event::Dispatcher::RunType::Block);

    EXPECT_CALL(*read_filter_, onNewConnection());
    EXPECT_CALL(*read_filter_, onData(_, _)).Times(testing::AnyNumber());

    std::string data_to_write(bytes_to_write, 'a');
    Buffer::OwnedImpl buffer_to_write(data_to_write);
    std::string data_written;
    EXPECT_CALL(*client_write_buffer, move(_))
        .WillRepeatedly(DoAll(AddBufferToStringWithoutDraining(&data_written),
                              Invoke(client_write_buffer, &MockWatermarkBuffer::baseMove)));
    EXPECT_CALL(*client_write_buffer, drain(_)).WillOnce(Invoke([&](uint64_t n) -> void {
      client_write_buffer->baseDrain(n);
      dispatcher_->exit();
    }));
    client_connection_->write(buffer_to_write, false);
    dispatcher_->run(Event::Dispatcher::RunType::Block);
    EXPECT_EQ(data_to_write, data_written);

    disconnect();
  }

  void disconnect() {
    EXPECT_CALL(client_callbacks_, onEvent(Network::ConnectionEvent::LocalClose));
    EXPECT_CALL(server_callbacks_, onEvent(Network::ConnectionEvent::RemoteClose))
        .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));

    client_connection_->close(Network::ConnectionCloseType::NoFlush);
    dispatcher_->run(Event::Dispatcher::RunType::Block);
  }

  Stats::IsolatedStoreImpl server_stats_store_;
  Stats::IsolatedStoreImpl client_stats_store_;
  Network::TcpListenSocket socket_{Network::Test::getCanonicalLoopbackAddress(GetParam()), nullptr,
                                   true};
  Network::MockListenerCallbacks listener_callbacks_;
  Network::MockConnectionHandler connection_handler_;
  const std::string server_ctx_yaml_ = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_tmpdir }}/unittestcert.pem"
      private_key:
        filename: "{{ test_tmpdir }}/unittestkey.pem"
    validation_context:
      trusted_ca:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"
)EOF";

  const std::string client_ctx_yaml_ = R"EOF(
  common_tls_context:
    tls_certificates:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/no_san_key.pem"
)EOF";

  envoy::api::v2::auth::DownstreamTlsContext downstream_tls_context_;
  std::unique_ptr<ContextManagerImpl> manager_;
  Network::TransportSocketFactoryPtr server_ssl_socket_factory_;
  Network::ListenerPtr listener_;
  envoy::api::v2::auth::UpstreamTlsContext upstream_tls_context_;
  Envoy::Ssl::ClientContextSharedPtr client_ctx_;
  Network::TransportSocketFactoryPtr client_ssl_socket_factory_;
  Network::ClientConnectionPtr client_connection_;
  Network::TransportSocket* client_transport_socket_{};
  Network::ConnectionPtr server_connection_;
  NiceMock<Network::MockConnectionCallbacks> server_callbacks_;
  std::shared_ptr<Network::MockReadFilter> read_filter_;
  StrictMock<Network::MockConnectionCallbacks> client_callbacks_;
  Network::Address::InstanceConstSharedPtr source_address_;
};

INSTANTIATE_TEST_SUITE_P(IpVersions, SslReadBufferLimitTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

TEST_P(SslReadBufferLimitTest, NoLimit) {
  readBufferLimitTest(0, 256 * 1024, 256 * 1024, 1, false);
}

TEST_P(SslReadBufferLimitTest, NoLimitReserveSpace) { readBufferLimitTest(0, 512, 512, 1, true); }

TEST_P(SslReadBufferLimitTest, NoLimitSmallWrites) {
  readBufferLimitTest(0, 256 * 1024, 1, 256 * 1024, false);
}

TEST_P(SslReadBufferLimitTest, SomeLimit) {
  readBufferLimitTest(32 * 1024, 32 * 1024, 256 * 1024, 1, false);
}

TEST_P(SslReadBufferLimitTest, WritesSmallerThanBufferLimit) { singleWriteTest(5 * 1024, 1024); }

TEST_P(SslReadBufferLimitTest, WritesLargerThanBufferLimit) { singleWriteTest(1024, 5 * 1024); }

TEST_P(SslReadBufferLimitTest, TestBind) {
  std::string address_string = TestUtility::getIpv4Loopback();
  if (GetParam() == Network::Address::IpVersion::v4) {
    source_address_ = Network::Address::InstanceConstSharedPtr{
        new Network::Address::Ipv4Instance(address_string, 0)};
  } else {
    address_string = "::1";
    source_address_ = Network::Address::InstanceConstSharedPtr{
        new Network::Address::Ipv6Instance(address_string, 0)};
  }

  initialize();

  EXPECT_CALL(listener_callbacks_, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
            std::move(socket), server_ssl_socket_factory_->createTransportSocket(nullptr));
        new_connection->setBufferLimits(0);
        listener_callbacks_.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(listener_callbacks_, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection_ = std::move(conn);
        server_connection_->addConnectionCallbacks(server_callbacks_);
        server_connection_->addReadFilter(read_filter_);
        EXPECT_EQ("", server_connection_->nextProtocol());
      }));

  EXPECT_CALL(client_callbacks_, onEvent(Network::ConnectionEvent::Connected))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));
  dispatcher_->run(Event::Dispatcher::RunType::Block);

  EXPECT_EQ(address_string, server_connection_->remoteAddress()->ip()->addressAsString());

  disconnect();
}

// Regression test for https://github.com/envoyproxy/envoy/issues/6617
TEST_P(SslReadBufferLimitTest, SmallReadsIntoSameSlice) {
  // write_size * num_writes must be large enough to cause buffer reserving fragmentation,
  // but smaller than one reservation so the expected slice to be 1.
  const uint32_t write_size = 1;
  const uint32_t num_writes = 12 * 1024;
  const uint32_t read_buffer_limit = write_size * num_writes;
  const uint32_t expected_chunk_size = write_size * num_writes;

  initialize();

  EXPECT_CALL(listener_callbacks_, onAccept_(_, _))
      .WillOnce(Invoke([&](Network::ConnectionSocketPtr& socket, bool) -> void {
        Network::ConnectionPtr new_connection = dispatcher_->createServerConnection(
            std::move(socket), server_ssl_socket_factory_->createTransportSocket(nullptr));
        new_connection->setBufferLimits(read_buffer_limit);
        listener_callbacks_.onNewConnection(std::move(new_connection));
      }));
  EXPECT_CALL(listener_callbacks_, onNewConnection_(_))
      .WillOnce(Invoke([&](Network::ConnectionPtr& conn) -> void {
        server_connection_ = std::move(conn);
        server_connection_->addConnectionCallbacks(server_callbacks_);
        server_connection_->addReadFilter(read_filter_);
        EXPECT_EQ("", server_connection_->nextProtocol());
        EXPECT_EQ(read_buffer_limit, server_connection_->bufferLimit());
      }));

  EXPECT_CALL(client_callbacks_, onEvent(Network::ConnectionEvent::Connected))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void { dispatcher_->exit(); }));
  dispatcher_->run(Event::Dispatcher::RunType::Block);

  uint32_t filter_seen = 0;

  EXPECT_CALL(*read_filter_, onNewConnection());
  EXPECT_CALL(*read_filter_, onData(_, _))
      .WillRepeatedly(Invoke([&](Buffer::Instance& data, bool) -> Network::FilterStatus {
        EXPECT_GE(expected_chunk_size, data.length());
        EXPECT_EQ(1, data.getRawSlices(nullptr, 0));
        filter_seen += data.length();
        data.drain(data.length());
        if (filter_seen == (write_size * num_writes)) {
          server_connection_->close(Network::ConnectionCloseType::FlushWrite);
        }
        return Network::FilterStatus::StopIteration;
      }));

  EXPECT_CALL(client_callbacks_, onEvent(Network::ConnectionEvent::RemoteClose))
      .WillOnce(Invoke([&](Network::ConnectionEvent) -> void {
        EXPECT_EQ((write_size * num_writes), filter_seen);
        dispatcher_->exit();
      }));

  for (uint32_t i = 0; i < num_writes; i++) {
    Buffer::OwnedImpl data(std::string(write_size, 'a'));
    client_transport_socket_->doWrite(data, false);
  }

  dispatcher_->run(Event::Dispatcher::RunType::Block);
}

} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
