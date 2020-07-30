#include "extensions/common/wasm/wasm_vm.h"

#include <algorithm>
#include <memory>

#include "extensions/common/wasm/context.h"
#include "extensions/common/wasm/ext/envoy_null_vm_wasm_api.h"
#include "extensions/common/wasm/wasm_extension.h"
#include "extensions/common/wasm/well_known_names.h"

#include "include/proxy-wasm/null.h"
#include "include/proxy-wasm/null_plugin.h"
#include "include/proxy-wasm/v8.h"

using ContextBase = proxy_wasm::ContextBase;
using Word = proxy_wasm::Word;

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

void EnvoyWasmVmIntegration::error(absl::string_view message) { ENVOY_LOG(trace, message); }

bool EnvoyWasmVmIntegration::getNullVmFunction(absl::string_view function_name, bool returns_word,
                                               int number_of_arguments,
                                               proxy_wasm::NullPlugin* plugin,
                                               void* ptr_to_function_return) {
  if (function_name == "envoy_on_resolve_dns" && returns_word == false &&
      number_of_arguments == 3) {
    *reinterpret_cast<proxy_wasm::WasmCallVoid<3>*>(ptr_to_function_return) =
        [plugin](ContextBase* context, Word context_id, Word token, Word result_size) {
          proxy_wasm::SaveRestoreContext saved_context(context);
          // Need to add a new API header available to both .wasm and null vm targets.
          auto context_base = plugin->getContextBase(context_id);
          if (auto root = context_base->asRoot()) {
            static_cast<proxy_wasm::null_plugin::EnvoyRootContext*>(root)->onResolveDns(
                token, result_size);
          } else {
            static_cast<proxy_wasm::null_plugin::EnvoyContext*>(context_base->asContext())
                ->onResolveDns(token, result_size);
          }
        };
    return true;
  }
  return false;
}

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
    wasm->integration() = getWasmExtension()->createEnvoyWasmVmIntegration(scope, runtime, "null");
    return wasm;
  } else if (runtime == WasmRuntimeNames::get().V8) {
    auto wasm = proxy_wasm::createV8Vm();
    if (!wasm) {
      return nullptr;
    }
    wasm->integration() = getWasmExtension()->createEnvoyWasmVmIntegration(scope, runtime, "null");
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
