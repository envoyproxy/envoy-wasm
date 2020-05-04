// NOLINT(namespace-envoy)
#include <cassert>
#include <cstdlib>
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t) override;
  FilterHeadersStatus onResponseHeaders(uint32_t) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) override;

private:
  FilterDataStatus onBody(WasmBufferType type, size_t buffer_length, bool end);
  static void logBody(WasmBufferType type);

  std::string test_op_;
  int num_chunks_ = 0;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
  test_op_ = getRequestHeader("x-test-operation")->toString();
  return FilterHeadersStatus::Continue;
}

FilterHeadersStatus ExampleContext::onResponseHeaders(uint32_t) {
  test_op_ = getResponseHeader("x-test-operation")->toString();
  return FilterHeadersStatus::Continue;
}

void ExampleContext::logBody(WasmBufferType type) {
  size_t buffered_size;
  uint32_t flags;
  getBufferStatus(type, &buffered_size, &flags);
  auto body = getBufferBytes(type, 0, buffered_size);
  logError(std::string("onRequestBody ") + std::string(body->view()));
}

FilterDataStatus ExampleContext::onBody(WasmBufferType type, size_t buffer_length,
                                        bool end_of_stream) {
  size_t size;
  uint32_t flags;
  if (test_op_ == "ReadBody") {
    auto body = getBufferBytes(type, 0, buffer_length);
    logError("onRequestBody " + std::string(body->view()));

  } else if (test_op_ == "PrependAndAppendToBody") {
    setBuffer(WasmBufferType::HttpRequestBody, 0, 0, "prepend.");
    getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
    setBuffer(WasmBufferType::HttpRequestBody, size, 0, ".append");
    getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
    auto updated = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
    logError("onRequestBody " + std::string(updated->view()));

  } else if (test_op_ == "ReplaceBody") {
    setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "replace");
    getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
    auto replaced = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
    logError("onRequestBody " + std::string(replaced->view()));

  } else if (test_op_ == "RemoveBody") {
    setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "");
    getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
    auto erased = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
    logError("onRequestBody " + std::string(erased->view()));

  } else if (test_op_ == "BufferBody") {
    logBody(type);
    return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "PrependAndAppendToBufferedBody") {
    setBuffer(WasmBufferType::HttpRequestBody, 0, 0, "prepend.");
    getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
    setBuffer(WasmBufferType::HttpRequestBody, size, 0, ".append");
    logBody(type);
    return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "ReplaceBufferedBody") {
    setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "replace");
    getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
    auto replaced = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
    logBody(type);
    return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "RemoveBufferedBody") {
    setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "");
    getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
    auto erased = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
    logBody(type);
    return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "BufferTwoBodies") {
    logBody(type);
    num_chunks_++;
    if (end_of_stream || num_chunks_ > 2) {
      return FilterDataStatus::Continue;
    }
    return FilterDataStatus::StopIterationAndBuffer;

  } else {
    // This is a test and the test was configured incorrectly.
    logError("Invalid test op " + test_op_);
    abort();
  }
  return FilterDataStatus::Continue;
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  return onBody(WasmBufferType::HttpRequestBody, body_buffer_length, end_of_stream);
}

FilterDataStatus ExampleContext::onResponseBody(size_t body_buffer_length, bool end_of_stream) {
  return onBody(WasmBufferType::HttpResponseBody, body_buffer_length, end_of_stream);
}
