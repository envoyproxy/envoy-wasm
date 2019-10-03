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

FilterConfig::FilterConfig(const envoy::config::filter::http::wasm::v2::Wasm& config,
                           Server::Configuration::FactoryContext& context)
    : tls_slot_(context.threadLocal().allocateSlot()) {
  // Create a base Wasm to verify that the code loads before setting/cloning the for the
  // individual threads.
  plugin_ = std::make_shared<Common::Wasm::Plugin>(
      config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
      context.direction(), context.localInfo(), &context.listenerMetadata(), context.scope(),
      nullptr /* owned_scope */);
  auto base_wasm =
      Common::Wasm::createWasm(config.config().vm_config(), plugin_, context.clusterManager(),
                               context.dispatcher(), context.api());
  auto configuration = std::make_shared<std::string>(config.config().configuration());
  // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
  tls_slot_->set([base_wasm, configuration](Event::Dispatcher& dispatcher) {
    return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(
        Common::Wasm::getOrCreateThreadLocalWasm(*base_wasm, *configuration, dispatcher));
  });
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
