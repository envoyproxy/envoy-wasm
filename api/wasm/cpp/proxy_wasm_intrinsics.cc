// NOLINT(namespace-envoy)
#include "proxy_wasm_intrinsics.h"

std::unique_ptr<Context> NewContext(uint32_t id);

static std::unordered_map<int32_t, std::unique_ptr<Context>> context_map;

static Context* ensureContext(uint32_t context_id) {
  auto e = context_map.insert(std::make_pair(context_id, nullptr));
  if (e.second)
    e.first->second = NewContext(context_id);
  return e.first->second.get();
}

static Context* getContext(uint32_t context_id) {
  auto it = context_map.find(context_id);
  if (it == context_map.end())
    return nullptr;
  return it->second.get();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onConfigure(uint32_t ptr, uint32_t size) {
  ensureContext(0)->onConfigure(std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart() { ensureContext(0)->onStart(); }

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onCreate(uint32_t context_id) {
  ensureContext(context_id)->onCreate();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick() { ensureContext(0)->onTick(); }

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus proxy_onRequestHeaders(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return FilterHeadersStatus::Continue;
  return c->onRequestHeaders();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterMetadataStatus proxy_onRequestMetadata(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return FilterMetadataStatus::Continue;
  return c->onRequestMetadata();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterDataStatus proxy_onRequestBody(uint32_t context_id,
                                                                     uint32_t body_buffer_length,
                                                                     uint32_t end_of_stream) {
  auto c = getContext(context_id);
  if (!c)
    return FilterDataStatus::Continue;
  return c->onRequestBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterTrailersStatus proxy_onRequestTrailers(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return FilterTrailersStatus::Continue;
  return c->onRequestTrailers();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus proxy_onResponseHeaders(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return FilterHeadersStatus::Continue;
  return c->onResponseHeaders();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterMetadataStatus proxy_onResponseMetadata(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return FilterMetadataStatus::Continue;
  return c->onResponseMetadata();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterDataStatus proxy_onResponseBody(uint32_t context_id,
                                                                      uint32_t body_buffer_length,
                                                                      uint32_t end_of_stream) {
  auto c = getContext(context_id);
  if (!c)
    return FilterDataStatus::Continue;
  return c->onResponseBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterTrailersStatus proxy_onResponseTrailers(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return FilterTrailersStatus::Continue;
  return c->onResponseTrailers();
}

extern "C" EMSCRIPTEN_KEEPALIVE void
proxy_onHttpCallResponse(uint32_t context_id, uint32_t token, uint32_t header_pairs_ptr,
                         uint32_t header_pairs_size, uint32_t body_ptr, uint32_t body_size,
                         uint32_t trailer_pairs_ptr, uint32_t trailer_pairs_size) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onHttpCallResponse(
      token,
      std::make_unique<WasmData>(reinterpret_cast<char*>(header_pairs_ptr), header_pairs_size),
      std::make_unique<WasmData>(reinterpret_cast<char*>(body_ptr), body_size),
      std::make_unique<WasmData>(reinterpret_cast<char*>(trailer_pairs_ptr), trailer_pairs_size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDone(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onDone();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onLog(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onLog();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDelete(uint32_t context_id) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onDelete();
  context_map.erase(context_id);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcCreateInitialMetadata(uint32_t context_id, uint32_t token) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onGrpcCreateInitialMetadata(token);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceiveInitialMetadata(uint32_t context_id, uint32_t token) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onGrpcReceiveInitialMetadata(token);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceiveTrailingMetadata(uint32_t context_id, uint32_t token) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onGrpcReceiveTrailingMetadata(token);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceive(uint32_t context_id, uint32_t token,
    uint32_t response_ptr, uint32_t response_size) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onGrpcReceive(token, std::make_unique<WasmData>(reinterpret_cast<char*>(response_ptr), response_size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcClose(uint32_t context_id, uint32_t token,
    uint32_t status_code, uint32_t status_message_ptr, uint32_t status_message_size) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onGrpcClose(token, static_cast<GrpcStatus>(status_code), std::make_unique<WasmData>(reinterpret_cast<char*>(status_message_ptr), status_message_size));
}
