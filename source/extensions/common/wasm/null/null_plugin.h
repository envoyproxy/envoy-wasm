#pragma once

#include <memory>

#include "extensions/common/wasm/null/null_vm_plugin.h"
#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {
namespace Plugin {
using LogLevel = Envoy::Logger::Logger::Levels;
using FilterStatus = Network::FilterStatus;
using FilterHeadersStatus = Http::FilterHeadersStatus;
using FilterMetadataStatus = Http::FilterMetadataStatus;
using FilterTrailersStatus = Http::FilterTrailersStatus;
using FilterDataStatus = Http::FilterDataStatus;
using GrpcStatus = Envoy::Grpc::Status::WellKnownGrpcStatus;
using MetricType = Envoy::Extensions::Common::Wasm::Context::MetricType;
using PeerType = Envoy::Extensions::Common::Wasm::Context::PeerType;
using WasmResult = Envoy::Extensions::Common::Wasm::WasmResult;
using StringView = absl::string_view;
template <typename T> using Optional = absl::optional<T>;
} // namespace Plugin
} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy

#include "extensions/common/wasm/null/wasm_api_impl.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {
namespace Plugin {
#include "api/wasm/cpp/proxy_wasm_api.h"
} // namespace Plugin

/**
 * Registry for Plugin implementation.
 */
struct NullPluginRegistry {
  uint32_t (*proxy_validate_configuration_)(uint32_t root_context_id,
                                            uint32_t plugin_configuration_size) = nullptr;
  uint32_t (*proxy_on_context_create_)(uint32_t context_id, uint32_t parent_context_id) = nullptr;
  uint32_t (*proxy_on_vm_start_)(uint32_t root_context_id,
                                 uint32_t vm_configuration_size) = nullptr;
  uint32_t (*proxy_on_configure_)(uint32_t root_context_id,
                                  uint32_t plugin_configuration_size) = nullptr;
  void (*proxy_on_tick_)(uint32_t context_id) = nullptr;
  uint32_t (*proxy_on_done_)(uint32_t context_id) = nullptr;
  void (*proxy_on_delete_)(uint32_t context_id) = nullptr;
  std::unordered_map<std::string, Plugin::RootFactory> root_factories;
  std::unordered_map<std::string, Plugin::ContextFactory> context_factories;
};

/**
 * Base class for all plugins, subclass to create a new plugin.
 * NB: this class must implement
 */
class NullPlugin : public NullVmPlugin {
public:
  using NewContextFnPtr = std::unique_ptr<Context> (*)(uint32_t /* id */);

  explicit NullPlugin(NullPluginRegistry* registry) : registry_(registry) {}
  NullPlugin(const NullPlugin& other) : registry_(other.registry_) {}

#define _DECLARE_OVERRIDE(_t) void getFunction(absl::string_view function_name, _t* f) override;
  FOR_ALL_WASM_VM_EXPORTS(_DECLARE_OVERRIDE)
#undef _DECLARE_OVERRIDE

  bool validateConfiguration(uint64_t root_context_id, uint64_t plugin_configuration_size);
  bool onStart(uint64_t root_context_id, uint64_t vm_configuration_size);
  bool onConfigure(uint64_t root_context_id, uint64_t plugin_configuration_size);
  void onTick(uint64_t root_context_id);
  void onQueueReady(uint64_t root_context_id, uint64_t token);

  void onCreate(uint64_t context_id, uint64_t root_context_id);

  uint64_t onNewConnection(uint64_t context_id);
  uint64_t onDownstreamData(uint64_t context_id, uint64_t data_length, uint64_t end_of_stream);
  uint64_t onUpstreamData(uint64_t context_id, uint64_t data_length, uint64_t end_of_stream);
  void onDownstreamConnectionClose(uint64_t context_id, uint64_t peer_type);
  void onUpstreamConnectionClose(uint64_t context_id, uint64_t peer_type);

  uint64_t onRequestHeaders(uint64_t context_id, uint64_t headers);
  uint64_t onRequestBody(uint64_t context_id, uint64_t body_buffer_length, uint64_t end_of_stream);
  uint64_t onRequestTrailers(uint64_t context_id, uint64_t trailers);
  uint64_t onRequestMetadata(uint64_t context_id, uint64_t elements);

