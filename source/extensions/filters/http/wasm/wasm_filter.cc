#include "extensions/filters/http/wasm/wasm_filter.h"

#include "envoy/http/codes.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/enum_to_int.h"
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/http/utility.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

FilterConfig::FilterConfig(const envoy::extensions::filters::http::wasm::v3::Wasm& config,
                           Server::Configuration::FactoryContext& context)
    : tls_slot_(context.threadLocal().allocateSlot()) {
  plugin_ = std::make_shared<Common::Wasm::Plugin>(
      config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
      context.direction(), context.localInfo(), &context.listenerMetadata());

  auto plugin = plugin_;
  auto callback = [&config, plugin, this](Common::Wasm::WasmHandleSharedPtr base_wasm) {
    auto configuration = std::make_shared<std::string>(config.config().configuration());
    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    tls_slot_->set([base_wasm, plugin, configuration](Event::Dispatcher& dispatcher) {
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(
          Common::Wasm::getOrCreateThreadLocalWasm(*base_wasm, plugin, *configuration, dispatcher));
    });
  };

  Common::Wasm::createWasm(config.config().vm_config(), plugin_, context.scope().createScope(""),
                           context.clusterManager(), context.initManager(), context.dispatcher(),
                           context.api(), remote_data_provider_, std::move(callback));
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
