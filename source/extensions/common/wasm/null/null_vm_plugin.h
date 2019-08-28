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

#define _DECLARE_PURE(_t) virtual void getFunction(absl::string_view function_name, _t* f) PURE;
  FOR_ALL_WASM_VM_EXPORTS(_DECLARE_PURE)
#undef _DECLARE_PURE

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
