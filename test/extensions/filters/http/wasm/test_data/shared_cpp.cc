// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
  void onLog() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  auto result1 = setSharedData("shared_data_key1", "shared_data_value1");
  auto result2 = setSharedData("shared_data_key2", "shared_data_value2");
  uint32_t cas = 0;
  auto value2 = getSharedData("shared_data_key2", &cas);
  auto result3 = setSharedData("shared_data_key2", "shared_data_value3", cas+1);  // Bad cas.
  logInfo("set " + std::to_string(result1) + " " + std::to_string(result2) + " " + std::to_string(result3));
  return FilterHeadersStatus::Continue;
}

void ExampleContext::onLog() {
  auto value1 = getSharedData("shared_data_key1");
  logDebug("get 1 " + value1->toString());
  auto value2 = getSharedData("shared_data_key2");
  logWarn("get 2 " + value2->toString());
}
