#pragma once

#include "envoy/server/wasm_config.h"
#include "envoy/thread_local/thread_local.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContextImpl : public WasmFactoryContext {
public:
  WasmFactoryContextImpl(Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher,
                         ThreadLocal::SlotAllocator& tls, Api::Api& api, Stats::Scope& scope,
                         Stats::ScopeSharedPtr owned_scope, const LocalInfo::LocalInfo& local_info)
      : cluster_manager_(cluster_manager), dispatcher_(dispatcher), tls_(tls), api_(api),
        scope_(scope), owned_scope_(owned_scope), local_info_(local_info) {}

  Upstream::ClusterManager& clusterManager() override { return cluster_manager_; }
  Event::Dispatcher& dispatcher() override { return dispatcher_; }
  ThreadLocal::SlotAllocator& threadLocal() override { return tls_; }
  Api::Api& api() override { return api_; }
  Stats::Scope& scope() override { return scope_; }
  Stats::ScopeSharedPtr owned_scope() override { return owned_scope_; }
  const LocalInfo::LocalInfo& localInfo() const override { return local_info_; }

private:
  Upstream::ClusterManager& cluster_manager_;
  Event::Dispatcher& dispatcher_;
  ThreadLocal::SlotAllocator& tls_;
  Api::Api& api_;
  Stats::Scope& scope_;
  Stats::ScopeSharedPtr owned_scope_;
  const LocalInfo::LocalInfo& local_info_;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
