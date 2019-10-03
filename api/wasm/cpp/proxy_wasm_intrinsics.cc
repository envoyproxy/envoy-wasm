// NOLINT(namespace-envoy)
#include "proxy_wasm_intrinsics.h"

static std::unordered_map<std::string, RootFactory>* root_factories = nullptr;
static std::unordered_map<std::string, ContextFactory>* context_factories = nullptr;
static std::unordered_map<int32_t, std::unique_ptr<ContextBase>> context_map;
static std::unordered_map<std::string, RootContext*> root_context_map;

RegisterContextFactory::RegisterContextFactory(ContextFactory context_factory,
                                               RootFactory root_factory, StringView root_id) {
  if (!root_factories) {
    root_factories = new std::unordered_map<std::string, RootFactory>;
    context_factories = new std::unordered_map<std::string, ContextFactory>;
  }
  if (context_factory)
    (*context_factories)[std::string(root_id)] = context_factory;
  if (root_factory)
    (*root_factories)[std::string(root_id)] = root_factory;
}

static Context* ensureContext(uint32_t context_id, uint32_t root_context_id) {
  auto e = context_map.insert(std::make_pair(context_id, nullptr));
  if (e.second) {
    RootContext* root = context_map[root_context_id].get()->asRoot();
    std::string root_id = std::string(root->root_id());
    if (!context_factories) {
      e.first->second = std::make_unique<Context>(context_id, root);
      return e.first->second->asContext();
    }
    auto factory = context_factories->find(root_id);
    if (factory == context_factories->end()) {
      logError("no context factory for root_id: " + root_id);
      return nullptr;
    } else {
      e.first->second = factory->second(context_id, root);
    }
  }
  return e.first->second->asContext();
}

static RootContext* ensureRootContext(uint32_t context_id, std::unique_ptr<WasmData> root_id) {
  auto it = context_map.find(context_id);
  if (it != context_map.end()) {
    return it->second->asRoot();
  }
  if (!root_factories) {
    auto context = std::make_unique<RootContext>(context_id, root_id->view());
    RootContext* root_context = context->asRoot();
    context_map[context_id] = std::move(context);
    return root_context;
  }
  auto root_id_string = root_id->toString();
  auto factory = root_factories->find(root_id_string);
  RootContext* root_context;
  if (factory != root_factories->end()) {
    auto context = factory->second(context_id, root_id->view());
    root_context = context->asRoot();
    root_context_map[root_id_string] = root_context;
    context_map[context_id] = std::move(context);
  } else {
    // Default handlers.
    auto context = std::make_unique<RootContext>(context_id, root_id->view());
    root_context = context->asRoot();
    context_map[context_id] = std::move(context);
  }
  return root_context;
}

static ContextBase* getContextBase(uint32_t context_id) {
  auto it = context_map.find(context_id);
  if (it == context_map.end() || !(it->second->asContext() || it->second->asRoot())) {
    return nullptr;
  }
  return it->second.get();
}

Context* getContext(uint32_t context_id) {
  auto it = context_map.find(context_id);
  if (it == context_map.end() || !it->second->asContext()) {
    return nullptr;
  }
  return it->second->asContext();
}

static RootContext* getRootContext(uint32_t context_id) {
  auto it = context_map.find(context_id);
  if (it == context_map.end() || !it->second->asRoot()) {
    return nullptr;
  }
  return it->second->asRoot();
}

