#include <stdio.h>

#include "common/event/dispatcher_impl.h"
#include "common/stats/isolated_store_impl.h"

#include "extensions/wasm/wasm.h"

#include "test/test_common/environment.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/utility.h"

#include "absl/types/optional.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace Wasm {

class TestContext : public Extensions::Wasm::Context {
public:
  TestContext(Wasm *wasm) : Extensions::Wasm::Context(wasm) {}
  ~TestContext() override {}
  MOCK_METHOD2(scriptLog, void(spdlog::level::level_enum level, absl::string_view message));
  MOCK_METHOD1(setTickPeriodMilliseconds, void(uint32_t tick_period_milliseconds)); 
};

TEST(WasmTest, Logging) {
  Event::SimulatedTimeSystem time_system;
  Event::DispatcherImpl dispatcher(time_system);
  auto wasm = std::make_unique<Wasm>("envoy.wasm.vm.wavm");
  EXPECT_NE(wasm, nullptr);
  const auto code = TestEnvironment::readFileToStringForTest(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  EXPECT_FALSE(code.empty());
  auto context = std::make_unique<TestContext>(wasm.get());
  EXPECT_CALL(*context, scriptLog(spdlog::level::debug, Eq("test debug logging")));
  EXPECT_CALL(*context, scriptLog(spdlog::level::info, Eq("test info logging")));
  EXPECT_CALL(*context, scriptLog(spdlog::level::warn, Eq("warn configure-test")));
  EXPECT_CALL(*context, scriptLog(spdlog::level::err, Eq("test tick logging")));
  EXPECT_TRUE(wasm->initialize(code, "<test>", false));
  // NB: Must be done after initialize has created the context.
  wasm->setContext(context.release());
  wasm->configure("configure-test");
  wasm->start(dispatcher, std::chrono::milliseconds(0));
  wasm->tickHandler();
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
