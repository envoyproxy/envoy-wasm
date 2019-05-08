// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id) : Context(id) {}

  FilterHeadersStatus onRequestHeaders() override;
};

std::unique_ptr<Context> Context::New(uint32_t id) {
  return std::unique_ptr<Context>(new ExampleContext(id));
}

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  auto callback = [](std::unique_ptr<WasmData> response_headers, std::unique_ptr<WasmData> body,
                     std::unique_ptr<WasmData> response_trailers) {
    for (auto& p : response_headers->pairs()) {
      logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
    }
    logDebug(std::string(body->view()));
    for (auto& p : response_trailers->pairs()) {
      logWarn(std::string(p.first) + std::string(" -> ") + std::string(p.second));
    }
  };
  httpCall("cluster", {{":method", "POST"}, {":path", "/"}, {":authority", "foo"}}, "hello world",
           {{"trail", "cow"}}, 1000, callback);
  return FilterHeadersStatus::StopIteration;
}
