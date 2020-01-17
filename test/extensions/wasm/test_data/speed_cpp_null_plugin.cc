#include "extensions/common/wasm/null/null_plugin.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {
namespace Plugin {
namespace WasmSpeedCpp {
ThreadSafeSingleton<NullPluginRegistry> null_plugin_registry_;
} // namespace WasmSpeedCpp

/**
 * Config registration for a Wasm filter plugin. @see NamedHttpFilterConfigFactory.
 */
class WasmSpeedPluginFactory : public NullVmPluginFactory {
public:
  WasmSpeedPluginFactory() {}

  std::string name() const override { return "WasmSpeedCpp"; }

  std::unique_ptr<NullVmPlugin> create() const override {
    return std::make_unique<NullPlugin>(
        &Envoy::Extensions::Common::Wasm::Null::Plugin::WasmSpeedCpp::null_plugin_registry_.get());
  }
};

/**
 * Static registration for the null Wasm filter. @see RegisterFactory.
 */
static Registry::RegisterFactory<WasmSpeedPluginFactory, NullVmPluginFactory> register_;
} // namespace Plugin
} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
