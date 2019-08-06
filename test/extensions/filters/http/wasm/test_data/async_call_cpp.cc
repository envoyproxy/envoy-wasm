// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  auto context_id = id();
  auto callback = [context_id](std::unique_ptr<WasmData> response_headers, std::unique_ptr<WasmData> body,
                     std::unique_ptr<WasmData> response_trailers) {
    getContext(context_id)->setEffectiveContext();
    for (auto& p : response_headers->pairs()) {
      logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
    }
    logDebug(std::string(body->view()));
    for (auto& p : response_trailers->pairs()) {
      logWarn(std::string(p.first) + std::string(" -> ") + std::string(p.second));
    }
  };
  root()->httpCall("cluster", {{":method", "POST"}, {":path", "/"}, {":authority", "foo"}}, "hello world",
           {{"trail", "cow"}}, 1000, callback);
  return FilterHeadersStatus::StopIteration;
}
