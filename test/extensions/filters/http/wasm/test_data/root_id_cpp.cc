// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class Context1 : public Context {
public:
  Context1(uint32_t id, RootContext* root) : Context(id, root) {}
  FilterHeadersStatus onRequestHeaders(uint32_t) override;
};

class Context2 : public Context {
public:
  Context2(uint32_t id, RootContext* root) : Context(id, root) {}
  FilterHeadersStatus onRequestHeaders(uint32_t) override;
};

static RegisterContextFactory register_Context1(CONTEXT_FACTORY(Context1), "context1");
static RegisterContextFactory register_Contxt2(CONTEXT_FACTORY(Context2), "context2");

FilterHeadersStatus Context1::onRequestHeaders(uint32_t) {
  logDebug(std::string("onRequestHeaders1 ") + std::to_string(id()));
  return FilterHeadersStatus::Continue;
}

FilterHeadersStatus Context2::onRequestHeaders(uint32_t) {
  logDebug(std::string("onRequestHeaders2 ") + std::to_string(id()));
  return FilterHeadersStatus::Continue;
}
