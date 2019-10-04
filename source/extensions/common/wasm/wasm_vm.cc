#include "extensions/common/wasm/wasm_vm.h"

#include <memory>

#include "extensions/common/wasm/null/null.h"

#ifdef ENVOY_WASM_V8
#include "extensions/common/wasm/v8/v8.h"
#endif
#ifdef ENVOY_WASM_WAVM
#include "extensions/common/wasm/wavm/wavm.h"
#endif
#include "extensions/common/wasm/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

thread_local Envoy::Extensions::Common::Wasm::Context* current_context_ = nullptr;
thread_local uint32_t effective_context_id_ = 0;

std::unique_ptr<WasmVm> createWasmVm(absl::string_view wasm_vm) {
  if (wasm_vm.empty()) {
    throw WasmVmException("Failed to create WASM VM with unspecified runtime.");
  } else if (wasm_vm == WasmRuntimeNames::get().Null) {
    return Null::createVm();
  } else
#ifdef ENVOY_WASM_V8
      if (wasm_vm == WasmRuntimeNames::get().v8) {
    return V8::createVm();
  } else
#endif
#ifdef ENVOY_WASM_WAVM
      if (wasm_vm == WasmRuntimeNames::get().Wavm) {
    return Wavm::createVm();
  } else
#endif
  {
    throw WasmVmException(fmt::format(
        "Failed to create WASM VM using {} runtime. Envoy was compiled without support for it.",
        wasm_vm));
  }
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
