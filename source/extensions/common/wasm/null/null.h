#pragma once

#include <memory>

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {
namespace Plugin {
using LogLevel = Envoy::Logger::Logger::levels;
using FilterHeadersStatus = Http::FilterHeadersStatus;
using FilterMetadataStatus = Http::FilterMetadataStatus;
using FilterTrailersStatus = Http::FilterTrailersStatus;
using FilterDataStatus = Http::FilterDataStatus;
using GrpcStatus = Envoy::Grpc::Status::GrpcStatus;
using MetricType = Envoy::Extensions::Common::Wasm::Context::MetricType;
using StringView = absl::string_view;
using EnumType = int32_t;
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
#include "api/wasm/cpp/proxy_wasm_impl.h"
} // namespace Plugin

/**
 * Registry for roots.
 */
struct NullVmPluginRootRegistry {
  std::unordered_map<std::string, Plugin::RootFactory> root_factories;
  std::unordered_map<std::string, Plugin::ContextFactory> context_factories;
};

class NullVmPlugin {
public:
  using NewContextFnPtr = std::unique_ptr<Context> (*)(uint32_t /* id */);

  explicit NullVmPlugin(NullVmPluginRootRegistry* registry) : registry_(registry) {}
  NullVmPlugin(const NullVmPlugin& other) : registry_(other.registry_) {}

  void start() {}
  void onStart(uint64_t root_context_id, uint64_t root_id_ptr, uint64_t root_id_size);
  void onConfigure(uint64_t root_context_id, uint64_t ptr, uint64_t size);
  void onTick(uint64_t root_context_id);
  void onQueueReady(uint64_t root_context_id, uint64_t token);

  void onCreate(uint64_t context_id, uint64_t root_context_id);

  uint64_t onRequestHeaders(uint64_t context_id);
  uint64_t onRequestBody(uint64_t context_id, uint64_t body_buffer_length, uint64_t end_of_stream);
  uint64_t onRequestTrailers(uint64_t context_id);
  uint64_t onRequestMetadata(uint64_t context_id);

  uint64_t onResponseHeaders(uint64_t context_id);
  uint64_t onResponseBody(uint64_t context_id, uint64_t body_buffer_length, uint64_t end_of_stream);
  uint64_t onResponseTrailers(uint64_t context_id);
  uint64_t onResponseMetadata(uint64_t context_id);

  void onHttpCallResponse(uint64_t context_id, uint64_t token, uint64_t header_pairs_ptr,
                          uint64_t header_pairs_size, uint64_t body_ptr, uint64_t body_size,
                          uint64_t trailer_pairs_ptr, uint64_t trailer_pairs_size);

  void onGrpcReceive(uint64_t context_id, uint64_t token, uint64_t response_ptr,
                     uint64_t response_size);
  void onGrpcClose(uint64_t context_id, uint64_t token, uint64_t status_code,
                   uint64_t status_message_ptr, uint64_t status_message_size);
  void onGrpcCreateInitialMetadata(uint64_t context_id, uint64_t token);
  void onGrpcReceiveInitialMetadata(uint64_t context_id, uint64_t token);
  void onGrpcReceiveTrailingMetadata(uint64_t context_id, uint64_t token);

  void onLog(uint64_t context_id);
  void onDone(uint64_t context_id);
  void onDelete(uint64_t context_id);

  Plugin::RootContext* getRoot(absl::string_view root_id);

private:
  Plugin::Context* ensureContext(uint64_t context_id, uint64_t root_context_id);
  Plugin::RootContext* ensureRootContext(uint64_t context_id,
                                         std::unique_ptr<Plugin::WasmData> root_id);
  Plugin::Context* getContext(uint64_t context_id);
  Plugin::RootContext* getRootContext(uint64_t context_id);
  Plugin::ContextBase* getContextBase(uint64_t context_id);

  NullVmPluginRootRegistry* registry_{};
  std::unordered_map<std::string, Plugin::RootContext*> root_context_map_;
  std::unordered_map<int64_t, std::unique_ptr<Plugin::ContextBase>> context_map_;
};

/**
 * Pseudo-WASM plugins using the NullVM should implement this factory and register via
 * Registry::registerFactory or the convenience class RegisterFactory.
 */
class NullVmPluginFactory {
public:
  virtual ~NullVmPluginFactory() {}

  /**
   * Name of the plugin.
   */
  virtual const std::string name() const PURE;

  /**
   * Create an instance of the plugin.
   */
  virtual std::unique_ptr<NullVmPlugin> create() const PURE;
};

#define NULL_PLUGIN_ROOT_REGISTRY                                                                  \
  extern NullVmPluginRootRegistry* context_registry_;                                              \
  struct RegisterContextFactory {                                                                  \
    explicit RegisterContextFactory(ContextFactory context_factory,                                \
                                    RootFactory root_factory = nullptr, StringView root_id = "") { \
      if (!context_registry_) {                                                                    \
        context_registry_ = new NullVmPluginRootRegistry;                                          \
      }                                                                                            \
      context_registry_->context_factories[std::string(root_id)] = context_factory;                \
      context_registry_->root_factories[std::string(root_id)] = root_factory;                      \
    }                                                                                              \
  };

std::unique_ptr<WasmVm> createVm();

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
