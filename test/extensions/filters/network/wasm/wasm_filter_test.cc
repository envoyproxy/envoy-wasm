#include "envoy/server/lifecycle_notifier.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/network/wasm/wasm_filter.h"

#include "test/mocks/network/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/test_common/wasm_base.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Wasm {

using Envoy::Extensions::Common::Wasm::Context;
using Envoy::Extensions::Common::Wasm::Plugin;
using Envoy::Extensions::Common::Wasm::PluginSharedPtr;
using Envoy::Extensions::Common::Wasm::Wasm;
using proxy_wasm::ContextBase;

class TestFilter : public Context {
public:
  TestFilter(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin)
      : Context(wasm, root_context_id, plugin) {}
  MOCK_CONTEXT_LOG_;
};

class TestRoot : public Context {
public:
  TestRoot(Wasm* wasm, const std::shared_ptr<Plugin>& plugin) : Context(wasm, plugin) {}
  MOCK_CONTEXT_LOG_;
};

class WasmNetworkFilterTest
    : public Common::Wasm::WasmNetworkFilterTestBase<testing::TestWithParam<std::string>> {
public:
  WasmNetworkFilterTest() {}
  ~WasmNetworkFilterTest() {}

  void setupConfig(const std::string& code) {
    setupBase(GetParam(), code,
              [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
                return new TestRoot(wasm, plugin);
              });
  }

  void setupFilter() { setupFilterBase<TestFilter>(""); }

  TestFilter& filter() { return *static_cast<TestFilter*>(context_.get()); }
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmNetworkFilterTest, testing::Values("v8", "null"));

// Bad code in initial config.
TEST_P(WasmNetworkFilterTest, BadCode) {
  setupConfig("bad code");
  EXPECT_EQ(wasm_, nullptr);
  setupFilter();
  filter().isFailed();
  EXPECT_CALL(read_filter_callbacks_.connection_,
              close(Envoy::Network::ConnectionCloseType::FlushWrite));
  EXPECT_EQ(Network::FilterStatus::StopIteration, filter().onNewConnection());
}

// Test happy path.
TEST_P(WasmNetworkFilterTest, HappyPath) {
  const std::string code =
      GetParam() != "null"
          ? TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
                "{{ test_rundir }}/test/extensions/filters/network/wasm/test_data/test_cpp.wasm"))
          : "NetworkTestCpp";
  setupConfig(code);
  setupFilter();

  EXPECT_CALL(filter(), log_(spdlog::level::trace, Eq(absl::string_view("onNewConnection 2"))));
  EXPECT_EQ(Network::FilterStatus::Continue, filter().onNewConnection());

  Buffer::OwnedImpl fake_downstream_data("Fake");
  EXPECT_CALL(filter(), log_(spdlog::level::trace,
                             Eq(absl::string_view("onDownstreamData 2 len=4 end_stream=0\nFake"))));
  EXPECT_EQ(Network::FilterStatus::Continue, filter().onData(fake_downstream_data, false));
  EXPECT_EQ(fake_downstream_data.toString(), "write");

  Buffer::OwnedImpl fake_upstream_data("Done");
  EXPECT_CALL(filter(), log_(spdlog::level::trace,
                             Eq(absl::string_view("onUpstreamData 2 len=4 end_stream=1\nDone"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::trace, Eq(absl::string_view("onUpstreamConnectionClose 2 0"))));
  EXPECT_EQ(Network::FilterStatus::Continue, filter().onWrite(fake_upstream_data, true));

  EXPECT_CALL(filter(),
              log_(spdlog::level::trace, Eq(absl::string_view("onDownstreamConnectionClose 2 1"))));
  read_filter_callbacks_.connection_.close(Network::ConnectionCloseType::FlushWrite);
}

} // namespace Wasm
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
