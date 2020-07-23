#include "envoy/server/lifecycle_notifier.h"

#include "common/common/hex.h"
#include "common/event/dispatcher_impl.h"
#include "common/stats/isolated_store_impl.h"

#include "extensions/common/wasm/wasm.h"

#include "test/mocks/server/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"
#include "test/test_common/wasm_base.h"

#include "absl/types/optional.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openssl/bytestring.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"

using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

std::string Sha256(absl::string_view data) {
  std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);
  EVP_MD_CTX* ctx(EVP_MD_CTX_new());
  auto rc = EVP_DigestInit(ctx, EVP_sha256());
  RELEASE_ASSERT(rc == 1, "Failed to init digest context");
  rc = EVP_DigestUpdate(ctx, data.data(), data.size());
  RELEASE_ASSERT(rc == 1, "Failed to update digest");
  rc = EVP_DigestFinal(ctx, digest.data(), nullptr);
  RELEASE_ASSERT(rc == 1, "Failed to finalize digest");
  EVP_MD_CTX_free(ctx);
  return std::string(reinterpret_cast<const char*>(&digest[0]), digest.size());
}

class TestContext : public Extensions::Common::Wasm::Context {
public:
  TestContext() : Extensions::Common::Wasm::Context() {}
  TestContext(Extensions::Common::Wasm::Wasm* wasm) : Extensions::Common::Wasm::Context(wasm) {}
  TestContext(Extensions::Common::Wasm::Wasm* wasm,
              const Extensions::Common::Wasm::PluginSharedPtr& plugin)
      : Extensions::Common::Wasm::Context(wasm, plugin) {}
  TestContext(Extensions::Common::Wasm::Wasm* wasm, uint32_t root_context_id,
              const Extensions::Common::Wasm::PluginSharedPtr& plugin)
      : Extensions::Common::Wasm::Context(wasm, root_context_id, plugin) {}
  ~TestContext() override = default;
  proxy_wasm::WasmResult log(uint32_t level, absl::string_view message) override {
    std::cerr << std::string(message) << "\n";
    log_(static_cast<spdlog::level::level_enum>(level), message);
    Extensions::Common::Wasm::Context::log(static_cast<spdlog::level::level_enum>(level), message);
    return proxy_wasm::WasmResult::Ok;
  }
  MOCK_METHOD2(log_, void(spdlog::level::level_enum level, absl::string_view message));
};

class WasmCommonTest : public testing::TestWithParam<std::string> {
public:
  void SetUp() { clearCodeCacheForTesting(false); }
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmCommonTest, testing::Values("v8", "null"));
TEST_P(WasmCommonTest, Logging) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "logging";
  auto plugin_configuration = "configure-test";
  std::string code;
  if (GetParam() != "null") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
        absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestCpp";
  }
  EXPECT_FALSE(code.empty());
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  auto wasm_weak = std::weak_ptr<Extensions::Common::Wasm::Wasm>(wasm);
  auto wasm_handle = std::make_shared<Extensions::Common::Wasm::WasmHandle>(std::move(wasm));
  EXPECT_TRUE(wasm_weak.lock()->initialize(code, false));
  auto thread_local_wasm = std::make_shared<Wasm>(wasm_handle, *dispatcher);
  thread_local_wasm.reset();

  auto wasm_lock = wasm_weak.lock();
  wasm_lock->setCreateContextForTesting(
      nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto root_context = new TestContext(wasm, plugin);
        EXPECT_CALL(*root_context,
                    log_(spdlog::level::info, Eq("on_configuration configure-test")));
        EXPECT_CALL(*root_context, log_(spdlog::level::trace, Eq("test trace logging")));
        EXPECT_CALL(*root_context, log_(spdlog::level::debug, Eq("test debug logging")));
        EXPECT_CALL(*root_context, log_(spdlog::level::warn, Eq("test warn logging")));
        EXPECT_CALL(*root_context, log_(spdlog::level::err, Eq("test error logging")));
        EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_done logging")));
        EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_delete logging")));
        return root_context;
      });

  auto root_context = wasm_weak.lock()->start(plugin);
  wasm_weak.lock()->configure(root_context, plugin);

  wasm_handle.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  // This will fault on nullptr if wasm has been deleted.
  plugin->plugin_configuration_ = "done";
  wasm_weak.lock()->configure(root_context, plugin);
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  dispatcher->clearDeferredDeleteList();
}

TEST_P(WasmCommonTest, BadSignature) {
  if (GetParam() != "v8") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "";
  auto plugin_configuration = "";
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/bad_signature_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_FALSE(wasm->initialize(code, false));
  EXPECT_TRUE(wasm->isFailed());
}

