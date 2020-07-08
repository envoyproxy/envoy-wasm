#include "envoy/server/lifecycle_notifier.h"

#include "extensions/common/wasm/wasm.h"

#include "test/mocks/upstream/mocks.h"
#include "test/test_common/wasm_base.h"

#include "absl/types/optional.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

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

TEST_P(WasmCommonContextTest, OnStat) {
  std::string code;
  NiceMock<Stats::MockMetricSnapshot> snapshot_;
  if (GetParam() != "null") {
    code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(absl::StrCat(
        "{{ test_rundir }}/test/extensions/stats_sinks/wasm/test_data/test_context_cpp.wasm")));
  } else {
    // The name of the Null VM plugin.
    code = "CommonWasmTestContextCpp";
  }
  EXPECT_FALSE(code.empty());
  setup(code);
  setupContext();

  EXPECT_CALL(context(), log_(spdlog::level::warn, Eq("TestContext::onStat")));
  EXPECT_CALL(context(),
              log_(spdlog::level::info, Eq("TestContext::onStats stat: test_counter.1|c/n")));
  EXPECT_CALL(context(),
              log_(spdlog::level::info, Eq("TestContext::onStats stat: test_gauge.2|g/n")));

  EXPECT_CALL(root_context(), log_(spdlog::level::warn, Eq("TestRootContext::onDone 1")));

  NiceMock<Stats::MockCounter> counter;
  counter.name_ = "test_counter";
  counter.latch_ = 1;
  counter.used_ = true;
  snapshot_.counters_.push_back({1, counter});

  NiceMock<Stats::MockGauge> gauge;
  gauge.name_ = "test_gauge";
  gauge.value_ = 2;
  gauge.used_ = true;
  snapshot_.gauges_.push_back(gauge);

  context_->onStat(snapshot_);
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy