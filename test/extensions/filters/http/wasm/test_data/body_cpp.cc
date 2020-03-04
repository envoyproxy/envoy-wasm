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
  FilterDataStatus onBody(BufferType type, size_t buffer_length, bool end);
  static void logBody(BufferType type);

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

void ExampleContext::logBody(BufferType type) {
  size_t buffered_size;
  uint32_t flags;
  getBufferStatus(type, &buffered_size, &flags);
  auto body = getBufferBytes(type, 0, buffered_size);
  logError(std::string("onRequestBody ") + std::string(body->view()));
}

FilterDataStatus ExampleContext::onBody(BufferType type, size_t buffer_length, bool end_of_stream) {
  if (test_op_ == "ReadBody") {
    auto body = getBufferBytes(type, 0, buffer_length);
    logError("onRequestBody " + std::string(body->view()));

  } else if (test_op_ == "PrependAndAppendToBody") {
    setBuffer(BufferType::HttpRequestBody, 0, 0, "prepend.");
    setBuffer(BufferType::HttpRequestBody, 0xFFFFFFFF, 0, ".append");
    auto updated = getBufferBytes(BufferType::HttpRequestBody, 0, 0xFFFFFFFF);
    logError("onRequestBody " + std::string(updated->view()));

  } else if (test_op_ == "ReplaceBody") {
    setBuffer(BufferType::HttpRequestBody, 0, 0xFFFFFFFF, "replace");
    auto replaced = getBufferBytes(BufferType::HttpRequestBody, 0, 0xFFFFFFFF);
    logError("onRequestBody " + std::string(replaced->view()));

  } else if (test_op_ == "RemoveBody") {
    setBuffer(BufferType::HttpRequestBody, 0, 0xFFFFFFFF, "");
    auto erased = getBufferBytes(BufferType::HttpRequestBody, 0, 0xFFFFFFFF);
    logError("onRequestBody " + std::string(erased->view()));  

  } else if (test_op_ == "BufferBody") {
    logBody(type);
    return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "PrependAndAppendToBufferedBody") {
    setBuffer(BufferType::HttpRequestBody, 0, 0, "prepend.");
    setBuffer(BufferType::HttpRequestBody, 0xFFFFFFFF, 0, ".append");
    logBody(type);
    return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "ReplaceBufferedBody") {
    setBuffer(BufferType::HttpRequestBody, 0, 0xFFFFFFFF, "replace");
    auto replaced = getBufferBytes(BufferType::HttpRequestBody, 0, 0xFFFFFFFF);
    logBody(type);
    return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "RemoveBufferedBody") {
    setBuffer(BufferType::HttpRequestBody, 0, 0xFFFFFFFF, "");
    auto erased = getBufferBytes(BufferType::HttpRequestBody, 0, 0xFFFFFFFF);
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
  return onBody(BufferType::HttpRequestBody, body_buffer_length, end_of_stream);
}

FilterDataStatus ExampleContext::onResponseBody(size_t body_buffer_length, bool end_of_stream) {
  return onBody(BufferType::HttpResponseBody, body_buffer_length, end_of_stream);
}
