#include "extensions/transport_sockets/tls/context_impl.h"

#include <netinet/in.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "envoy/common/exception.h"
#include "envoy/stats/scope.h"

#include "common/common/assert.h"
#include "common/common/base64.h"
#include "common/common/fmt.h"
#include "common/common/hex.h"
#include "common/common/utility.h"
#include "common/network/address_impl.h"
#include "common/protobuf/utility.h"

#include "extensions/transport_sockets/tls/utility.h"

#include "openssl/evp.h"
#include "openssl/hmac.h"
#include "openssl/rand.h"
#include "openssl/x509v3.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {

namespace {

bool cbsContainsU16(CBS& cbs, uint16_t n) {
  while (CBS_len(&cbs) > 0) {
    uint16_t v;
    if (!CBS_get_u16(&cbs, &v)) {
      return false;
    }
    if (v == n) {
      return true;
    }
  }

  return false;
}

} // namespace

ContextImpl::ContextImpl(Stats::Scope& scope, const Envoy::Ssl::ContextConfig& config,
                         TimeSource& time_source)
    : scope_(scope), stats_(generateStats(scope)), time_source_(time_source),
      tls_max_version_(config.maxProtocolVersion()),
      stat_name_set_(scope.symbolTable().makeSet("TransportSockets::Tls")),
      unknown_ssl_cipher_(stat_name_set_->add("unknown_ssl_cipher")),
      unknown_ssl_curve_(stat_name_set_->add("unknown_ssl_curve")),
      unknown_ssl_algorithm_(stat_name_set_->add("unknown_ssl_algorithm")),
      unknown_ssl_version_(stat_name_set_->add("unknown_ssl_version")),
      ssl_ciphers_(stat_name_set_->add("ssl.ciphers")),
      ssl_versions_(stat_name_set_->add("ssl.versions")),
      ssl_curves_(stat_name_set_->add("ssl.curves")),
      ssl_sigalgs_(stat_name_set_->add("ssl.sigalgs")) {
  const auto tls_certificates = config.tlsCertificates();
  tls_contexts_.resize(std::max(static_cast<size_t>(1), tls_certificates.size()));

  for (auto& ctx : tls_contexts_) {
    ctx.ssl_ctx_.reset(SSL_CTX_new(TLS_method()));

    int rc = SSL_CTX_set_app_data(ctx.ssl_ctx_.get(), this);
    RELEASE_ASSERT(rc == 1, "");

    rc = SSL_CTX_set_min_proto_version(ctx.ssl_ctx_.get(), config.minProtocolVersion());
    RELEASE_ASSERT(rc == 1, "");

    rc = SSL_CTX_set_max_proto_version(ctx.ssl_ctx_.get(), config.maxProtocolVersion());
    RELEASE_ASSERT(rc == 1, "");

    if (!SSL_CTX_set_strict_cipher_list(ctx.ssl_ctx_.get(), config.cipherSuites().c_str())) {
      std::vector<absl::string_view> ciphers =
          StringUtil::splitToken(config.cipherSuites(), ":+-![|]", false);
      std::vector<std::string> bad_ciphers;
      for (const auto& cipher : ciphers) {
        std::string cipher_str(cipher);
        if (!SSL_CTX_set_strict_cipher_list(ctx.ssl_ctx_.get(), cipher_str.c_str())) {
          bad_ciphers.push_back(cipher_str);
        }
      }
      throw EnvoyException(fmt::format("Failed to initialize cipher suites {}. The following "
                                       "ciphers were rejected when tried individually: {}",
                                       config.cipherSuites(), StringUtil::join(bad_ciphers, ", ")));
    }

    if (!SSL_CTX_set1_curves_list(ctx.ssl_ctx_.get(), config.ecdhCurves().c_str())) {
      throw EnvoyException(fmt::format("Failed to initialize ECDH curves {}", config.ecdhCurves()));
    }
  }

  int verify_mode = SSL_VERIFY_NONE;
  if (config.certificateValidationContext() != nullptr &&
      !config.certificateValidationContext()->caCert().empty()) {
    ca_file_path_ = config.certificateValidationContext()->caCertPath();
    bssl::UniquePtr<BIO> bio(
        BIO_new_mem_buf(const_cast<char*>(config.certificateValidationContext()->caCert().data()),
                        config.certificateValidationContext()->caCert().size()));
    RELEASE_ASSERT(bio != nullptr, "");
    // Based on BoringSSL's X509_load_cert_crl_file().
    bssl::UniquePtr<STACK_OF(X509_INFO)> list(
        PEM_X509_INFO_read_bio(bio.get(), nullptr, nullptr, nullptr));
    if (list == nullptr) {
      throw EnvoyException(fmt::format("Failed to load trusted CA certificates from {}",
                                       config.certificateValidationContext()->caCertPath()));
    }

    for (auto& ctx : tls_contexts_) {
      X509_STORE* store = SSL_CTX_get_cert_store(ctx.ssl_ctx_.get());
      bool has_crl = false;
      for (const X509_INFO* item : list.get()) {
        if (item->x509) {
          X509_STORE_add_cert(store, item->x509);
          if (ca_cert_ == nullptr) {
            X509_up_ref(item->x509);
            ca_cert_.reset(item->x509);
          }
        }
        if (item->crl) {
          X509_STORE_add_crl(store, item->crl);
          has_crl = true;
        }
      }
      if (ca_cert_ == nullptr) {
        throw EnvoyException(fmt::format("Failed to load trusted CA certificates from {}",
                                         config.certificateValidationContext()->caCertPath()));
      }
      if (has_crl) {
        X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
      }
      verify_mode = SSL_VERIFY_PEER;
      verify_trusted_ca_ = true;

      // NOTE: We're using SSL_CTX_set_cert_verify_callback() instead of X509_verify_cert()
      // directly. However, our new callback is still calling X509_verify_cert() under
      // the hood. Therefore, to ignore cert expiration, we need to set the callback
      // for X509_verify_cert to ignore that error.
      if (config.certificateValidationContext()->allowExpiredCertificate()) {
        X509_STORE_set_verify_cb(store, ContextImpl::ignoreCertificateExpirationCallback);
      }
    }
  }

  if (config.certificateValidationContext() != nullptr &&
      !config.certificateValidationContext()->certificateRevocationList().empty()) {
    bssl::UniquePtr<BIO> bio(BIO_new_mem_buf(
        const_cast<char*>(
            config.certificateValidationContext()->certificateRevocationList().data()),
        config.certificateValidationContext()->certificateRevocationList().size()));
    RELEASE_ASSERT(bio != nullptr, "");

    // Based on BoringSSL's X509_load_cert_crl_file().
    bssl::UniquePtr<STACK_OF(X509_INFO)> list(
        PEM_X509_INFO_read_bio(bio.get(), nullptr, nullptr, nullptr));
    if (list == nullptr) {
      throw EnvoyException(
          fmt::format("Failed to load CRL from {}",
                      config.certificateValidationContext()->certificateRevocationListPath()));
    }

    for (auto& ctx : tls_contexts_) {
      X509_STORE* store = SSL_CTX_get_cert_store(ctx.ssl_ctx_.get());
      for (const X509_INFO* item : list.get()) {
        if (item->crl) {
          X509_STORE_add_crl(store, item->crl);
        }
      }

      X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    }
  }

  if (config.certificateValidationContext() != nullptr &&
      !config.certificateValidationContext()->verifySubjectAltNameList().empty()) {
    verify_subject_alt_name_list_ =
        config.certificateValidationContext()->verifySubjectAltNameList();
    verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  }

  if (config.certificateValidationContext() != nullptr &&
      !config.certificateValidationContext()->verifyCertificateHashList().empty()) {
    for (auto hash : config.certificateValidationContext()->verifyCertificateHashList()) {
      // Remove colons from the 95 chars long colon-separated "fingerprint"
      // in order to get the hex-encoded string.
      if (hash.size() == 95) {
        hash.erase(std::remove(hash.begin(), hash.end(), ':'), hash.end());
      }
      const auto& decoded = Hex::decode(hash);
      if (decoded.size() != SHA256_DIGEST_LENGTH) {
        throw EnvoyException(fmt::format("Invalid hex-encoded SHA-256 {}", hash));
      }
      verify_certificate_hash_list_.push_back(decoded);
    }
    verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  }

  if (config.certificateValidationContext() != nullptr &&
      !config.certificateValidationContext()->verifyCertificateSpkiList().empty()) {
    for (const auto& hash : config.certificateValidationContext()->verifyCertificateSpkiList()) {
      const auto decoded = Base64::decode(hash);
      if (decoded.size() != SHA256_DIGEST_LENGTH) {
        throw EnvoyException(fmt::format("Invalid base64-encoded SHA-256 {}", hash));
      }
      verify_certificate_spki_list_.emplace_back(decoded.begin(), decoded.end());
    }
    verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  }

  for (auto& ctx : tls_contexts_) {
    if (verify_mode != SSL_VERIFY_NONE) {
      SSL_CTX_set_verify(ctx.ssl_ctx_.get(), verify_mode, nullptr);
      SSL_CTX_set_cert_verify_callback(ctx.ssl_ctx_.get(), ContextImpl::verifyCallback, this);
    }
  }

  std::unordered_set<int> cert_pkey_ids;
  for (uint32_t i = 0; i < tls_certificates.size(); ++i) {
    auto& ctx = tls_contexts_[i];
    // Load certificate chain.
    const auto& tls_certificate = tls_certificates[i].get();
    ctx.cert_chain_file_path_ = tls_certificate.certificateChainPath();
    bssl::UniquePtr<BIO> bio(
        BIO_new_mem_buf(const_cast<char*>(tls_certificate.certificateChain().data()),
                        tls_certificate.certificateChain().size()));
    RELEASE_ASSERT(bio != nullptr, "");
    ctx.cert_chain_.reset(PEM_read_bio_X509_AUX(bio.get(), nullptr, nullptr, nullptr));
    if (ctx.cert_chain_ == nullptr ||
        !SSL_CTX_use_certificate(ctx.ssl_ctx_.get(), ctx.cert_chain_.get())) {
      while (uint64_t err = ERR_get_error()) {
        ENVOY_LOG_MISC(debug, "SSL error: {}:{}:{}:{}", err, ERR_lib_error_string(err),
                       ERR_func_error_string(err), ERR_GET_REASON(err),
                       ERR_reason_error_string(err));
      }
      throw EnvoyException(
          fmt::format("Failed to load certificate chain from {}", ctx.cert_chain_file_path_));
    }
    // Read rest of the certificate chain.
    while (true) {
      bssl::UniquePtr<X509> cert(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
      if (cert == nullptr) {
        break;
      }
      if (!SSL_CTX_add_extra_chain_cert(ctx.ssl_ctx_.get(), cert.get())) {
        throw EnvoyException(
            fmt::format("Failed to load certificate chain from {}", ctx.cert_chain_file_path_));
      }
      // SSL_CTX_add_extra_chain_cert() takes ownership.
      cert.release();
    }
    // Check for EOF.
    uint32_t err = ERR_peek_last_error();
    if (ERR_GET_LIB(err) == ERR_LIB_PEM && ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
      ERR_clear_error();
    } else {
      throw EnvoyException(
          fmt::format("Failed to load certificate chain from {}", ctx.cert_chain_file_path_));
    }

    bssl::UniquePtr<EVP_PKEY> public_key(X509_get_pubkey(ctx.cert_chain_.get()));
    const int pkey_id = EVP_PKEY_id(public_key.get());
    if (!cert_pkey_ids.insert(pkey_id).second) {
      throw EnvoyException(fmt::format("Failed to load certificate chain from {}, at most one "
                                       "certificate of a given type may be specified",
                                       ctx.cert_chain_file_path_));
    }
    ctx.is_ecdsa_ = pkey_id == EVP_PKEY_EC;
    switch (pkey_id) {
    case EVP_PKEY_EC: {
      // We only support P-256 ECDSA today.
      const EC_KEY* ecdsa_public_key = EVP_PKEY_get0_EC_KEY(public_key.get());
      // Since we checked the key type above, this should be valid.
      ASSERT(ecdsa_public_key != nullptr);
      const EC_GROUP* ecdsa_group = EC_KEY_get0_group(ecdsa_public_key);
      if (ecdsa_group == nullptr || EC_GROUP_get_curve_name(ecdsa_group) != NID_X9_62_prime256v1) {
        throw EnvoyException(fmt::format("Failed to load certificate chain from {}, only P-256 "
                                         "ECDSA certificates are supported",
                                         ctx.cert_chain_file_path_));
      }
      ctx.is_ecdsa_ = true;
    } break;
    case EVP_PKEY_RSA: {
      // We require RSA certificates with 2048-bit or larger keys.
      const RSA* rsa_public_key = EVP_PKEY_get0_RSA(public_key.get());
      // Since we checked the key type above, this should be valid.
      ASSERT(rsa_public_key != nullptr);
      const unsigned rsa_key_length = RSA_size(rsa_public_key);
#ifdef BORINGSSL_FIPS
      if (rsa_key_length != 2048 / 8 && rsa_key_length != 3072 / 8) {
        throw EnvoyException(
            fmt::format("Failed to load certificate chain from {}, only RSA certificates with "
                        "2048-bit or 3072-bit keys are supported in FIPS mode",
                        ctx.cert_chain_file_path_));
      }
#else
      if (rsa_key_length < 2048 / 8) {
        throw EnvoyException(fmt::format("Failed to load certificate chain from {}, only RSA "
                                         "certificates with 2048-bit or larger keys are supported",
                                         ctx.cert_chain_file_path_));
      }
#endif
    } break;
#ifdef BORINGSSL_FIPS
    default:
      throw EnvoyException(fmt::format("Failed to load certificate chain from {}, only RSA and "
                                       "ECDSA certificates are supported in FIPS mode",
                                       ctx.cert_chain_file_path_));
#endif
    }

    Envoy::Ssl::PrivateKeyMethodProviderSharedPtr private_key_method_provider =
        tls_certificate.privateKeyMethod();
    // We either have a private key or a BoringSSL private key method provider.
    if (private_key_method_provider) {
      ctx.private_key_method_provider_ = private_key_method_provider;
      // The provider has a reference to the private key method for the context lifetime.
      Ssl::BoringSslPrivateKeyMethodSharedPtr private_key_method =
          private_key_method_provider->getBoringSslPrivateKeyMethod();
      if (private_key_method == nullptr) {
        throw EnvoyException(
            fmt::format("Failed to get BoringSSL private key method from provider"));
      }
#ifdef BORINGSSL_FIPS
      if (!ctx.private_key_method_provider_->checkFips()) {
        throw EnvoyException(
            fmt::format("Private key method doesn't support FIPS mode with current parameters"));
      }
#endif
      SSL_CTX_set_private_key_method(ctx.ssl_ctx_.get(), private_key_method.get());
    } else {
      // Load private key.
      bio.reset(BIO_new_mem_buf(const_cast<char*>(tls_certificate.privateKey().data()),
                                tls_certificate.privateKey().size()));
      RELEASE_ASSERT(bio != nullptr, "");
      bssl::UniquePtr<EVP_PKEY> pkey(
          PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr,
                                  !tls_certificate.password().empty()
                                      ? const_cast<char*>(tls_certificate.password().c_str())
                                      : nullptr));
      if (pkey == nullptr || !SSL_CTX_use_PrivateKey(ctx.ssl_ctx_.get(), pkey.get())) {
        throw EnvoyException(
            fmt::format("Failed to load private key from {}", tls_certificate.privateKeyPath()));
      }

#ifdef BORINGSSL_FIPS
      // Verify that private keys are passing FIPS pairwise consistency tests.
      switch (pkey_id) {
      case EVP_PKEY_EC: {
        const EC_KEY* ecdsa_private_key = EVP_PKEY_get0_EC_KEY(pkey.get());
        if (!EC_KEY_check_fips(ecdsa_private_key)) {
          throw EnvoyException(fmt::format("Failed to load private key from {}, ECDSA key failed "
                                           "pairwise consistency test required in FIPS mode",
                                           tls_certificate.privateKeyPath()));
        }
      } break;
      case EVP_PKEY_RSA: {
        RSA* rsa_private_key = EVP_PKEY_get0_RSA(pkey.get());
        if (!RSA_check_fips(rsa_private_key)) {
          throw EnvoyException(fmt::format("Failed to load private key from {}, RSA key failed "
                                           "pairwise consistency test required in FIPS mode",
                                           tls_certificate.privateKeyPath()));
        }
      } break;
      }
#endif
    }
  }

  // use the server's cipher list preferences
  for (auto& ctx : tls_contexts_) {
    SSL_CTX_set_options(ctx.ssl_ctx_.get(), SSL_OP_CIPHER_SERVER_PREFERENCE);
  }

  parsed_alpn_protocols_ = parseAlpnProtocols(config.alpnProtocols());

  // To enumerate the required builtin ciphers, curves, algorithms, and
  // versions, uncomment '#define LOG_BUILTIN_STAT_NAMES' below, and run
  //  bazel test //test/extensions/transport_sockets/tls/... --test_output=streamed
  //      | grep " Builtin ssl." | sort | uniq
  // #define LOG_BUILTIN_STAT_NAMES
  //
  // TODO(#8035): improve tooling to find any other built-ins needed to avoid
  // contention.

  // Ciphers
  stat_name_set_->rememberBuiltin("AEAD-AES128-GCM-SHA256");
  stat_name_set_->rememberBuiltin("ECDHE-ECDSA-AES128-GCM-SHA256");
  stat_name_set_->rememberBuiltin("ECDHE-RSA-AES128-GCM-SHA256");
  stat_name_set_->rememberBuiltin("ECDHE-RSA-AES128-SHA");
  stat_name_set_->rememberBuiltin("ECDHE-RSA-CHACHA20-POLY1305");
  stat_name_set_->rememberBuiltin("TLS_AES_128_GCM_SHA256");

  // Curves from
  // https://github.com/google/boringssl/blob/f4d8b969200f1ee2dd872ffb85802e6a0976afe7/ssl/ssl_key_share.cc#L384
  stat_name_set_->rememberBuiltins(
      {"P-224", "P-256", "P-384", "P-521", "X25519", "CECPQ2", "CECPQ2b"});

  // Algorithms
  stat_name_set_->rememberBuiltins({"ecdsa_secp256r1_sha256", "rsa_pss_rsae_sha256"});

  // Versions
  stat_name_set_->rememberBuiltins({"TLSv1", "TLSv1.1", "TLSv1.2", "TLSv1.3"});
}

