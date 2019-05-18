#include "extensions/common/wasm/null/null.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {
namespace Plugin {
namespace ExamplePlugin {
std::unique_ptr<Plugin::Context> NewContext(uint32_t id);
} // namespace ExamplePlugin

/**
 * Config registration for a Wasm filter plugin. @see NamedHttpFilterConfigFactory.
 */
class PluginFactory : public NullVmPluginFactory {
public:
  PluginFactory() {}

  const std::string name() const override { return "null_vm_plugin"; }
  std::unique_ptr<NullVmPlugin> create() const override {
    return std::make_unique<NullVmPlugin>(
        &Envoy::Extensions::Common::Wasm::Null::Plugin::ExamplePlugin::NewContext);
  }
};

/**
 * Static registration for the null Wasm filter. @see RegisterFactory.
 */
static Registry::RegisterFactory<PluginFactory, NullVmPluginFactory> register_;

} // namespace Plugin
} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