RootContext* getRoot(StringView root_id) {
  auto it = root_context_map.find(std::string(root_id));
  if (it != root_context_map.end()) {
    return it->second;
  }
  return nullptr;
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t root_context_id, uint32_t root_id_ptr,
                                                   uint32_t root_id_size,
                                                   uint32_t vm_configuration_ptr,
                                                   uint32_t vm_configuration_size) {
  ensureRootContext(root_context_id,
                    std::make_unique<WasmData>(reinterpret_cast<char*>(root_id_ptr), root_id_size))
      ->onStart(std::make_unique<WasmData>(reinterpret_cast<char*>(vm_configuration_ptr),
                                           vm_configuration_size));
}

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_validateConfiguration(uint32_t root_context_id,
                                                                     uint32_t ptr, uint32_t size) {
  return getRootContext(root_context_id)
                 ->validateConfiguration(
                     std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size))
             ? 1
             : 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE uint32_t proxy_onConfigure(uint32_t root_context_id, uint32_t ptr,
                                                           uint32_t size) {
  return getRootContext(root_context_id)
                 ->onConfigure(std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size))
             ? 1
             : 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick(uint32_t root_context_id) {
  getRootContext(root_context_id)->onTick();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onCreate(uint32_t context_id, uint32_t root_context_id) {
  ensureContext(context_id, root_context_id)->onCreate();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterStatus proxy_onNewConnection(uint32_t context_id) {
  return getContext(context_id)->onNewConnection();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterStatus proxy_onDownstreamData(uint32_t context_id,
                                                                    uint32_t data_length,
                                                                    uint32_t end_of_stream) {
  return getContext(context_id)
      ->onDownstreamData(static_cast<size_t>(data_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterStatus proxy_onUpstreamData(uint32_t context_id,
                                                                  uint32_t data_length,
                                                                  uint32_t end_of_stream) {
  return getContext(context_id)
      ->onUpstreamData(static_cast<size_t>(data_length), end_of_stream != 0);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDownstreamConnectionClose(uint32_t context_id,
                                                                       uint32_t peer_type) {
  return getContext(context_id)->onDownstreamConnectionClose(static_cast<PeerType>(peer_type));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onUpstreamConnectionClose(uint32_t context_id,
                                                                     uint32_t peer_type) {
  return getContext(context_id)->onUpstreamConnectionClose(static_cast<PeerType>(peer_type));
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterHeadersStatus proxy_onRequestHeaders(uint32_t context_id) {
  return getContext(context_id)->onRequestHeaders();
}

extern "C" EMSCRIPTEN_KEEPALIVE FilterMetadataStatus proxy_onRequestMetadata(uint32_t context_id) {
  return getContext(context_id)->onRequestMetadata();
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

extern "C" EMSCRIPTEN_KEEPALIVE FilterMetadataStatus proxy_onResponseMetadata(uint32_t context_id) {
  return getContext(context_id)->onResponseMetadata();
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

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDone(uint32_t context_id) {
  getContext(context_id)->onDone();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onLog(uint32_t context_id) {
  getContext(context_id)->onLog();
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onDelete(uint32_t context_id) {
  getContext(context_id)->onDelete();
  context_map.erase(context_id);
}

extern "C" EMSCRIPTEN_KEEPALIVE void
proxy_onHttpCallResponse(uint32_t context_id, uint32_t token, uint32_t header_pairs_ptr,
                         uint32_t header_pairs_size, uint32_t body_ptr, uint32_t body_size,
                         uint32_t trailer_pairs_ptr, uint32_t trailer_pairs_size) {
  getRootContext(context_id)
      ->onHttpCallResponse(
          token,
          std::make_unique<WasmData>(reinterpret_cast<char*>(header_pairs_ptr), header_pairs_size),
          std::make_unique<WasmData>(reinterpret_cast<char*>(body_ptr), body_size),
          std::make_unique<WasmData>(reinterpret_cast<char*>(trailer_pairs_ptr),
                                     trailer_pairs_size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcCreateInitialMetadata(uint32_t context_id,
                                                                       uint32_t token) {
  getRootContext(context_id)->onGrpcCreateInitialMetadata(token);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceiveInitialMetadata(uint32_t context_id,
                                                                        uint32_t token) {
  getRootContext(context_id)->onGrpcReceiveInitialMetadata(token);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceiveTrailingMetadata(uint32_t context_id,
                                                                         uint32_t token) {
  getRootContext(context_id)->onGrpcReceiveTrailingMetadata(token);
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcReceive(uint32_t context_id, uint32_t token,
                                                         uint32_t response_ptr,
                                                         uint32_t response_size) {
  getRootContext(context_id)
      ->onGrpcReceive(
          token, std::make_unique<WasmData>(reinterpret_cast<char*>(response_ptr), response_size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onGrpcClose(uint32_t context_id, uint32_t token,
                                                       uint32_t status_code,
                                                       uint32_t status_message_ptr,
                                                       uint32_t status_message_size) {
  getRootContext(context_id)
      ->onGrpcClose(token, static_cast<GrpcStatus>(status_code),
                    std::make_unique<WasmData>(reinterpret_cast<char*>(status_message_ptr),
                                               status_message_size));
}

extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onQueueReady(uint32_t context_id, uint32_t token) {
  getRootContext(context_id)->onQueueReady(token);
}
