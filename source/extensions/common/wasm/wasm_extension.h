#pragma once

#include <memory>

#include "envoy/extensions/wasm/v3/wasm.pb.validate.h"
#include "envoy/server/lifecycle_notifier.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/logger.h"
#include "common/stats/symbol_table_impl.h"

#include "extensions/common/wasm/context.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

class WasmHandle;
class EnvoyWasmVmIntegration;

using WasmHandleSharedPtr = std::shared_ptr<WasmHandle>;
using VmConfig = envoy::extensions::wasm::v3::VmConfig;
using CreateContextFn =
    std::function<ContextBase*(Wasm* wasm, const std::shared_ptr<Plugin>& plugin)>;
using WasmHandleExtensionFactory = std::function<WasmHandleBaseSharedPtr(
    const VmConfig& vm_config, const Stats::ScopeSharedPtr& scope,
    Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher,
    Server::ServerLifecycleNotifier& lifecycle_notifier, absl::string_view vm_key)>;
using WasmHandleExtensionCloneFactory = std::function<WasmHandleBaseSharedPtr(
    const WasmHandleSharedPtr& base_wasm, Event::Dispatcher& dispatcher,
    CreateContextFn create_root_context_for_testing)>;
using ScopeWeakPtr = std::weak_ptr<Stats::Scope>;

// Extension point for Wasm clients in enbedded Envoy.
class WasmExtension : Logger::Loggable<Logger::Id::wasm> {
public:
  WasmExtension() = default;
  virtual ~WasmExtension() = default;

  virtual void initialize() = 0;
  virtual std::unique_ptr<EnvoyWasmVmIntegration>
  createEnvoyWasmVmIntegration(const Stats::ScopeSharedPtr& scope, absl::string_view runtime,
                               absl::string_view short_runtime) = 0;
  virtual WasmHandleExtensionFactory wasmFactory() = 0;
  virtual WasmHandleExtensionCloneFactory wasmCloneFactory() = 0;
  enum class WasmEvent : int {
    RemoteLoadCacheHit,
    RemoteLoadCacheNegativeHit,
    RemoteLoadCacheMiss,
    RemoteLoadCacheFetchSuccess,
    RemoteLoadCacheFetchFailure,
  };
  virtual void onEvent(WasmEvent event, const PluginSharedPtr& plugin) = 0;
  virtual void onRemoteCacheEntriesChanged(int remote_cache_entries) = 0;
  virtual void createStats(const Stats::ScopeSharedPtr& scope, const PluginSharedPtr& plugin) = 0;
  virtual void resetStats() = 0; // Delete stats pointers

  // NB: the Scope can become invalid if, for example, the owning FilterChain is deleted. When that
  // happens the stats must be recreated. This hook verifies the Scope of any existing stats and if
  // necessary recreates the stats with the newly provided scope.
  Stats::ScopeSharedPtr lockOrResetScope(const Stats::ScopeSharedPtr& scope,
                                         const PluginSharedPtr& plugin);

protected:
  ScopeWeakPtr scope_;
};

// Register a Wasm extension. Note: only one extension may be registered.
struct RegisterWasmExtension {
  RegisterWasmExtension(WasmExtension* extension);
};
#define REGISTER_WASM_EXTENSION(_class)                                                            \
  ::Envoy::Extensions::Common::Wasm::RegisterWasmExtension register_wasm_extension(new _class());

WasmExtension* getWasmExtension();

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
