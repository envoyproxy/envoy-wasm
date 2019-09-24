// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_onConfigure(uint32_t, char* configuration,
                                                           int size) {
  logWarn(std::string("warn " + std::string(configuration, size)));
  return 1;
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t, uint32_t, uint32_t, uint32_t,
                                                   uint32_t) {
  logTrace(std::string("test trace") + " logging");
  logDebug(std::string("test debug") + " logging");
  logError(std::string("test error") + " logging");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick(uint32_t) {
  logInfo(std::string("test tick") + " logging");
}
