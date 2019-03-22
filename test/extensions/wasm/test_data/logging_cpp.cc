// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(char* configuration, int size) {
  logWarn(std::string("warn " + std::string(configuration, size)));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart() {
  logTrace(std::string("test trace") + " logging");
  logDebug(std::string("test debug") + " logging");
  logError(std::string("test error") + " logging");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick() {
  logInfo(std::string("test tick") + " logging");
}
