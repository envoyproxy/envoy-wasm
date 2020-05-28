// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>
#include <vector>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics.h"
#include "source/extensions/common/wasm/ext/envoy_context.h"
#else
#include "include/proxy-wasm/null_plugin.h"
using namespace proxy_wasm::null_plugin;
#include "extensions/common/wasm/ext/envoy_context.h"
#endif

START_WASM_PLUGIN(CommonWasmTestContextCpp)

class TestContext : public EnvoyContext {
public:
  explicit TestContext(uint32_t id, RootContext* root) : EnvoyContext(id, root) {}

  void onResolveDns(uint32_t token, uint32_t results_size) override;
  void onDone() override;
};

class TestRootContext : public RootContext {
public:
  explicit TestRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  bool onDone() override;
};

static RegisterContextFactory register_TestContext(CONTEXT_FACTORY(TestContext),
                                                   ROOT_FACTORY(TestRootContext));

struct DnsResult {
  uint32_t ttl_seconds;
  std::string address;
};

std::vector<DnsResult> convertDnsResults(StringView data) {
  if (data.size() < 4) {
    return {};
  }
  const uint32_t* pn = reinterpret_cast<const uint32_t*>(data.data());
  uint32_t n = *pn++;
  std::vector<DnsResult> results;
  results.resize(n);
  const char* pa = data.data() + (1 + n) * sizeof(uint32_t); // skip n + n TTLs
  for (uint32_t i = 0; i < n; i++) {
    auto& e = results[i];
    e.ttl_seconds = *pn++;
    auto alen = strlen(pa);
    e.address.assign(pa, alen);
    pa += alen + 1;
  }
  return results;
}

void TestContext::onResolveDns(uint32_t token, uint32_t result_size) {
  logWarn("TestContext::onResolveDns " + std::to_string(token));
  auto dns_buffer = getBufferBytes(WasmBufferType::CallData, 0, result_size);
  auto dns = convertDnsResults(dns_buffer->view());
  for (auto& e : dns) {
    logInfo("TestContext::onResolveDns dns " + std::to_string(e.ttl_seconds) + " " + e.address);
  }
}

void TestContext::onDone() { logWarn("TestContext::onDone " + std::to_string(id())); }

bool TestRootContext::onDone() {
  logWarn("TestRootContext::onDone " + std::to_string(id()));
  return true;
}

END_WASM_PLUGIN
