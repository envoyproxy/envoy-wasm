#pragma once

#include <atomic>
#include <map>
#include <memory>

#include "envoy/common/exception.h"
#include "envoy/config/wasm/v3/wasm.pb.validate.h"
#include "envoy/http/filter.h"
#include "envoy/server/lifecycle_notifier.h"
#include "envoy/server/wasm.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats.h"
#include "envoy/thread_local/thread_local.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/assert.h"
#include "common/common/logger.h"
#include "common/config/datasource.h"
#include "common/stats/symbol_table_impl.h"

#include "extensions/common/wasm/context.h"
#include "extensions/common/wasm/wasm_vm.h"
#include "extensions/common/wasm/well_known_names.h"

#include "include/proxy-wasm/exports.h"
#include "include/proxy-wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

#define ALL_WASM_STATS(COUNTER, GAUGE)                                                             \
  COUNTER(created)                                                                                 \
  GAUGE(active, NeverImport)

struct WasmStats {
  ALL_WASM_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT)
};

using VmConfig = envoy::config::wasm::v3::VmConfig;

class WasmHandle;

// Wasm execution instance. Manages the Envoy side of the Wasm interface.
class Wasm : public WasmBase, Logger::Loggable<Logger::Id::wasm> {
public:
  Wasm(absl::string_view runtime, absl::string_view vm_id, absl::string_view vm_configuration,
       absl::string_view vm_key, const Stats::ScopeSharedPtr& scope,
       Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher);
  Wasm(std::shared_ptr<WasmHandle> other, Event::Dispatcher& dispatcher);
  ~Wasm() override;

  Upstream::ClusterManager& clusterManager() const { return cluster_manager_; }
  Event::Dispatcher& dispatcher() { return dispatcher_; }
  Context* getRootContext(absl::string_view root_id) {
    return static_cast<Context*>(WasmBase::getRootContext(root_id));
  }
  void setTickPeriod(uint32_t root_context_id, std::chrono::milliseconds tick_period) override;
  virtual void tickHandler(uint32_t root_context_id);

  // WasmBase
  void error(absl::string_view message) override { throw WasmException(std::string(message)); }
  proxy_wasm::CallOnThreadFunction callOnThreadFunction() override;
  ContextBase* createContext(std::shared_ptr<PluginBase> plugin) override;

  // AccessLog::Instance
  void log(absl::string_view root_id, const Http::RequestHeaderMap* request_headers,
           const Http::ResponseHeaderMap* response_headers,
           const Http::ResponseTrailerMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info);

  void initializeLifecycle(Server::ServerLifecycleNotifier& lifecycle_notifier);

private:
  friend class Context;

  void initializeStats();

  Stats::ScopeSharedPtr scope_;
  Upstream::ClusterManager& cluster_manager_;
  Event::Dispatcher& dispatcher_;
  Event::PostCb server_shutdown_post_cb_;
  std::unordered_map<uint32_t, Event::TimerPtr> timer_; // per root_id.
  TimeSource& time_source_;

  // Host Stats/Metrics
  WasmStats wasm_stats_;

  // Plugin Stats/Metrics
  absl::flat_hash_map<uint32_t, Stats::Counter*> counters_;
  absl::flat_hash_map<uint32_t, Stats::Gauge*> gauges_;
  absl::flat_hash_map<uint32_t, Stats::Histogram*> histograms_;
};
using WasmSharedPtr = std::shared_ptr<Wasm>;

class WasmHandle : public WasmHandleBase,
                   public Envoy::Server::Wasm,
                   public ThreadLocal::ThreadLocalObject {
public:
  explicit WasmHandle(const WasmSharedPtr& wasm)
      : WasmHandleBase(std::static_pointer_cast<WasmBase>(wasm)), wasm_(wasm) {}

  WasmSharedPtr& wasm() { return wasm_; }

private:
  WasmSharedPtr wasm_;
};
using WasmHandleSharedPtr = std::shared_ptr<WasmHandle>;

using CreateWasmCallback = std::function<void(WasmHandleSharedPtr)>;

void createWasm(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                const Stats::ScopeSharedPtr& scope, Upstream::ClusterManager& cluster_manager,
                Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                Runtime::RandomGenerator& random, Api::Api& api,
                Envoy::Server::ServerLifecycleNotifier& lifecycle_notifier,
                Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                CreateWasmCallback&& cb);

void createWasmForTesting(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                          const Stats::ScopeSharedPtr& scope,
                          Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                          Event::Dispatcher& dispatcher, Runtime::RandomGenerator& random,
                          Api::Api& api, Envoy::Server::ServerLifecycleNotifier& lifecycle_notifier,
                          Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                          std::unique_ptr<Context> root_context_for_testing,
                          CreateWasmCallback&& cb);

WasmHandleSharedPtr getOrCreateThreadLocalWasm(const WasmHandleSharedPtr& base_wasm,
                                               const PluginSharedPtr& plugin,
                                               Event::Dispatcher& dispatcher);

void clearCodeCacheForTesting(bool fail_if_not_cached);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
