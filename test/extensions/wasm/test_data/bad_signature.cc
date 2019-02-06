#include <string>

#include "envoy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(int bad, char* configuration, int size) {
  logWarn(std::string("warn " + std::string(configuration, size)));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart() {
  logDebug(std::string("test debug") + " logging");
  logInfo(std::string("test info") + " logging");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick() {
  logError(std::string("test tick") + " logging");
}
