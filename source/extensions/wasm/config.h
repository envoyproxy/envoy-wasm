#pragma once

#include "envoy/config/wasm/v3/wasm.pb.validate.h"
#include "envoy/server/wasm_config.h"

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

class WasmFactory : public Server::Configuration::WasmFactory {
public:
  ~WasmFactory() override {}
  std::string name() override { return "envoy.wasm"; }
  void createWasm(const envoy::config::wasm::v3::WasmService& config,
                  Server::Configuration::WasmFactoryContext& context,
                  Server::CreateWasmServiceCallback&& cb) override;

private:
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider_;
};

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
