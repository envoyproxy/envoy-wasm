#pragma once

#include "envoy/secret/secret_manager.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/http/context_impl.h"
#include "common/upstream/cluster_manager_impl.h"

#include "server/config_validation/async_client.h"

namespace Envoy {
namespace Upstream {

/**
 * Config-validation-only implementation of ClusterManagerFactory, which creates
 * ValidationClusterManagers. It also creates, but never returns, CdsApiImpls.
 */
class ValidationClusterManagerFactory : public ProdClusterManagerFactory {
public:
  using ProdClusterManagerFactory::ProdClusterManagerFactory;

  explicit ValidationClusterManagerFactory(
      Server::Admin& admin, Runtime::Loader& runtime, Stats::Store& stats,
      ThreadLocal::Instance& tls, Runtime::RandomGenerator& random,
      Network::DnsResolverSharedPtr dns_resolver, Ssl::ContextManager& ssl_context_manager,
      Event::Dispatcher& main_thread_dispatcher, const LocalInfo::LocalInfo& local_info,
      Secret::SecretManager& secret_manager, ProtobufMessage::ValidationContext& validation_context,
      Api::Api& api, Http::Context& http_context, AccessLog::AccessLogManager& log_manager,
      Singleton::Manager& singleton_manager, Event::TimeSystem& time_system)
      : ProdClusterManagerFactory(admin, runtime, stats, tls, random, dns_resolver,
                                  ssl_context_manager, main_thread_dispatcher, local_info,
                                  secret_manager, validation_context, api, http_context,
                                  log_manager, singleton_manager),
        time_system_(time_system) {}

  ClusterManagerPtr
  clusterManagerFromProto(const envoy::config::bootstrap::v2::Bootstrap& bootstrap) override;

  // Delegates to ProdClusterManagerFactory::createCds, but discards the result and returns nullptr
  // unconditionally.
  CdsApiPtr createCds(const envoy::api::v2::core::ConfigSource& cds_config,
                      ClusterManager& cm) override;

private:
  Event::TimeSystem& time_system_;
};

/**
 * Config-validation-only implementation of ClusterManager, which opens no upstream connections.
 */
class ValidationClusterManager : public ClusterManagerImpl {
public:
  ValidationClusterManager(const envoy::config::bootstrap::v2::Bootstrap& bootstrap,
                           ClusterManagerFactory& factory, Stats::Store& stats,
                           ThreadLocal::Instance& tls, Runtime::Loader& runtime,
                           Runtime::RandomGenerator& random, const LocalInfo::LocalInfo& local_info,
                           AccessLog::AccessLogManager& log_manager, Event::Dispatcher& dispatcher,
                           Server::Admin& admin,
                           ProtobufMessage::ValidationContext& validation_context, Api::Api& api,
                           Http::Context& http_context, Event::TimeSystem& time_system);

  Http::ConnectionPool::Instance* httpConnPoolForCluster(const std::string&, ResourcePriority,
                                                         Http::Protocol,
                                                         LoadBalancerContext*) override;
  Host::CreateConnectionData tcpConnForCluster(const std::string&, LoadBalancerContext*) override;
  Http::AsyncClient& httpAsyncClientForCluster(const std::string&) override;

private:
  // ValidationAsyncClient always returns null on send() and start(), so it has
  // no internal state -- we might as well just keep one and hand out references
  // to it.
  Http::ValidationAsyncClient async_client_;
};

} // namespace Upstream
} // namespace Envoy
