// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(char* configuration, int size) {
  logWarn(std::string("warn " + std::string(configuration, size)));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick() {
  logErr(std::string("test tick") + " logging onTick");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart() {
  logDebug(std::string("test debug") + " logging onStart");
  logInfo(std::string("test info") + " logging onStart");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onLog() {
  logDebug(std::string("test debug") + " logging onLog");
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDone() {
  logDebug(std::string("test debug") + " logging onDone");
}
