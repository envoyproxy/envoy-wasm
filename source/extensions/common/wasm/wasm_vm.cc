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

void EnvoyWasmVmIntegration::error(absl::string_view message) { ENVOY_LOG(trace, message); }

WasmVmPtr createWasmVm(absl::string_view runtime, const Stats::ScopeSharedPtr& scope) {
  if (runtime.empty()) {
    ENVOY_LOG_TO_LOGGER(Envoy::Logger::Registry::getLog(Envoy::Logger::Id::wasm), warn,
                        "Failed to create Wasm VM with unspecified runtime");
    return nullptr;
  } else if (runtime == WasmRuntimeNames::get().Null) {
    auto wasm = proxy_wasm::createNullVm();
    if (!wasm) {
      return nullptr;
    }
    wasm->integration() = std::make_unique<EnvoyWasmVmIntegration>(scope, runtime, "null");
    return wasm;
  } else if (runtime == WasmRuntimeNames::get().V8) {
    auto wasm = proxy_wasm::createV8Vm();
    if (!wasm) {
      return nullptr;
    }
    wasm->integration() = std::make_unique<EnvoyWasmVmIntegration>(scope, runtime, "v8");
    return wasm;
  } else {
    ENVOY_LOG_TO_LOGGER(
        Envoy::Logger::Registry::getLog(Envoy::Logger::Id::wasm), warn,
        "Failed to create Wasm VM using {} runtime. Envoy was compiled without support for it",
        runtime);
    return nullptr;
  }
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