TEST_P(WasmCommonTest, Segv) {
  if (GetParam() != "v8") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "segv";
  auto plugin_configuration = "";
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->setCreateContextForTesting(
      nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto root_context = new TestContext(wasm, plugin);
        EXPECT_CALL(*root_context, log_(spdlog::level::err, Eq("before badptr")));
        return root_context;
      });
  wasm->start(plugin);
  EXPECT_TRUE(wasm->isFailed());
}

TEST_P(WasmCommonTest, DivByZero) {
  if (GetParam() != "v8") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "divbyzero";
  auto plugin_configuration = "";
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->setCreateContextForTesting(
      nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto root_context = new TestContext(wasm, plugin);
        EXPECT_CALL(*root_context, log_(spdlog::level::err, Eq("before div by zero")));
        return root_context;
      });
  wasm->start(plugin);
}

TEST_P(WasmCommonTest, EmscriptenVersion) {
  if (GetParam() != "v8") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "";
  auto plugin_configuration = "";
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_TRUE(wasm->initialize(code, false));

  uint32_t major = 9, minor = 9, abi_major = 9, abi_minor = 9;
  EXPECT_TRUE(wasm->getEmscriptenVersion(&major, &minor, &abi_major, &abi_minor));
  EXPECT_EQ(major, 0);
  EXPECT_LE(minor, 3);
  // Up to (at least) emsdk 1.39.6.
  EXPECT_EQ(abi_major, 0);
  EXPECT_LE(abi_minor, 20);
}

TEST_P(WasmCommonTest, IntrinsicGlobals) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "globals";
  auto plugin_configuration = "";
  std::string code;
  if (GetParam() == "v8") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
        absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestCpp";
  }
  EXPECT_FALSE(code.empty());
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->setCreateContextForTesting(
      nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto root_context = new TestContext(wasm, plugin);
        EXPECT_CALL(*root_context, log_(spdlog::level::warn, Eq("NaN nan")));
        EXPECT_CALL(*root_context, log_(spdlog::level::warn, Eq("inf inf"))).Times(3);
        return root_context;
      });
  wasm->start(plugin);
}

TEST_P(WasmCommonTest, Stats) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "stats";
  auto plugin_configuration = "";
  std::string code;
  if (GetParam() == "v8") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
        absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestCpp";
  }
  EXPECT_FALSE(code.empty());
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->setCreateContextForTesting(
      nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto root_context = new TestContext(wasm, plugin);
        EXPECT_CALL(*root_context, log_(spdlog::level::trace, Eq("get counter = 1")));
        EXPECT_CALL(*root_context, log_(spdlog::level::debug, Eq("get counter = 2")));
        // recordMetric on a Counter is the same as increment.
        EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("get counter = 5")));
        EXPECT_CALL(*root_context, log_(spdlog::level::warn, Eq("get gauge = 2")));
        // Get is not supported on histograms.
        EXPECT_CALL(*root_context, log_(spdlog::level::err, Eq("get histogram = Unsupported")));
        return root_context;
      });
  wasm->start(plugin);
}

TEST_P(WasmCommonTest, Foreign) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "foreign";
  auto vm_key = "";
  auto plugin_configuration = "";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  std::string code;
  if (GetParam() != "null") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
        absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestCpp";
  }
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->setCreateContextForTesting(
      nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto root_context = new TestContext(wasm, plugin);
        EXPECT_CALL(*root_context, log_(spdlog::level::trace, Eq("compress 41 -> 34")));
        EXPECT_CALL(*root_context, log_(spdlog::level::debug, Eq("uncompress 34 -> 41")));
        return root_context;
      });
  wasm->start(plugin);
}

TEST_P(WasmCommonTest, OnForeign) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "on_foreign";
  auto vm_key = "";
  auto plugin_configuration = "";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  std::string code;
  if (GetParam() != "null") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
        absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestCpp";
  }
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm->initialize(code, false));
  TestContext* test_context = nullptr;
  wasm->setCreateContextForTesting(
      nullptr, [&test_context](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto context = new TestContext(wasm, plugin);
        EXPECT_CALL(*context, log_(spdlog::level::debug, Eq("on_foreign start")));
        EXPECT_CALL(*context, log_(spdlog::level::info, Eq("on_foreign_function 7 13")));
        test_context = context;
        return context;
      });
  wasm->start(plugin);
  test_context->onForeignFunction(7, 13);
}

