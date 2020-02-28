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
  auto context_id = id();
  auto callback = [context_id](uint32_t, size_t body_size, uint32_t) {
    auto response_headers = getHeaderMapPairs(HeaderMapType::HttpCallResponseHeaders);
    // Switch context after getting headers, but before getting body to exercise both code paths.
    getContext(context_id)->setEffectiveContext();
    auto body = getBufferBytes(BufferType::HttpCallResponseBody, 0, body_size);
    auto response_trailers = getHeaderMapPairs(HeaderMapType::HttpCallResponseTrailers);
    for (auto& p : response_headers->pairs()) {
      logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
    }
    logDebug(std::string(body->view()));
    for (auto& p : response_trailers->pairs()) {
      logWarn(std::string(p.first) + std::string(" -> ") + std::string(p.second));
    }
  };
  root()->httpCall("cluster", {{":method", "POST"}, {":path", "/"}, {":authority", "foo"}},
                   "hello world", {{"trail", "cow"}}, 1000, callback);
  return FilterHeadersStatus::StopIteration;
}