int ServerContextImpl::alpnSelectCallback(const unsigned char** out, unsigned char* outlen,
                                          const unsigned char* in, unsigned int inlen) {
  // Currently this uses the standard selection algorithm in priority order.
  const uint8_t* alpn_data = &parsed_alpn_protocols_[0];
  size_t alpn_data_size = parsed_alpn_protocols_.size();

  if (SSL_select_next_proto(const_cast<unsigned char**>(out), outlen, alpn_data, alpn_data_size, in,
                            inlen) != OPENSSL_NPN_NEGOTIATED) {
    return SSL_TLSEXT_ERR_NOACK;
  } else {
    return SSL_TLSEXT_ERR_OK;
  }
}

std::vector<uint8_t> ContextImpl::parseAlpnProtocols(const std::string& alpn_protocols) {
  if (alpn_protocols.empty()) {
    return {};
  }

  if (alpn_protocols.size() >= 65535) {
    throw EnvoyException("invalid ALPN protocol string");
  }

  std::vector<uint8_t> out(alpn_protocols.size() + 1);
  size_t start = 0;
  for (size_t i = 0; i <= alpn_protocols.size(); i++) {
    if (i == alpn_protocols.size() || alpn_protocols[i] == ',') {
      if (i - start > 255) {
        throw EnvoyException("invalid ALPN protocol string");
      }

      out[start] = i - start;
      start = i + 1;
    } else {
      out[i + 1] = alpn_protocols[i];
    }
  }

  return out;
}

