#include "extensions/access_loggers/wasm/config.h"

#include <unordered_map>

#include "envoy/config/accesslog/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "common/access_log/access_log_formatter.h"
#include "common/common/logger.h"
#include "common/protobuf/protobuf.h"

#include "extensions/access_loggers/well_known_names.h"
#include "extensions/access_loggers/wasm/wasm_access_log_impl.h"
#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace Wasm {

AccessLog::InstanceSharedPtr
WasmAccessLogFactory::createAccessLogInstance(const Protobuf::Message& config,
                                              AccessLog::FilterPtr&& filter,
                                              Server::Configuration::FactoryContext& context) {
  const auto& wal_config =
      MessageUtil::downcastAndValidate<const envoy::config::accesslog::v2::WasmAccessLog&>(config);
  auto tls_slot = context.threadLocal().allocateSlot();
  if (wal_config.has_vm_config()) {
    auto base_wasm = Common::Wasm::createWasm(wal_config.id(), wal_config.vm_config(), context.api());
    tls_slot->set([&wal_config, &base_wasm, &context](Event::Dispatcher& dispatcher) {
        auto result = Common::Wasm::createThreadLocalWasm(*base_wasm, wal_config.vm_config(), dispatcher, wal_config.configuration(), context.api());
        result->setClusterManager(context. clusterManager());
        return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
        });
  } else {
    if (wal_config.id().empty()) {
      throw Common::Wasm::WasmVmException("No WASM VM Id or vm_config specified");
    }
    tls_slot->set([&wal_config, &context](Event::Dispatcher& dispatcher) {
        auto result = Common::Wasm::getThreadLocalWasm(wal_config.id(), wal_config.configuration());
        result->setDispatcher(dispatcher);
        result->setClusterManager(context.clusterManager());
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
