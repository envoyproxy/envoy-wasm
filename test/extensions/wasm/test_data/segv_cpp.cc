// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

static int* badptr = nullptr;

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t, uint32_t, uint32_t, uint32_t,
                                                   uint32_t) {
  logError("before badptr");
  *badptr = 1;
  logError("after badptr");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onLog(uint32_t context_id) {
  logError("before div by zero");
  logError("divide by zero: " + std::to_string(100 / context_id));
  logError("after div by zero");
}
