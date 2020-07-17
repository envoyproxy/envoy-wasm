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
  TestContext(Extensions::Common::Wasm::Wasm* wasm,
              const std::shared_ptr<Extensions::Common::Wasm::Plugin>& plugin)
      : Extensions::Common::Wasm::Context(wasm, plugin) {}
  ~TestContext() override = default;
  proxy_wasm::WasmResult log(uint32_t level, absl::string_view message) override {
    std::cerr << std::string(message) << "\n";
    log_(static_cast<spdlog::level::level_enum>(level), message);
    return proxy_wasm::WasmResult::Ok;
  }
  MOCK_METHOD2(log_, void(spdlog::level::level_enum level, absl::string_view message));
};

class WasmTestBase {
public:
  WasmTestBase()
      : api_(Api::createApiForTest(stats_store_)),
        dispatcher_(api_->allocateDispatcher("wasm_test")),
        scope_(stats_store_.createScope("wasm.")) {}

  void createWasm(absl::string_view runtime) {
    plugin_ = std::make_shared<Extensions::Common::Wasm::Plugin>(
        name_, root_id_, vm_id_, plugin_configuration_, false,
        envoy::config::core::v3::TrafficDirection::UNSPECIFIED, local_info_, nullptr);
    wasm_ = std::make_shared<Extensions::Common::Wasm::Wasm>(
        absl::StrCat("envoy.wasm.runtime.", runtime), vm_id_, vm_configuration_, vm_key_, scope_,
        cluster_manager, *dispatcher_);
    EXPECT_NE(wasm_, nullptr);
    wasm_->setCreateContextForTesting(
        nullptr,
        [](Extensions::Common::Wasm::Wasm* wasm,
           const std::shared_ptr<Extensions::Common::Wasm::Plugin>& plugin)
            -> proxy_wasm::ContextBase* { return new TestContext(wasm, plugin); });
  }

  Stats::IsolatedStoreImpl stats_store_;
  Api::ApiPtr api_;
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher_;
  Stats::ScopeSharedPtr scope_;
  NiceMock<LocalInfo::MockLocalInfo> local_info_;
  std::string name_;
  std::string root_id_;
  std::string vm_id_;
  std::string vm_configuration_;
  std::string vm_key_;
  std::string plugin_configuration_;
  std::shared_ptr<Extensions::Common::Wasm::Plugin> plugin_;
  std::shared_ptr<Extensions::Common::Wasm::Wasm> wasm_;
};

class WasmTest : public WasmTestBase, public testing::TestWithParam<std::string> {
public:
  void createWasm() { WasmTestBase::createWasm(GetParam()); }
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmTest, testing::Values("v8"));

class WasmNullTest : public WasmTestBase, public testing::TestWithParam<std::string> {
public:
  void createWasm() {
    WasmTestBase::createWasm(GetParam());
    const auto code =
        GetParam() != "null"
            ? TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
                  "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/stats_cpp.wasm"))
            : "WasmStatsCpp";
    EXPECT_FALSE(code.empty());
    EXPECT_TRUE(wasm_->initialize(code, false));
  }
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmNullTest,
                         testing::Values("v8",
#if defined(ENVOY_WASM_WAVM)
                                         "wavm",
#endif
                                         "null"));

class WasmTestMatrix : public WasmTestBase,
                       public testing::TestWithParam<std::tuple<std::string, std::string>> {
public:
  void createWasm() { WasmTestBase::createWasm(std::get<0>(GetParam())); }
};

INSTANTIATE_TEST_SUITE_P(RuntimesAndLanguages, WasmTestMatrix,
                         testing::Combine(testing::Values("v8"
#if defined(ENVOY_WASM_WAVM)
                                                          ,
                                                          "wavm"
#endif
                                                          ),
                                          testing::Values("cpp", "rust")));

TEST_P(WasmTestMatrix, Logging) {
  plugin_configuration_ = "configure-test";
  createWasm();

  auto wasm_weak = std::weak_ptr<Extensions::Common::Wasm::Wasm>(wasm_);
  auto wasm_handler = std::make_unique<Extensions::Common::Wasm::WasmHandle>(std::move(wasm_));
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      absl::StrCat("{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/logging_",
                   std::get<1>(GetParam()), ".wasm")));
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm_weak.lock()->initialize(code, false));
  auto context = static_cast<TestContext*>(wasm_weak.lock()->start(plugin_));

  if (std::get<1>(GetParam()) == "cpp") {
    EXPECT_CALL(*context, log_(spdlog::level::info, Eq("printf stdout test")));
    EXPECT_CALL(*context, log_(spdlog::level::err, Eq("printf stderr test")));
  }
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("warn configure-test")));
  EXPECT_CALL(*context, log_(spdlog::level::trace, Eq("test trace logging")));
  EXPECT_CALL(*context, log_(spdlog::level::debug, Eq("test debug logging")));
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("test error logging")));
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("test tick logging")));
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("onDone logging")));
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("onDelete logging")));

  EXPECT_TRUE(wasm_weak.lock()->configure(context, plugin_));
  wasm_handler.reset();
  dispatcher_->run(Event::Dispatcher::RunType::NonBlock);
  // This will SEGV on nullptr if wasm has been deleted.
  context->onTick(0);
  dispatcher_->run(Event::Dispatcher::RunType::NonBlock);
  dispatcher_->clearDeferredDeleteList();
}

