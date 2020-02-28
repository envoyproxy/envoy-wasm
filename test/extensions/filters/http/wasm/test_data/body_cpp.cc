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

  std::string testOp_;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
  testOp_ = getRequestHeader("x-test-operation")->toString();
  return FilterHeadersStatus::Continue;
}

FilterHeadersStatus ExampleContext::onResponseHeaders(uint32_t) {
  testOp_ = getResponseHeader("x-test-operation")->toString();
  return FilterHeadersStatus::Continue;
}

FilterDataStatus ExampleContext::onBody(BufferType bt, size_t bufLen, bool end) {
  if (testOp_ == "ReadBody") {
    auto body = getBufferBytes(bt, 0, bufLen);
    logError("onRequestBody " + std::string(body->view()));

  } else if (testOp_ == "BufferBody") {
    if (!end) {
      logError("Requesting buffering");
      return FilterDataStatus::StopIterationAndBuffer;
    }
    size_t tmpSize;
    uint32_t flags;
    getBufferStatus(bt, &tmpSize, &flags);
    assert(tmpSize == bufLen);
    auto body = getBufferBytes(bt, 0, bufLen);
    logError(std::string("onRequestBody ") + std::string(body->view()));
    
  } else {
    // This is a test and the test was configured incorrectly.
    logError("Invalid test op " + testOp_);
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