bssl::UniquePtr<SSL> ContextImpl::newSsl(const Network::TransportSocketOptions*) {
  // We use the first certificate for a new SSL object, later in the
  // SSL_CTX_set_select_certificate_cb() callback following ClientHello, we replace with the
  // selected certificate via SSL_set_SSL_CTX().
  return bssl::UniquePtr<SSL>(SSL_new(tls_contexts_[0].ssl_ctx_.get()));
}

int ContextImpl::ignoreCertificateExpirationCallback(int ok, X509_STORE_CTX* ctx) {
  if (!ok) {
    int err = X509_STORE_CTX_get_error(ctx);
    if (err == X509_V_ERR_CERT_HAS_EXPIRED || err == X509_V_ERR_CERT_NOT_YET_VALID) {
      return 1;
    }
  }

  return ok;
}

int ContextImpl::verifyCallback(X509_STORE_CTX* store_ctx, void* arg) {
  ContextImpl* impl = reinterpret_cast<ContextImpl*>(arg);

  if (impl->verify_trusted_ca_) {
    int ret = X509_verify_cert(store_ctx);
    if (ret <= 0) {
      impl->stats_.fail_verify_error_.inc();
      return ret;
    }
  }

  SSL* ssl = reinterpret_cast<SSL*>(
      X509_STORE_CTX_get_ex_data(store_ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));
  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl));

  const Network::TransportSocketOptions* transport_socket_options =
      static_cast<const Network::TransportSocketOptions*>(SSL_get_app_data(ssl));
  return impl->verifyCertificate(
      cert.get(), transport_socket_options &&
                          !transport_socket_options->verifySubjectAltNameListOverride().empty()
                      ? transport_socket_options->verifySubjectAltNameListOverride()
                      : impl->verify_subject_alt_name_list_);
}

