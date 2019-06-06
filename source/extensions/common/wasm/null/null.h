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
using GrpcStatus = ProtobufUtil::error::Code;
using MetricType = Envoy::Extensions::Common::Wasm::Context::MetricType;
using StringView = absl::string_view;
using EnumType = uint32_t;
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

class NullVmPlugin {
public:
  using Context = Plugin::Context;
  using NewContextFnPtr = std::unique_ptr<Context> (*)(uint32_t /* id */);

  NullVmPlugin(NewContextFnPtr new_context) : new_context_(new_context) {}
  NullVmPlugin(const NullVmPlugin& other) : new_context_(other.new_context_) {}

  std::unique_ptr<Context> newContext(uint64_t context_id) { return new_context_(context_id); }

  void start() {}
  void onStart();
  void onConfigure(uint64_t ptr, uint64_t size);
  void onTick();

  void onCreate(uint64_t context_id);

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

private:
  Context* ensureContext(uint64_t context_id);
  Context* getContext(uint64_t context_id);

  NewContextFnPtr new_context_;
  std::unordered_map<int64_t, std::unique_ptr<Context>> context_map_;
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

std::unique_ptr<WasmVm> createVm();

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
