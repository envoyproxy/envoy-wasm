#include <stdio.h>

#include <memory>
#include <string>
#include <vector>

#include "envoy/common/exception.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/server/wasm.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/assert.h"
#include "common/common/c_smart_ptr.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/wavm/wavm.h"
#include "extensions/common/wasm/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

Server::WasmPtr createWasmVm(const std::string& wasm_vm) {
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
