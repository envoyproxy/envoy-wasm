// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_on_configure(uint32_t, uint32_t) {
  const char* configuration = nullptr;
  size_t size;
  proxy_get_configuration(&configuration, &size);
  logWarn(std::string("warn " + std::string(configuration, size)));
  ::free((void*)configuration);
  return 1;
}

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_on_start(uint32_t, uint32_t) {
  logTrace("test trace logging");
  logDebug("test debug logging");
  logError("test error logging");
  return 1;
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_on_tick(uint32_t) {
  logInfo("test tick logging");
  proxy_done();
}

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_on_done(uint32_t) {
  logInfo("onDone logging");
  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_on_delete(uint32_t) { logInfo("onDelete logging"); }
