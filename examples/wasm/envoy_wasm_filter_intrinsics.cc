#include "envoy_wasm_filter_intrinsics.h"


static std::unordered_map<int32_t, std::unique_ptr<Context>> context_map;

static Context* getContext(uint32_t context_id) {
  auto e = context_map.insert(std::make_pair(context_id, nullptr));
  if (e.second) e.first->second = Context::New(context_id);
  return e.first->second.get();
}

extern "C" EMSCRIPTEN_KEEPALIVE int onStart(uint32_t context_id) {
  getContext(context_id)->onStart();
  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE void onConfigure(uint32_t context_id, uint32_t ptr, uint32_t size) {
  getContext(context_id)->onConfigure(std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size));
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus onRequestHeaders(uint32_t context_id) {
  return getContext(context_id)->onRequestHeaders();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterDataStatus onRequestBody(uint32_t context_id, uint32_t body_buffer_length, uint32_t end_of_stream) {
  return getContext(context_id)->onRequestBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterTrailersStatus onRequestTrailers(uint32_t context_id) {
  return getContext(context_id)->onRequestTrailers();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus onResponseHeaders(uint32_t context_id) {
  return getContext(context_id)->onResponseHeaders();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterDataStatus onResponseBody(uint32_t context_id, uint32_t body_buffer_length, uint32_t end_of_stream) {
  return getContext(context_id)->onResponseBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterTrailersStatus onResponseTrailers(uint32_t context_id) {
  return getContext(context_id)->onResponseTrailers();
}

extern "C" EMSCRIPTEN_KEEPALIVE void onLogs(uint32_t context_id) {
  getContext(context_id)->onLogs();
}

extern "C" EMSCRIPTEN_KEEPALIVE void onDestroy(uint32_t context_id) {
  getContext(context_id)->onDestroy();
  context_map.erase(context_id);
}

