#pragma once

#include <string>

#include "envoy/api/api.h"
#include "envoy/extensions/transport_sockets/tls/v3alpha/cert.pb.h"
#include "envoy/ssl/certificate_validation_context_config.h"
#include "envoy/type/matcher/v3alpha/string.pb.h"

namespace Envoy {
namespace Ssl {

class CertificateValidationContextConfigImpl : public CertificateValidationContextConfig {
public:
  CertificateValidationContextConfigImpl(
      const envoy::extensions::transport_sockets::tls::v3alpha::CertificateValidationContext&
          config,
      Api::Api& api);

  const std::string& caCert() const override { return ca_cert_; }
  const std::string& caCertPath() const override { return ca_cert_path_; }
  const std::string& certificateRevocationList() const override {
    return certificate_revocation_list_;
  }
  const std::string& certificateRevocationListPath() const override {
    return certificate_revocation_list_path_;
  }
  const std::vector<std::string>& verifySubjectAltNameList() const override {
    return verify_subject_alt_name_list_;
  }
  const std::vector<envoy::type::matcher::v3alpha::StringMatcher>&
  subjectAltNameMatchers() const override {
    return subject_alt_name_matchers_;
  }
  const std::vector<std::string>& verifyCertificateHashList() const override {
    return verify_certificate_hash_list_;
  }
  const std::vector<std::string>& verifyCertificateSpkiList() const override {
    return verify_certificate_spki_list_;
  }
  bool allowExpiredCertificate() const override { return allow_expired_certificate_; }

private:
  const std::string ca_cert_;
  const std::string ca_cert_path_;
  const std::string certificate_revocation_list_;
  const std::string certificate_revocation_list_path_;
  const std::vector<std::string> verify_subject_alt_name_list_;
  const std::vector<envoy::type::matcher::v3alpha::StringMatcher> subject_alt_name_matchers_;
  const std::vector<std::string> verify_certificate_hash_list_;
  const std::vector<std::string> verify_certificate_spki_list_;
  const bool allow_expired_certificate_;
};

} // namespace Ssl
} // namespace Envoy
