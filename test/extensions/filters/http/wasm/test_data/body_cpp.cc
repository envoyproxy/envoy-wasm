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
  FilterDataStatus onBody(BufferType bt, size_t bufLen, bool end);
  static void logBody(BufferType bt);

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

void ExampleContext::logBody(BufferType bt) {
  size_t bufferedSize;
  uint32_t flags;
  getBufferStatus(bt, &bufferedSize, &flags);
  auto body = getBufferBytes(bt, 0, bufferedSize);
  logError(std::string("onRequestBody ") + std::string(body->view()));
}

FilterDataStatus ExampleContext::onBody(BufferType bt, size_t bufLen, bool end) {
  if (test_op_ == "ReadBody") {
    auto body = getBufferBytes(bt, 0, bufLen);
    logError("onRequestBody " + std::string(body->view()));

  } else if (test_op_ == "BufferBody") {
    logBody(bt);
    return end ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

  } else if (test_op_ == "BufferTwoBodies") {
    logBody(bt);
    num_chunks_++;
    if (end || num_chunks_ > 2) {
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