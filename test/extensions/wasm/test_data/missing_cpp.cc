// NOLINT(namespace-envoy)
#include "proxy_wasm_intrinsics.h"

extern "C" void missing();

extern "C" PROXY_WASM_KEEPALIVE uint32_t proxy_on_vm_start(uint32_t, uint32_t) {
  missing();
  return 1;
}
