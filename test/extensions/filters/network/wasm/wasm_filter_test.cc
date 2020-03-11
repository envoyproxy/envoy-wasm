#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/network/wasm/wasm_filter.h"

#include "test/mocks/network/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Wasm {

using Envoy::Extensions::Common::Wasm::Context;
using Envoy::Extensions::Common::Wasm::PluginSharedPtr;
using Envoy::Extensions::Common::Wasm::Wasm;

class TestFilter : public Context {
public:
  TestFilter(Wasm* wasm, uint32_t root_context_id, PluginSharedPtr plugin)
      : Context(wasm, root_context_id, plugin) {}

  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
};

class TestRoot : public Context {
public:
  TestRoot() {}

  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
};

class WasmNetworkFilterTest : public testing::TestWithParam<std::string> {
public:
  WasmNetworkFilterTest() {}
  ~WasmNetworkFilterTest() {}

  void setupConfig(const std::string& code) {
    root_context_ = new TestRoot();
    envoy::extensions::filters::network::wasm::v3::Wasm proto_config;
    proto_config.mutable_config()->mutable_vm_config()->set_vm_id("vm_id");
    proto_config.mutable_config()->mutable_vm_config()->set_runtime(
        absl::StrCat("envoy.wasm.runtime.", GetParam()));
    proto_config.mutable_config()
        ->mutable_vm_config()
        ->mutable_code()
        ->mutable_local()
        ->set_inline_bytes(code);
    Api::ApiPtr api = Api::createApiForTest(stats_store_);
    scope_ = Stats::ScopeSharedPtr(stats_store_.createScope("wasm."));
    plugin_ = std::make_shared<Extensions::Common::Wasm::Plugin>(
        "", proto_config.config().root_id(), proto_config.config().vm_config().vm_id(),
        envoy::config::core::v3::TrafficDirection::INBOUND, local_info_, &listener_metadata_);
    Extensions::Common::Wasm::createWasmForTesting(
        proto_config.config().vm_config(), plugin_, scope_, cluster_manager_, init_manager_,
        dispatcher_, random_, *api,
        std::unique_ptr<Envoy::Extensions::Common::Wasm::Context>(root_context_),
        remote_data_provider_, [this](Common::Wasm::WasmHandleSharedPtr wasm) { wasm_ = wasm; });
  }

  void setupFilter() {
    filter_ = std::make_unique<TestFilter>(wasm_->wasm().get(),
                                           wasm_->wasm()->getRootContext("")->id(), plugin_);
    filter_->initializeReadFilterCallbacks(read_filter_callbacks_);
  }

  Stats::IsolatedStoreImpl stats_store_;
  Stats::ScopeSharedPtr scope_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  NiceMock<Upstream::MockClusterManager> cluster_manager_;
  NiceMock<Runtime::MockRandomGenerator> random_;
  NiceMock<Init::MockManager> init_manager_;
  Common::Wasm::WasmHandleSharedPtr wasm_;
  Common::Wasm::PluginSharedPtr plugin_;
  std::unique_ptr<TestFilter> filter_;
  NiceMock<Network::MockReadFilterCallbacks> read_filter_callbacks_;
  NiceMock<LocalInfo::MockLocalInfo> local_info_;
  envoy::config::core::v3::Metadata listener_metadata_;
  TestRoot* root_context_ = nullptr;
  Config::DataSource::RemoteAsyncDataProviderPtr remote_data_provider_;
}; // namespace Wasm

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmNetworkFilterTest,
                         testing::Values("v8"
#if defined(ENVOY_WASM_WAVM)
                                         ,
                                         "wavm"
#endif
                                         ));

// Bad code in initial config.
TEST_P(WasmNetworkFilterTest, BadCode) {
  EXPECT_THROW_WITH_MESSAGE(setupConfig("bad code"), Common::Wasm::WasmException,
                            "Failed to initialize WASM code from <inline>");
}

// Test happy path.
TEST_P(WasmNetworkFilterTest, HappyPath) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/network/wasm/test_data/logging_cpp.wasm")));
  setupFilter();

  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::trace, Eq(absl::string_view("onNewConnection 2"))));
  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onNewConnection());

  Buffer::OwnedImpl fake_downstream_data("Fake");
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::trace,
                         Eq(absl::string_view("onDownstreamData 2 len=4 end_stream=0\nFake"))));
  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onData(fake_downstream_data, false));

  Buffer::OwnedImpl fake_upstream_data("Done");
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::trace,
                         Eq(absl::string_view("onUpstreamData 2 len=4 end_stream=1\nDone"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::trace,
                                   Eq(absl::string_view("onUpstreamConnectionClose 2 0"))));
  EXPECT_EQ(Network::FilterStatus::Continue, filter_->onWrite(fake_upstream_data, true));

  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::trace,
                                   Eq(absl::string_view("onDownstreamConnectionClose 2 1"))));
  read_filter_callbacks_.connection_.close(Network::ConnectionCloseType::FlushWrite);
}

} // namespace Wasm
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
