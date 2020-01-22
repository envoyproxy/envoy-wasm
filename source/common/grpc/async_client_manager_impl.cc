#include "common/grpc/async_client_manager_impl.h"

#include "envoy/config/core/v3alpha/grpc_service.pb.h"
#include "envoy/stats/scope.h"

#include "common/grpc/async_client_impl.h"

#ifdef ENVOY_GOOGLE_GRPC
#include "common/grpc/google_async_client_impl.h"
#endif

namespace Envoy {
namespace Grpc {

AsyncClientFactoryImpl::AsyncClientFactoryImpl(
    Upstream::ClusterManager& cm, const envoy::config::core::v3alpha::GrpcService& config,
    bool skip_cluster_check, TimeSource& time_source)
    : cm_(cm), config_(config), time_source_(time_source) {
  if (skip_cluster_check) {
    return;
  }

  const std::string& cluster_name = config.envoy_grpc().cluster_name();
  auto clusters = cm_.clusters();
  const auto& it = clusters.find(cluster_name);
  if (it == clusters.end()) {
    throw EnvoyException(fmt::format("Unknown gRPC client cluster '{}'", cluster_name));
  }
  if (it->second.get().info()->addedViaApi()) {
    throw EnvoyException(fmt::format("gRPC client cluster '{}' is not static", cluster_name));
  }
}

AsyncClientManagerImpl::AsyncClientManagerImpl(Upstream::ClusterManager& cm,
                                               ThreadLocal::Instance& tls, TimeSource& time_source,
                                               Api::Api& api)
    : cm_(cm), tls_(tls), time_source_(time_source), api_(api) {
#ifdef ENVOY_GOOGLE_GRPC
  google_tls_slot_ = tls.allocateSlot();
  google_tls_slot_->set(
      [&api](Event::Dispatcher&) { return std::make_shared<GoogleAsyncClientThreadLocal>(api); });
#else
  UNREFERENCED_PARAMETER(api_);
#endif
}

RawAsyncClientPtr AsyncClientFactoryImpl::create() {
  return std::make_unique<AsyncClientImpl>(cm_, config_, time_source_);
}

GoogleAsyncClientFactoryImpl::GoogleAsyncClientFactoryImpl(
    ThreadLocal::Instance& tls, ThreadLocal::Slot* google_tls_slot, Stats::Scope& scope,
    const envoy::config::core::v3alpha::GrpcService& config, Api::Api& api)
    : tls_(tls), google_tls_slot_(google_tls_slot),
      scope_(scope.createScope(fmt::format("grpc.{}.", config.google_grpc().stat_prefix()))),
      config_(config), api_(api) {

#ifndef ENVOY_GOOGLE_GRPC
  UNREFERENCED_PARAMETER(tls_);
  UNREFERENCED_PARAMETER(google_tls_slot_);
  UNREFERENCED_PARAMETER(scope_);
  UNREFERENCED_PARAMETER(config_);
  UNREFERENCED_PARAMETER(api_);
  throw EnvoyException("Google C++ gRPC client is not linked");
#else
  ASSERT(google_tls_slot_ != nullptr);
#endif
}

RawAsyncClientPtr GoogleAsyncClientFactoryImpl::create() {
#ifdef ENVOY_GOOGLE_GRPC
  GoogleGenericStubFactory stub_factory;
  return std::make_unique<GoogleAsyncClientImpl>(
      tls_.dispatcher(), google_tls_slot_->getTyped<GoogleAsyncClientThreadLocal>(), stub_factory,
      scope_, config_, api_);
#else
  return nullptr;
#endif
}

AsyncClientFactoryPtr AsyncClientManagerImpl::factoryForGrpcService(
    const envoy::config::core::v3alpha::GrpcService& config, Stats::Scope& scope,
    bool skip_cluster_check) {
  switch (config.target_specifier_case()) {
  case envoy::config::core::v3alpha::GrpcService::TargetSpecifierCase::kEnvoyGrpc:
    return std::make_unique<AsyncClientFactoryImpl>(cm_, config, skip_cluster_check, time_source_);
  case envoy::config::core::v3alpha::GrpcService::TargetSpecifierCase::kGoogleGrpc:
    return std::make_unique<GoogleAsyncClientFactoryImpl>(tls_, google_tls_slot_.get(), scope,
                                                          config, api_);
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
  return nullptr;
}

} // namespace Grpc
} // namespace Envoy
