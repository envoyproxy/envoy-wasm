// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  void onLog() override;
  void onDone() override;
};

class ExampleRootContext : public RootContext {
public:
  explicit ExampleRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  void onTick() override;

  uint32_t stream_context_id_;
};

static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext),
                                                      ROOT_FACTORY(ExampleRootContext));

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
  static_cast<ExampleRootContext*>(root())->stream_context_id_ = id();
  logDebug(std::string("onRequestHeaders ") + std::to_string(id()));
  auto path = getRequestHeader(":path");
  logInfo(std::string("header path ") + std::string(path->view()));
  std::string protocol;
  addRequestHeader("newheader", "newheadervalue");
  auto server = getRequestHeader("server");
  replaceRequestHeader("server", "envoy-wasm");
  if (server->view() == "envoy-wasm-pause") {
    return FilterHeadersStatus::StopIteration;
  } else {
    return FilterHeadersStatus::Continue;
  }
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  auto body = getBufferBytes(BufferType::HttpRequestBody, 0, body_buffer_length);
  logError(std::string("onRequestBody ") + std::string(body->view()));
  return FilterDataStatus::Continue;
}

void ExampleContext::onLog() {
  auto path = getRequestHeader(":path");
  logWarn("onLog " + std::to_string(id()) + " " + std::string(path->view()));
}

void ExampleContext::onDone() { logWarn("onDone " + std::to_string(id())); }

void ExampleRootContext::onTick() {
  getContext(stream_context_id_)->setEffectiveContext();
  replaceRequestHeader("server", "envoy-wasm-continue");
  continueRequest();
}

