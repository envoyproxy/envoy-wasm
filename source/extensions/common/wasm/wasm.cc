#include "extensions/common/wasm/wasm.h"

#include <chrono>

#include "absl/strings/str_cat.h"
#include "envoy/event/deferred_deletable.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

namespace {

struct CodeCacheEntry {
  std::string code;
  bool in_progress;
};

const std::string INLINE_STRING = "<inline>";
// NB: xDS currently does not support failing asynchronously, so we fail immediately
// if remote Wasm code is not cached and do a background fill.
const bool DEFAULT_FAIL_IF_NOT_CACHED = true;
bool fail_if_code_not_cached = DEFAULT_FAIL_IF_NOT_CACHED;

std::atomic<int64_t> active_wasms;
std::mutex code_cache_mutex;
std::unordered_map<std::string, CodeCacheEntry>* code_cache = nullptr;

// Downcast WasmBase to the actual Wasm.
inline Wasm* getWasm(WasmHandleSharedPtr& base_wasm_handle) {
  return static_cast<Wasm*>(base_wasm_handle->wasm().get());
}

} // namespace

void Wasm::initializeStats() {
  active_wasms++;
  wasm_stats_.active_.set(active_wasms);
  wasm_stats_.created_.inc();
}

void Wasm::initializeLifecycle(Server::ServerLifecycleNotifier& lifecycle_notifier) {
  auto weak = std::weak_ptr<Wasm>(std::static_pointer_cast<Wasm>(shared_from_this()));
  lifecycle_notifier.registerCallback(Server::ServerLifecycleNotifier::Stage::ShutdownExit,
                                      [this, weak](Event::PostCb post_cb) {
                                        auto lock = weak.lock();
                                        if (lock) { // See if we are still alive.
                                          server_shutdown_post_cb_ = post_cb;
                                        }
                                      });
}

