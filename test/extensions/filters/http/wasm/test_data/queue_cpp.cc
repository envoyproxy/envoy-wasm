// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
};

class ExampleRootContext : public RootContext {
public:
  explicit ExampleRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  void onStart() override;
  void onQueueReady(uint32_t token) override;
};

static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext), ROOT_FACTORY(ExampleRootContext));

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  auto token = resolveSharedQueue("vm_id", "my_shared_queue");
  auto result = enqueueSharedQueue(token, "data1");
  logWarn("onRequestHeaders " + std::to_string(result));
  return FilterHeadersStatus::Continue;
}

void ExampleRootContext::onStart() {
  registerSharedQueue("my_shared_queue");
}

void ExampleRootContext::onQueueReady(uint32_t token) {
  logInfo("onQueueReady");
  std::unique_ptr<WasmData> data;
  auto result = dequeueSharedQueue(token, &data);
  logDebug("data " + data->toString() + " " + std::to_string(result));
}
