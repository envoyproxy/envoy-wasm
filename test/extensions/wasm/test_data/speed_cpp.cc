// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick(uint32_t /* context_zero */) {
  uint64_t t;
  if (WasmResult::Ok != proxy_getCurrentTimeNanoseconds(&t)) {
    logError("bad result from getCurrentTimeNanoseconds");
  }
}
