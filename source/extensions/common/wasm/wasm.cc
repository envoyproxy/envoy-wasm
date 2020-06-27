#include "extensions/common/wasm/wasm.h"

#include <algorithm>
#include <chrono>

#include "envoy/event/deferred_deletable.h"

#include "common/common/logger.h"

#include "absl/strings/str_cat.h"

#define WASM_CONTEXT(_c)                                                                           \
  static_cast<Context*>(proxy_wasm::exports::ContextOrEffectiveContext(                            \
      static_cast<proxy_wasm::ContextBase*>((void)_c, proxy_wasm::current_context_)))

namespace Envoy {

using ScopeWeakPtr = std::weak_ptr<Stats::Scope>;

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
  ScopeWeakPtr scope_;
  CREATE_WASM_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT)
};

struct CodeCacheEntry {
  std::string code;
  bool in_progress;
  MonotonicTime use_time;
  MonotonicTime fetch_time;
};

class RemoteDataFetcherAdapter : public Config::DataFetcher::RemoteDataFetcherCallback,
                                 public Event::DeferredDeletable {
public:
  RemoteDataFetcherAdapter(std::function<void(std::string cb)> cb) : cb_(cb) {}
  ~RemoteDataFetcherAdapter() override = default;
  void onSuccess(const std::string& data) override { cb_(data); }
  void onFailure(Config::DataFetcher::FailureReason) override { cb_(""); }
  void setFetcher(std::unique_ptr<Config::DataFetcher::RemoteDataFetcher>&& fetcher) {
    fetcher_ = std::move(fetcher);
  }

private:
  std::function<void(std::string)> cb_;
  std::unique_ptr<Config::DataFetcher::RemoteDataFetcher> fetcher_;
};

const std::string INLINE_STRING = "<inline>";
// NB: xDS currently does not support failing asynchronously, so we fail immediately
// if remote Wasm code is not cached and do a background fill.
const bool DEFAULT_FAIL_IF_NOT_CACHED = true;
bool fail_if_code_not_cached = DEFAULT_FAIL_IF_NOT_CACHED;
const int CODE_CACHE_SECONDS_NEGATIVE_CACHING = 10;
const int CODE_CACHE_SECONDS_CACHING_TTL = 24 * 3600; // 24 hours.
MonotonicTime::duration cache_time_offset_for_testing{};

std::atomic<int64_t> active_wasms;
std::mutex code_cache_mutex;
std::unordered_map<std::string, CodeCacheEntry>* code_cache = nullptr;
CreateWasmStats* create_wasm_stats = nullptr;

// Downcast WasmBase to the actual Wasm.
inline Wasm* getWasm(WasmHandleSharedPtr& base_wasm_handle) {
  return static_cast<Wasm*>(base_wasm_handle->wasm().get());
}

} // namespace

std::string anyToBytes(const ProtobufWkt::Any& any) {
  if (any.Is<ProtobufWkt::StringValue>()) {
    ProtobufWkt::StringValue s;
    MessageUtil::unpackTo(any, s);
    return s.value();
  }
  if (any.Is<ProtobufWkt::BytesValue>()) {
    Protobuf::BytesValue b;
    MessageUtil::unpackTo(any, b);
    return b.value();
  }
  return any.value();
}

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

void Wasm::error(absl::string_view message) { ENVOY_LOG(error, "Wasm VM failed {}", message); }

void Wasm::setTimerPeriod(uint32_t context_id, std::chrono::milliseconds new_period) {
  auto& period = timer_period_[context_id];
  auto& timer = timer_[context_id];
  bool was_running = timer && period.count() > 0;
  period = new_period;
  if (was_running) {
    timer->disableTimer();
  }
  if (period.count() > 0) {
    timer = dispatcher_.createTimer(
        [weak = std::weak_ptr<Wasm>(std::static_pointer_cast<Wasm>(shared_from_this())),
         context_id]() {
          auto shared = weak.lock();
          if (shared) {
            shared->tickHandler(context_id);
          }
        });
    timer->enableTimer(period);
  }
}

