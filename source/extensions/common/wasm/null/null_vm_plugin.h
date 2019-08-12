#pragma once

#include "extensions/common/wasm/wasm_vm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {

class NullVmPlugin {
public:
  NullVmPlugin() {}
  virtual ~NullVmPlugin() {}

  virtual void getFunction(absl::string_view functionName, WasmCall0Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall1Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall2Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall3Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall4Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall5Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall8Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall1Word* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall3Word* f) PURE;

  virtual void start() PURE;
};

/**
 * Pseudo-WASM plugins using the NullVM should implement this factory and register via
 * Registry::registerFactory or the convenience class RegisterFactory.
 */
class NullPluginFactory {
public:
  virtual ~NullPluginFactory() {}

  /**
   * Name of the plugin.
   */
  virtual const std::string name() const PURE;

  /**
   * Create an instance of the plugin.
   */
  virtual std::unique_ptr<NullVmPlugin> create() const PURE;
};

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
