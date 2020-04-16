#pragma once

#include "envoy/api/api.h"
#include "envoy/common/pure.h"
#include "envoy/server/instance.h"
#include "envoy/server/wasm.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContext {
public:
  virtual ~WasmFactoryContext() = default;

  /**
   * @return Server::Instance&.
   */
  virtual Server::Instance& server() PURE;

  /**
   * @return Stats::ScopeSharedPtr the service's stats scope.
   */
  virtual Stats::ScopeSharedPtr& scope() PURE;
};

/**
 * Implemented by Wasm and registered via Registry::registerFactory()
 * or the convenience class RegistryFactory.
 */
class WasmFactory {
public:
  virtual ~WasmFactory() = default;

  virtual std::string name() PURE;

  static std::string category() { return "wasm_vm"; }

  /**
   * Create a particular wasm VM.
   * @param config const ProtoBuf::Message& supplies the config for the resource monitor
   *        implementation.
   * @param context WasmFactoryContext& supplies the resource monitor's context.
   * @param cb CreateWasmServiceCallback&& supplies the callback to be called after wasm is created.
   * @throw EnvoyException if the implementation is unable to produce an instance with
   *        the provided parameters.
   */
  virtual void createWasm(const envoy::config::wasm::v3::WasmService& config,
                          WasmFactoryContext& context, CreateWasmServiceCallback&& cb) PURE;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
