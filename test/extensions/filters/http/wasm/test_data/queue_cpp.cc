// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t, bool) override;
};

class ExampleRootContext : public RootContext {
public:
  explicit ExampleRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  bool onStart(size_t configuration_size) override;
  void onQueueReady(uint32_t token) override;

  uint32_t token_;
};

static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext),
                                                      ROOT_FACTORY(ExampleRootContext));

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t, bool) {
  uint32_t token;
  if (resolveSharedQueue("vm_id", "bad_shared_queue", &token) == WasmResult::NotFound) {
    logWarn("onRequestHeaders not found bad_shared_queue");
  }
  CHECK_RESULT(resolveSharedQueue("vm_id", "my_shared_queue", &token));
  if (enqueueSharedQueue(token, "data1") == WasmResult::Ok) {
    logWarn("onRequestHeaders enqueue Ok");
  }
  return FilterHeadersStatus::Continue;
}

bool ExampleRootContext::onStart(size_t) {
  CHECK_RESULT(registerSharedQueue("my_shared_queue", &token_));
  return true;
}

void ExampleRootContext::onQueueReady(uint32_t token) {
  if (token == token_) {
    logInfo("onQueueReady");
  }
  std::unique_ptr<WasmData> data;
  if (dequeueSharedQueue(9999999 /* bad token */, &data) == WasmResult::NotFound) {
    logWarn("onQueueReady bad token not found");
  }
  if (dequeueSharedQueue(token, &data) == WasmResult::Ok) {
    logDebug("data " + data->toString() + " Ok");
  }
  if (dequeueSharedQueue(token, &data) == WasmResult::Empty) {
    logWarn("onQueueReady extra data not found");
  }
}
