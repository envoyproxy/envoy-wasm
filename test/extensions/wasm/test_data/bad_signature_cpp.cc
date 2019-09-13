// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_onConfigure(uint32_t, int bad, char* configuration,
                                                           int size) {
  logWarn(std::string("warn " + std::string(configuration, size)));
  return 1;
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t, uint32_t, uint32_t, uint32_t,
                                                   uint32_t) {
  logDebug(std::string("test debug") + " logging");
  logInfo(std::string("test info") + " logging");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick(uint32_t) {
  logError(std::string("test tick") + " logging");
}
