// NOLINT(namespace-envoy)
#pragma once

#include <map>

class EnvoyContextBase {
public:
  virtual ~EnvoyContextBase() = default;
};

class EnvoyRootContext : public RootContext, public EnvoyContextBase {
public:
  EnvoyRootContext(uint32_t id, std::string_view root_id) : RootContext(id, root_id) {}
  ~EnvoyRootContext() override = default;

  virtual void onResolveDns(uint32_t /* token */, uint32_t /* result_size */) {}
  virtual void onStatsUpdate(uint32_t /* result_size */) {}
  virtual void onConfigurationRequests(uint32_t /* result_size */) {}
  virtual void onConfigurationResponse(uint32_t /* response_token */, uint32_t /* result_size */) {}
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

struct CounterResult {
  uint64_t delta;
  std::string_view name;
  uint64_t value;
};

struct GaugeResult {
  uint64_t value;
  std::string_view name;
};

struct StatResult {
  std::vector<CounterResult> counters;
  std::vector<GaugeResult> gauges;
};

enum class StatType : uint32_t {
  Counter = 1,
  Gauge = 2,
};

struct ConfigurationRequest {
  uint32_t request_token;
  std::string_view type_url;
  std::vector<std::string_view> subscribed_resources;
  std::vector<std::string_view> unsubscribed_resources;
  std::map<std::string_view, std::string_view> resource_versions;
};

inline std::vector<DnsResult> parseDnsResults(std::string_view data) {
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

inline StatResult parseStatResults(std::string_view data) {
  StatResult results;
  uint32_t data_len = 0;
  while (data_len < data.length()) {
    const uint32_t* n = reinterpret_cast<const uint32_t*>(data.data() + data_len);
    uint32_t block_size = *n++;
    uint32_t block_type = *n++;
    uint32_t num_stats = *n++;
    if (static_cast<StatType>(block_type) == StatType::Counter) { // counter
      std::vector<CounterResult> counters(num_stats);
      uint32_t stat_index = data_len + 3 * sizeof(uint32_t);
      for (uint32_t i = 0; i < num_stats; i++) {
        const uint32_t* stat_name = reinterpret_cast<const uint32_t*>(data.data() + stat_index);
        uint32_t name_len = *stat_name;
        stat_index += sizeof(uint32_t);

        auto& e = counters[i];
        e.name = {data.data() + stat_index, name_len};
        stat_index += name_len + sizeof(uint64_t);

        const uint64_t* stat_vals = reinterpret_cast<const uint64_t*>(data.data() + stat_index);
        e.value = *stat_vals++;
        e.delta = *stat_vals++;

        stat_index += 2 * sizeof(uint64_t);
      }
      results.counters = counters;
    } else if (static_cast<StatType>(block_type) == StatType::Gauge) { // gauge
      std::vector<GaugeResult> gauges(num_stats);
      uint32_t stat_index = data_len + 3 * sizeof(uint32_t);
      for (uint32_t i = 0; i < num_stats; i++) {
        const uint32_t* stat_name = reinterpret_cast<const uint32_t*>(data.data() + stat_index);
        uint32_t name_len = *stat_name;
        stat_index += sizeof(uint32_t);

        auto& e = gauges[i];
        e.name = {data.data() + stat_index, name_len};
        stat_index += name_len + sizeof(uint64_t);

        const uint64_t* stat_vals = reinterpret_cast<const uint64_t*>(data.data() + stat_index);
        e.value = *stat_vals++;

        stat_index += sizeof(uint64_t);
      }
      results.gauges = gauges;
    }
    data_len += block_size;
  }

  return results;
}

inline std::vector<ConfigurationRequest> parseConfigurationRequests(std::string_view data) {
  if (data.size() < 4) {
    return {};
  }
  const uint32_t* pn = reinterpret_cast<const uint32_t*>(data.data());
  uint32_t n = *pn++;
  uint32_t request_index = 2 * sizeof(uint32_t);
  std::vector<ConfigurationRequest> results;
  results.resize(n);
  for (uint32_t i = 0; i < n; i++) {
    auto& e = results[i];
    const uint32_t* request_tokens_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
    uint32_t request_tokens = *request_tokens_ptr++;
    e.request_token = request_tokens;
    request_index += 2 * sizeof(uint32_t);

    uint32_t name_len = *request_tokens_ptr++;
    e.type_url = {data.data() + request_index, name_len};
    request_index += name_len;
    
    const uint32_t* number_subscribed_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
    uint32_t number_subscribed = *number_subscribed_ptr++;
    request_index += sizeof(uint32_t);
    if (number_subscribed) {
      std::vector<std::string_view> subscribed;
      subscribed.resize(number_subscribed);
      for (uint32_t j = 0; j < number_subscribed; j++) {
        const uint32_t* subscribed_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
        name_len = *subscribed_ptr;
        request_index += sizeof(uint32_t);

        auto& sub_resource = subscribed[i];
        sub_resource = {data.data() + request_index, name_len};
        request_index += name_len;
      }
      e.subscribed_resources = subscribed;
    }

    const uint32_t* number_unsubscribed_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
    uint32_t number_unsubscribed = *number_unsubscribed_ptr++;
    request_index += sizeof(uint32_t);
    if (number_unsubscribed) {
      std::vector<std::string_view> unsubscribed;
      unsubscribed.resize(number_unsubscribed);
      for (uint32_t j = 0; j < number_unsubscribed; j++) {
        const uint32_t* unsubscribed_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
        name_len = *unsubscribed_ptr;
        request_index += sizeof(uint32_t);

        auto& unsub_resource = unsubscribed[i];
        unsub_resource = {data.data() + request_index, name_len};
        request_index += name_len;
      }
      e.unsubscribed_resources = unsubscribed;
    }

    const uint32_t* number_version_pair_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
    uint32_t number_version_pair = *number_version_pair_ptr++;
    request_index += sizeof(uint32_t);
    if (number_version_pair) {
      std::map<std::string_view, std::string_view> resource_version_pair;
      for (uint32_t j = 0; j < number_version_pair; j++) {
        const uint32_t* resource_pair_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
        name_len = *resource_pair_ptr;
        request_index += sizeof(uint32_t);

        std::string_view resouce = {data.data() + request_index, name_len};
        request_index += name_len;

        const uint32_t* version_pair_ptr = reinterpret_cast<const uint32_t*>(data.data() + request_index);
        name_len = *version_pair_ptr;
        request_index += sizeof(uint32_t);
        std::string_view version = {data.data() + request_index, name_len};
        request_index += name_len;

        resource_version_pair.emplace(resouce, version);
      }
      e.resource_versions = resource_version_pair;
    }
  }
  return results;
}
