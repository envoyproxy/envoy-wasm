#pragma once

#include "envoy/api/api.h"
#include "envoy/config/core/v3alpha/grpc_service.pb.h"
#include "envoy/grpc/async_client_manager.h"
#include "envoy/singleton/manager.h"
#include "envoy/stats/scope.h"
#include "envoy/thread_local/thread_local.h"
#include "envoy/upstream/cluster_manager.h"

namespace Envoy {
namespace Grpc {

class AsyncClientFactoryImpl : public AsyncClientFactory {
public:
  AsyncClientFactoryImpl(Upstream::ClusterManager& cm,
                         const envoy::config::core::v3alpha::GrpcService& config,
                         bool skip_cluster_check, TimeSource& time_source);

  RawAsyncClientPtr create() override;

private:
  Upstream::ClusterManager& cm_;
  const envoy::config::core::v3alpha::GrpcService config_;
  TimeSource& time_source_;
};

class GoogleAsyncClientFactoryImpl : public AsyncClientFactory {
public:
  GoogleAsyncClientFactoryImpl(ThreadLocal::Instance& tls, ThreadLocal::Slot* google_tls_slot,
                               Stats::Scope& scope,
                               const envoy::config::core::v3alpha::GrpcService& config,
                               Api::Api& api);

  RawAsyncClientPtr create() override;

private:
  ThreadLocal::Instance& tls_;
  ThreadLocal::Slot* google_tls_slot_;
  Stats::ScopeSharedPtr scope_;
  const envoy::config::core::v3alpha::GrpcService config_;
  Api::Api& api_;
};

class AsyncClientManagerImpl : public AsyncClientManager {
public:
  AsyncClientManagerImpl(Upstream::ClusterManager& cm, ThreadLocal::Instance& tls,
                         TimeSource& time_source, Api::Api& api);

  // Grpc::AsyncClientManager
  AsyncClientFactoryPtr
  factoryForGrpcService(const envoy::config::core::v3alpha::GrpcService& config,
                        Stats::Scope& scope, bool skip_cluster_check) override;

private:
  Upstream::ClusterManager& cm_;
  ThreadLocal::Instance& tls_;
  ThreadLocal::SlotPtr google_tls_slot_;
  TimeSource& time_source_;
  Api::Api& api_;
};

} // namespace Grpc
} // namespace Envoy
