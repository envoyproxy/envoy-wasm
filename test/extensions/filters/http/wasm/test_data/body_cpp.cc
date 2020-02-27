// NOLINT(namespace-envoy)
#include <cstdlib>
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;

private:
  std::string testOp_;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
  testOp_ = getRequestHeader("x-test-operation")->toString();
  return FilterHeadersStatus::Continue;
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  if (testOp_ == "ReadBody") {
    auto body = getBufferBytes(BufferType::HttpRequestBody, 0, body_buffer_length);
    logError(std::string("onRequestBody ") + std::string(body->view()));

  } else if (testOp_ == "BufferBody") {
    if (!end_of_stream) {
      return FilterDataStatus::StopIterationAndBuffer;
    }
    size_t bufferedSize;
    uint32_t flags;
    getBufferStatus(BufferType::BufferedBody, &bufferedSize, &flags);
    auto body = getBufferBytes(BufferType::BufferedBody, 0, bufferedSize);
    logError(std::string("onRequestBody ") + std::string(body->view()));
    
  } else {
    // This is a test and the test was configured incorrectly.
    logError("Invalid test op " + testOp_);
    abort();
  }
  return FilterDataStatus::Continue;
}

