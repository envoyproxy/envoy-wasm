#include "envoy/server/lifecycle_notifier.h"

#include "common/common/hex.h"
#include "common/event/dispatcher_impl.h"
#include "common/stats/isolated_store_impl.h"

#include "extensions/common/wasm/wasm.h"

#include "test/mocks/server/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

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
  proxy_wasm::WasmResult log(uint64_t level, absl::string_view message) override {
    std::cerr << std::string(message) << "\n";
    log_(static_cast<spdlog::level::level_enum>(level), message);
    Extensions::Common::Wasm::Context::log(static_cast<spdlog::level::level_enum>(level), message);
    return proxy_wasm::WasmResult::Ok;
  }
  MOCK_METHOD2(log_, void(spdlog::level::level_enum level, absl::string_view message));
};

class WasmCommonTest : public testing::TestWithParam<std::string> {};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmCommonTest,
                         testing::Values("v8",
#if defined(ENVOY_WASM_WAVM)
                                         "wavm",
#endif
                                         "null"));
TEST_P(WasmCommonTest, Logging) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "logging";
  auto plugin_configuration = "configure-test";
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
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  EXPECT_THROW_WITH_MESSAGE(wasm->error("foo"), Extensions::Common::Wasm::WasmException, "foo");
  auto wasm_weak = std::weak_ptr<Extensions::Common::Wasm::Wasm>(wasm);
  auto wasm_handle = std::make_shared<Extensions::Common::Wasm::WasmHandle>(std::move(wasm));
  auto vm_context = std::make_unique<TestContext>(wasm_weak.lock().get());
  auto root_context = std::make_unique<TestContext>(wasm_weak.lock().get(), plugin);
  auto test_root_context = std::make_unique<TestContext>();
  auto context =
      std::make_unique<TestContext>(wasm_weak.lock().get(), test_root_context->id(), plugin);
  EXPECT_THROW_WITH_MESSAGE(context->error("bar"), Extensions::Common::Wasm::WasmException, "bar");

  EXPECT_CALL(*test_root_context, log_(spdlog::level::info, Eq("on_configuration configure-test")));
  EXPECT_CALL(*test_root_context, log_(spdlog::level::trace, Eq("test trace logging")));
  EXPECT_CALL(*test_root_context, log_(spdlog::level::debug, Eq("test debug logging")));
  EXPECT_CALL(*test_root_context, log_(spdlog::level::warn, Eq("test warn logging")));
  EXPECT_CALL(*test_root_context, log_(spdlog::level::err, Eq("test error logging")));
  EXPECT_CALL(*test_root_context, log_(spdlog::level::info, Eq("on_done logging")));
  EXPECT_CALL(*test_root_context, log_(spdlog::level::info, Eq("on_delete logging")));

  EXPECT_TRUE(wasm_weak.lock()->initialize(code, false));
  auto thread_local_wasm = std::make_shared<Wasm>(wasm_handle, *dispatcher);
  thread_local_wasm.reset();
  wasm_weak.lock()->setContext(test_root_context.get());
  auto test_root_context_ptr = test_root_context.get();
  wasm_weak.lock()->startForTesting(std::move(test_root_context), plugin);
  wasm_weak.lock()->configure(test_root_context_ptr, plugin);

  wasm_handle.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  // This will fault on nullptr if wasm has been deleted.
  plugin->plugin_configuration_ = "done";
  wasm_weak.lock()->configure(test_root_context_ptr, plugin);
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
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
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
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  EXPECT_THROW_WITH_MESSAGE(wasm->initialize(code, false), Extensions::Common::Wasm::WasmException,
                            "Bad function signature for: proxy_on_configure");
}

TEST_P(WasmCommonTest, Segv) {
  if (GetParam() != "v8") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
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
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("before badptr")));
  EXPECT_TRUE(wasm->initialize(code, false));

  EXPECT_THROW_WITH_MESSAGE(
      wasm->startForTesting(std::move(context), plugin), Extensions::Common::Wasm::WasmException,
      "Function: proxy_on_vm_start failed: Uncaught RuntimeError: unreachable");
}

