// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleRootContext : public RootContext {
public:
  explicit ExampleRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  void onStart(WasmDataPtr) override;
};

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  void onCreate() override;
  FilterHeadersStatus onRequestHeaders() override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  FilterHeadersStatus onResponseHeaders() override;
  void onDone() override;
  void onLog() override;
  void onDelete() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext),
                                                      ROOT_FACTORY(ExampleRootContext),
                                                      "my_root_id");

void ExampleRootContext::onStart(WasmDataPtr) { logTrace("onStart"); }

void ExampleContext::onCreate() { logWarn(std::string("onCreate " + std::to_string(id()))); }

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  logDebug(std::string("onRequestHeaders ") + std::to_string(id()));
  auto result = getRequestHeaderPairs();
  auto pairs = result->pairs();
  logInfo(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto& p : pairs) {
    logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
  return FilterHeadersStatus::Continue;
}

FilterHeadersStatus ExampleContext::onResponseHeaders() {
  logDebug(std::string("onResponseHeaders ") + std::to_string(id()));
  auto result = getResponseHeaderPairs();
  auto pairs = result->pairs();
  logInfo(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto& p : pairs) {
    logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
  addResponseHeader("newheader", "newheadervalue");
  replaceResponseHeader("location", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  auto body = getRequestBodyBufferBytes(0, body_buffer_length);
  logError(std::string("onRequestBody ") + std::string(body->view()));
  return FilterDataStatus::Continue;
}

void ExampleContext::onDone() { logWarn(std::string("onDone " + std::to_string(id()))); }

void ExampleContext::onLog() { logWarn(std::string("onLog " + std::to_string(id()))); }

void ExampleContext::onDelete() { logWarn(std::string("onDelete " + std::to_string(id()))); }
