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
    : root_id_(config.root_id()), tls_slot_(context.threadLocal().allocateSlot()) {
  auto vm_id = config.vm_id();
  auto root_id = config.root_id();
  auto configuration = std::make_shared<std::string>(config.configuration());
  if (config.has_vm_config()) {
    // Create a base WASM to verify that the code loads before setting/cloning the for the
    // individual threads.
    auto base_wasm = Common::Wasm::createWasm(
        vm_id, config.vm_config(), root_id, context.clusterManager(), context.dispatcher(),
        context.api(), context.scope(),
        Common::Wasm::pluginDirectionFromTrafficDirection(context.direction()), context.localInfo(),
        &context.listenerMetadata(), nullptr /* owned_scope */);
    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    tls_slot_->set([base_wasm, root_id, configuration](Event::Dispatcher& dispatcher) {
      auto result = Extensions::Common::Wasm::createThreadLocalWasm(*base_wasm, root_id,
                                                                    *configuration, dispatcher);
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
    });
  } else {
    if (vm_id.empty()) {
      throw Common::Wasm::WasmVmException("No WASM VM Id or vm_config specified");
    }
    tls_slot_->set([vm_id, root_id, configuration](Event::Dispatcher&) {
      auto result = Extensions::Common::Wasm::getThreadLocalWasm(vm_id, root_id, *configuration);
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
    });
  }
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