TEST_P(WasmCommonTest, DivByZero) {
  if (GetParam() != "v8") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
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
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("before div by zero")));
  EXPECT_TRUE(wasm->initialize(code, false));

  EXPECT_THROW_WITH_MESSAGE(
      wasm->startForTesting(std::move(context), plugin), Extensions::Common::Wasm::WasmException,
      "Function: proxy_on_vm_start failed: Uncaught RuntimeError: divide by zero");
}

TEST_P(WasmCommonTest, EmscriptenVersion) {
  if (GetParam() != "v8") {
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
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
      name, root_id, vm_id, plugin_configuration,
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
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
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
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("NaN nan")));
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("inf inf"))).Times(3);
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->startForTesting(std::move(context), plugin);
}

TEST_P(WasmCommonTest, Stats) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
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
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);
  auto vm_key = proxy_wasm::makeVmKey(vm_id, vm_configuration, code);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", GetParam()), vm_id, vm_configuration, vm_key, scope,
      cluster_manager, *dispatcher);
  EXPECT_NE(wasm, nullptr);
  auto context = std::make_unique<TestContext>(wasm.get());

  EXPECT_CALL(*context, log_(spdlog::level::trace, Eq("get counter = 1")));
  EXPECT_CALL(*context, log_(spdlog::level::debug, Eq("get counter = 2")));
  // recordMetric on a Counter is the same as increment.
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("get counter = 5")));
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("get gauge = 2")));
  // Get is not supported on histograms.
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("get histogram = Unsupported")));

  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->startForTesting(std::move(context), plugin);
}

TEST_P(WasmCommonTest, Foreign) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "foreign";
  auto vm_key = "";
  auto plugin_configuration = "";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration,
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
  auto context = std::make_unique<TestContext>(wasm.get());

  EXPECT_CALL(*context, log_(spdlog::level::trace, Eq("compress 41 -> 34")));
  EXPECT_CALL(*context, log_(spdlog::level::debug, Eq("uncompress 34 -> 41")));

  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->startForTesting(std::move(context), plugin);
}

TEST_P(WasmCommonTest, WASI) {
  if (GetParam() == "null") {
    // This test has no meaning unless it is invoked by actual WASM code
    return;
  }
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "WASI";
  auto vm_key = "";
  auto plugin_configuration = "";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration,
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
  auto context = std::make_unique<TestContext>(wasm.get());

  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("WASI write to stdout"))).Times(1);
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("WASI write to stderr"))).Times(1);

  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->startForTesting(std::move(context), plugin);
}

