#include "common/event/dispatcher_impl.h"
#include "common/stats/isolated_store_impl.h"

#include "extensions/common/wasm/wasm.h"

#include "test/test_common/environment.h"
#include "test/test_common/test_time.h"
#include "test/test_common/utility.h"

#include "absl/types/optional.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::StrEq;

namespace Envoy {
namespace Extensions {
namespace Wasm {

class TestWasmCallbacks : public Server::Wasm::WasmCallbacks {
public:
  ~TestWasmCallbacks() override {}
  MOCK_METHOD2(scriptLog, void(spdlog::level::level_enum level, const std::string& message));
};

TEST(WasmTest, Logging) {
  TestWasmCallbacks callbacks;
  EXPECT_CALL(callbacks, scriptLog(spdlog::level::debug, StrEq("test debug logging")));
  EXPECT_CALL(callbacks, scriptLog(spdlog::level::info, StrEq("test info logging")));
  DangerousDeprecatedTestTime test_time;
  Event::DispatcherImpl dispatcher(test_time.timeSystem());
  Server::WasmPtr wasm(Common::Wasm::createWasmVm("envoy.wasm_vm.wavm"));
  EXPECT_NE(wasm, nullptr);
  wasm->setWasmCallbacks(callbacks);
  auto wasm_fn =
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/envoy_wasm_test.wasm");
  EXPECT_TRUE(wasm->initialize(wasm_fn, true));
  wasm->configure("");
  wasm->start(dispatcher);
  wasm->tick();
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
