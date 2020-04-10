#include "extensions/common/wasm/wasm.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/extensions/wasm/v3/wasm.pb.validate.h"
#include "envoy/grpc/status.h"
#include "envoy/http/codes.h"
#include "envoy/local_info/local_info.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/assert.h"
#include "common/common/base64.h"
#include "common/common/empty_string.h"
#include "common/common/enum_to_int.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/wasm_vm.h"
#include "extensions/common/wasm/well_known_names.h"

#include "absl/base/casts.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "openssl/bytestring.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

namespace {

const std::string INLINE_STRING = "<inline>";

std::atomic<int64_t> active_wasm_;

// Downcast WasmBase to the actual Wasm.
inline Wasm* getWasm(WasmHandleSharedPtr& base_wasm_handle) {
  return static_cast<Wasm*>(base_wasm_handle->wasm().get());
}

} // namespace

Wasm::Wasm(absl::string_view runtime, absl::string_view vm_id, absl::string_view vm_configuration,
           absl::string_view vm_key, const Stats::ScopeSharedPtr& scope,
           Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher)
    : WasmBase(createWasmVm(runtime, scope), vm_id, vm_configuration, vm_key), scope_(scope),
      cluster_manager_(cluster_manager), dispatcher_(dispatcher),
      time_source_(dispatcher.timeSource()),
      wasm_stats_(WasmStats{
          ALL_WASM_STATS(POOL_COUNTER_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")),
                         POOL_GAUGE_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")))}) {
  active_wasm_++;
  wasm_stats_.active_.set(active_wasm_);
  wasm_stats_.created_.inc();
  ENVOY_LOG(debug, "Base Wasm created {} now active", active_wasm_);
}

Wasm::Wasm(WasmHandleSharedPtr base_wasm_handle, Event::Dispatcher& dispatcher)
    : WasmBase(base_wasm_handle,
               [&base_wasm_handle]() {
                 return createWasmVm(
                     getEnvoyWasmIntegration(*base_wasm_handle->wasm()->wasm_vm()).runtime(),
                     getWasm(base_wasm_handle)->scope_);
               }),
      scope_(getWasm(base_wasm_handle)->scope_),
      cluster_manager_(getWasm(base_wasm_handle)->clusterManager()), dispatcher_(dispatcher),
      time_source_(dispatcher.timeSource()), wasm_stats_(getWasm(base_wasm_handle)->wasm_stats_) {
  active_wasm_++;
  wasm_stats_.active_.set(active_wasm_);
  wasm_stats_.created_.inc();
  ENVOY_LOG(debug, "Thread-Local Wasm created {} now active", active_wasm_);
}

void Wasm::setTickPeriod(uint32_t context_id, std::chrono::milliseconds new_tick_period) {
  auto& tick_period = tick_period_[context_id];
  auto& timer = timer_[context_id];
  bool was_running = timer && tick_period.count() > 0;
  tick_period = new_tick_period;
  if (was_running) {
    timer->disableTimer();
  }
  if (tick_period.count() > 0) {
    timer = dispatcher_.createTimer(
        [weak = std::weak_ptr<Wasm>(std::static_pointer_cast<Wasm>(shared_from_this())),
         context_id]() {
          auto shared = weak.lock();
          if (shared) {
            shared->tickHandler(context_id);
          }
        });
    timer->enableTimer(tick_period);
  }
}

void Wasm::tickHandler(uint32_t root_context_id) {
  auto tick_period = tick_period_.find(root_context_id);
  auto timer = timer_.find(root_context_id);
  if (tick_period == tick_period_.end() || timer == timer_.end() || !on_tick_) {
    return;
  }
  tick(root_context_id);
  if (timer->second && tick_period->second.count() > 0) {
    timer->second->enableTimer(tick_period->second);
  }
}

Wasm::~Wasm() {
  active_wasm_--;
  wasm_stats_.active_.set(active_wasm_);
  ENVOY_LOG(debug, "~Wasm {} remaining active", active_wasm_);
}

proxy_wasm::CallOnThreadFunction Wasm::callOnThreadFunction() {
  auto& dispatcher = dispatcher_;
  return [&dispatcher](const std::function<void()>& f) { return dispatcher.post(f); };
}

ContextBase* Wasm::createContext(std::shared_ptr<PluginBase> plugin) {
  if (plugin) {
    return new Context(this, std::static_pointer_cast<Plugin>(plugin));
  }
  return new Context(this);
}

void Wasm::log(absl::string_view root_id, const Http::RequestHeaderMap* request_headers,
               const Http::ResponseHeaderMap* response_headers,
               const Http::ResponseTrailerMap* response_trailers,
               const StreamInfo::StreamInfo& stream_info) {
  auto context = getRootContext(root_id);
  context->log(request_headers, response_headers, response_trailers, stream_info);
}

static void createWasmInternal(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                               Stats::ScopeSharedPtr scope,
                               Upstream::ClusterManager& cluster_manager,
                               Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                               Runtime::RandomGenerator& random, Api::Api& api,
                               std::unique_ptr<Context> root_context_for_testing,
                               Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                               CreateWasmCallback&& cb) {
  std::string source, code;
  if (vm_config.code().has_remote()) {
    source = vm_config.code().remote().http_uri().uri();
  } else if (vm_config.code().has_local()) {
    code = Config::DataSource::read(vm_config.code().local(), true, api);
    source = Config::DataSource::getPath(vm_config.code().local())
                 .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  }

  auto callback = [vm_config, scope, &cluster_manager, &dispatcher, plugin, cb, source,
                   context_ptr = root_context_for_testing ? root_context_for_testing.release()
                                                          : nullptr](const std::string& code) {
    if (code.empty()) {
      throw WasmException(fmt::format("Failed to load WASM code from {}", source));
    }
    std::unique_ptr<Context> context(context_ptr);
    auto configuration = vm_config.configuration();
    auto vm_key = proxy_wasm::makeVmKey(vm_config.vm_id(), configuration, code);
    cb(std::static_pointer_cast<WasmHandle>(proxy_wasm::createWasm(
        vm_key, code, plugin,
        [&vm_config, &configuration, &scope, &cluster_manager,
         &dispatcher](absl::string_view vm_key) {
          return std::static_pointer_cast<WasmHandleBase>(std::make_shared<WasmHandle>(
              std::make_shared<Wasm>(vm_config.runtime(), vm_config.vm_id(), configuration, vm_key,
                                     scope, cluster_manager, dispatcher)));
        },
        vm_config.allow_precompiled(), std::move(context))));
  };

  if (vm_config.code().has_remote()) {
    remote_data_provider = std::make_unique<Config::DataSource::RemoteAsyncDataProvider>(
        cluster_manager, init_manager, vm_config.code().remote(), dispatcher, random, true,
        std::move(callback));
  } else if (vm_config.code().has_local()) {
    callback(code);
  } else {
    callback(EMPTY_STRING);
  }
}

void createWasm(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                const Stats::ScopeSharedPtr& scope, Upstream::ClusterManager& cluster_manager,
                Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                Runtime::RandomGenerator& random, Api::Api& api,
                Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, random,
                     api, nullptr /* root_context_for_testing */, remote_data_provider,
                     std::move(cb));
}

void createWasmForTesting(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                          const Stats::ScopeSharedPtr& scope,
                          Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                          Event::Dispatcher& dispatcher, Runtime::RandomGenerator& random,
                          Api::Api& api, std::unique_ptr<Context> root_context_for_testing,
                          Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                          CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, random,
                     api, std::move(root_context_for_testing), remote_data_provider, std::move(cb));
}

WasmHandleSharedPtr getOrCreateThreadLocalWasm(const WasmHandleSharedPtr& base_wasm,
                                               const PluginSharedPtr& plugin,
                                               Event::Dispatcher& dispatcher) {
  auto wasm_handle = proxy_wasm::getOrCreateThreadLocalWasm(
      std::static_pointer_cast<WasmHandle>(base_wasm), plugin,
      [&dispatcher](const WasmHandleBaseSharedPtr& base_wasm) -> WasmHandleBaseSharedPtr {
        return std::make_shared<WasmHandle>(
            std::make_shared<Wasm>(std::static_pointer_cast<WasmHandle>(base_wasm), dispatcher));
      });
  if (!wasm_handle) {
    throw WasmException("Failed to configure WASM code");
  }
  return std::static_pointer_cast<WasmHandle>(wasm_handle);
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
