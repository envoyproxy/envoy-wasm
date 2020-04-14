#include "extensions/common/wasm/wasm.h"

#include <stdio.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/config/wasm/v3/wasm.pb.validate.h"
#include "envoy/grpc/status.h"
#include "envoy/http/codes.h"
#include "envoy/local_info/local_info.h"
#include "envoy/server/wasm.h"
#include "envoy/thread_local/thread_local.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/base64.h"
#include "common/common/empty_string.h"
#include "common/common/enum_to_int.h"
#include "common/common/logger.h"
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/common/wasm/foreign.h"
#include "extensions/common/wasm/wasm_state.h"
#include "extensions/common/wasm/well_known_names.h"
#include "extensions/filters/common/expr/context.h"

#include "absl/base/casts.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "eval/eval/field_access.h"
#include "eval/eval/field_backed_list_impl.h"
#include "eval/eval/field_backed_map_impl.h"
#include "eval/public/cel_value.h"
#include "eval/public/value_export_util.h"
#include "openssl/bytestring.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

std::string Sha256(absl::string_view data) {
  std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);
  EVP_MD_CTX* ctx(EVP_MD_CTX_new());
  auto rc = EVP_DigestInit(ctx, EVP_sha256());
  RELEASE_ASSERT(rc == 1, "Failed to init digest context");
  rc = EVP_DigestUpdate(ctx, data.data(), data.size());
  RELEASE_ASSERT(rc == 1, "Failed to update digest");
  rc = EVP_DigestFinal(ctx, digest.data(), nullptr);
  RELEASE_ASSERT(rc == 1, "Failed to finalize digest");
  EVP_MD_CTX_free(ctx);
  return std::string(reinterpret_cast<const char*>(&digest[0]), digest.size());
}

namespace {

std::atomic<int64_t> active_wasm_;

std::string Xor(absl::string_view a, absl::string_view b) {
  ASSERT(a.size() == b.size());
  std::string result;
  result.reserve(a.size());
  for (size_t i = 0; i < a.size(); i++) {
    result.push_back(a[i] ^ b[i]);
  }
  return result;
}

// Map from Wasm Key to the local Wasm instance.
thread_local absl::flat_hash_map<std::string, std::weak_ptr<WasmHandle>> local_wasms_;
// Map from Wasm Key to the base Wasm instance, using a pointer to avoid the initialization fiasco.
ABSL_CONST_INIT absl::Mutex base_wasms_mutex_(absl::kConstInit);
absl::flat_hash_map<std::string, std::weak_ptr<WasmHandle>>* base_wasms_ = nullptr;

const std::string INLINE_STRING = "<inline>";

const uint8_t* decodeVarint(const uint8_t* pos, const uint8_t* end, uint32_t* out) {
  uint32_t ret = 0;
  int shift = 0;
  while (pos < end && (*pos & 0x80)) {
    ret |= (*pos & 0x7f) << shift;
    shift += 7;
    pos++;
  }
  if (pos < end) {
    ret |= *pos << shift;
    pos++;
  }
  *out = ret;
  return pos;
}

} // namespace

Wasm::Wasm(absl::string_view runtime, absl::string_view vm_id, absl::string_view vm_configuration,
           absl::string_view vm_key, Stats::ScopeSharedPtr scope,
           Upstream::ClusterManager& cluster_manager, Event::Dispatcher& dispatcher)
    : vm_id_(std::string(vm_id)), vm_key_(std::string(vm_key)),
      wasm_vm_(Common::Wasm::createWasmVm(runtime, scope)), scope_(scope),
      cluster_manager_(cluster_manager), dispatcher_(dispatcher),
      time_source_(dispatcher.timeSource()), vm_configuration_(vm_configuration),
      wasm_stats_(WasmStats{
          ALL_WASM_STATS(POOL_COUNTER_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")),
                         POOL_GAUGE_PREFIX(*scope_, absl::StrCat("wasm.", runtime, ".")))}) {
  active_wasm_++;
  wasm_stats_.active_.set(active_wasm_);
  wasm_stats_.created_.inc();
  ENVOY_LOG(debug, "Base Wasm created {} now active", active_wasm_);
}

