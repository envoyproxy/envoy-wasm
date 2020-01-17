#pragma once

#include "envoy/config/typed_config.h"

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

/**
 * Foreign functtions for Wasm extensions should implement this factory and register via
 * Registry::registerFactory or the convenience class RegisterFactory.
 */
class ForeignFunctionFactory : public Config::UntypedFactory {
public:
  virtual ~ForeignFunctionFactory() = default;

  /**
   * Category of the factory.
   */
  std::string category() const override { return "envoy.wasm.foreign_functions"; }

  /**
   * Create an instance of the Foreign Function.
   */
  virtual WasmForeignFunction create() const PURE;
};

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
