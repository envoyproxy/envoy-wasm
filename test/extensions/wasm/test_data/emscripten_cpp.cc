// NOLINT(namespace-envoy)
#include <cmath>
#include <limits>
#include <string>

#include "proxy_wasm_intrinsics.h"

float gNan = std::nan("1");
float gInfinity = INFINITY;

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_on_start(uint32_t, uint32_t) {
  logInfo(std::string("NaN ") + std::to_string(gNan));
  logWarn("inf " + std::to_string(gInfinity));
  logWarn("inf " + std::to_string(1.0 / 0.0));
  logWarn(std::string("inf ") + (std::isinf(gInfinity) ? "inf" : "nan"));
  return 1;
}