TEST_P(WasmCommonTest, WASI) {
  if (GetParam() == "null") {
    // This test has no meaning unless it is invoked by actual Wasm code
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "WASI";
  auto vm_key = "";
  auto plugin_configuration = "";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  std::string code;
  if (GetParam() != "null") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
        absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestCpp";
  }
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->setCreateContextForTesting(
      nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
        auto root_context = new TestContext(wasm, plugin);
        EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("WASI write to stdout"))).Times(1);
        EXPECT_CALL(*root_context, log_(spdlog::level::err, Eq("WASI write to stderr"))).Times(1);
        return root_context;
      });
  wasm->start(plugin);
}

TEST_P(WasmCommonTest, VmCache) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  NiceMock<Upstream::MockClusterManager> cluster_manager;
  NiceMock<Runtime::MockRandomGenerator> random;
  NiceMock<Init::MockManager> init_manager;
  NiceMock<Server::MockServerLifecycleNotifier> lifecycle_notifier;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "vm_cache";
  auto plugin_configuration = "init";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);

  VmConfig vm_config;
  vm_config.set_runtime(absl::StrCat("envoy.wasm.runtime.", GetParam()));
  ProtobufWkt::StringValue vm_configuration_string;
  vm_configuration_string.set_value(vm_configuration);
  vm_config.mutable_configuration()->PackFrom(vm_configuration_string);
  std::string code;
  if (GetParam() != "null") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
        absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestCpp";
  }
  EXPECT_FALSE(code.empty());
  vm_config.mutable_code()->mutable_local()->set_inline_bytes(code);
  WasmHandleSharedPtr wasm_handle;
  createWasm(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random, *api,
             lifecycle_notifier, remote_data_provider,
             [&wasm_handle](const WasmHandleSharedPtr& w) { wasm_handle = w; });
  EXPECT_NE(wasm_handle, nullptr);

  WasmHandleSharedPtr wasm_handle2;
  createWasm(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random, *api,
             lifecycle_notifier, remote_data_provider,
             [&wasm_handle2](const WasmHandleSharedPtr& w) { wasm_handle2 = w; });
  EXPECT_NE(wasm_handle2, nullptr);
  EXPECT_EQ(wasm_handle, wasm_handle2);

  auto wasm_handle_local = getOrCreateThreadLocalWasm(
      wasm_handle, plugin,
      [&dispatcher](const WasmHandleBaseSharedPtr& base_wasm) -> WasmHandleBaseSharedPtr {
        auto wasm =
            std::make_shared<Wasm>(std::static_pointer_cast<WasmHandle>(base_wasm), *dispatcher);
        wasm->setCreateContextForTesting(
            nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
              auto root_context = new TestContext(wasm, plugin);
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_vm_start vm_cache")));
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_configuration init")));
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_done logging")));
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_delete logging")));
              return root_context;
            });
        return std::make_shared<WasmHandle>(wasm);
      });
  wasm_handle.reset();
  wasm_handle2.reset();

  auto wasm = wasm_handle_local->wasm().get();
  wasm_handle_local.reset();

  dispatcher->run(Event::Dispatcher::RunType::NonBlock);

  plugin->plugin_configuration_ = "done";
  wasm->configure(wasm->getContext(1), plugin);
  plugin.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  dispatcher->clearDeferredDeleteList();

  proxy_wasm::clearWasmCachesForTesting();
}

