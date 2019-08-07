// NOLINT(namespace-envoy)
#include "proxy_wasm_intrinsics.h"

extern "C" void missing();

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t, uint32_t, uint32_t, uint32_t,
                                                   uint32_t) {
  missing();
}
