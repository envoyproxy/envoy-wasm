// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t) override;
  void onLog() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
  CHECK_RESULT(setSharedData("shared_data_key1", "shared_data_value1"));
  CHECK_RESULT(setSharedData("shared_data_key2", "shared_data_value2"));
  uint32_t cas = 0;
  auto value2 = getSharedDataValue("shared_data_key2", &cas);
  if (WasmResult::CasMismatch ==
      setSharedData("shared_data_key2", "shared_data_value3", cas + 1)) { // Bad cas.
    logInfo("set CasMismatch");
  }
  return FilterHeadersStatus::Continue;
}

void ExampleContext::onLog() {
  auto value1 = getSharedDataValue("shared_data_key1");
  logDebug("get 1 " + value1->toString());
  auto value2 = getSharedDataValue("shared_data_key2");
  logWarn("get 2 " + value2->toString());
}
