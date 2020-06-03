// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>
#include <vector>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics.h"
#include "source/extensions/common/wasm/ext/envoy_proxy_wasm_api.h"
#else
#include "extensions/common/wasm/ext/envoy_null_plugin.h"
#endif

START_WASM_PLUGIN(CommonWasmTestContextCpp)

class TestContext : public EnvoyContext {
public:
  explicit TestContext(uint32_t id, RootContext* root) : EnvoyContext(id, root) {}

  void onResolveDns(uint32_t token, uint32_t results_size) override;
};

class TestRootContext : public EnvoyRootContext {
public:
  explicit TestRootContext(uint32_t id, StringView root_id) : EnvoyRootContext(id, root_id) {}

  bool onDone() override;
};

static RegisterContextFactory register_TestContext(CONTEXT_FACTORY(TestContext),
                                                   ROOT_FACTORY(TestRootContext));

void TestContext::onResolveDns(uint32_t token, uint32_t result_size) {
  logWarn("TestContext::onResolveDns " + std::to_string(token));
  auto dns_buffer = getBufferBytes(WasmBufferType::CallData, 0, result_size);
  auto dns = parseDnsResults(dns_buffer->view());
  for (auto& e : dns) {
    logInfo("TestContext::onResolveDns dns " + std::to_string(e.ttl_seconds) + " " + e.address);
  }
}

bool TestRootContext::onDone() {
  logWarn("TestRootContext::onDone " + std::to_string(id()));
  return true;
}

END_WASM_PLUGIN