int ContextImpl::verifyCertificate(X509* cert, const std::vector<std::string>& verify_san_list) {
  if (!verify_san_list.empty() && !verifySubjectAltName(cert, verify_san_list)) {
    stats_.fail_verify_san_.inc();
    return 0;
  }

  if (!verify_certificate_hash_list_.empty() || !verify_certificate_spki_list_.empty()) {
    const bool valid_certificate_hash =
        !verify_certificate_hash_list_.empty() &&
        verifyCertificateHashList(cert, verify_certificate_hash_list_);
    const bool valid_certificate_spki =
        !verify_certificate_spki_list_.empty() &&
        verifyCertificateSpkiList(cert, verify_certificate_spki_list_);

    if (!valid_certificate_hash && !valid_certificate_spki) {
      stats_.fail_verify_cert_hash_.inc();
      return 0;
    }
  }

  return 1;
}

void ContextImpl::incCounter(const Stats::StatName name, absl::string_view value,
                             const Stats::StatName fallback) const {
  Stats::SymbolTable& symbol_table = scope_.symbolTable();
  Stats::SymbolTable::StoragePtr storage =
      symbol_table.join({name, stat_name_set_->getBuiltin(value, fallback)});
  scope_.counterFromStatName(Stats::StatName(storage.get())).inc();

#ifdef LOG_BUILTIN_STAT_NAMES
  std::cerr << absl::StrCat("Builtin ", symbol_table.toString(name), ": ", value, "\n")
            << std::flush;
#endif
}

void ContextImpl::logHandshake(SSL* ssl) const {
  stats_.handshake_.inc();

  if (SSL_session_reused(ssl)) {
    stats_.session_reused_.inc();
  }

  incCounter(ssl_ciphers_, SSL_get_cipher_name(ssl), unknown_ssl_cipher_);
  incCounter(ssl_versions_, SSL_get_version(ssl), unknown_ssl_version_);

  uint16_t curve_id = SSL_get_curve_id(ssl);
  if (curve_id) {
    incCounter(ssl_curves_, SSL_get_curve_name(curve_id), unknown_ssl_curve_);
  }

  uint16_t sigalg_id = SSL_get_peer_signature_algorithm(ssl);
  if (sigalg_id) {
    const char* sigalg = SSL_get_signature_algorithm_name(sigalg_id, 1 /* include curve */);
    incCounter(ssl_sigalgs_, sigalg, unknown_ssl_algorithm_);
  }

  bssl::UniquePtr<X509> cert(SSL_get_peer_certificate(ssl));
  if (!cert.get()) {
    stats_.no_certificate_.inc();
  }
}

std::vector<Ssl::PrivateKeyMethodProviderSharedPtr> ContextImpl::getPrivateKeyMethodProviders() {
  std::vector<Envoy::Ssl::PrivateKeyMethodProviderSharedPtr> providers;

  for (auto& tls_context : tls_contexts_) {
    Envoy::Ssl::PrivateKeyMethodProviderSharedPtr provider =
        tls_context.getPrivateKeyMethodProvider();
    if (provider) {
      providers.push_back(provider);
    }
  }
  return providers;
}

bool ContextImpl::verifySubjectAltName(X509* cert,
                                       const std::vector<std::string>& subject_alt_names) {
  bssl::UniquePtr<GENERAL_NAMES> san_names(
      static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr)));
  if (san_names == nullptr) {
    return false;
  }
  for (const GENERAL_NAME* san : san_names.get()) {
    switch (san->type) {
    case GEN_DNS: {
      ASN1_STRING* str = san->d.dNSName;
      const char* dns_name = reinterpret_cast<const char*>(ASN1_STRING_data(str));
      for (auto& config_san : subject_alt_names) {
        if (dnsNameMatch(config_san, dns_name)) {
          return true;
        }
      }
      break;
    }
    case GEN_URI: {
      ASN1_STRING* str = san->d.uniformResourceIdentifier;
      const char* uri = reinterpret_cast<const char*>(ASN1_STRING_data(str));
      for (auto& config_san : subject_alt_names) {
        if (config_san.compare(uri) == 0) {
          return true;
        }
      }
      break;
    }
    case GEN_IPADD: {
      if (san->d.ip->length == 4) {
        sockaddr_in sin;
        sin.sin_port = 0;
        sin.sin_family = AF_INET;
        memcpy(&sin.sin_addr, san->d.ip->data, sizeof(sin.sin_addr));
        Network::Address::Ipv4Instance addr(&sin);
        for (auto& config_san : subject_alt_names) {
          if (config_san == addr.ip()->addressAsString()) {
            return true;
          }
        }
      } else if (san->d.ip->length == 16) {
        sockaddr_in6 sin6;
        sin6.sin6_port = 0;
        sin6.sin6_family = AF_INET6;
        memcpy(&sin6.sin6_addr, san->d.ip->data, sizeof(sin6.sin6_addr));
        Network::Address::Ipv6Instance addr(sin6);
        for (auto& config_san : subject_alt_names) {
          if (config_san == addr.ip()->addressAsString()) {
            return true;
          }
        }
      }
      break;
    }
    }
  }
  return false;
}

bool ContextImpl::dnsNameMatch(const std::string& dns_name, const char* pattern) {
  if (dns_name == pattern) {
    return true;
  }

  size_t pattern_len = strlen(pattern);
  if (pattern_len > 1 && pattern[0] == '*' && pattern[1] == '.') {
    if (dns_name.length() > pattern_len - 1) {
      size_t off = dns_name.length() - pattern_len + 1;
      return dns_name.compare(off, pattern_len - 1, pattern + 1) == 0;
    }
  }

  return false;
}

bool ContextImpl::verifyCertificateHashList(
    X509* cert, const std::vector<std::vector<uint8_t>>& expected_hashes) {
  std::vector<uint8_t> computed_hash(SHA256_DIGEST_LENGTH);
  unsigned int n;
  X509_digest(cert, EVP_sha256(), computed_hash.data(), &n);
  RELEASE_ASSERT(n == computed_hash.size(), "");

  for (const auto& expected_hash : expected_hashes) {
    if (computed_hash == expected_hash) {
      return true;
    }
  }
  return false;
}

bool ContextImpl::verifyCertificateSpkiList(
    X509* cert, const std::vector<std::vector<uint8_t>>& expected_hashes) {
  X509_PUBKEY* pubkey = X509_get_X509_PUBKEY(cert);
  if (pubkey == nullptr) {
    return false;
  }
  uint8_t* spki = nullptr;
  const int len = i2d_X509_PUBKEY(pubkey, &spki);
  if (len < 0) {
    return false;
  }
  bssl::UniquePtr<uint8_t> free_spki(spki);

  std::vector<uint8_t> computed_hash(SHA256_DIGEST_LENGTH);
  SHA256(spki, len, computed_hash.data());

  for (const auto& expected_hash : expected_hashes) {
    if (computed_hash == expected_hash) {
      return true;
    }
  }
  return false;
}

SslStats ContextImpl::generateStats(Stats::Scope& store) {
  std::string prefix("ssl.");
  return {ALL_SSL_STATS(POOL_COUNTER_PREFIX(store, prefix), POOL_GAUGE_PREFIX(store, prefix),
                        POOL_HISTOGRAM_PREFIX(store, prefix))};
}

size_t ContextImpl::daysUntilFirstCertExpires() const {
  int daysUntilExpiration = Utility::getDaysUntilExpiration(ca_cert_.get(), time_source_);
  for (auto& ctx : tls_contexts_) {
    daysUntilExpiration = std::min<int>(
        Utility::getDaysUntilExpiration(ctx.cert_chain_.get(), time_source_), daysUntilExpiration);
  }
  if (daysUntilExpiration < 0) { // Ensure that the return value is unsigned
    return 0;
  }
  return daysUntilExpiration;
}

Envoy::Ssl::CertificateDetailsPtr ContextImpl::getCaCertInformation() const {
  if (ca_cert_ == nullptr) {
    return nullptr;
  }
  return certificateDetails(ca_cert_.get(), getCaFileName());
}

std::vector<Envoy::Ssl::CertificateDetailsPtr> ContextImpl::getCertChainInformation() const {
  std::vector<Envoy::Ssl::CertificateDetailsPtr> cert_details;
  for (const auto& ctx : tls_contexts_) {
    if (ctx.cert_chain_ == nullptr) {
      continue;
    }
    cert_details.emplace_back(
        certificateDetails(ctx.cert_chain_.get(), ctx.getCertChainFileName()));
  }
  return cert_details;
}

Envoy::Ssl::CertificateDetailsPtr ContextImpl::certificateDetails(X509* cert,
                                                                  const std::string& path) const {
  Envoy::Ssl::CertificateDetailsPtr certificate_details =
      std::make_unique<envoy::admin::v2alpha::CertificateDetails>();
  certificate_details->set_path(path);
  certificate_details->set_serial_number(Utility::getSerialNumberFromCertificate(*cert));
  certificate_details->set_days_until_expiration(
      Utility::getDaysUntilExpiration(cert, time_source_));
  ProtobufWkt::Timestamp* valid_from = certificate_details->mutable_valid_from();
  TimestampUtil::systemClockToTimestamp(Utility::getValidFrom(*cert), *valid_from);
  ProtobufWkt::Timestamp* expiration_time = certificate_details->mutable_expiration_time();
  TimestampUtil::systemClockToTimestamp(Utility::getExpirationTime(*cert), *expiration_time);

  for (auto& dns_san : Utility::getSubjectAltNames(*cert, GEN_DNS)) {
    envoy::admin::v2alpha::SubjectAlternateName& subject_alt_name =
        *certificate_details->add_subject_alt_names();
    subject_alt_name.set_dns(dns_san);
  }
  for (auto& uri_san : Utility::getSubjectAltNames(*cert, GEN_URI)) {
    envoy::admin::v2alpha::SubjectAlternateName& subject_alt_name =
        *certificate_details->add_subject_alt_names();
    subject_alt_name.set_uri(uri_san);
  }
  return certificate_details;
}

ClientContextImpl::ClientContextImpl(Stats::Scope& scope,
                                     const Envoy::Ssl::ClientContextConfig& config,
                                     TimeSource& time_source)
    : ContextImpl(scope, config, time_source),
      server_name_indication_(config.serverNameIndication()),
      allow_renegotiation_(config.allowRenegotiation()),
      max_session_keys_(config.maxSessionKeys()) {
  // This should be guaranteed during configuration ingestion for client contexts.
  ASSERT(tls_contexts_.size() == 1);
  if (!parsed_alpn_protocols_.empty()) {
    for (auto& ctx : tls_contexts_) {
      int rc = SSL_CTX_set_alpn_protos(ctx.ssl_ctx_.get(), &parsed_alpn_protocols_[0],
                                       parsed_alpn_protocols_.size());
      RELEASE_ASSERT(rc == 0, "");
    }
  }

  if (!config.signingAlgorithmsForTest().empty()) {
    const uint16_t sigalgs = parseSigningAlgorithmsForTest(config.signingAlgorithmsForTest());
    RELEASE_ASSERT(sigalgs != 0, "");

    for (auto& ctx : tls_contexts_) {
      int rc = SSL_CTX_set_verify_algorithm_prefs(ctx.ssl_ctx_.get(), &sigalgs, 1);
      RELEASE_ASSERT(rc == 1, "");
    }
  }

  if (max_session_keys_ > 0) {
    SSL_CTX_set_session_cache_mode(tls_contexts_[0].ssl_ctx_.get(), SSL_SESS_CACHE_CLIENT);
    SSL_CTX_sess_set_new_cb(
        tls_contexts_[0].ssl_ctx_.get(), [](SSL* ssl, SSL_SESSION* session) -> int {
          ContextImpl* context_impl =
              static_cast<ContextImpl*>(SSL_CTX_get_app_data(SSL_get_SSL_CTX(ssl)));
          ClientContextImpl* client_context_impl = dynamic_cast<ClientContextImpl*>(context_impl);
          RELEASE_ASSERT(client_context_impl != nullptr, ""); // for Coverity
          return client_context_impl->newSessionKey(session);
        });
  }
}

