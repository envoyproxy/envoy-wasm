#include <memory>
#include <stdio.h>
#include <string>
#include <vector>

#include "envoy/common/exception.h"
#include "envoy/server/wasm.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/assert.h"
#include "common/common/c_smart_ptr.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/common/wasm/wavm/wavm.h"
#include "extensions/common/wasm/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

std::unique_ptr<WasmVm> createWasmVm(absl::string_view wasm_vm) {
  if (wasm_vm == WasmVmNames::get().Wavm) {
    return Wavm::createWavm();
  } else {
    return nullptr;
  }
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
