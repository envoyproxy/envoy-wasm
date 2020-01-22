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

RegisterContextFactory::RegisterContextFactory(RootFactory root_factory, StringView root_id) {
  if (!root_factories) {
    root_factories = new std::unordered_map<std::string, RootFactory>;
    context_factories = new std::unordered_map<std::string, ContextFactory>;
  }
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

static RootContext* ensureRootContext(uint32_t context_id) {
  auto it = context_map.find(context_id);
  if (it != context_map.end()) {
    return it->second->asRoot();
  }
  const char* root_id_ptr = nullptr;
  size_t root_id_size = 0;
  CHECK_RESULT(proxy_get_property("plugin_root_id", sizeof("plugin_root_id") - 1, &root_id_ptr,
                                  &root_id_size));
  auto root_id = std::make_unique<WasmData>(root_id_ptr, root_id_size);
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

extern "C" PROXY_WASM_KEEPALIVE uint32_t proxy_on_vm_start(uint32_t root_context_id,
                                                           uint32_t vm_configuration_size) {
  return getRootContext(root_context_id)->onStart(vm_configuration_size);
}

extern "C" PROXY_WASM_KEEPALIVE uint32_t proxy_validate_configuration(uint32_t root_context_id,
                                                                      uint32_t configuration_size) {
  return getRootContext(root_context_id)->validateConfiguration(configuration_size) ? 1 : 0;
}

extern "C" PROXY_WASM_KEEPALIVE uint32_t proxy_on_configure(uint32_t root_context_id,
                                                            uint32_t configuration_size) {
  return getRootContext(root_context_id)->onConfigure(configuration_size) ? 1 : 0;
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_tick(uint32_t root_context_id) {
  getRootContext(root_context_id)->onTick();
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_context_create(uint32_t context_id,
                                                             uint32_t parent_context_id) {
  if (parent_context_id) {
    ensureContext(context_id, parent_context_id)->onCreate();
  } else {
    ensureRootContext(context_id);
  }
}

extern "C" PROXY_WASM_KEEPALIVE FilterStatus proxy_on_new_connection(uint32_t context_id) {
  return getContext(context_id)->onNewConnection();
}

extern "C" PROXY_WASM_KEEPALIVE FilterStatus proxy_on_downstream_data(uint32_t context_id,
                                                                      uint32_t data_length,
                                                                      uint32_t end_of_stream) {
  return getContext(context_id)
      ->onDownstreamData(static_cast<size_t>(data_length), end_of_stream != 0);
}

extern "C" PROXY_WASM_KEEPALIVE FilterStatus proxy_on_upstream_data(uint32_t context_id,
                                                                    uint32_t data_length,
                                                                    uint32_t end_of_stream) {
  return getContext(context_id)
      ->onUpstreamData(static_cast<size_t>(data_length), end_of_stream != 0);
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_downstream_connection_close(uint32_t context_id,
                                                                          uint32_t peer_type) {
  return getContext(context_id)->onDownstreamConnectionClose(static_cast<PeerType>(peer_type));
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_upstream_connection_close(uint32_t context_id,
                                                                        uint32_t peer_type) {
  return getContext(context_id)->onUpstreamConnectionClose(static_cast<PeerType>(peer_type));
}

extern "C" PROXY_WASM_KEEPALIVE FilterHeadersStatus proxy_on_request_headers(uint32_t context_id,
                                                                             uint32_t headers) {
  return getContext(context_id)->onRequestHeaders(headers);
}

extern "C" PROXY_WASM_KEEPALIVE FilterMetadataStatus proxy_on_request_metadata(uint32_t context_id,
                                                                               uint32_t elements) {
  return getContext(context_id)->onRequestMetadata(elements);
}

extern "C" PROXY_WASM_KEEPALIVE FilterDataStatus proxy_on_request_body(uint32_t context_id,
                                                                       uint32_t body_buffer_length,
                                                                       uint32_t end_of_stream) {
  return getContext(context_id)
      ->onRequestBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" PROXY_WASM_KEEPALIVE FilterTrailersStatus proxy_on_request_trailers(uint32_t context_id,
                                                                               uint32_t trailers) {
  return getContext(context_id)->onRequestTrailers(trailers);
}

extern "C" PROXY_WASM_KEEPALIVE FilterHeadersStatus proxy_on_response_headers(uint32_t context_id,
                                                                              uint32_t headers) {
  return getContext(context_id)->onResponseHeaders(headers);
}

extern "C" PROXY_WASM_KEEPALIVE FilterMetadataStatus proxy_on_response_metadata(uint32_t context_id,
                                                                                uint32_t elements) {
  return getContext(context_id)->onResponseMetadata(elements);
}

extern "C" PROXY_WASM_KEEPALIVE FilterDataStatus proxy_on_response_body(uint32_t context_id,
                                                                        uint32_t body_buffer_length,
                                                                        uint32_t end_of_stream) {
  return getContext(context_id)
      ->onResponseBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0);
}

extern "C" PROXY_WASM_KEEPALIVE FilterTrailersStatus proxy_on_response_trailers(uint32_t context_id,
                                                                                uint32_t trailers) {
  return getContext(context_id)->onResponseTrailers(trailers);
}

extern "C" PROXY_WASM_KEEPALIVE uint32_t proxy_on_done(uint32_t context_id) {
  return getContextBase(context_id)->onDoneBase();
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_log(uint32_t context_id) {
  getContext(context_id)->onLog();
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_delete(uint32_t context_id) {
  getContextBase(context_id)->onDelete();
  context_map.erase(context_id);
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_http_call_response(uint32_t context_id,
                                                                 uint32_t token, uint32_t headers,
                                                                 uint32_t body_size,
                                                                 uint32_t trailers) {
  getRootContext(context_id)
      ->onHttpCallResponse(token, headers, static_cast<size_t>(body_size), trailers);
}

extern "C" PROXY_WASM_KEEPALIVE void
proxy_on_grpc_create_initial_metadata(uint32_t context_id, uint32_t token, uint32_t headers) {
  getRootContext(context_id)->onGrpcCreateInitialMetadata(token, headers);
}

extern "C" PROXY_WASM_KEEPALIVE void
proxy_on_grpc_receive_initial_metadata(uint32_t context_id, uint32_t token, uint32_t headers) {
  getRootContext(context_id)->onGrpcReceiveInitialMetadata(token, headers);
}

extern "C" PROXY_WASM_KEEPALIVE void
proxy_on_grpc_receive_trailing_metadata(uint32_t context_id, uint32_t token, uint32_t trailers) {
  getRootContext(context_id)->onGrpcReceiveTrailingMetadata(token, trailers);
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_grpc_receive(uint32_t context_id, uint32_t token,
                                                           uint32_t response_size) {
  getRootContext(context_id)->onGrpcReceive(token, static_cast<size_t>(response_size));
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_grpc_close(uint32_t context_id, uint32_t token,
                                                         uint32_t status_code) {
  getRootContext(context_id)->onGrpcClose(token, static_cast<GrpcStatus>(status_code));
}

extern "C" PROXY_WASM_KEEPALIVE void proxy_on_queue_ready(uint32_t context_id, uint32_t token) {
  getRootContext(context_id)->onQueueReady(token);
}
