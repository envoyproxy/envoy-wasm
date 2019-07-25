// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
 public:
  explicit ExampleContext(uint32_t id, RootContext *root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
};
static RegisterContextFactory
    register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  try {
    WasmDataPtr data = getRequestHeader("x-callout-url");

    if (!data) {
      logWarn("Missing x-callout-url header, cannot forward");
      return FilterHeadersStatus::Continue;
    }

    StringView callout_url{data->view()};

    logInfo("Forwarding to: " + std::string(callout_url));

    auto callback = [](std::unique_ptr<WasmData> response_headers,
                       std::unique_ptr<WasmData> body,
                       std::unique_ptr<WasmData> response_trailers) {
      logInfo("Got response");
    };
    httpCall(callout_url,
             {{":method", "POST"}, {":path", "/"}, {":authority", "foo"}},
             "hello world",
             {{"trail", "cow"}},
             1000,
             callback);
  } catch (const std::exception &ex) {
    logError("Caught exception: " + std::string{ex.what()});
  } catch (const std::string &ex) {
    logError("Caught exception: " + ex);
  } catch (...) {
    logError("Caught exception");
  }

  return FilterHeadersStatus::StopIteration;
}

