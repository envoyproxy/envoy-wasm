#include "extensions/common/wasm/wasm_vm.h"

#include <algorithm>
#include <memory>

#include "extensions/common/wasm/well_known_names.h"

#include "include/proxy-wasm/null.h"
#include "include/proxy-wasm/v8.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

void EnvoyWasmVmIntegration::error(absl::string_view message) {
  throw WasmException(std::string(message));
}

WasmVmPtr createWasmVm(absl::string_view runtime, const Stats::ScopeSharedPtr& scope) {
  if (runtime.empty()) {
    throw WasmException("Failed to create Wasm VM with unspecified runtime.");
  } else if (runtime == WasmRuntimeNames::get().Null) {
    auto wasm = proxy_wasm::createNullVm();
    wasm->integration() = std::make_unique<EnvoyWasmVmIntegration>(scope, runtime, "null");
    return wasm;
  } else if (runtime == WasmRuntimeNames::get().V8) {
    auto wasm = proxy_wasm::createV8Vm();
    wasm->integration() = std::make_unique<EnvoyWasmVmIntegration>(scope, runtime, "v8");
    return wasm;
  } else {
    throw WasmException(fmt::format(
        "Failed to create Wasm VM using {} runtime. Envoy was compiled without support for it.",
        runtime));
  }
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
