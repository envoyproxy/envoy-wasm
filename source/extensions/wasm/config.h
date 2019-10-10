#pragma once

#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/server/wasm_config.h"

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

class WasmFactory : public Server::Configuration::WasmFactory {
public:
  ~WasmFactory() override {}
  std::string name() override { return "envoy.wasm"; }
  Server::WasmSharedPtr createWasm(const envoy::config::wasm::v2::WasmService& config,
                                   Server::Configuration::WasmFactoryContext& context) override;
};

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
