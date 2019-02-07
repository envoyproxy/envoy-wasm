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

FilterConfig::FilterConfig(const envoy::config::filter::http::wasm::v2::Wasm& proto_config,
                           Server::Configuration::FactoryContext& context)
    : tls_slot_(context.threadLocal().allocateSlot()) {
  if (proto_config.has_vm_config()) {
    auto base_wasm = std::shared_ptr<Common::Wasm::Wasm>(
        Common::Wasm::createWasm(proto_config.id(), proto_config.vm_config(), context.api())
            .release());
    tls_slot_->set([&proto_config, base_wasm, &context](Event::Dispatcher& dispatcher) {
      auto result = Extensions::Common::Wasm::createThreadLocalWasm(
          *base_wasm, proto_config.vm_config(), dispatcher, proto_config.configuration(),
          context.api());
      result->setClusterManager(context.clusterManager());
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
    });
  } else {
    tls_slot_->set([&proto_config, &context](Event::Dispatcher& dispatcher) {
      auto result = Extensions::Common::Wasm::getThreadLocalWasm(proto_config.id(),
                                                                 proto_config.configuration());
      result->setDispatcher(dispatcher);
      result->setClusterManager(context.clusterManager());
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
    });
  }
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