void Wasm::registerCallbacks() {
#define _REGISTER(_fn)                                                                             \
  wasm_vm_->registerCallback(                                                                      \
      "env", #_fn, &Exports::_fn,                                                                  \
      &ConvertFunctionWordToUint32<decltype(Exports::_fn),                                         \
                                   Exports::_fn>::convertFunctionWordToUint32)
  _REGISTER(pthread_equal);
#undef _REGISTER

#define _REGISTER_WASI(_fn)                                                                        \
  wasm_vm_->registerCallback(                                                                      \
      "wasi_unstable", #_fn, &Exports::wasi_unstable_##_fn,                                        \
      &ConvertFunctionWordToUint32<decltype(Exports::wasi_unstable_##_fn),                         \
                                   Exports::wasi_unstable_##_fn>::convertFunctionWordToUint32);    \
  wasm_vm_->registerCallback(                                                                      \
      "wasi_snapshot_preview1", #_fn, &Exports::wasi_unstable_##_fn,                               \
      &ConvertFunctionWordToUint32<decltype(Exports::wasi_unstable_##_fn),                         \
                                   Exports::wasi_unstable_##_fn>::convertFunctionWordToUint32)
  _REGISTER_WASI(fd_write);
  _REGISTER_WASI(fd_read);
  _REGISTER_WASI(fd_seek);
  _REGISTER_WASI(fd_close);
  _REGISTER_WASI(fd_fdstat_get);
  _REGISTER_WASI(environ_get);
  _REGISTER_WASI(environ_sizes_get);
  _REGISTER_WASI(args_get);
  _REGISTER_WASI(args_sizes_get);
  _REGISTER_WASI(proc_exit);
#undef _REGISTER_WASI

  // Calls with the "proxy_" prefix.
#define _REGISTER_PROXY(_fn)                                                                       \
  wasm_vm_->registerCallback(                                                                      \
      "env", "proxy_" #_fn, &Exports::_fn,                                                         \
      &ConvertFunctionWordToUint32<decltype(Exports::_fn),                                         \
                                   Exports::_fn>::convertFunctionWordToUint32);
  _REGISTER_PROXY(log);

  _REGISTER_PROXY(get_status);
  _REGISTER_PROXY(get_configuration);

  _REGISTER_PROXY(set_property);
  _REGISTER_PROXY(get_property);

  _REGISTER_PROXY(continue_request);
  _REGISTER_PROXY(continue_response);
  _REGISTER_PROXY(send_local_response);
  _REGISTER_PROXY(clear_route_cache);

  _REGISTER_PROXY(get_shared_data);
  _REGISTER_PROXY(set_shared_data);

  _REGISTER_PROXY(register_shared_queue);
  _REGISTER_PROXY(resolve_shared_queue);
  _REGISTER_PROXY(dequeue_shared_queue);
  _REGISTER_PROXY(enqueue_shared_queue);

  _REGISTER_PROXY(get_header_map_value);
  _REGISTER_PROXY(add_header_map_value);
  _REGISTER_PROXY(replace_header_map_value);
  _REGISTER_PROXY(remove_header_map_value);
  _REGISTER_PROXY(get_header_map_pairs);
  _REGISTER_PROXY(set_header_map_pairs);
  _REGISTER_PROXY(get_header_map_size);

  _REGISTER_PROXY(get_buffer_status);
  _REGISTER_PROXY(get_buffer_bytes);
  _REGISTER_PROXY(set_buffer_bytes);

  _REGISTER_PROXY(http_call);

  _REGISTER_PROXY(grpc_call);
  _REGISTER_PROXY(grpc_stream);
  _REGISTER_PROXY(grpc_close);
  _REGISTER_PROXY(grpc_cancel);
  _REGISTER_PROXY(grpc_send);

  _REGISTER_PROXY(set_tick_period_milliseconds);
  _REGISTER_PROXY(get_current_time_nanoseconds);

  _REGISTER_PROXY(define_metric);
  _REGISTER_PROXY(increment_metric);
  _REGISTER_PROXY(record_metric);
  _REGISTER_PROXY(get_metric);

  _REGISTER_PROXY(set_effective_context);
  _REGISTER_PROXY(done);
  _REGISTER_PROXY(call_foreign_function);
#undef _REGISTER_PROXY
}

void Wasm::getFunctions() {
#define _GET(_fn) wasm_vm_->getFunction(#_fn, &_fn##_);
  _GET(_start);
  _GET(__wasm_call_ctors);

  _GET(malloc);
  if (!malloc_) {
    throw WasmException("WASM missing malloc");
  }
#undef _GET

#define _GET_PROXY(_fn) wasm_vm_->getFunction("proxy_" #_fn, &_fn##_);
  _GET_PROXY(abi_version_0_1_0);
  if (!abi_version_0_1_0_) {
    throw WasmException("WASM missing Proxy-Wasm ABI version or requires an unsupported version.");
  }

  _GET_PROXY(validate_configuration);
  _GET_PROXY(on_vm_start);
  _GET_PROXY(on_configure);
  _GET_PROXY(on_tick);

  _GET_PROXY(on_context_create);

  _GET_PROXY(on_new_connection);
  _GET_PROXY(on_downstream_data);
  _GET_PROXY(on_upstream_data);
  _GET_PROXY(on_downstream_connection_close);
  _GET_PROXY(on_upstream_connection_close);

  _GET_PROXY(on_request_headers);
  _GET_PROXY(on_request_body);
  _GET_PROXY(on_request_trailers);
  _GET_PROXY(on_request_metadata);
  _GET_PROXY(on_response_headers);
  _GET_PROXY(on_response_body);
  _GET_PROXY(on_response_trailers);
  _GET_PROXY(on_response_metadata);
  _GET_PROXY(on_http_call_response);
  _GET_PROXY(on_grpc_receive);
  _GET_PROXY(on_grpc_close);
  _GET_PROXY(on_grpc_receive_initial_metadata);
  _GET_PROXY(on_grpc_receive_trailing_metadata);
  _GET_PROXY(on_queue_ready);
  _GET_PROXY(on_done);
  _GET_PROXY(on_log);
  _GET_PROXY(on_delete);
#undef _GET_PROXY
}

Wasm::Wasm(WasmHandleSharedPtr& base_wasm_handle, Event::Dispatcher& dispatcher)
    : std::enable_shared_from_this<Wasm>(*base_wasm_handle->wasm()),
      vm_id_(base_wasm_handle->wasm()->vm_id_), vm_key_(base_wasm_handle->wasm()->vm_key_),
      started_from_(base_wasm_handle->wasm()->wasm_vm()->cloneable()),
      scope_(base_wasm_handle->wasm()->scope_),
      cluster_manager_(base_wasm_handle->wasm()->cluster_manager_), dispatcher_(dispatcher),
      time_source_(dispatcher.timeSource()), base_wasm_handle_(base_wasm_handle),
      wasm_stats_(base_wasm_handle->wasm()->wasm_stats_) {
  if (started_from_ != Cloneable::NotCloneable) {
    wasm_vm_ = base_wasm_handle->wasm()->wasm_vm()->clone();
  } else {
    wasm_vm_ = Common::Wasm::createWasmVm(base_wasm_handle->wasm()->wasm_vm()->runtime(), scope_);
  }
  if (!initialize(base_wasm_handle->wasm()->code(),
                  base_wasm_handle->wasm()->allow_precompiled())) {
    throw WasmException("Failed to load WASM code");
  }
  active_wasm_++;
  wasm_stats_.active_.set(active_wasm_);
  wasm_stats_.created_.inc();
  ENVOY_LOG(debug, "Thread-Local Wasm created {} now active", active_wasm_);
}

Wasm::~Wasm() {
  active_wasm_--;
  wasm_stats_.active_.set(active_wasm_);
  ENVOY_LOG(debug, "~Wasm {} remaining active", active_wasm_);
  for (auto& p : timer_) {
    if (p.second && p.second->enabled()) {
      p.second->disableTimer();
    }
  }
}

bool Wasm::initialize(const std::string& code, bool allow_precompiled) {
  if (!wasm_vm_) {
    return false;
  }

  if (started_from_ == Cloneable::NotCloneable) {
    auto ok = wasm_vm_->load(code, allow_precompiled);
    if (!ok) {
      return false;
    }
    auto metadata = wasm_vm_->getCustomSection("emscripten_metadata");
    if (!metadata.empty()) {
      // See https://github.com/emscripten-core/emscripten/blob/incoming/tools/shared.py#L3059
      is_emscripten_ = true;
      auto start = reinterpret_cast<const uint8_t*>(metadata.data());
      auto end = reinterpret_cast<const uint8_t*>(metadata.data() + metadata.size());
      start = decodeVarint(start, end, &emscripten_metadata_major_version_);
      start = decodeVarint(start, end, &emscripten_metadata_minor_version_);
      start = decodeVarint(start, end, &emscripten_abi_major_version_);
      start = decodeVarint(start, end, &emscripten_abi_minor_version_);
      uint32_t temp;
      if (emscripten_metadata_major_version_ > 0 || emscripten_metadata_minor_version_ > 1) {
        // metadata 0.2 - added: wasm_backend.
        start = decodeVarint(start, end, &temp);
      }
      start = decodeVarint(start, end, &temp);
      start = decodeVarint(start, end, &temp);
      if (emscripten_metadata_major_version_ > 0 || emscripten_metadata_minor_version_ > 0) {
        // metadata 0.1 - added: global_base, dynamic_base, dynamictop_ptr and tempdouble_ptr.
        start = decodeVarint(start, end, &temp);
        start = decodeVarint(start, end, &temp);
        start = decodeVarint(start, end, &temp);
        decodeVarint(start, end, &temp);
        if (emscripten_metadata_major_version_ > 0 || emscripten_metadata_minor_version_ > 2) {
          // metadata 0.3 - added: standalone_wasm.
          start = decodeVarint(start, end, &emscripten_standalone_wasm_);
        }
      }
    }

    code_ = code;
    allow_precompiled_ = allow_precompiled;
  }

  if (started_from_ != Cloneable::InstantiatedModule) {
    registerCallbacks();
    wasm_vm_->link(vm_id_);
  }

  vm_context_ = std::make_shared<Context>(this);
  getFunctions();

  if (started_from_ != Cloneable::InstantiatedModule) {
    // Base VM was already started, so don't try to start cloned VMs again.
    startVm(vm_context_.get());
  }

  return true;
}

Context* Wasm::getOrCreateRootContext(const PluginSharedPtr& plugin) {
  auto root_context = getRootContext(plugin->root_id_);
  if (!root_context) {
    auto context = std::make_unique<Context>(this, plugin);
    root_context = context.get();
    root_contexts_[plugin->root_id_] = std::move(context);
  }
  return root_context;
}

void Wasm::startVm(Context* root_context) {
  /* Call "_start" function, and fallback to "__wasm_call_ctors" if the former is not available. */
  if (_start_) {
    _start_(root_context);
  } else if (__wasm_call_ctors_) {
    __wasm_call_ctors_(root_context);
  }
}

bool Wasm::configure(Context* root_context, PluginSharedPtr plugin,
                     absl::string_view configuration) {
  return root_context->onConfigure(configuration, plugin);
}

Context* Wasm::start(PluginSharedPtr plugin) {
  auto root_id = plugin->root_id_;
  auto it = root_contexts_.find(root_id);
  if (it != root_contexts_.end()) {
    it->second->onStart(vm_configuration(), plugin);
    return it->second.get();
  }
  auto context = std::make_unique<Context>(this, plugin);
  auto context_ptr = context.get();
  root_contexts_[root_id] = std::move(context);
  context_ptr->onStart(vm_configuration(), plugin);
  return context_ptr;
};

void Wasm::startForTesting(std::unique_ptr<Context> context, PluginSharedPtr plugin) {
  auto context_ptr = context.get();
  if (!context->wasm_) {
    // Initialization was delayed till the Wasm object was created.
    context->initializeRoot(this, plugin);
  }
  root_contexts_[plugin->root_id_] = std::move(context);
  // Set the current plugin over the lifetime of the onConfigure call to the RootContext.
  context_ptr->onStart(vm_configuration_, plugin);
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
    timer = dispatcher_.createTimer([weak = std::weak_ptr<Wasm>(shared_from_this()), context_id]() {
      auto shared = weak.lock();
      if (shared) {
        shared->tickHandler(context_id);
      }
    });
    timer->enableTimer(tick_period);
  }
}

void Wasm::tickHandler(uint32_t root_context_id) {
  auto& tick_period = tick_period_[root_context_id];
  auto& timer = timer_[root_context_id];
  if (on_tick_) {
    auto it = contexts_.find(root_context_id);
    if (it == contexts_.end() || !it->second->isRootContext()) {
      return;
    }
    it->second->onTick();
    if (timer && tick_period.count() > 0) {
      timer->enableTimer(tick_period);
    }
  }
}

uint32_t Wasm::allocContextId() {
  while (true) {
    auto id = next_context_id_++;
    // Prevent reuse.
    if (contexts_.find(id) == contexts_.end()) {
      return id;
    }
  }
}

class Wasm::ShutdownHandle : public Envoy::Event::DeferredDeletable {
public:
  ~ShutdownHandle() { wasm_->finishShutdown(); }
  ShutdownHandle(WasmSharedPtr wasm) : wasm_(wasm) {}

private:
  WasmSharedPtr wasm_;
};

void Wasm::startShutdown() {
  bool all_done = true;
  for (auto& p : root_contexts_) {
    if (!p.second->onDone()) {
      all_done = false;
      pending_done_.insert(p.second.get());
    }
  }
  if (!all_done) {
    shutdown_handle_ = std::make_unique<ShutdownHandle>(shared_from_this());
  } else {
    finishShutdown();
  }
}

WasmResult Wasm::done(Context* root_context) {
  auto it = pending_done_.find(root_context);
  if (it == pending_done_.end()) {
    return WasmResult::NotFound;
  }
  pending_done_.erase(it);
  if (pending_done_.empty() && shutdown_handle_) {
    dispatcher_.deferredDelete(std::move(shutdown_handle_));
  }
  return WasmResult::Ok;
}

void Wasm::finishShutdown() {
  for (auto& p : root_contexts_) {
    p.second->onDelete();
  }
}

void Wasm::queueReady(uint32_t root_context_id, uint32_t token) {
  auto it = contexts_.find(root_context_id);
  if (it == contexts_.end() || !it->second->isRootContext()) {
    return;
  }
  it->second->onQueueReady(token);
}

WasmForeignFunction Wasm::getForeignFunction(absl::string_view function_name) {
  auto it = foreign_functions_.find(function_name);
  if (it != foreign_functions_.end()) {
    return it->second;
  }
  auto factory = Registry::FactoryRegistry<ForeignFunctionFactory>::getFactory(function_name);
  if (factory) {
    auto f = factory->create();
    foreign_functions_[function_name] = f;
    return f;
  }
  return WasmForeignFunction();
}

static void createWasmInternal(const VmConfig& vm_config, PluginSharedPtr plugin,
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
    std::unique_ptr<Context> context(context_ptr);
    if (code.empty()) {
      throw WasmException(fmt::format("Failed to load WASM code from {}", source));
    }
    // Construct a unique identifier for the VM based on a hash of the code, vm configuration data
    // and vm_id.
    std::string vm_key = Sha256(vm_config.vm_id());
    vm_key = Xor(vm_key, Sha256(vm_config.configuration()));
    vm_key = Xor(vm_key, Sha256(code));
    vm_key = Base64::encode(&*vm_key.begin(), vm_key.size());

    std::shared_ptr<WasmHandle> wasm;
    {
      absl::MutexLock l(&base_wasms_mutex_);
      if (!base_wasms_) {
        base_wasms_ = new std::remove_reference<decltype(*base_wasms_)>::type;
      }
      auto it = base_wasms_->find(vm_key);
      if (it != base_wasms_->end()) {
        wasm = it->second.lock();
        if (!wasm) {
          base_wasms_->erase(it);
        }
      }
      if (!wasm) {
        wasm = std::make_shared<WasmHandle>(std::make_shared<Wasm>(
            vm_config.runtime(), vm_config.vm_id(), vm_config.configuration(), vm_key, scope,
            cluster_manager, dispatcher));
        if (!wasm->wasm()->initialize(code, vm_config.allow_precompiled())) {
          throw WasmException(fmt::format("Failed to initialize WASM code from {}", source));
        }
        if (!context) {
          wasm->wasm()->start(plugin);
        } else {
          wasm->wasm()->startForTesting(std::move(context), plugin);
        }
        (*base_wasms_)[vm_key] = wasm;
      }
    }

    cb(std::move(wasm));
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

void createWasm(const VmConfig& vm_config, PluginSharedPtr plugin, Stats::ScopeSharedPtr scope,
                Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                Event::Dispatcher& dispatcher, Runtime::RandomGenerator& random, Api::Api& api,
                Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, random,
                     api, nullptr /* root_context_for_testing */, remote_data_provider,
                     std::move(cb));
}

void createWasmForTesting(const envoy::config::wasm::v3::VmConfig& vm_config,
                          PluginSharedPtr plugin, Stats::ScopeSharedPtr scope,
                          Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                          Event::Dispatcher& dispatcher, Runtime::RandomGenerator& random,
                          Api::Api& api, std::unique_ptr<Context> root_context_for_testing,
                          Config::DataSource::RemoteAsyncDataProviderPtr& remote_data_provider,
                          CreateWasmCallback&& cb) {
  createWasmInternal(vm_config, plugin, scope, cluster_manager, init_manager, dispatcher, random,
                     api, std::move(root_context_for_testing), remote_data_provider, std::move(cb));
}

WasmHandleSharedPtr createThreadLocalWasm(WasmHandleSharedPtr& base_wasm, PluginSharedPtr plugin,
                                          absl::string_view configuration,
                                          Event::Dispatcher& dispatcher) {
  auto wasm_handle = std::make_shared<WasmHandle>(std::make_shared<Wasm>(base_wasm, dispatcher));
  Context* root_context = wasm_handle->wasm()->start(plugin);
  if (!wasm_handle->wasm()->configure(root_context, plugin, configuration)) {
    throw WasmException("Failed to configure WASM code");
  }
  local_wasms_[wasm_handle->wasm()->vm_key()] = wasm_handle;
  return wasm_handle;
}

WasmHandleSharedPtr getThreadLocalWasmPtr(absl::string_view vm_key) {
  auto it = local_wasms_.find(vm_key);
  if (it == local_wasms_.end()) {
    return nullptr;
  }
  auto wasm = it->second.lock();
  if (!wasm) {
    local_wasms_.erase(vm_key);
  }
  return wasm;
}

WasmHandleSharedPtr getOrCreateThreadLocalWasm(WasmHandleSharedPtr base_wasm,
                                               PluginSharedPtr plugin,
                                               absl::string_view configuration,
                                               Event::Dispatcher& dispatcher) {
  auto wasm_handle = getThreadLocalWasmPtr(base_wasm->wasm()->vm_key());
  if (wasm_handle) {
    auto root_context = wasm_handle->wasm()->getOrCreateRootContext(plugin);
    if (!wasm_handle->wasm()->configure(root_context, plugin, configuration)) {
      throw WasmException("Failed to configure WASM code");
    }
    return wasm_handle;
  }
  return createThreadLocalWasm(base_wasm, plugin, configuration, dispatcher);
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