  uint64_t onResponseHeaders(uint64_t context_id, uint64_t headers);
  uint64_t onResponseBody(uint64_t context_id, uint64_t body_buffer_length, uint64_t end_of_stream);
  uint64_t onResponseTrailers(uint64_t context_id, uint64_t trailers);
  uint64_t onResponseMetadata(uint64_t context_id, uint64_t elements);

  void onHttpCallResponse(uint64_t context_id, uint64_t token, uint64_t headers, uint64_t body_size,
                          uint64_t trailers);

  void onGrpcReceive(uint64_t context_id, uint64_t token, size_t body_size);
  void onGrpcClose(uint64_t context_id, uint64_t token, uint64_t status_code);
  void onGrpcReceiveInitialMetadata(uint64_t context_id, uint64_t token, uint64_t headers);
  void onGrpcReceiveTrailingMetadata(uint64_t context_id, uint64_t token, uint64_t trailers);

  void onLog(uint64_t context_id);
  uint64_t onDone(uint64_t context_id);
  void onDelete(uint64_t context_id);

  Plugin::RootContext* getRoot(absl::string_view root_id);
  Plugin::Context* getContext(uint64_t context_id);
  Plugin::RootContext* getRootContext(uint64_t context_id);
  Plugin::ContextBase* getContextBase(uint64_t context_id);

private:
  Plugin::Context* ensureContext(uint64_t context_id, uint64_t root_context_id);
  Plugin::RootContext* ensureRootContext(uint64_t context_id);

  NullPluginRegistry* registry_{};
  std::unordered_map<std::string, Plugin::RootContext*> root_context_map_;
  std::unordered_map<int64_t, std::unique_ptr<Plugin::ContextBase>> context_map_;
};

#define NULL_PLUGIN_REGISTRY                                                                       \
  extern ThreadSafeSingleton<Envoy::Extensions::Common::Wasm::Null::NullPluginRegistry>            \
      null_plugin_registry_;                                                                       \
  extern Envoy::Extensions::Common::Wasm::Null::NullPluginRegistry* context_registry_;             \
  struct RegisterContextFactory {                                                                  \
    explicit RegisterContextFactory(                                                               \
        Envoy::Extensions::Common::Wasm::Null::Plugin::ContextFactory context_factory,             \
        Envoy::Extensions::Common::Wasm::Null::Plugin::RootFactory root_factory = nullptr,         \
        StringView root_id = "") {                                                                 \
      if (!context_registry_) {                                                                    \
        context_registry_ = new Envoy::Extensions::Common::Wasm::Null::NullPluginRegistry;         \
      }                                                                                            \
      context_registry_->context_factories[std::string(root_id)] = context_factory;                \
      context_registry_->root_factories[std::string(root_id)] = root_factory;                      \
    }                                                                                              \
    explicit RegisterContextFactory(                                                               \
        Envoy::Extensions::Common::Wasm::Null::Plugin::RootFactory root_factory,                   \
        StringView root_id = "") {                                                                 \
      if (!context_registry_) {                                                                    \
        context_registry_ = new Envoy::Extensions::Common::Wasm::Null::NullPluginRegistry;         \
      }                                                                                            \
      context_registry_->root_factories[std::string(root_id)] = root_factory;                      \
    }                                                                                              \
  };

#define START_WASM_PLUGIN(_name)                                                                   \
  namespace Envoy {                                                                                \
  namespace Extensions {                                                                           \
  namespace Common {                                                                               \
  namespace Wasm {                                                                                 \
  namespace Null {                                                                                 \
  namespace Plugin {                                                                               \
  namespace _name {                                                                                \
  NULL_PLUGIN_REGISTRY

#define END_WASM_PLUGIN                                                                            \
  }                                                                                                \
  }                                                                                                \
  }                                                                                                \
  }                                                                                                \
  }                                                                                                \
  }                                                                                                \
  }

#define WASM_EXPORT(_t, _f, _a)                                                                    \
  _t _f _a;                                                                                        \
  static int register_export_##_f() {                                                              \
    null_plugin_registry_.get()._f##_ = _f;                                                        \
    return 0;                                                                                      \
  };                                                                                               \
  static int register_export_##_f##_ = register_export_##_f();                                     \
  _t _f _a

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