Wasm::Wasm(absl::string_view runtime, absl::string_view vm_id, absl::string_view vm_configuration,
           absl::string_view vm_key, const Stats::ScopeSharedPtr& scope,
           Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher)
    : WasmBase(createWasmVm(runtime, scope), vm_id, vm_configuration, vm_key), scope_(scope),
      cluster_manager_(cluster_manager), dispatcher_(dispatcher),
      time_source_(dispatcher.timeSource()),
      wasm_stats_(WasmStats{
          ALL_WASM_STATS(POOL_COUNTER_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")),
                         POOL_GAUGE_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")))}) {
  initializeStats();
  ENVOY_LOG(debug, "Base Wasm created {} now active", active_wasms);
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
  initializeStats();
  ENVOY_LOG(debug, "Thread-Local Wasm created {} now active", active_wasms);
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
  active_wasms--;
  wasm_stats_.active_.set(active_wasms);
  ENVOY_LOG(debug, "~Wasm {} remaining active", active_wasms);
  if (server_shutdown_post_cb_) {
    dispatcher_.post(server_shutdown_post_cb_);
  }
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

void clearCodeCacheForTesting(bool fail_if_not_cached) {
  std::lock_guard<std::mutex> guard(code_cache_mutex);
  fail_if_code_not_cached = fail_if_not_cached;
  if (code_cache) {
    delete code_cache;
    code_cache = nullptr;
  }
}

static void createWasmInternal(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                               const Stats::ScopeSharedPtr& scope,
                               Upstream::ClusterManager& cluster_manager,
                               Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                               Runtime::RandomGenerator& random, Api::Api& api,
                               Server::ServerLifecycleNotifier& lifecycle_notifier,
                               Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                               std::unique_ptr<Context> root_context_for_testing,
                               CreateWasmCallback&& cb) {
  std::string source, code;
  bool fetch = false;
  if (vm_config.code().has_remote()) {
    source = vm_config.code().remote().http_uri().uri();
    std::lock_guard<std::mutex> guard(code_cache_mutex);
    if (!code_cache) {
      code_cache = new std::remove_reference<decltype(*code_cache)>::type;
    }
    auto it = code_cache->find(vm_config.code().remote().sha256());
    if (it != code_cache->end()) {
      if (it->second.in_progress) {
        throw WasmException(
            fmt::format("Failed to load WASM code (fetch in progress) from {}", source));
      }
      code = it->second.code;
      if (code.empty()) {
        fetch = true; // Fetch failed, retry.
        it->second.in_progress = true;
      }
    } else {
      fetch = true; // Not in cache, fetch.
      (*code_cache)[vm_config.code().remote().sha256()].in_progress = true;
    }
  } else if (vm_config.code().has_local()) {
    code = Config::DataSource::read(vm_config.code().local(), true, api);
    source = Config::DataSource::getPath(vm_config.code().local())
                 .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  }

  auto complete_cb =
      [cb, vm_config, plugin, scope, &cluster_manager, &dispatcher, &lifecycle_notifier,
       root_context_for_testing_ptr = root_context_for_testing.release()](std::string code) {
        auto root_context = std::unique_ptr<Context>(root_context_for_testing_ptr);
        auto vm_key = proxy_wasm::makeVmKey(vm_config.vm_id(), vm_config.configuration(), code);
        cb(std::static_pointer_cast<WasmHandle>(proxy_wasm::createWasm(
            vm_key, code, plugin,
            [&vm_config, &scope, &cluster_manager, &dispatcher,
             &lifecycle_notifier](absl::string_view vm_key) {
              auto wasm = std::make_shared<Wasm>(vm_config.runtime(), vm_config.vm_id(),
                                                 vm_config.configuration(), vm_key, scope,
                                                 cluster_manager, dispatcher);
              // NB: we need the shared_ptr to have been created for shared_from_this() to work.
              wasm->initializeLifecycle(lifecycle_notifier);
              return std::static_pointer_cast<WasmHandleBase>(std::make_shared<WasmHandle>(wasm));
            },
            vm_config.allow_precompiled(), std::move(root_context))));
      };

  if (fetch) {
    // NB: if the (fetching) exception is thrown below, the remote_data provider will be deleted
    // immediately rather than completing the async fetch, so allow for self-delete.
    auto remote_data_provider_holder =
        std::make_shared<std::unique_ptr<Config::DataSource::RemoteAsyncDataProvider>>();
    auto fetch_callback = [vm_config, complete_cb, source, &dispatcher,
                           remote_data_provider_holder](const std::string& code) {
      {
        std::lock_guard<std::mutex> guard(code_cache_mutex);
        auto& e = (*code_cache)[vm_config.code().remote().sha256()];
        e.in_progress = false;
        e.code = code;
      }
      if (!fail_if_code_not_cached) {
        if (code.empty()) {
          throw WasmException(
              fmt::format("Failed to load WASM code (fetch failed) from {}", source));
        }
        complete_cb(code);
      }
      // NB: must be deleted explicitly.
      dispatcher.deferredDelete(
          Envoy::Event::DeferredDeletablePtr{remote_data_provider_holder->release()});
      remote_data_provider_holder->reset();
    };
    remote_data_provider = std::make_unique<Config::DataSource::RemoteAsyncDataProvider>(
        cluster_manager, init_manager, vm_config.code().remote(), dispatcher, random, true,
        fetch_callback);
    if (fail_if_code_not_cached) {
      *remote_data_provider_holder = std::move(remote_data_provider);
      throw WasmException(fmt::format("Failed to load WASM code (fetching) from {}", source));
    }
  } else {
    complete_cb(code);
  }
}

void createWasm(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                const Stats::ScopeSharedPtr& scope, Upstream::ClusterManager& cluster_manager,
                Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                Runtime::RandomGenerator& random, Api::Api& api,
                Envoy::Server::ServerLifecycleNotifier& lifecycle_notifier,
                Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, random,
                     api, lifecycle_notifier, remote_data_provider,
                     nullptr /* root_context_for_testing */, std::move(cb));
}

void createWasmForTesting(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                          const Stats::ScopeSharedPtr& scope,
                          Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                          Event::Dispatcher& dispatcher, Runtime::RandomGenerator& random,
                          Api::Api& api, Envoy::Server::ServerLifecycleNotifier& lifecycle_notifier,
                          Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                          std::unique_ptr<Context> root_context_for_testing,
                          CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, random,
                     api, lifecycle_notifier, remote_data_provider,
                     std::move(root_context_for_testing), std::move(cb));
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