TEST_P(WasmCommonTest, VmCache) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  NiceMock<Upstream::MockClusterManager> cluster_manager;
  NiceMock<Runtime::MockRandomGenerator> random;
  NiceMock<Init::MockManager> init_manager;
  NiceMock<Server::MockServerLifecycleNotifier> lifecycle_notifier;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "vm_cache";
  auto plugin_configuration = "done";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);

  VmConfig vm_config;
  vm_config.set_runtime(absl::StrCat("envoy.wasm.runtime.", GetParam()));
  vm_config.set_configuration(vm_configuration);
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
  auto root_context = new Extensions::Common::Wasm::TestContext();
  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_vm_start vm_cache")));
  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_done logging")));
  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_delete logging")));
  createWasmForTesting(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random,
                       *api, lifecycle_notifier, remote_data_provider,
                       std::unique_ptr<Context>(root_context),
                       [&wasm_handle](const WasmHandleSharedPtr& w) { wasm_handle = w; });

  EXPECT_NE(wasm_handle, nullptr);

  WasmHandleSharedPtr wasm_handle2;
  auto root_context2 = new Extensions::Common::Wasm::Context();
  createWasmForTesting(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random,
                       *api, lifecycle_notifier, remote_data_provider,
                       std::unique_ptr<Context>(root_context2),
                       [&wasm_handle2](const WasmHandleSharedPtr& w) { wasm_handle2 = w; });
  EXPECT_NE(wasm_handle2, nullptr);
  EXPECT_EQ(wasm_handle, wasm_handle2);

  auto wasm = wasm_handle->wasm().get();
  wasm_handle.reset();
  wasm_handle2.reset();

  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  wasm->configure(root_context, plugin);
  plugin.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  dispatcher->clearDeferredDeleteList();
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
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "vm_cache";
  auto plugin_configuration = "done";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);

  std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));

  VmConfig vm_config;
  vm_config.set_runtime(absl::StrCat("envoy.wasm.runtime.", GetParam()));
  vm_config.set_configuration(vm_configuration);
  std::string sha256 = Extensions::Common::Wasm::Sha256(code);
  std::string sha256Hex =
      Hex::encode(reinterpret_cast<const uint8_t*>(&*sha256.begin()), sha256.size());
  vm_config.mutable_code()->mutable_remote()->set_sha256(sha256Hex);
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->set_uri(
      "http://example.com/test.wasm");
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->set_cluster("example_com");
  vm_config.mutable_code()->mutable_remote()->mutable_http_uri()->mutable_timeout()->set_seconds(5);
  WasmHandleSharedPtr wasm_handle;
  auto root_context = new Extensions::Common::Wasm::TestContext();

  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_vm_start vm_cache")));
  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_done logging")));
  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_delete logging")));

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
            callbacks.onSuccess(std::move(response));
            return nullptr;
          }));

  Init::TargetHandlePtr init_target_handle;
  EXPECT_CALL(init_manager, add(_)).WillOnce(Invoke([&](const Init::Target& target) {
    init_target_handle = target.createHandle("test");
  }));
  createWasmForTesting(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random,
                       *api, lifecycle_notifier, remote_data_provider,
                       std::unique_ptr<Context>(root_context),
                       [&wasm_handle](const WasmHandleSharedPtr& w) { wasm_handle = w; });

  EXPECT_CALL(init_watcher, ready());
  init_target_handle->initialize(init_watcher);

  EXPECT_NE(wasm_handle, nullptr);

  auto wasm = wasm_handle->wasm().get();
  wasm_handle.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  wasm->configure(root_context, plugin);
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
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider;
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "vm_cache";
  auto plugin_configuration = "done";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, plugin_configuration,
      envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info, nullptr);

  std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      absl::StrCat("{{ test_rundir }}/test/extensions/common/wasm/test_data/test_cpp.wasm")));

  VmConfig vm_config;
  vm_config.set_runtime(absl::StrCat("envoy.wasm.runtime.", GetParam()));
  vm_config.set_configuration(vm_configuration);
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
  auto root_context = new Extensions::Common::Wasm::TestContext();

  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_vm_start vm_cache")));
  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_done logging")));
  EXPECT_CALL(*root_context, log_(spdlog::level::info, Eq("on_delete logging")));

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
          callbacks.onSuccess(std::move(response));
          return nullptr;
        } else {
          Http::ResponseMessagePtr response(new Http::ResponseMessageImpl(
              Http::ResponseHeaderMapPtr{new Http::TestResponseHeaderMapImpl{{":status", "200"}}}));
          response->body() = std::make_unique<::Envoy::Buffer::OwnedImpl>(code);
          callbacks.onSuccess(std::move(response));
          return nullptr;
        }
      }));

  Init::TargetHandlePtr init_target_handle;
  EXPECT_CALL(init_manager, add(_)).WillOnce(Invoke([&](const Init::Target& target) {
    init_target_handle = target.createHandle("test");
  }));
  createWasmForTesting(vm_config, plugin, scope, cluster_manager, init_manager, *dispatcher, random,
                       *api, lifecycle_notifier, remote_data_provider,
                       std::unique_ptr<Context>(root_context),
                       [&wasm_handle](const WasmHandleSharedPtr& w) { wasm_handle = w; });

  EXPECT_CALL(init_watcher, ready());
  init_target_handle->initialize(init_watcher);

  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  EXPECT_NE(wasm_handle, nullptr);

  auto wasm = wasm_handle->wasm().get();
  wasm_handle.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  wasm->configure(root_context, plugin);
  plugin.reset();
  dispatcher->run(Event::Dispatcher::RunType::NonBlock);
  dispatcher->clearDeferredDeleteList();
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
