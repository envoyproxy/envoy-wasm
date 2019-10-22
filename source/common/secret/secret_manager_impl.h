#pragma once

#include <unordered_map>

#include "envoy/secret/secret_manager.h"
#include "envoy/secret/secret_provider.h"
#include "envoy/server/transport_socket_config.h"
#include "envoy/ssl/certificate_validation_context_config.h"
#include "envoy/ssl/tls_certificate_config.h"

#include "common/common/logger.h"
#include "common/secret/sds_api.h"

namespace Envoy {
namespace Secret {

class SecretManagerImpl : public SecretManager {
public:
  SecretManagerImpl(Server::ConfigTracker& config_tracker);
  void addStaticSecret(const envoy::api::v2::auth::Secret& secret) override;

  TlsCertificateConfigProviderSharedPtr
  findStaticTlsCertificateProvider(const std::string& name) const override;

  CertificateValidationContextConfigProviderSharedPtr
  findStaticCertificateValidationContextProvider(const std::string& name) const override;

  TlsSessionTicketKeysConfigProviderSharedPtr
  findStaticTlsSessionTicketKeysContextProvider(const std::string& name) const override;

  TlsCertificateConfigProviderSharedPtr createInlineTlsCertificateProvider(
      const envoy::api::v2::auth::TlsCertificate& tls_certificate) override;

  CertificateValidationContextConfigProviderSharedPtr
  createInlineCertificateValidationContextProvider(
      const envoy::api::v2::auth::CertificateValidationContext& certificate_validation_context)
      override;

  TlsSessionTicketKeysConfigProviderSharedPtr createInlineTlsSessionTicketKeysProvider(
      const envoy::api::v2::auth::TlsSessionTicketKeys& tls_session_ticket_keys) override;

  TlsCertificateConfigProviderSharedPtr findOrCreateTlsCertificateProvider(
      const envoy::api::v2::core::ConfigSource& config_source, const std::string& config_name,
      Server::Configuration::TransportSocketFactoryContext& secret_provider_context) override;

  CertificateValidationContextConfigProviderSharedPtr
  findOrCreateCertificateValidationContextProvider(
      const envoy::api::v2::core::ConfigSource& config_source, const std::string& config_name,
      Server::Configuration::TransportSocketFactoryContext& secret_provider_context) override;

  TlsSessionTicketKeysConfigProviderSharedPtr findOrCreateTlsSessionTicketKeysContextProvider(
      const envoy::api::v2::core::ConfigSource& config_source, const std::string& config_name,
      Server::Configuration::TransportSocketFactoryContext& secret_provider_context) override;

private:
  ProtobufTypes::MessagePtr dumpSecretConfigs();

  template <class SecretType>
  class DynamicSecretProviders : public Logger::Loggable<Logger::Id::secret> {
  public:
    // Finds or creates SdsApi object.
    std::shared_ptr<SecretType>
    findOrCreate(const envoy::api::v2::core::ConfigSource& sds_config_source,
                 const std::string& config_name,
                 Server::Configuration::TransportSocketFactoryContext& secret_provider_context) {
      const std::string map_key =
          absl::StrCat(MessageUtil::hash(sds_config_source), ".", config_name);

      std::shared_ptr<SecretType> secret_provider = dynamic_secret_providers_[map_key].lock();
      if (!secret_provider) {
        // SdsApi is owned by ListenerImpl and ClusterInfo which are destroyed before
        // SecretManagerImpl. It is safe to invoke this callback at the destructor of SdsApi.
        std::function<void()> unregister_secret_provider = [map_key, this]() {
          removeDynamicSecretProvider(map_key);
        };
        ASSERT(secret_provider_context.initManager() != nullptr);
        secret_provider = SecretType::create(secret_provider_context, sds_config_source,
                                             config_name, unregister_secret_provider);
        dynamic_secret_providers_[map_key] = secret_provider;
      }
      return secret_provider;
    }

    std::vector<std::shared_ptr<SecretType>> allSecretProviders() {
      std::vector<std::shared_ptr<SecretType>> providers;
      for (const auto& secret_entry : dynamic_secret_providers_) {
        std::shared_ptr<SecretType> secret_provider = secret_entry.second.lock();
        if (secret_provider) {
          providers.push_back(std::move(secret_provider));
        }
      }
      return providers;
    }

  private:
    // Removes dynamic secret provider which has been deleted.
    void removeDynamicSecretProvider(const std::string& map_key) {
      ENVOY_LOG(debug, "Unregister secret provider. hash key: {}", map_key);

      auto num_deleted = dynamic_secret_providers_.erase(map_key);
      ASSERT(num_deleted == 1, "");
    }

    std::unordered_map<std::string, std::weak_ptr<SecretType>> dynamic_secret_providers_;
  };

  // Manages pairs of secret name and TlsCertificateConfigProviderSharedPtr.
  std::unordered_map<std::string, TlsCertificateConfigProviderSharedPtr>
      static_tls_certificate_providers_;

  // Manages pairs of secret name and CertificateValidationContextConfigProviderSharedPtr.
  std::unordered_map<std::string, CertificateValidationContextConfigProviderSharedPtr>
      static_certificate_validation_context_providers_;

  std::unordered_map<std::string, TlsSessionTicketKeysConfigProviderSharedPtr>
      static_session_ticket_keys_providers_;

  // map hash code of SDS config source and SdsApi object.
  DynamicSecretProviders<TlsCertificateSdsApi> certificate_providers_;
  DynamicSecretProviders<CertificateValidationContextSdsApi> validation_context_providers_;
  DynamicSecretProviders<TlsSessionTicketKeysSdsApi> session_ticket_keys_providers_;

  Server::ConfigTracker::EntryOwnerPtr config_tracker_entry_;
};

} // namespace Secret
} // namespace Envoy
