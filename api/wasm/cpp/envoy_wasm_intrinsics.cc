#include "envoy_wasm_intrinsics.h"

static std::unordered_map<int32_t, std::unique_ptr<Context>> context_map;

static Context* getContext(uint32_t context_id) {
  auto e = context_map.insert(std::make_pair(context_id, nullptr));
  if (e.second)
    e.first->second = Context::New(context_id);
  return e.first->second.get();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(uint32_t ptr, uint32_t size) {
  getContext(0)->onConfigure(std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart() { getContext(0)->onStart(); }

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus proxy_onRequestHeaders(uint32_t context_id) {
  return getContext(context_id)->onRequestHeaders();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterDataStatus proxy_onRequestBody(uint32_t context_id,
                                                                     uint32_t body_buffer_length,
                                                                     uint32_t end_of_stream) {
  return getContext(context_id)
      ->onRequestBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterTrailersStatus proxy_onRequestTrailers(uint32_t context_id) {
  return getContext(context_id)->onRequestTrailers();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus proxy_onResponseHeaders(uint32_t context_id) {
  return getContext(context_id)->onResponseHeaders();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterDataStatus proxy_onResponseBody(uint32_t context_id,
                                                                      uint32_t body_buffer_length,
                                                                      uint32_t end_of_stream) {
  return getContext(context_id)
      ->onResponseBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterTrailersStatus proxy_onResponseTrailers(uint32_t context_id) {
  return getContext(context_id)->onResponseTrailers();
}

extern "C" EMSCRIPTEN_KEEPALIVE void
proxy_onHttpCallResponse(uint32_t context_id, uint32_t token, uint32_t header_pairs_ptr,
                         uint32_t header_pairs_size, uint32_t body_ptr, uint32_t body_size,
                         uint32_t trailer_pairs_ptr, uint32_t trailer_pairs_size) {
  return getContext(context_id)
      ->onHttpCallResponse(
          token,
          std::make_unique<WasmData>(reinterpret_cast<char*>(header_pairs_ptr), header_pairs_size),
          std::make_unique<WasmData>(reinterpret_cast<char*>(body_ptr), body_size),
          std::make_unique<WasmData>(reinterpret_cast<char*>(trailer_pairs_ptr),
                                     trailer_pairs_size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDone(uint32_t context_id) {
  getContext(context_id)->onDone();
  context_map.erase(context_id);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onLog(uint32_t context_id) {
  getContext(context_id)->onLog();
}
