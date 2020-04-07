// NOLINT(namespace-envoy)
#include "include/proxy-wasm/null_plugin.h"

namespace proxy_wasm {
namespace null_plugin {
namespace HttpFilterTestPlugin {
NullPluginRegistry* context_registry_;
} // namespace HttpFilterTestPlugin

RegisterNullVmPluginFactory register_common_wasm_test_cpp_plugin("HttpFilterTestPlugin", []() {
  return std::make_unique<NullPlugin>(HttpFilterTestPlugin::context_registry_);
});

} // namespace null_plugin
} // namespace proxy_wasm