TEST_P(WasmCommonTest, RemoteCode) {
  if (GetParam() == "null") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  NiceMock<Upstream::MockClusterManager> cluster_manager;
  NiceMock<Runtime::MockRandomGenerator> random;
  NiceMock<Init::MockManager> init_manager;
  NiceMock<Server::MockServerLifecycleNotifier> lifecycle_notifier;
  Init::ExpectableWatcherImpl init_watcher;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "vm_cache";
  auto plugin_configuration = "done";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);

  std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));

  VmConfig vm_config;
  vm_config.set_runtime(absl::StrCat("envoy.wasm.runtime.", GetParam()));
  ProtobufWkt::StringValue vm_configuration_string;
  vm_configuration_string.set_value(vm_configuration);
  vm_config.mutable_configuration()->PackFrom(vm_configuration_string);
  std::string sha256 = Extensions::Common::Wasm::Sha256(code);
  std::string sha256Hex =
      Hex::encode(reinterpret_cast<const uint8_t*>(&*sha256.begin()), sha256.size());
  vm_config.mutable_code()->mutable_remote()->set_sha256(sha256Hex);
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->set_uri(
      "http://example.com/test.wasm");
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->set_cluster("example_com");
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->mutable_timeout()->set_seconds(5);
  WasmHandleSharedPtr wasm_handle;
  NiceMock<Http::MockAsyncClient> client;
  NiceMock<Http::MockAsyncClientRequest> request(&client);

  EXPECT_CALL(cluster_manager, httpAsyncClientForCluster("example_com"))
      .WillOnce(ReturnRef(cluster_manager.async_client_));
  EXPECT_CALL(cluster_manager.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::RequestMessagePtr&, Http::AsyncClient::Callbacks& callbacks,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            Http::ResponseMessagePtr response(
                new Http::ResponseMessageImpl(Http::ResponseHeaderMapPtr{
                    new Http::TestResponseHeaderMapImpl{{":status", "200"}}}));
            response->body() = std::make_unique<::Envoy::Buffer::OwnedImpl>(code);
            callbacks.onSuccess(request, std::move(response));
            return nullptr;
          }));

  Init::TargetHandlePtr init_target_handle;
  EXPECT_CALL(init_manager, add(_)).WillOnce(Invoke([&](const Init::Target& target) {
    init_target_handle = target.createHandle("test");
  }));
  createWasm(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random, *api,
             lifecycle_notifier, remote_data_provider,
             [&wasm_handle](const WasmHandleSharedPtr& w) { wasm_handle = w; });

  EXPECT_CALL(init_watcher, ready());
  init_target_handle->initialize(init_watcher);

  EXPECT_NE(wasm_handle, nullptr);

  auto wasm_handle_local = getOrCreateThreadLocalWasm(
      wasm_handle, plugin,
      [&dispatcher](const WasmHandleBaseSharedPtr& base_wasm) -> WasmHandleBaseSharedPtr {
        auto wasm =
            std::make_shared<Wasm>(std::static_pointer_cast<WasmHandle>(base_wasm), *dispatcher);
        wasm->setCreateContextForTesting(
            nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
              auto root_context = new TestContext(wasm, plugin);
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_vm_start vm_cache")));
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_done logging")));
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_delete logging")));
              return root_context;
            });
        return std::make_shared<WasmHandle>(wasm);
      });
  wasm_handle.reset();

  auto wasm = wasm_handle_local->wasm().get();
  wasm_handle_local.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  wasm->configure(wasm->getContext(1), plugin);
  plugin.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  dispatcher->clearDeferredDeleteList();
}

