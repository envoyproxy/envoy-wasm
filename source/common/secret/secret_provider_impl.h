#pragma once

#include <functional>

#include "envoy/api/v2/auth/cert.pb.h"
#include "envoy/secret/secret_provider.h"
#include "envoy/ssl/certificate_validation_context_config.h"
#include "envoy/ssl/tls_certificate_config.h"

namespace Envoy {
namespace Secret {

class TlsCertificateConfigProviderImpl : public TlsCertificateConfigProvider {
public:
  TlsCertificateConfigProviderImpl(const envoy::api::v2::auth::TlsCertificate& tls_certificate);

  const envoy::api::v2::auth::TlsCertificate* secret() const override {
    return tls_certificate_.get();
  }

  Common::CallbackHandle*
  addValidationCallback(std::function<void(const envoy::api::v2::auth::TlsCertificate&)>) override {
    return nullptr;
  }

  Common::CallbackHandle* addUpdateCallback(std::function<void()>) override { return nullptr; }

private:
  Secret::TlsCertificatePtr tls_certificate_;
};

class CertificateValidationContextConfigProviderImpl
    : public CertificateValidationContextConfigProvider {
public:
  CertificateValidationContextConfigProviderImpl(
      const envoy::api::v2::auth::CertificateValidationContext& certificate_validation_context);

  const envoy::api::v2::auth::CertificateValidationContext* secret() const override {
    return certificate_validation_context_.get();
  }

  Common::CallbackHandle* addValidationCallback(
      std::function<void(const envoy::api::v2::auth::CertificateValidationContext&)>) override {
    return nullptr;
  }

  Common::CallbackHandle* addUpdateCallback(std::function<void()>) override { return nullptr; }

private:
  Secret::CertificateValidationContextPtr certificate_validation_context_;
};

class TlsSessionTicketKeysConfigProviderImpl : public TlsSessionTicketKeysConfigProvider {
public:
  TlsSessionTicketKeysConfigProviderImpl(
      const envoy::api::v2::auth::TlsSessionTicketKeys& tls_session_ticket_keys);

  const envoy::api::v2::auth::TlsSessionTicketKeys* secret() const override {
    return tls_session_ticket_keys_.get();
  }

  Common::CallbackHandle* addValidationCallback(
      std::function<void(const envoy::api::v2::auth::TlsSessionTicketKeys&)>) override {
    return nullptr;
  }

  Common::CallbackHandle* addUpdateCallback(std::function<void()>) override { return nullptr; }

private:
  Secret::TlsSessionTicketKeysPtr tls_session_ticket_keys_;
};

} // namespace Secret
} // namespace Envoy