TEST_P(WasmTest, BadSignature) {
  createWasm();
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/bad_signature_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  EXPECT_FALSE(wasm_->initialize(code, false));
  EXPECT_TRUE(wasm_->isFailed());
}

TEST_P(WasmTest, Segv) {
  createWasm();
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/segv_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm_->initialize(code, false));
  auto context = static_cast<TestContext*>(wasm_->start(plugin_));
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("before badptr")));
  EXPECT_FALSE(wasm_->configure(context, plugin_));
  wasm_->isFailed();
}

TEST_P(WasmTest, DivByZero) {
  createWasm();
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/segv_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm_->initialize(code, false));
  auto context = static_cast<TestContext*>(wasm_->start(plugin_));
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("before div by zero")));
  context->onLog();
  wasm_->isFailed();
}

TEST_P(WasmTest, EmscriptenVersion) {
  createWasm();
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/segv_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm_->initialize(code, false));
  uint32_t major = 9, minor = 9, abi_major = 9, abi_minor = 9;
  EXPECT_TRUE(wasm_->getEmscriptenVersion(&major, &minor, &abi_major, &abi_minor));
  EXPECT_EQ(major, 0);
  EXPECT_LE(minor, 3);
  // Up to (at least) emsdk 1.39.6.
  EXPECT_EQ(abi_major, 0);
  EXPECT_LE(abi_minor, 20);
}

TEST_P(WasmTest, IntrinsicGlobals) {
  createWasm();
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/emscripten_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm_->initialize(code, false));
  auto context = static_cast<TestContext*>(wasm_->start(plugin_));
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("NaN nan")));
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("inf inf"))).Times(3);
  EXPECT_TRUE(wasm_->configure(context, plugin_));
}

// The asm2wasm.wasm file uses operations which would require the asm2wasm Emscripten module *if*
// em++ is invoked with the trap mode "clamp". See
// https://emscripten.org/docs/compiling/WebAssembly.html This test demonstrates that the asm2wasm
// module is not required with the trap mode is set to "allow". Note: future Wasm standards will
// change this behavior by providing non-trapping instructions, but in the mean time we support the
// default Emscripten behavior.
TEST_P(WasmTest, Asm2Wasm) {
  createWasm();
  const auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/bootstrap/wasm/test_data/asm2wasm_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm_->initialize(code, false));
  auto context = static_cast<TestContext*>(wasm_->start(plugin_));
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("out 0 0 0")));
  EXPECT_TRUE(wasm_->configure(context, plugin_));
}

TEST_P(WasmNullTest, Stats) {
  createWasm();
  auto context = static_cast<TestContext*>(wasm_->start(plugin_));

  EXPECT_CALL(*context, log_(spdlog::level::trace, Eq("get counter = 1")));
  EXPECT_CALL(*context, log_(spdlog::level::debug, Eq("get counter = 2")));
  // recordMetric on a Counter is the same as increment.
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("get counter = 5")));
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("get gauge = 2")));
  // Get is not supported on histograms.
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("get histogram = Unsupported")));

  EXPECT_TRUE(wasm_->configure(context, plugin_));
}

TEST_P(WasmNullTest, StatsHigherLevel) {
  createWasm();
  auto context = static_cast<TestContext*>(wasm_->start(plugin_));

  EXPECT_CALL(*context, log_(spdlog::level::trace, Eq("get counter = 1")));
  EXPECT_CALL(*context, log_(spdlog::level::debug, Eq("get counter = 2")));
  // recordMetric on a Counter is the same as increment.
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("get counter = 5")));
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("get gauge = 2")));
  // Get is not supported on histograms.
  EXPECT_CALL(*context, log_(spdlog::level::err,
                             Eq(std::string("resolved histogram name = "
                                            "histogram_int_tag.7.histogram_string_tag.test_tag."
                                            "histogram_bool_tag.true.test_histogram"))));

  context->onTick(0);
}

TEST_P(WasmNullTest, StatsHighLevel) {
  createWasm();
  auto context = static_cast<TestContext*>(wasm_->start(plugin_));

  EXPECT_CALL(*context, log_(spdlog::level::trace, Eq("get counter = 1")));
  EXPECT_CALL(*context, log_(spdlog::level::debug, Eq("get counter = 2")));
  // recordMetric on a Counter is the same as increment.
  EXPECT_CALL(*context, log_(spdlog::level::info, Eq("get counter = 5")));
  EXPECT_CALL(*context, log_(spdlog::level::warn, Eq("get gauge = 2")));
  // Get is not supported on histograms.
  // EXPECT_CALL(*context, log_(spdlog::level::err, Eq(std::string("resolved histogram name
  // = int_tag.7_string_tag.test_tag.bool_tag.true.test_histogram"))));
  EXPECT_CALL(*context,
              log_(spdlog::level::err,
                   Eq("h_id = int_tag.7.string_tag.test_tag.bool_tag.true.test_histogram")));
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("stack_c = 1")));
  EXPECT_CALL(*context, log_(spdlog::level::err, Eq("stack_g = 2")));
  // Get is not supported on histograms.
  // EXPECT_CALL(*context, log_(spdlog::level::err, Eq("stack_h = 3")));
  context->onLog();
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
