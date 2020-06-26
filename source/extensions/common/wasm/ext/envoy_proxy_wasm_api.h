// NOLINT(namespace-envoy)
#pragma once

class EnvoyContextBase {
public:
  virtual ~EnvoyContextBase() = default;

  virtual void onResolveDns(uint32_t /* token */, uint32_t /* result_size */) {}
};

class EnvoyRootContext : public RootContext, public EnvoyContextBase {
public:
  EnvoyRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}
  ~EnvoyRootContext() override = default;
};

class EnvoyContext : public Context, public EnvoyContextBase {
public:
  EnvoyContext(uint32_t id, RootContext* root) : Context(id, root) {}
  ~EnvoyContext() override = default;
};

struct DnsResult {
  uint32_t ttl_seconds;
  std::string address;
};

inline std::vector<DnsResult> parseDnsResults(StringView data) {
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
