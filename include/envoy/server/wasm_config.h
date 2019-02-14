#pragma once

#include "envoy/api/api.h"
#include "envoy/common/pure.h"
#include "envoy/event/dispatcher.h"
#include "envoy/server/wasm.h"
#include "envoy/thread_local/thread_local.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContext {
public:
  virtual ~WasmFactoryContext() {}

  /**
   * @return Upstream::ClusterManager& singleton for use by the entire server.
   */
  virtual Upstream::ClusterManager& clusterManager() PURE;

  /**
   * @return Event::Dispatcher& the main thread's dispatcher. This dispatcher should be used
   *         for all singleton processing.
   */
  virtual Event::Dispatcher& dispatcher() PURE;

  /**
   * @return ThreadLocal::SlotAllocator& the thread local storage engine for the server. This is
   *         used to allow runtime lockless updates to configuration, etc. across multiple threads.
   */
  virtual ThreadLocal::SlotAllocator& threadLocal() PURE;
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

  virtual std::string name() PURE;

  /**
   * Create a particular wasm VM.
   * @param config const ProtoBuf::Message& supplies the config for the resource monitor
   *        implementation.
   * @param context WasmFactoryContext& supplies the resource monitor's context.
   * @return WasmSharedPtr a singleton Wasm service. May be be nullptr if per silo.
   * @throw EnvoyException if the implementation is unable to produce an instance with
   *        the provided parameters.
   */
  virtual WasmSharedPtr createWasm(const envoy::config::wasm::v2::WasmConfig& config,
                                   WasmFactoryContext& context) PURE;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
