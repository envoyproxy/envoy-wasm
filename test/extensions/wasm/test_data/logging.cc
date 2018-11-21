#include <string>

#include "envoy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE int configure(char* configuration, int size) {
  logWarn(std::string("warn " + std::string(configuration, size)));
  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE int tick() {
  logErr(std::string("test tick") + " logging");
  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE int main() {
  logDebug(std::string("test debug") + " logging");
  logInfo(std::string("test info") + " logging");
  return 0;
}