void Wasm::tickHandler(uint32_t root_context_id) {
  auto period = timer_period_.find(root_context_id);
  auto timer = timer_.find(root_context_id);
  if (period == timer_period_.end() || timer == timer_.end() || !on_tick_) {
    return;
  }
  auto context = getContext(root_context_id);
  if (context) {
    context->onTick(0);
  }
  if (timer->second && period->second.count() > 0) {
    timer->second->enableTimer(period->second);
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

// NOLINTNEXTLINE(readability-identifier-naming)
Word resolve_dns(void* raw_context, Word dns_address_ptr, Word dns_address_size, Word token_ptr) {
  auto context = WASM_CONTEXT(raw_context);
  auto root_context = context->isRootContext() ? context : context->rootContext();
  auto path = context->wasmVm()->getMemory(dns_address_ptr, dns_address_size);
  if (!path) {
    return WasmResult::InvalidMemoryAccess;
  }
  // Verify set and verify token_ptr before initiating the async resolve.
  uint32_t token = context->wasm()->nextDnsToken();
  if (!context->wasm()->setDatatype(token_ptr, token)) {
    return WasmResult::InvalidMemoryAccess;
  }
  auto callback = [weak_wasm = std::weak_ptr<Wasm>(context->wasm()->sharedThis()), root_context,
                   context_id = context->id(),
                   token](Envoy::Network::DnsResolver::ResolutionStatus status,
                          std::list<Envoy::Network::DnsResponse>&& response) {
    auto wasm = weak_wasm.lock();
    if (!wasm) {
      return;
    }
    root_context->onResolveDns(token, status, std::move(response));
  };
  if (!context->wasm()->dnsResolver()) {
    context->wasm()->dnsResolver() = context->wasm()->dispatcher().createDnsResolver({}, false);
  }
  context->wasm()->dnsResolver()->resolve(std::string(path.value()), Network::DnsLookupFamily::Auto,
                                          callback);
  return WasmResult::Ok;
}

void Wasm::registerCallbacks() {
  WasmBase::registerCallbacks();
#define _REGISTER(_fn)                                                                             \
  wasm_vm_->registerCallback(                                                                      \
      "env", "envoy_" #_fn, &_fn,                                                                  \
      &proxy_wasm::ConvertFunctionWordToUint32<decltype(_fn), _fn>::convertFunctionWordToUint32)
  _REGISTER(resolve_dns);
#undef _REGISTER
}

void Wasm::getFunctions() {
  WasmBase::getFunctions();
#define _GET(_fn) wasm_vm_->getFunction("envoy_" #_fn, &_fn##_);
  _GET(on_resolve_dns)
#undef _GET
}

proxy_wasm::CallOnThreadFunction Wasm::callOnThreadFunction() {
  auto& dispatcher = dispatcher_;
  return [&dispatcher](const std::function<void()>& f) { return dispatcher.post(f); };
}

ContextBase* Wasm::createContext(const std::shared_ptr<PluginBase>& plugin) {
  if (create_context_for_testing_) {
    return create_context_for_testing_(this, std::static_pointer_cast<Plugin>(plugin));
  }
  return new Context(this, std::static_pointer_cast<Plugin>(plugin));
}

ContextBase* Wasm::createRootContext(const std::shared_ptr<PluginBase>& plugin) {
  if (create_root_context_for_testing_) {
    return create_root_context_for_testing_(this, std::static_pointer_cast<Plugin>(plugin));
  }
  return new Context(this, std::static_pointer_cast<Plugin>(plugin));
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
  if (create_wasm_stats) {
    delete create_wasm_stats;
    create_wasm_stats = nullptr;
  }
}

// TODO: remove this post #4160: Switch default to SimulatedTimeSystem.
void setTimeOffsetForCodeCacheForTesting(MonotonicTime::duration d) {
  cache_time_offset_for_testing = d;
}

static bool createWasmInternal(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                               const Stats::ScopeSharedPtr& scope,
                               Upstream::ClusterManager& cluster_manager,
                               Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                               Runtime::RandomGenerator& random, Api::Api& api,
                               Server::ServerLifecycleNotifier& lifecycle_notifier,
                               Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                               CreateWasmCallback&& cb,
                               CreateContextFn create_root_context_for_testing = nullptr) {
  std::string source, code;
  bool fetch = false;
  if (vm_config.code().has_remote()) {
    auto now = dispatcher.timeSource().monotonicTime() + cache_time_offset_for_testing;
    source = vm_config.code().remote().http_uri().uri();
    std::lock_guard<std::mutex> guard(code_cache_mutex);
    if (!code_cache) {
      code_cache = new std::remove_reference<decltype(*code_cache)>::type;
    }
    Stats::ScopeSharedPtr create_wasm_stats_scope;
    if (!create_wasm_stats || !(create_wasm_stats_scope = create_wasm_stats->scope_.lock())) {
      if (create_wasm_stats) {
        delete create_wasm_stats;
      }
      create_wasm_stats =
          new CreateWasmStats{scope, CREATE_WASM_STATS(POOL_COUNTER_PREFIX(*scope, "wasm."),
                                                       POOL_GAUGE_PREFIX(*scope, "wasm."))};
    }
    // Remove entries older than CODE_CACHE_SECONDS_CACHING_TTL except for our target.
    for (auto it = code_cache->begin(); it != code_cache->end();) {
      if (now - it->second.use_time > std::chrono::seconds(CODE_CACHE_SECONDS_CACHING_TTL) &&
          it->first != vm_config.code().remote().sha256()) {
        it = code_cache->erase(it);
      } else {
        ++it;
      }
    }
    create_wasm_stats->remote_load_cache_entries_.set(code_cache->size());
    auto it = code_cache->find(vm_config.code().remote().sha256());
    if (it != code_cache->end()) {
      it->second.use_time = now;
      if (it->second.in_progress) {
        create_wasm_stats->remote_load_cache_misses_.inc();
        ENVOY_LOG_TO_LOGGER(Envoy::Logger::Registry::getLog(Envoy::Logger::Id::wasm), warn,
                            "createWasm: failed to load (in progress) from {}", source);
        cb(nullptr);
      }
      code = it->second.code;
      if (code.empty()) {
        if (now - it->second.fetch_time <
            std::chrono::seconds(CODE_CACHE_SECONDS_NEGATIVE_CACHING)) {
          create_wasm_stats->remote_load_cache_negative_hits_.inc();
          ENVOY_LOG_TO_LOGGER(Envoy::Logger::Registry::getLog(Envoy::Logger::Id::wasm), warn,
                              "createWasm: failed to load (cached) from {}", source);
          cb(nullptr);
        }
        fetch = true; // Fetch failed, retry.
        it->second.in_progress = true;
        it->second.fetch_time = now;
      } else {
        create_wasm_stats->remote_load_cache_hits_.inc();
      }
    } else {
      fetch = true; // Not in cache, fetch.
      auto& e = (*code_cache)[vm_config.code().remote().sha256()];
      e.in_progress = true;
      e.use_time = e.fetch_time = now;
      create_wasm_stats->remote_load_cache_entries_.set(code_cache->size());
      create_wasm_stats->remote_load_cache_misses_.inc();
    }
  } else if (vm_config.code().has_local()) {
    code = Config::DataSource::read(vm_config.code().local(), true, api);
    source = Config::DataSource::getPath(vm_config.code().local())
                 .value_or(code.empty() ? EMPTY_STRING : INLINE_STRING);
  }

  auto complete_cb = [cb, vm_config, plugin, scope, &cluster_manager, &dispatcher,
                      &lifecycle_notifier,
                      create_root_context_for_testing](std::string code) -> bool {
    if (code.empty()) {
      cb(nullptr);
      return false;
    }
    auto vm_key =
        proxy_wasm::makeVmKey(vm_config.vm_id(), anyToBytes(vm_config.configuration()), code);
    auto wasm = proxy_wasm::createWasm(
        vm_key, code, plugin,
        [&vm_config, &scope, &cluster_manager, &dispatcher,
         &lifecycle_notifier](absl::string_view vm_key) {
          auto wasm = std::make_shared<Wasm>(vm_config.runtime(), vm_config.vm_id(),
                                             anyToBytes(vm_config.configuration()), vm_key, scope,
                                             cluster_manager, dispatcher);
          // NB: we need the shared_ptr to have been created for shared_from_this() to work.
          wasm->initializeLifecycle(lifecycle_notifier);
          return std::static_pointer_cast<WasmHandleBase>(std::make_shared<WasmHandle>(wasm));
        },
        [&dispatcher, create_root_context_for_testing](
            const WasmHandleBaseSharedPtr& base_wasm) -> WasmHandleBaseSharedPtr {
          auto wasm =
              std::make_shared<Wasm>(std::static_pointer_cast<WasmHandle>(base_wasm), dispatcher);
          wasm->setCreateContextForTesting(nullptr, create_root_context_for_testing);
          return std::make_shared<WasmHandle>(wasm);
        },
        vm_config.allow_precompiled());
    if (!wasm || wasm->wasm()->isFailed()) {
      ENVOY_LOG_TO_LOGGER(Envoy::Logger::Registry::getLog(Envoy::Logger::Id::wasm), trace,
                          "Unable to create Wasm");
      cb(nullptr);
      return false;
    }
    cb(std::static_pointer_cast<WasmHandle>(wasm));
    return true;
  };

  if (fetch) {
    auto holder = std::make_shared<std::unique_ptr<Event::DeferredDeletable>>();
    auto fetch_callback = [vm_config, complete_cb, source, &dispatcher, scope,
                           holder](const std::string& code) {
      {
        std::lock_guard<std::mutex> guard(code_cache_mutex);
        auto& e = (*code_cache)[vm_config.code().remote().sha256()];
        e.in_progress = false;
        e.code = code;
        Stats::ScopeSharedPtr create_wasm_stats_scope;
        if (!create_wasm_stats || !(create_wasm_stats_scope = create_wasm_stats->scope_.lock())) {
          if (create_wasm_stats) {
            delete create_wasm_stats;
          }
          create_wasm_stats =
              new CreateWasmStats{scope, CREATE_WASM_STATS(POOL_COUNTER_PREFIX(*scope, "wasm."),
                                                           POOL_GAUGE_PREFIX(*scope, "wasm."))};
        }
        if (code.empty()) {
          create_wasm_stats->remote_load_fetch_failures_.inc();
        } else {
          create_wasm_stats->remote_load_fetch_successes_.inc();
        }
        create_wasm_stats->remote_load_cache_entries_.set(code_cache->size());
      }
      if (!fail_if_code_not_cached) {
        if (code.empty()) {
          ENVOY_LOG_TO_LOGGER(Envoy::Logger::Registry::getLog(Envoy::Logger::Id::wasm), trace,
                              "Failed to load Wasm code (fetch failed) from {}", source);
        }
        complete_cb(code);
      }
      // NB: must be deleted explicitly.
      if (*holder) {
        dispatcher.deferredDelete(Envoy::Event::DeferredDeletablePtr{holder->release()});
      }
    };
    if (fail_if_code_not_cached) {
      auto adapter = std::make_unique<RemoteDataFetcherAdapter>(fetch_callback);
      auto fetcher = std::make_unique<Config::DataFetcher::RemoteDataFetcher>(
          cluster_manager, vm_config.code().remote().http_uri(), vm_config.code().remote().sha256(),
          *adapter);
      auto fetcher_ptr = fetcher.get();
      adapter->setFetcher(std::move(fetcher));
      *holder = std::move(adapter);
      fetcher_ptr->fetch();
      ENVOY_LOG_TO_LOGGER(Envoy::Logger::Registry::getLog(Envoy::Logger::Id::wasm), trace,
                          fmt::format("Failed to load Wasm code (fetching) from {}", source));
      cb(nullptr);
    } else {
      remote_data_provider = std::make_unique<Config::DataSource::RemoteAsyncDataProvider>(
          cluster_manager, init_manager, vm_config.code().remote(), dispatcher, random, true,
          fetch_callback);
    }
  } else {
    return complete_cb(code);
  }
  return true;
}

bool createWasm(const VmConfig& vm_config, const PluginSharedPtr& plugin,
                const Stats::ScopeSharedPtr& scope, Upstream::ClusterManager& cluster_manager,
                Init::Manager& init_manager, Event::Dispatcher& dispatcher,
                Runtime::RandomGenerator& random, Api::Api& api,
                Envoy::Server::ServerLifecycleNotifier& lifecycle_notifier,
                Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                CreateWasmCallback&& cb, CreateContextFn create_root_context_for_testing) {
  return createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher,
                            random, api, lifecycle_notifier, remote_data_provider, std::move(cb),
                            create_root_context_for_testing);
}

WasmHandleSharedPtr getOrCreateThreadLocalWasm(const WasmHandleSharedPtr& base_wasm,
                                               const PluginSharedPtr& plugin,
                                               Event::Dispatcher& dispatcher,
                                               CreateContextFn create_root_context_for_testing) {
  auto wasm_handle = proxy_wasm::getOrCreateThreadLocalWasm(
      std::static_pointer_cast<WasmHandle>(base_wasm), plugin,
      [&dispatcher, create_root_context_for_testing](
          const WasmHandleBaseSharedPtr& base_wasm) -> WasmHandleBaseSharedPtr {
        auto wasm =
            std::make_shared<Wasm>(std::static_pointer_cast<WasmHandle>(base_wasm), dispatcher);
        wasm->setCreateContextForTesting(nullptr, create_root_context_for_testing);
        return std::make_shared<WasmHandle>(wasm);
      });
  return std::static_pointer_cast<WasmHandle>(wasm_handle);
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
