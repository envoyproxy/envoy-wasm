// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

// Required Proxy-Wasm ABI version.
extern "C" PROXY_WASM_KEEPALIVE void proxy_abi_version_0_1_0() {}

static int* badptr = nullptr;

extern "C" PROXY_WASM_KEEPALIVE uint32_t proxy_on_vm_start(uint32_t, uint32_t) {
  logError("before badptr");
  *badptr = 1;
  logError("after badptr");
  return 1;
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_log(uint32_t context_id) {
  logError("before div by zero");
  logError("divide by zero: " + std::to_string(100 / context_id));
  logError("after div by zero");
}