TEST_P(WasmCommonTest, RemoteCodeMultipleRetry) {
  if (GetParam() == "null") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  NiceMock<Upstream::MockClusterManager> cluster_manager;
  NiceMock<Runtime::MockRandomGenerator> random;
  NiceMock<Init::MockManager> init_manager;
  NiceMock<Server::MockServerLifecycleNotifier> lifecycle_notifier;
  Init::ExpectableWatcherImpl init_watcher;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher("wasm_test"));
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "vm_cache";
  auto plugin_configuration = "done";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration, false,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);

  std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));

  VmConfig vm_config;
  vm_config.set_runtime(absl::StrCat("envoy.wasm.runtime.", GetParam()));
  ProtobufWkt::StringValue vm_configuration_string;
  vm_configuration_string.set_value(vm_configuration);
  vm_config.mutable_configuration()->PackFrom(vm_configuration_string);
  std::string sha256 = Extensions::Common::Wasm::Sha256(code);
  std::string sha256Hex =
      Hex::encode(reinterpret_cast<const uint8_t*>(&*sha256.begin()), sha256.size());
  int num_retries = 3;
  vm_config.mutable_code()->mutable_remote()->set_sha256(sha256Hex);
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->set_uri(
      "http://example.com/test.wasm");
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->set_cluster("example_com");
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->mutable_timeout()->set_seconds(5);
  vm_config.mutable_code()
      ->mutable_remote()
      ->mutable_retry_policy()
      ->mutable_num_retries()
      ->set_value(num_retries);
  WasmHandleSharedPtr wasm_handle;
  NiceMock<Http::MockAsyncClient> client;
  NiceMock<Http::MockAsyncClientRequest> request(&client);

  EXPECT_CALL(cluster_manager, httpAsyncClientForCluster("example_com"))
      .WillRepeatedly(ReturnRef(cluster_manager.async_client_));
  EXPECT_CALL(cluster_manager.async_client_, send_(_, _, _))
      .WillRepeatedly(Invoke([&, retry = num_retries](
                                 Http::RequestMessagePtr&, Http::AsyncClient::Callbacks& callbacks,
                                 const Http::AsyncClient::RequestOptions&) mutable
                             -> Http::AsyncClient::Request* {
        if (retry-- == 0) {
          Http::ResponseMessagePtr response(new Http::ResponseMessageImpl(
              Http::ResponseHeaderMapPtr{new Http::TestResponseHeaderMapImpl{{":status", "503"}}}));
          callbacks.onSuccess(request, std::move(response));
          return nullptr;
        } else {
          Http::ResponseMessagePtr response(new Http::ResponseMessageImpl(
              Http::ResponseHeaderMapPtr{new Http::TestResponseHeaderMapImpl{{":status", "200"}}}));
          response->body() = std::make_unique<::Envoy::Buffer::OwnedImpl>(code);
          callbacks.onSuccess(request, std::move(response));
          return nullptr;
        }
      }));

  Init::TargetHandlePtr init_target_handle;
  EXPECT_CALL(init_manager, add(_)).WillOnce(Invoke([&](const Init::Target& target) {
    init_target_handle = target.createHandle("test");
  }));
  createWasm(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random, *api,
             lifecycle_notifier, remote_data_provider,
             [&wasm_handle](const WasmHandleSharedPtr& w) { wasm_handle = w; });

  EXPECT_CALL(init_watcher, ready());
  init_target_handle->initialize(init_watcher);

  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  EXPECT_NE(wasm_handle, nullptr);

  auto wasm_handle_local = getOrCreateThreadLocalWasm(
      wasm_handle, plugin,
      [&dispatcher](const WasmHandleBaseSharedPtr& base_wasm) -> WasmHandleBaseSharedPtr {
        auto wasm =
            std::make_shared<Wasm>(std::static_pointer_cast<WasmHandle>(base_wasm), *dispatcher);
        wasm->setCreateContextForTesting(
            nullptr, [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
              auto root_context = new TestContext(wasm, plugin);
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_vm_start vm_cache")));
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_done logging")));
              EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_delete logging")));
              return root_context;
            });
        return std::make_shared<WasmHandle>(wasm);
      });
  wasm_handle.reset();

  auto wasm = wasm_handle_local->wasm().get();
  wasm_handle_local.reset();

  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  wasm->configure(wasm->getContext(1), plugin);
  plugin.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  dispatcher->clearDeferredDeleteList();
}

class WasmCommonContextTest
    : public Common::Wasm::WasmTestBase<testing::TestWithParam<std::string>> {
public:
  WasmCommonContextTest() = default;

  void setup(const std::string& code, std::string root_id = "") {
    setupBase(
        GetParam(), code,
        [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
          return new TestContext(wasm, plugin);
        },
        root_id);
  }
  void setupContext() {
    context_ = std::make_unique<TestContext>(wasm_->wasm().get(), root_context_->id(), plugin_);
    context_->onCreate();
  }

  TestContext& root_context() { return *static_cast<TestContext*>(root_context_); }
  TestContext& context() { return *context_; }

  std::unique_ptr<TestContext> context_;
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmCommonContextTest,
                         testing::Values("v8",
#if defined(ENVOY_WASM_WAVM)
                                         "wavm",
#endif
                                         "null"));

TEST_P(WasmCommonContextTest, OnDnsResolve) {
  std::string code;
  if (GetParam() != "null") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(absl::StrCat(
        "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_context_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestContextCpp";
  }
  EXPECT_FALSE(code.empty());
  setup(code);
  setupContext();

  EXPECT_CALL(context(), log_(spdlog::level::warn, Eq("TestContext::onResolveDns 7")));
  EXPECT_CALL(context(),
              log_(spdlog::level::info, Eq("TestContext::onResolveDns dns 1 192.168.1.101:1001")));
  EXPECT_CALL(context(),
              log_(spdlog::level::info, Eq("TestContext::onResolveDns dns 2 192.168.1.102:1002")));
  EXPECT_CALL(root_context(), log_(spdlog::level::warn, Eq("TestRootContext::onDone 1")));

  uint32_t token = 7;
  std::list<Envoy::Network::DnsResponse> dns_results;
  dns_results.emplace(dns_results.end(),
                      std::make_shared<Network::Address::Ipv4Instance>("192.168.1.101", 1001),
                      std::chrono::seconds(1));
  dns_results.emplace(dns_results.end(),
                      std::make_shared<Network::Address::Ipv4Instance>("192.168.1.102", 1002),
                      std::chrono::seconds(2));
  context_->onResolveDns(token, Envoy::Network::DnsResolver::ResolutionStatus::Success,
                         std::move(dns_results));
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
