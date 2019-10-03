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
          proto_config, context.messageValidationVisitor());
  auto vm_id = config.vm_id();
  auto root_id = config.root_id();
  auto configuration = std::make_shared<std::string>(config.configuration());
  if (config.has_vm_config()) {
    auto access_log = std::make_shared<WasmAccessLog>(root_id, nullptr, std::move(filter));
    auto callback = [&context, config, access_log, root_id, vm_id,
                     configuration](const std::string& code) {
      // Create a base WASM to verify that the code loads before setting/cloning the for the
      // individual threads.
      auto tls_slot = context.threadLocal().allocateSlot();
      auto base_wasm = Common::Wasm::createWasm(
          vm_id, config.vm_config(), root_id, code, context.clusterManager(), context.dispatcher(),
          context.scope(), context.direction(), context.localInfo(), &context.listenerMetadata(),
          nullptr /* owned_scope */);
      // NB: the Slot set() call doesn't complete inline, so all arguments must outlive this call.
      tls_slot->set([base_wasm, root_id, configuration](Event::Dispatcher& dispatcher) {
        auto result =
            Common::Wasm::createThreadLocalWasm(*base_wasm, root_id, *configuration, dispatcher);
        return ThreadLocal::ThreadLocalObjectSharedPtr{result};
      });
      access_log->setTlsSlot(std::move(tls_slot));
    };

    if (config.vm_config().code().has_local()) {
      local_data_provider_ = std::make_unique<Config::DataSource::LocalAsyncDataProvider>(
          context.initManager(), config.vm_config().code().local(), true, context.api(),
          std::move(callback));
    } else {
      remote_data_provider_ = std::make_unique<Config::DataSource::RemoteAsyncDataProvider>(
          context.clusterManager(), context.initManager(), config.vm_config().code().remote(), true,
          std::move(callback));
    }
    return access_log;
  } else {
    if (vm_id.empty()) {
      throw Common::Wasm::WasmVmException("No WASM VM Id or vm_config specified");
    }
    auto tls_slot = context.threadLocal().allocateSlot();
    tls_slot->set([vm_id, root_id, configuration](Event::Dispatcher&) {
      auto result = Common::Wasm::getThreadLocalWasm(vm_id, root_id, *configuration);
      return std::static_pointer_cast<ThreadLocal::ThreadLocalObject>(result);
    });
    return std::make_shared<WasmAccessLog>(root_id, std::move(tls_slot), std::move(filter));
  }
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
