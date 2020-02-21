#include "extensions/common/wasm/null/null_plugin.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {
namespace Plugin {
namespace ExamplePlugin {
NullPluginRegistry* context_registry_{};
} // namespace ExamplePlugin

/**
 * Config registration for a Wasm filter plugin. @see NamedHttpFilterConfigFactory.
 */
class SamplePluginFactory : public NullVmPluginFactory {
public:
  SamplePluginFactory() {}

  std::string name() const override { return "null_vm_plugin"; }

  std::unique_ptr<NullVmPlugin> create() const override {
    return std::make_unique<NullPlugin>(
        Envoy::Extensions::Common::Wasm::Null::Plugin::ExamplePlugin::context_registry_);
  }
};

/**
 * Static registration for the null Wasm filter. @see RegisterFactory.
 */
REGISTER_FACTORY(SamplePluginFactory, NullVmPluginFactory);

} // namespace Plugin
} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
