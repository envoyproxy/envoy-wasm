#pragma once

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

/**
 * Well-known wasm VM names.
 * NOTE: New wasm VMs should use the well known name: envoy.wasm_vm.name.
 */
class WasmVmValues {
public:
  // V8 (https://v8.dev) WASM VM.
  const std::string v8 = "envoy.wasm.vm.v8";
  // WAVM (https://github.com/WAVM/WAVM) Wasm VM.
  const std::string Wavm = "envoy.wasm.vm.wavm";
};

typedef ConstSingleton<WasmVmValues> WasmVmNames;

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
