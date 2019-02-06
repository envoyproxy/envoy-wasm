#include <string>
#include <unordered_map>

#include "envoy_wasm_intrinsics.h"


class ExampleContext : public Context {
  public:
    explicit ExampleContext(uint32_t id) : Context(id) {}

    void onStart() override;
    FilterHeadersStatus onRequestHeaders() override;
    FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
    FilterHeadersStatus onResponseHeaders() override;
    void onLog() override;
    void onDone() override;
};

std::unique_ptr<Context> Context::New(uint32_t id) {
  return std::unique_ptr<Context>(new ExampleContext(id));
}

void ExampleContext::onStart() {
  logTrace("onStart");
}

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  logDebug(std::string("onRequestHaders ") + std::to_string(id()));
  auto result = getRequestHeaderPairs();
  auto pairs = result->pairs();
  logInfo(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto& p : pairs) {
    logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
  return FilterHeadersStatus::Continue;
}

FilterHeadersStatus ExampleContext::onResponseHeaders() {
  logDebug(std::string("onResponseHaders ") + std::to_string(id()));
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

void ExampleContext::onLog() {
  logWarn(std::string("onLog " + std::to_string(id())));
}

void ExampleContext::onDone() {
  logWarn(std::string("onDone " + std::to_string(id())));
}

