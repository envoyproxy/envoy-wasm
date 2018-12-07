#include <string>
#include <unordered_map>

#include "envoy_wasm_filter_intrinsics.h"


class Context {
public:
  Context(int32_t id) : id_(id) {}
  int32_t id() { return id_; }

  FilterHeadersStatus onStart();
  FilterDataStatus onBody(size_t body_buffer_length, bool end_of_stream);
  FilterTrailersStatus onTrailers();
  void onDestroy();

private:
  int32_t id_;
};

static std::unordered_map<int32_t, std::unique_ptr<Context>> context_map;

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus onStart(int32_t context_id) {
  context_map[context_id].reset(new Context(context_id));
  return context_map[context_id]->onStart();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterTrailersStatus onTrailers(int32_t context_id) {
  return context_map[context_id]->onTrailers();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterDataStatus onBody(int32_t context_id, uint32_t body_buffer_length, uint32_t end_of_stream) {
  return context_map[context_id]->onBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void onDestroy(int32_t context_id) {
  context_map[context_id]->onDestroy();
  context_map.erase(context_id);
}

FilterHeadersStatus Context::onStart() {
  logDebug(std::string("onStart ") + std::to_string(id_));
  auto result = getHeaderPairs(HeaderType::Header);
  auto pairs = result->pairs();
  logInfo(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto& p : pairs) {
    logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
  addHeader(HeaderType::Header, "newheader", "newheadervalue");
  replaceHeader(HeaderType::Header, "server", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus Context::onBody(size_t body_buffer_length, bool end_of_stream) {
  auto body = getBodyBufferBytes(0, body_buffer_length);
  logError(std::string("onBody ") + std::string(body->view()));
  return FilterDataStatus::Continue;
}

FilterTrailersStatus Context::onTrailers() {
  return FilterTrailersStatus::Continue;
}

void Context::onDestroy() {
  logWarn(std::string("onDestroy " + std::to_string(id_)));
}

extern "C" EMSCRIPTEN_KEEPALIVE int main() {
  logTrace("main");
  return 0;
}
