#include "extensions/access_loggers/wasm/config.h"

#include <unordered_map>

#include "envoy/config/accesslog/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "common/access_log/access_log_formatter.h"
#include "common/common/logger.h"
#include "common/protobuf/protobuf.h"

#include "extensions/access_loggers/wasm/wasm_access_log_impl.h"
#include "extensions/access_loggers/well_known_names.h"
#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace Wasm {

AccessLog::InstanceSharedPtr
WasmAccessLogFactory::createAccessLogInstance(const Protobuf::Message& proto_config,
                                              AccessLog::FilterPtr&& filter,
                                              Server::Configuration::FactoryContext& context) {
  const auto& config =
      MessageUtil::downcastAndValidate<const envoy::config::accesslog::v2::WasmAccessLog&>(
          proto_config);
  auto id = config.id();
  auto configuration = std::make_shared<std::string>(config.configuration());
  auto tls_slot = context.threadLocal().allocateSlot();
  if (config.has_vm_config()) {
    // Create a base WASM to verify that the code loads before setting/cloning the for the
    // individual threads.
    auto base_wasm = Common::Wasm::createWasm(id, config.vm_config(), context.clusterManager(),
                                              context.dispatcher(), context.api(), context.scope(),
                                              context.localInfo(), &context.listenerMetadata(),
                                              nullptr /* owned_scope */);
    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    tls_slot->set([base_wasm, configuration](Event::Dispatcher& dispatcher) {
      auto result = Common::Wasm::createThreadLocalWasm(*base_wasm, *configuration, dispatcher);
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
    });
  } else {
    if (id.empty()) {
      throw Common::Wasm::WasmVmException("No WASM VM Id or vm_config specified");
    }
    tls_slot->set([id, configuration](Event::Dispatcher&) {
      auto result = Common::Wasm::getThreadLocalWasm(id, *configuration);
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
    });
  }
  return std::make_shared<WasmAccessLog>(std::move(tls_slot), std::move(filter));
}

ProtobufTypes::MessagePtr WasmAccessLogFactory::createEmptyConfigProto() {
  return ProtobufTypes::MessagePtr{new envoy::config::accesslog::v2::WasmAccessLog()};
}

std::string WasmAccessLogFactory::name() const { return AccessLogNames::get().Wasm; }

std::unordered_map<std::string, std::string>
WasmAccessLogFactory::convertJsonFormatToMap(ProtobufWkt::Struct json_format) {
  std::unordered_map<std::string, std::string> output;
  for (const auto& pair : json_format.fields()) {
    if (pair.second.kind_case() != ProtobufWkt::Value::kStringValue) {
      throw EnvoyException("Only string values are supported in the JSON access log format.");
    }
    output.emplace(pair.first, pair.second.string_value());
  }
  return output;
}

/**
 * Static registration for the wasm access log. @see RegisterFactory.
 */
static Registry::RegisterFactory<WasmAccessLogFactory,
                                 Server::Configuration::AccessLogInstanceFactory>
    register_;

} // namespace Wasm
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