bssl::UniquePtr<SSL> ClientContextImpl::newSsl(const Network::TransportSocketOptions* options) {
  bssl::UniquePtr<SSL> ssl_con(ContextImpl::newSsl(options));

  const std::string server_name_indication = options && options->serverNameOverride().has_value()
                                                 ? options->serverNameOverride().value()
                                                 : server_name_indication_;

  if (!server_name_indication.empty()) {
    int rc = SSL_set_tlsext_host_name(ssl_con.get(), server_name_indication.c_str());
    RELEASE_ASSERT(rc, "");
  }

  if (options && !options->verifySubjectAltNameListOverride().empty()) {
    SSL_set_app_data(ssl_con.get(), options);
    SSL_set_verify(ssl_con.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
  }

  if (options && !options->applicationProtocolListOverride().empty()) {
    std::vector<uint8_t> parsed_override_alpn =
        parseAlpnProtocols(absl::StrJoin(options->applicationProtocolListOverride(), ","));
    if (!parsed_override_alpn.empty()) {
      int rc = SSL_set_alpn_protos(ssl_con.get(), parsed_override_alpn.data(),
                                   parsed_override_alpn.size());
      RELEASE_ASSERT(rc == 0, "");
    }
  }

  if (allow_renegotiation_) {
    SSL_set_renegotiate_mode(ssl_con.get(), ssl_renegotiate_freely);
  }

  if (max_session_keys_ > 0) {
    if (session_keys_single_use_) {
      // Stored single-use session keys, use write/write locks.
      absl::WriterMutexLock l(&session_keys_mu_);
      if (!session_keys_.empty()) {
        // Use the most recently stored session key, since it has the highest
        // probability of still being recognized/accepted by the server.
        SSL_SESSION* session = session_keys_.front().get();
        SSL_set_session(ssl_con.get(), session);
        // Remove single-use session key (TLS 1.3) after first use.
        if (SSL_SESSION_should_be_single_use(session)) {
          session_keys_.pop_front();
        }
      }
    } else {
      // Never stored single-use session keys, use read/write locks.
      absl::ReaderMutexLock l(&session_keys_mu_);
      if (!session_keys_.empty()) {
        // Use the most recently stored session key, since it has the highest
        // probability of still being recognized/accepted by the server.
        SSL_SESSION* session = session_keys_.front().get();
        SSL_set_session(ssl_con.get(), session);
      }
    }
  }

  return ssl_con;
}

int ClientContextImpl::newSessionKey(SSL_SESSION* session) {
  // In case we ever store single-use session key (TLS 1.3),
  // we need to switch to using write/write locks.
  if (SSL_SESSION_should_be_single_use(session)) {
    session_keys_single_use_ = true;
  }
  absl::WriterMutexLock l(&session_keys_mu_);
  // Evict oldest entries.
  while (session_keys_.size() >= max_session_keys_) {
    session_keys_.pop_back();
  }
  // Add new session key at the front of the queue, so that it's used first.
  session_keys_.push_front(bssl::UniquePtr<SSL_SESSION>(session));
  return 1; // Tell BoringSSL that we took ownership of the session.
}

uint16_t ClientContextImpl::parseSigningAlgorithmsForTest(const std::string& sigalgs) {
  // This is used only when testing RSA/ECDSA certificate selection, so only the signing algorithms
  // used in tests are supported here.
  if (sigalgs == "rsa_pss_rsae_sha256") {
    return SSL_SIGN_RSA_PSS_RSAE_SHA256;
  } else if (sigalgs == "ecdsa_secp256r1_sha256") {
    return SSL_SIGN_ECDSA_SECP256R1_SHA256;
  }
  return 0;
}

ServerContextImpl::ServerContextImpl(Stats::Scope& scope,
                                     const Envoy::Ssl::ServerContextConfig& config,
                                     const std::vector<std::string>& server_names,
                                     TimeSource& time_source)
    : ContextImpl(scope, config, time_source), session_ticket_keys_(config.sessionTicketKeys()) {
  if (config.tlsCertificates().empty()) {
    throw EnvoyException("Server TlsCertificates must have a certificate specified");
  }
  // First, configure the base context for ClientHello interception.
  // TODO(htuch): replace with SSL_IDENTITY when we have this as a means to do multi-cert in
  // BoringSSL.
  SSL_CTX_set_select_certificate_cb(
      tls_contexts_[0].ssl_ctx_.get(),
      [](const SSL_CLIENT_HELLO* client_hello) -> ssl_select_cert_result_t {
        return static_cast<ServerContextImpl*>(
                   SSL_CTX_get_app_data(SSL_get_SSL_CTX(client_hello->ssl)))
            ->selectTlsContext(client_hello);
      });
  // Compute the session context ID hash. We use all the certificate identities,
  // since we should have a common ID for session resumption no matter what cert
  // is used.
  uint8_t session_context_buf[EVP_MAX_MD_SIZE] = {};
  unsigned session_context_len = 0;
  generateHashForSessionContexId(server_names, session_context_buf, session_context_len);
  for (auto& ctx : tls_contexts_) {
    if (config.certificateValidationContext() != nullptr &&
        !config.certificateValidationContext()->caCert().empty()) {
      ctx.addClientValidationContext(*config.certificateValidationContext(),
                                     config.requireClientCertificate());
    }

    if (!parsed_alpn_protocols_.empty()) {
      SSL_CTX_set_alpn_select_cb(
          ctx.ssl_ctx_.get(),
          [](SSL*, const unsigned char** out, unsigned char* outlen, const unsigned char* in,
             unsigned int inlen, void* arg) -> int {
            return static_cast<ServerContextImpl*>(arg)->alpnSelectCallback(out, outlen, in, inlen);
          },
          this);
    }

    if (!session_ticket_keys_.empty()) {
      SSL_CTX_set_tlsext_ticket_key_cb(
          ctx.ssl_ctx_.get(),
          [](SSL* ssl, uint8_t* key_name, uint8_t* iv, EVP_CIPHER_CTX* ctx, HMAC_CTX* hmac_ctx,
             int encrypt) -> int {
            ContextImpl* context_impl =
                static_cast<ContextImpl*>(SSL_CTX_get_app_data(SSL_get_SSL_CTX(ssl)));
            ServerContextImpl* server_context_impl = dynamic_cast<ServerContextImpl*>(context_impl);
            RELEASE_ASSERT(server_context_impl != nullptr, ""); // for Coverity
            return server_context_impl->sessionTicketProcess(ssl, key_name, iv, ctx, hmac_ctx,
                                                             encrypt);
          });
    }

    int rc = SSL_CTX_set_session_id_context(ctx.ssl_ctx_.get(), session_context_buf,
                                            session_context_len);
    RELEASE_ASSERT(rc == 1, "");
  }
}

void ServerContextImpl::generateHashForSessionContexId(const std::vector<std::string>& server_names,
                                                       uint8_t* session_context_buf,
                                                       unsigned& session_context_len) {
  EVP_MD_CTX md;
  int rc = EVP_DigestInit(&md, EVP_sha256());
  RELEASE_ASSERT(rc == 1, "");

  // Hash the CommonName/SANs of all the server certificates. This makes sure that sessions can only
  // be resumed to certificate(s) for the same name(s), but allows resuming to unique certs in the
  // case that different Envoy instances each have their own certs. All certificates in a
  // ServerContextImpl context are hashed together, since they all constitute a match on a filter
  // chain for resumption purposes.
  for (const auto& ctx : tls_contexts_) {
    X509* cert = SSL_CTX_get0_certificate(ctx.ssl_ctx_.get());
    RELEASE_ASSERT(cert != nullptr, "");
    X509_NAME* cert_subject = X509_get_subject_name(cert);
    RELEASE_ASSERT(cert_subject != nullptr, "");
    int cn_index = X509_NAME_get_index_by_NID(cert_subject, NID_commonName, -1);
    // It's possible that the certificate doesn't have CommonName, but has SANs.
    if (cn_index >= 0) {
      X509_NAME_ENTRY* cn_entry = X509_NAME_get_entry(cert_subject, cn_index);
      RELEASE_ASSERT(cn_entry != nullptr, "");
      ASN1_STRING* cn_asn1 = X509_NAME_ENTRY_get_data(cn_entry);
      RELEASE_ASSERT(ASN1_STRING_length(cn_asn1) > 0, "");
      rc = EVP_DigestUpdate(&md, ASN1_STRING_data(cn_asn1), ASN1_STRING_length(cn_asn1));
      RELEASE_ASSERT(rc == 1, "");
    }

    bssl::UniquePtr<GENERAL_NAMES> san_names(static_cast<GENERAL_NAMES*>(
        X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr)));
    if (san_names != nullptr) {
      for (const GENERAL_NAME* san : san_names.get()) {
        if (san->type == GEN_DNS || san->type == GEN_URI) {
          rc = EVP_DigestUpdate(&md, ASN1_STRING_data(san->d.ia5), ASN1_STRING_length(san->d.ia5));
          RELEASE_ASSERT(rc == 1, "");
        }
      }
    } else {
      // Make sure that we have either CommonName or SANs.
      RELEASE_ASSERT(cn_index >= 0, "");
    }

    X509_NAME* cert_issuer_name = X509_get_issuer_name(cert);
    rc =
        X509_NAME_digest(cert_issuer_name, EVP_sha256(), session_context_buf, &session_context_len);
    RELEASE_ASSERT(rc == 1 && session_context_len == SHA256_DIGEST_LENGTH, "");
    rc = EVP_DigestUpdate(&md, session_context_buf, session_context_len);
    RELEASE_ASSERT(rc == 1, "");
  }

  // Hash all the settings that affect whether the server will allow/accept
  // the client connection. This ensures that the client is always validated against
  // the correct settings, even if session resumption across different listeners
  // is enabled.
  if (ca_cert_ != nullptr) {
    rc = X509_digest(ca_cert_.get(), EVP_sha256(), session_context_buf, &session_context_len);
    RELEASE_ASSERT(rc == 1 && session_context_len == SHA256_DIGEST_LENGTH, "");
    rc = EVP_DigestUpdate(&md, session_context_buf, session_context_len);
    RELEASE_ASSERT(rc == 1, "");

    // verify_subject_alt_name_list_ can only be set with a ca_cert
    for (const std::string& name : verify_subject_alt_name_list_) {
      rc = EVP_DigestUpdate(&md, name.data(), name.size());
      RELEASE_ASSERT(rc == 1, "");
    }
  }

  for (const auto& hash : verify_certificate_hash_list_) {
    rc = EVP_DigestUpdate(&md, hash.data(),
                          hash.size() *
                              sizeof(std::remove_reference<decltype(hash)>::type::value_type));
    RELEASE_ASSERT(rc == 1, "");
  }

  for (const auto& hash : verify_certificate_spki_list_) {
    rc = EVP_DigestUpdate(&md, hash.data(),
                          hash.size() *
                              sizeof(std::remove_reference<decltype(hash)>::type::value_type));
    RELEASE_ASSERT(rc == 1, "");
  }

  // Hash configured SNIs for this context, so that sessions cannot be resumed across different
  // filter chains, even when using the same server certificate.
  for (const auto& name : server_names) {
    rc = EVP_DigestUpdate(&md, name.data(), name.size());
    RELEASE_ASSERT(rc == 1, "");
  }

  rc = EVP_DigestFinal(&md, session_context_buf, &session_context_len);
  RELEASE_ASSERT(rc == 1, "");
}

