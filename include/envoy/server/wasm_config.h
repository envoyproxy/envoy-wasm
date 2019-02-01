#pragma once

#include "envoy/api/api.h"
#include "envoy/common/pure.h"
#include "envoy/event/dispatcher.h"
#include "envoy/server/wasm.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContext {
public:
  virtual ~WasmFactoryContext() {}

  /**
   * @return Event::Dispatcher& the main thread's dispatcher. This dispatcher should be used
   *         for all singleton processing.
   */
  virtual Event::Dispatcher& dispatcher() PURE;

  /**
   * @return Api::Api& a reference to the api object.
   */
  virtual Api::Api& api() PURE;
};

/**
 * Implemented by Wasm and registered via Registry::registerFactory()
 * or the convenience class RegistryFactory.
 */
class WasmFactory {
public:
  virtual ~WasmFactory() {}

  /**
   * Create a particular wasm VM.
   * @param config const ProtoBuf::Message& supplies the config for the resource monitor
   *        implementation.
   * @param context WasmFactoryContext& supplies the resource monitor's context.
   * @return WasmPtr the resource monitor instance. Should not be nullptr.
   * @throw EnvoyException if the implementation is unable to produce an instance with
   *        the provided parameters.
   */
  virtual WasmPtr createWasm(const envoy::config::wasm::v2::WasmConfig& config,
                             WasmFactoryContext& context) PURE;

  /**
   * @return std::string the identifying name for a particular wasm VM produced by the factory.
   */
  virtual std::string name() PURE;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
