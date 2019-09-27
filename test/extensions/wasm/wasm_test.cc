#include <stdio.h>

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

using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace Wasm {

class TestContext : public Extensions::Common::Wasm::Context {
public:
  TestContext(Extensions::Common::Wasm::Wasm* wasm) : Extensions::Common::Wasm::Context(wasm) {}
  ~TestContext() override {}
  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    std::cerr << std::string(message) << "\n";
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
  MOCK_METHOD1(setTickPeriodMilliseconds, void(uint32_t tick_period_milliseconds));
};

class WasmTest : public testing::TestWithParam<std::string> {};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmTest,
                         testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                             "v8", "wavm"
#elif defined(ENVOY_WASM_V8)
                             "v8"
#elif defined(ENVOY_WASM_WAVM)
                             "wavm"
#endif
                             ));

class WasmTestMatrix : public testing::TestWithParam<std::tuple<std::string, std::string>> {};

INSTANTIATE_TEST_SUITE_P(RuntimesAndLanguages, WasmTestMatrix,
                         testing::Combine(testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                                              "v8", "wavm"
#elif defined(ENVOY_WASM_V8)
                                              "v8"
#elif defined(ENVOY_WASM_WAVM)
                                              "wavm"
#endif
                                              ),
                                          testing::Values("cpp", "rust")));

TEST_P(WasmTestMatrix, Logging) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", std::get<0>(GetParam())), "", "", cluster_manager, *dispatcher,
      *scope, envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      absl::StrCat("{{ test_rundir }}/test/extensions/wasm/test_data/logging_",
                   std::get<1>(GetParam()), ".wasm")));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());

  EXPECT_CALL(*context, scriptLog_(spdlog::level::warn, Eq("warn configure-test")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::trace, Eq("test trace logging")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::debug, Eq("test debug logging")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("test error logging")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::info, Eq("test tick logging")));

  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  wasm->setContext(context.get());
  auto root_context = context.get();
  wasm->startForTesting(std::move(context));
  wasm->configure(root_context, "configure-test");
  wasm->tickHandler(root_context->id());
}

TEST_P(WasmTest, BadSignature) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/bad_signature_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  EXPECT_THROW_WITH_MESSAGE(wasm->initialize(code, "<test>", false),
                            Extensions::Common::Wasm::WasmVmException,
                            "Bad function signature for: _proxy_onConfigure");
}

// TODO(PiotrSikora): catch llvm_trap in v8.
#ifdef ENVOY_WASM_WAVM
TEST(WasmTestWavmOnly, Segv) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      "envoy.wasm.vm.wavm", "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/segv_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("before badptr")));
  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  EXPECT_THROW_WITH_MESSAGE(wasm->startForTesting(std::move(context)),
                            Extensions::Common::Wasm::WasmException, "emscripten llvm_trap");
}
#endif

TEST_P(WasmTest, DivByZero) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/segv_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("before div by zero")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("divide by zero: 0")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("after div by zero")));
  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  wasm->setContext(context.get());
  context->onLog();
}

TEST_P(WasmTest, EmscriptenVersion) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/segv_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  uint32_t major = 9, minor = 9, abi_major = 9, abi_minor = 9;
  EXPECT_TRUE(wasm->getEmscriptenVersion(&major, &minor, &abi_major, &abi_minor));
  EXPECT_EQ(major, 0);
  EXPECT_LE(minor, 3);
  // Up to (at least) emsdk 1.38.42.
  EXPECT_EQ(abi_major, 0);
  EXPECT_LE(abi_minor, 7);
}

TEST_P(WasmTest, IntrinsicGlobals) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/emscripten_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, scriptLog_(spdlog::level::info, Eq("NaN nan")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::warn, Eq("inf inf"))).Times(3);
  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  wasm->startForTesting(std::move(context));
}

// The asm2wasm.wasm file uses operations which would require the asm2wasm Emscripten module *if*
// em++ is invoked with the trap mode "clamp". See
// https://emscripten.org/docs/compiling/WebAssembly.html This test demonstrates that the asm2wasm
// module is not required with the trap mode is set to "allow". Note: future WASM standards will
// change this behavior by providing non-trapping instructions, but in the mean time we support the
// default Emscripten behavior.
TEST_P(WasmTest, Asm2Wasm) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_shared<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/asm2wasm_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, scriptLog_(spdlog::level::info, Eq("out 0 0 0")));
  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  wasm->startForTesting(std::move(context));
}

TEST_P(WasmTest, Stats) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/stats_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());

  EXPECT_CALL(*context, scriptLog_(spdlog::level::trace, Eq("get counter = 1")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::debug, Eq("get counter = 2")));
  // recordMetric on a Counter is the same as increment.
  EXPECT_CALL(*context, scriptLog_(spdlog::level::info, Eq("get counter = 5")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::warn, Eq("get gauge = 2")));
  // Get is not supported on histograms.
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("get histogram = Unsupported")));

  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  wasm->startForTesting(std::move(context));
}

TEST_P(WasmTest, StatsHigherLevel) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/stats_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());

  EXPECT_CALL(*context, scriptLog_(spdlog::level::trace, Eq("get counter = 1")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::debug, Eq("get counter = 2")));
  // recordMetric on a Counter is the same as increment.
  EXPECT_CALL(*context, scriptLog_(spdlog::level::info, Eq("get counter = 5")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::warn, Eq("get gauge = 2")));
  // Get is not supported on histograms.
  EXPECT_CALL(*context,
              scriptLog_(spdlog::level::err,
                         Eq(std::string("resolved histogram name = "
                                        "histogram_int_tag.7.histogram_string_tag.test_tag."
                                        "histogram_bool_tag.true.test_histogram"))));

  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  wasm->setContext(context.get());
  wasm->tickHandler(context->id());
}

TEST_P(WasmTest, StatsHighLevel) {
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.vm.", GetParam()), "", "", cluster_manager, *dispatcher, *scope,
      envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info, nullptr, scope);
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/wasm/test_data/stats_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());

  EXPECT_CALL(*context, scriptLog_(spdlog::level::trace, Eq("get counter = 1")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::debug, Eq("get counter = 2")));
  // recordMetric on a Counter is the same as increment.
  EXPECT_CALL(*context, scriptLog_(spdlog::level::info, Eq("get counter = 5")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::warn, Eq("get gauge = 2")));
  // Get is not supported on histograms.
  // EXPECT_CALL(*context, scriptLog(spdlog::level::err, Eq(std::string("resolved histogram name
  // = int_tag.7_string_tag.test_tag.bool_tag.true.test_histogram"))));
  EXPECT_CALL(*context,
              scriptLog_(spdlog::level::err,
                         Eq("h_id = int_tag.7.string_tag.test_tag.bool_tag.true.test_histogram")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("stack_c = 1")));
  EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("stack_g = 2")));
  // Get is not supported on histograms.
  // EXPECT_CALL(*context, scriptLog_(spdlog::level::err, Eq("stack_h = 3")));
  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  wasm->setContext(context.get());
  context->onLog();
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
