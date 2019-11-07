#pragma once

#include <memory>

#include "extensions/common/wasm/wasm_vm.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Wavm {

std::unique_ptr<WasmVm> createVm(Stats::ScopeSharedPtr scope);

} // namespace Wavm
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
