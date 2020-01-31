#include "extensions/access_loggers/wasm/config.h"

#include <unordered_map>

#include "envoy/extensions/access_loggers/wasm/v3/wasm.pb.validate.h"
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
  const auto& config = MessageUtil::downcastAndValidate<
      const envoy::extensions::access_loggers::wasm::v3::WasmAccessLog&>(
      proto_config, context.messageValidationVisitor());
  auto vm_id = config.config().vm_config().vm_id();
  auto root_id = config.config().root_id();
  auto configuration = std::make_shared<std::string>(config.config().configuration());
  auto access_log = std::make_shared<WasmAccessLog>(root_id, nullptr, std::move(filter));

  // Create a base WASM to verify that the code loads before setting/cloning the for the
  // individual threads.
  auto plugin = std::make_shared<Common::Wasm::Plugin>(
      config.config().name(), config.config().root_id(), config.config().vm_config().vm_id(),
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, context.localInfo(),
      nullptr /* listener_metadata */);

  auto callback = [access_log, &context, plugin,
                   configuration](Common::Wasm::WasmHandleSharedPtr base_wasm) {
    auto tls_slot = context.threadLocal().allocateSlot();

    // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
    tls_slot->set([base_wasm, plugin, configuration](Event::Dispatcher& dispatcher) {
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(
          Common::Wasm::getOrCreateThreadLocalWasm(base_wasm, plugin, *configuration, dispatcher));
    });
    access_log->setTlsSlot(std::move(tls_slot));
  };

  Common::Wasm::createWasm(config.config().vm_config(), plugin, context.scope().createScope(""),
                           context.clusterManager(), context.initManager(), context.dispatcher(),
                           context.api(), remote_data_provider_, std::move(callback));

  return access_log;
}

ProtobufTypes::MessagePtr WasmAccessLogFactory::createEmptyConfigProto() {
  return ProtobufTypes::MessagePtr{
      new envoy::extensions::access_loggers::wasm::v3::WasmAccessLog()};
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
