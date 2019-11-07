// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t) override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
  WasmDataPtr data = getRequestHeader("x-callout-url");

  if (!data) {
    logWarn("Missing x-callout-url header, cannot forward");
    return FilterHeadersStatus::Continue;
  }

  StringView callout_url{data->view()};

  logInfo("Forwarding to: " + std::string(callout_url));

  auto callback = [](uint32_t, size_t, uint32_t) { logInfo("Got response"); };
  root()->httpCall(callout_url, {{":method", "POST"}, {":path", "/"}, {":authority", "foo"}},
                   "hello world", {{"trail", "cow"}}, 1000, callback);

  return FilterHeadersStatus::StopIteration;
}

