// NOLINT(namespace-envoy)
#include <math.h>

#include <string>

#include "proxy_wasm_intrinsics.h"

//  Use global variables so the compiler cannot optimize the operations away.
int32_t i32a = 0;
int32_t i32b = 1;
double f64a = 0.0;
double f64b = 1.0;

// Emscripten in some modes and versions would use functions from the asm2wasm module to implement
// these operations: int32_t % /, double conversion to int32_t and remainder().
extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t, uint32_t, uint32_t, uint32_t,
                                                   uint32_t) {
  logInfo(std::string("out ") + std::to_string(i32a / i32b) + " " + std::to_string(i32a % i32b) +
          " " + std::to_string((int32_t)remainder(f64a, f64b)));
}
