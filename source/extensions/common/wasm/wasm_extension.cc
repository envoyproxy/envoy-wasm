#include "extensions/common/wasm/wasm_extension.h"

#include "extensions/common/wasm/context.h"
#include "extensions/common/wasm/wasm.h"
#include "extensions/common/wasm/wasm_vm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace {
#define CREATE_WASM_STATS(COUNTER, GAUGE)                                                          \
  COUNTER(remote_load_cache_hits)                                                                  \
  COUNTER(remote_load_cache_negative_hits)                                                         \
  COUNTER(remote_load_cache_misses)                                                                \
  COUNTER(remote_load_fetch_successes)                                                             \
  COUNTER(remote_load_fetch_failures)                                                              \
  GAUGE(remote_load_cache_entries, NeverImport)

struct CreateWasmStats {
  CREATE_WASM_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT)
};

WasmExtension* wasm_extension = nullptr;

class EnvoyWasm : public WasmExtension {
public:
  EnvoyWasm() = default;
  ~EnvoyWasm() override = default;
  void initialize() override {}
  std::unique_ptr<EnvoyWasmVmIntegration>
  createEnvoyWasmVmIntegration(const Stats::ScopeSharedPtr& scope, absl::string_view runtime,
                               absl::string_view short_runtime) override;
  WasmHandleExtensionFactory wasmFactory() override;
  WasmHandleExtensionCloneFactory wasmCloneFactory() override;
  void onEvent(WasmEvent event) override;
  void onRemoteCacheEntriesChanged(int remote_cache_entries) override;
  void createStats(const Stats::ScopeSharedPtr& scope, const PluginSharedPtr& plugin) override;
  void resetStats() override;

private:
  std::unique_ptr<CreateWasmStats> create_wasm_stats_;
};

std::unique_ptr<EnvoyWasmVmIntegration>
EnvoyWasm::createEnvoyWasmVmIntegration(const Stats::ScopeSharedPtr& scope,
                                        absl::string_view runtime,
                                        absl::string_view short_runtime) {
  return std::make_unique<EnvoyWasmVmIntegration>(scope, runtime, short_runtime);
}

WasmHandleExtensionFactory EnvoyWasm::wasmFactory() {
  return [](const VmConfig vm_config, const Stats::ScopeSharedPtr& scope,
            Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher,
            Server::ServerLifecycleNotifier& lifecycle_notifier,
            absl::string_view vm_key) -> WasmHandleBaseSharedPtr {
    auto wasm = std::make_shared<Wasm>(vm_config.runtime(), vm_config.vm_id(),
                                       anyToBytes(vm_config.configuration()), vm_key, scope,
                                       cluster_manager, dispatcher);
    wasm->initializeLifecycle(lifecycle_notifier);
    return std::static_pointer_cast<WasmHandleBase>(std::make_shared<WasmHandle>(std::move(wasm)));
  };
}

WasmHandleExtensionCloneFactory EnvoyWasm::wasmCloneFactory() {
  return [](const WasmHandleSharedPtr& base_wasm, Event::Dispatcher& dispatcher,
            CreateContextFn create_root_context_for_testing) -> WasmHandleBaseSharedPtr {
    auto wasm = std::make_shared<Wasm>(base_wasm, dispatcher);
    wasm->setCreateContextForTesting(nullptr, create_root_context_for_testing);
    return std::static_pointer_cast<WasmHandleBase>(std::make_shared<WasmHandle>(std::move(wasm)));
  };
}

void EnvoyWasm::onEvent(WasmEvent event) {
  switch (event) {
  case WasmEvent::RemoteLoadCacheHit:
    create_wasm_stats_->remote_load_cache_hits_.inc();
    break;
  case WasmEvent::RemoteLoadCacheNegativeHit:
    create_wasm_stats_->remote_load_cache_negative_hits_.inc();
    break;
  case WasmEvent::RemoteLoadCacheMiss:
    create_wasm_stats_->remote_load_cache_misses_.inc();
    break;
  case WasmEvent::RemoteLoadCacheFetchSuccess:
    create_wasm_stats_->remote_load_fetch_successes_.inc();
    break;
  case WasmEvent::RemoteLoadCacheFetchFailure:
    create_wasm_stats_->remote_load_fetch_failures_.inc();
    break;
  }
}

void EnvoyWasm::onRemoteCacheEntriesChanged(int entries) {
  create_wasm_stats_->remote_load_cache_entries_.set(entries);
}

void EnvoyWasm::createStats(const Stats::ScopeSharedPtr& scope, const PluginSharedPtr&) {
  create_wasm_stats_.reset(new CreateWasmStats{
      CREATE_WASM_STATS(POOL_COUNTER_PREFIX(*scope, "wasm."), POOL_GAUGE_PREFIX(*scope, "wasm."))});
}

void EnvoyWasm::resetStats() { create_wasm_stats_.reset(); }

} // namespace

Stats::ScopeSharedPtr WasmExtension::lockOrResetScope(const Stats::ScopeSharedPtr& scope,
                                                      const PluginSharedPtr& plugin) {
  Stats::ScopeSharedPtr lock;
  if (!(lock = scope_.lock())) {
    createStats(scope, plugin);
    scope_ = ScopeWeakPtr(scope);
    return scope;
  }
  return lock;
}

// Register a Wasm extension. Note: only one extension may be registered.
RegisterWasmExtension::RegisterWasmExtension(WasmExtension* extension) {
  RELEASE_ASSERT(!wasm_extension, "Multiple Wasm extensions registered.");
  wasm_extension = extension;
}

WasmExtension* getWasmExtension() {
  static WasmExtension* extension = wasm_extension ? wasm_extension : new EnvoyWasm();
  return extension;
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
