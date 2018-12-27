#include <string>
#include <unordered_map>

#include "envoy_wasm_filter_intrinsics.h"


class ExampleContext : public Context {
  public:
    explicit ExampleContext(uint32_t id) : Context(id) {}

    FilterHeadersStatus onRequestHeaders() override;
    FilterDataStatus onRequestBody(size_t body_size, bool end_of_stream) override;
    FilterTrailersStatus onRequestTrailers() override;

    void onHttpCallResponse(
        uint32_t token,
        std::unique_ptr<WasmData> response_headers,
        std::unique_ptr<WasmData> body,
        std::unique_ptr<WasmData> response_trailers) override;
};

std::unique_ptr<Context> Context::New(uint32_t id) {
  return std::unique_ptr<Context>(new ExampleContext(id));
}

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  return FilterHeadersStatus::StopIteration;
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_size, bool end_of_stream) {
  return FilterDataStatus::StopIterationAndBuffer;
}

FilterTrailersStatus ExampleContext::onRequestTrailers() {
  httpCall("cluster", {{ ":method", "POST"}, {":path", "/"}, {":authority", "foo"}}, "hello world", {{"trail", "cow"}}, 1000);
  return FilterTrailersStatus::StopIteration;
}

void ExampleContext::onHttpCallResponse( uint32_t token, std::unique_ptr<WasmData> response_headers,
    std::unique_ptr<WasmData> body, std::unique_ptr<WasmData> response_trailers) {
  logTrace(std::to_string(token));
  for (auto &p : response_headers->pairs()) {
    logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
  logDebug(std::string(body->view()));
  for (auto &p : response_trailers->pairs()) {
    logWarn(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
}
