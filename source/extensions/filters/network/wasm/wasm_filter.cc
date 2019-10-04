#include "extensions/filters/network/wasm/wasm_filter.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/enum_to_int.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Wasm {

FilterConfig::FilterConfig(const envoy::config::filter::network::wasm::v2::Wasm& config,
                           Server::Configuration::FactoryContext& context)
    : tls_slot_(context.threadLocal().allocateSlot()) {
  // Create a base WASM to verify that the code loads before setting/cloning the for the
  // individual threads.
  auto callback = [this, &config, &context](const std::string& code) {
    plugin_ = std::make_shared<Common::Wasm::Plugin>(
        config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
        code, context.direction(), context.localInfo(), &context.listenerMetadata(),
        context.scope(), nullptr /* owned_scope */);
    auto base_wasm = Common::Wasm::createWasm(config.config().vm_config(), plugin_,
                                              context.clusterManager(), context.dispatcher());
    auto configuration = std::make_shared<std::string>(config.config().configuration());
    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    tls_slot_->set([base_wasm, configuration](Event::Dispatcher& dispatcher) {
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(
          Common::Wasm::getOrCreateThreadLocalWasm(*base_wasm, *configuration, dispatcher));
    });
  };

  if (config.config().vm_config().code().has_local()) {
    local_data_provider_ = std::make_unique<Config::DataSource::LocalAsyncDataProvider>(
        context.initManager(), config.config().vm_config().code().local(), true, context.api(),
        std::move(callback));
  } else {
    remote_data_provider_ = std::make_unique<Config::DataSource::RemoteAsyncDataProvider>(
        context.clusterManager(), context.initManager(),
        config.config().vm_config().code().remote(), true, std::move(callback));
  }
}

} // namespace Wasm
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