int ServerContextImpl::sessionTicketProcess(SSL*, uint8_t* key_name, uint8_t* iv,
                                            EVP_CIPHER_CTX* ctx, HMAC_CTX* hmac_ctx, int encrypt) {
  const EVP_MD* hmac = EVP_sha256();
  const EVP_CIPHER* cipher = EVP_aes_256_cbc();

  if (encrypt == 1) {
    // Encrypt
    RELEASE_ASSERT(!session_ticket_keys_.empty(), "");
    // TODO(ggreenway): validate in SDS that session_ticket_keys_ cannot be empty,
    // or if we allow it to be emptied, reconfigure the context so this callback
    // isn't set.

    const Envoy::Ssl::ServerContextConfig::SessionTicketKey& key = session_ticket_keys_.front();

    static_assert(std::tuple_size<decltype(key.name_)>::value == SSL_TICKET_KEY_NAME_LEN,
                  "Expected key.name length");
    std::copy_n(key.name_.begin(), SSL_TICKET_KEY_NAME_LEN, key_name);

    int rc = RAND_bytes(iv, EVP_CIPHER_iv_length(cipher));
    ASSERT(rc);

    // This RELEASE_ASSERT is logically a static_assert, but we can't actually get
    // EVP_CIPHER_key_length(cipher) at compile-time
    RELEASE_ASSERT(key.aes_key_.size() == EVP_CIPHER_key_length(cipher), "");
    if (!EVP_EncryptInit_ex(ctx, cipher, nullptr, key.aes_key_.data(), iv)) {
      return -1;
    }

    if (!HMAC_Init_ex(hmac_ctx, key.hmac_key_.data(), key.hmac_key_.size(), hmac, nullptr)) {
      return -1;
    }

    return 1; // success
  } else {
    // Decrypt
    bool is_enc_key = true; // first element is the encryption key
    for (const Envoy::Ssl::ServerContextConfig::SessionTicketKey& key : session_ticket_keys_) {
      static_assert(std::tuple_size<decltype(key.name_)>::value == SSL_TICKET_KEY_NAME_LEN,
                    "Expected key.name length");
      if (std::equal(key.name_.begin(), key.name_.end(), key_name)) {
        if (!HMAC_Init_ex(hmac_ctx, key.hmac_key_.data(), key.hmac_key_.size(), hmac, nullptr)) {
          return -1;
        }

        RELEASE_ASSERT(key.aes_key_.size() == EVP_CIPHER_key_length(cipher), "");
        if (!EVP_DecryptInit_ex(ctx, cipher, nullptr, key.aes_key_.data(), iv)) {
          return -1;
        }

        // If our current encryption was not the decryption key, renew
        return is_enc_key ? 1  // success; do not renew
                          : 2; // success: renew key
      }
      is_enc_key = false;
    }

    return 0; // decryption failed
  }
}

