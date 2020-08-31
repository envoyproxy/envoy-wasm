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
};

class TestRootContext : public EnvoyRootContext {
public:
  explicit TestRootContext(uint32_t id, std::string_view root_id) : EnvoyRootContext(id, root_id) {}

  bool onStart(size_t vm_configuration_size) override;
  bool onDone() override;
  void onResolveDns(uint32_t token, uint32_t results_size) override;

private:
  uint32_t dns_token_;
};

static RegisterContextFactory register_TestContext(CONTEXT_FACTORY(TestContext),
                                                   ROOT_FACTORY(TestRootContext));

bool TestRootContext::onStart(size_t) {
  envoy_resolve_dns("example.com", sizeof("example.com") - 1, &dns_token_);
  return true;
}

void TestRootContext::onResolveDns(uint32_t token, uint32_t result_size) {
  logWarn("TestRootContext::onResolveDns " + std::to_string(token));
  auto dns_buffer = getBufferBytes(WasmBufferType::CallData, 0, result_size);
  auto dns = parseDnsResults(dns_buffer->view());
  for (auto& e : dns) {
    logInfo("TestRootContext::onResolveDns dns " + std::to_string(e.ttl_seconds) + " " + e.address);
  }
}

bool TestRootContext::onDone() {
  logWarn("TestRootContext::onDone " + std::to_string(id()));
  return true;
}

END_WASM_PLUGIN
