#pragma once

#include "envoy/server/wasm_config.h"
#include "envoy/thread_local/thread_local.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContextImpl : public WasmFactoryContext {
public:
  WasmFactoryContextImpl(Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher,
                         ThreadLocal::SlotAllocator& tls, Api::Api& api)
      : cluster_manager_(cluster_manager), dispatcher_(dispatcher), tls_(tls), api_(api) {}

  Upstream::ClusterManager& clusterManager() override { return cluster_manager_; }
  Event::Dispatcher& dispatcher() override { return dispatcher_; }
  ThreadLocal::SlotAllocator& threadLocal() override { return tls_; }
  Api::Api& api() override { return api_; }

private:
  Upstream::ClusterManager& cluster_manager_;
  Event::Dispatcher& dispatcher_;
  ThreadLocal::SlotAllocator& tls_;
  Api::Api& api_;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
