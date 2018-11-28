#include <string>
#include <unordered_map>

#include "envoy_wasm_filter_intrinsics.h"


class Context {
public:
  Context(int32_t id) : id_(id) {}
  int32_t id() { return id_; }

  void onCreate();
  void onStart();
  void onDestroy();

private:
  int32_t id_;
};

static std::unordered_map<int32_t, std::unique_ptr<Context>> context_map;

extern "C" EMSCRIPTEN_KEEPALIVE void onCreate(int32_t context_id) {
  context_map[context_id].reset(new Context(context_id));
  context_map[context_id]->onCreate();
}

extern "C" EMSCRIPTEN_KEEPALIVE void onStart(int32_t context_id) {
  context_map[context_id]->onStart();
}

extern "C" EMSCRIPTEN_KEEPALIVE void onDestroy(int32_t context_id) {
  context_map[context_id]->onDestroy();
  context_map.erase(context_id);
}

void Context::onCreate() {
  logDebug(std::string("onCreate ") + std::to_string(id_));
}

void Context::onStart() {
  logInfo(std::string("onStart ") + std::to_string(id_));
}

void Context::onDestroy() {
  logInfo(std::string("onDestroy " + std::to_string(id_)));
}

#if 0
  auto result = getHeader(HeaderType::Header, "path");
  logInfo(std::string("header path ") + std::string(result->view()));
#endif

extern "C" EMSCRIPTEN_KEEPALIVE int main() {
  logTrace("main");
  return 0;
}
