#pragma once

#include "extensions/common/dynamic_forward_proxy/dns_cache.h"

#include "gmock/gmock.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace DynamicForwardProxy {

class MockDnsCache : public DnsCache {
public:
  MockDnsCache();
  ~MockDnsCache();

  struct MockLoadDnsCacheEntryResult {
    LoadDnsCacheEntryStatus status_;
    LoadDnsCacheEntryHandle* handle_;
  };

  LoadDnsCacheEntryResult loadDnsCacheEntry(absl::string_view host, uint16_t default_port,
                                            LoadDnsCacheEntryCallbacks& callbacks) override {
    MockLoadDnsCacheEntryResult result = loadDnsCacheEntry_(host, default_port, callbacks);
    return {result.status_, LoadDnsCacheEntryHandlePtr{result.handle_}};
  }
  MOCK_METHOD3(loadDnsCacheEntry_,
               MockLoadDnsCacheEntryResult(absl::string_view host, uint16_t default_port,
                                           LoadDnsCacheEntryCallbacks& callbacks));

  AddUpdateCallbacksHandlePtr addUpdateCallbacks(UpdateCallbacks& callbacks) override {
    return AddUpdateCallbacksHandlePtr{addUpdateCallbacks_(callbacks)};
  }
  MOCK_METHOD1(addUpdateCallbacks_,
               DnsCache::AddUpdateCallbacksHandle*(UpdateCallbacks& callbacks));
};

class MockLoadDnsCacheEntryHandle : public DnsCache::LoadDnsCacheEntryHandle {
public:
  MockLoadDnsCacheEntryHandle();
  ~MockLoadDnsCacheEntryHandle();

  MOCK_METHOD0(onDestroy, void());
};

class MockDnsCacheManager : public DnsCacheManager {
public:
  MockDnsCacheManager();
  ~MockDnsCacheManager();

  MOCK_METHOD1(
      getCache,
      DnsCacheSharedPtr(
          const envoy::config::common::dynamic_forward_proxy::v2alpha::DnsCacheConfig& config));

  std::shared_ptr<MockDnsCache> dns_cache_{new MockDnsCache()};
};

class MockDnsHostInfo : public DnsHostInfo {
public:
  MockDnsHostInfo();
  ~MockDnsHostInfo();

  MOCK_METHOD0(address, Network::Address::InstanceConstSharedPtr());
  MOCK_METHOD0(resolvedHost, const std::string&());
  MOCK_METHOD0(touch, void());

  Network::Address::InstanceConstSharedPtr address_;
  std::string resolved_host_;
};

class MockUpdateCallbacks : public DnsCache::UpdateCallbacks {
public:
  MockUpdateCallbacks();
  ~MockUpdateCallbacks();

  MOCK_METHOD2(onDnsHostAddOrUpdate,
               void(const std::string& host, const DnsHostInfoSharedPtr& address));
  MOCK_METHOD1(onDnsHostRemove, void(const std::string& host));
};

class MockLoadDnsCacheEntryCallbacks : public DnsCache::LoadDnsCacheEntryCallbacks {
public:
  MockLoadDnsCacheEntryCallbacks();
  ~MockLoadDnsCacheEntryCallbacks();

  MOCK_METHOD0(onLoadDnsCacheComplete, void());
};

} // namespace DynamicForwardProxy
} // namespace Common
} // namespace Extensions
} // namespace Envoy