bool ServerContextImpl::isClientEcdsaCapable(const SSL_CLIENT_HELLO* ssl_client_hello) {
  CBS client_hello;
  CBS_init(&client_hello, ssl_client_hello->client_hello, ssl_client_hello->client_hello_len);

  // This is the TLSv1.3 case (TLSv1.2 on the wire and the supported_versions extensions present).
  // We just need to look at signature algorithms.
  const uint16_t client_version = ssl_client_hello->version;
  if (client_version == TLS1_2_VERSION && tls_max_version_ == TLS1_3_VERSION) {
    // If the supported_versions extension is found then we assume that the client is competent
    // enough that just checking the signature_algorithms is sufficient.
    const uint8_t* supported_versions_data;
    size_t supported_versions_len;
    if (SSL_early_callback_ctx_extension_get(ssl_client_hello, TLSEXT_TYPE_supported_versions,
                                             &supported_versions_data, &supported_versions_len)) {
      const uint8_t* signature_algorithms_data;
      size_t signature_algorithms_len;
      if (SSL_early_callback_ctx_extension_get(ssl_client_hello, TLSEXT_TYPE_signature_algorithms,
                                               &signature_algorithms_data,
                                               &signature_algorithms_len)) {
        CBS signature_algorithms_ext, signature_algorithms;
        CBS_init(&signature_algorithms_ext, signature_algorithms_data, signature_algorithms_len);
        if (!CBS_get_u16_length_prefixed(&signature_algorithms_ext, &signature_algorithms) ||
            CBS_len(&signature_algorithms_ext) != 0) {
          return false;
        }
        if (cbsContainsU16(signature_algorithms, SSL_SIGN_ECDSA_SECP256R1_SHA256)) {
          return true;
        }
      }

      return false;
    }
  }

  // Otherwise we are < TLSv1.3 and need to look at both the curves in the supported_groups for
  // ECDSA and also for a compatible cipher suite. https://tools.ietf.org/html/rfc4492#section-5.1.1
  const uint8_t* curvelist_data;
  size_t curvelist_len;
  if (!SSL_early_callback_ctx_extension_get(ssl_client_hello, TLSEXT_TYPE_supported_groups,
                                            &curvelist_data, &curvelist_len)) {
    return false;
  }

  CBS curvelist;
  CBS_init(&curvelist, curvelist_data, curvelist_len);

  // We only support P256 ECDSA curves today.
  if (!cbsContainsU16(curvelist, SSL_CURVE_SECP256R1)) {
    return false;
  }

  // The client must have offered an ECDSA ciphersuite that we like.
  CBS cipher_suites;
  CBS_init(&cipher_suites, ssl_client_hello->cipher_suites, ssl_client_hello->cipher_suites_len);

  while (CBS_len(&cipher_suites) > 0) {
    uint16_t cipher_id;
    if (!CBS_get_u16(&cipher_suites, &cipher_id)) {
      return false;
    }
    // All tls_context_ share the same set of enabled ciphers, so we can just look at the base
    // context.
    if (tls_contexts_[0].isCipherEnabled(cipher_id, client_version)) {
      return true;
    }
  }

  return false;
}

enum ssl_select_cert_result_t
ServerContextImpl::selectTlsContext(const SSL_CLIENT_HELLO* ssl_client_hello) {
  const bool client_ecdsa_capable = isClientEcdsaCapable(ssl_client_hello);
  // Fallback on first certificate.
  const TlsContext* selected_ctx = &tls_contexts_[0];
  for (const auto& ctx : tls_contexts_) {
    if (client_ecdsa_capable == ctx.is_ecdsa_) {
      selected_ctx = &ctx;
      break;
    }
  }
  RELEASE_ASSERT(SSL_set_SSL_CTX(ssl_client_hello->ssl, selected_ctx->ssl_ctx_.get()) != nullptr,
                 "");
  return ssl_select_cert_success;
}

void ServerContextImpl::TlsContext::addClientValidationContext(
    const Envoy::Ssl::CertificateValidationContextConfig& config, bool require_client_cert) {
  bssl::UniquePtr<BIO> bio(
      BIO_new_mem_buf(const_cast<char*>(config.caCert().data()), config.caCert().size()));
  RELEASE_ASSERT(bio != nullptr, "");
  // Based on BoringSSL's SSL_add_file_cert_subjects_to_stack().
  bssl::UniquePtr<STACK_OF(X509_NAME)> list(sk_X509_NAME_new(
      [](const X509_NAME** a, const X509_NAME** b) -> int { return X509_NAME_cmp(*a, *b); }));
  RELEASE_ASSERT(list != nullptr, "");
  for (;;) {
    bssl::UniquePtr<X509> cert(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    if (cert == nullptr) {
      break;
    }
    X509_NAME* name = X509_get_subject_name(cert.get());
    if (name == nullptr) {
      throw EnvoyException(fmt::format("Failed to load trusted client CA certificates from {}",
                                       config.caCertPath()));
    }
    // Check for duplicates.
    if (sk_X509_NAME_find(list.get(), nullptr, name)) {
      continue;
    }
    bssl::UniquePtr<X509_NAME> name_dup(X509_NAME_dup(name));
    if (name_dup == nullptr || !sk_X509_NAME_push(list.get(), name_dup.release())) {
      throw EnvoyException(fmt::format("Failed to load trusted client CA certificates from {}",
                                       config.caCertPath()));
    }
  }
  // Check for EOF.
  uint32_t err = ERR_peek_last_error();
  if (ERR_GET_LIB(err) == ERR_LIB_PEM && ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
    ERR_clear_error();
  } else {
    throw EnvoyException(
        fmt::format("Failed to load trusted client CA certificates from {}", config.caCertPath()));
  }
  SSL_CTX_set_client_CA_list(ssl_ctx_.get(), list.release());

  // SSL_VERIFY_PEER or stronger mode was already set in ContextImpl::ContextImpl().
  if (require_client_cert) {
    SSL_CTX_set_verify(ssl_ctx_.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
  }
}

bool ServerContextImpl::TlsContext::isCipherEnabled(uint16_t cipher_id, uint16_t client_version) {
  const SSL_CIPHER* c = SSL_get_cipher_by_value(cipher_id);
  if (c == nullptr) {
    return false;
  }
  // Skip TLS 1.2 only ciphersuites unless the client supports it.
  if (SSL_CIPHER_get_min_version(c) > client_version) {
    return false;
  }
  if (SSL_CIPHER_get_auth_nid(c) != NID_auth_ecdsa) {
    return false;
  }
  for (const SSL_CIPHER* our_c : SSL_CTX_get_ciphers(ssl_ctx_.get())) {
    if (SSL_CIPHER_get_id(our_c) == SSL_CIPHER_get_id(c)) {
      return true;
    }
  }
  return false;
}

} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
