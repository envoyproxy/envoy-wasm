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

class TestFilter : public Envoy::Extensions::Common::Wasm::Context {
public:
  TestFilter(Wasm* wasm, uint32_t root_context_id)
      : Envoy::Extensions::Common::Wasm::Context(wasm, root_context_id) {}

  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
};

class TestRoot : public Envoy::Extensions::Common::Wasm::Context {
public:
  TestRoot() {}

  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
};

class WasmFilterTest : public testing::TestWithParam<std::string> {
public:
  WasmFilterTest() {}
  ~WasmFilterTest() {}

  void setupConfig(const std::string& code) {
    root_context_ = new TestRoot();
    envoy::config::filter::network::wasm::v2::Wasm proto_config;
    proto_config.set_vm_id("vm_id");
    proto_config.mutable_vm_config()->set_vm(absl::StrCat("envoy.wasm.vm.", GetParam()));
    proto_config.mutable_vm_config()->mutable_code()->set_inline_bytes(code);
    Api::ApiPtr api = Api::createApiForTest(stats_store_);
    scope_ = Stats::ScopeSharedPtr(stats_store_.createScope("wasm."));
    wasm_ = Extensions::Common::Wasm::createWasmForTesting(
        proto_config.vm_id(), proto_config.vm_config(), proto_config.root_id(), cluster_manager_,
        dispatcher_, *api, *scope_, envoy::api::v2::core::TrafficDirection::INBOUND, local_info_,
        &listener_metadata_, nullptr,
        std::unique_ptr<Envoy::Extensions::Common::Wasm::Context>(root_context_));
  }

  void setupFilter() {
    filter_ = std::make_unique<TestFilter>(wasm_.get(), wasm_->getRootContext("")->id());
    filter_->initializeReadFilterCallbacks(read_filter_callbacks_);
  }

  Stats::IsolatedStoreImpl stats_store_;
  Stats::ScopeSharedPtr scope_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  NiceMock<Upstream::MockClusterManager> cluster_manager_;
  std::shared_ptr<Wasm> wasm_;
  std::unique_ptr<TestFilter> filter_;
  NiceMock<Network::MockReadFilterCallbacks> read_filter_callbacks_;
  NiceMock<LocalInfo::MockLocalInfo> local_info_;
  envoy::api::v2::core::Metadata listener_metadata_;
  TestRoot* root_context_ = nullptr;
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmFilterTest,
                         testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                             "v8", "wavm"
#elif defined(ENVOY_WASM_V8)
                             "v8"
#elif defined(ENVOY_WASM_WAVM)
                             "wavm"
#endif
                             ));

// Bad code in initial config.
TEST_P(WasmFilterTest, BadCode) {
  EXPECT_THROW_WITH_MESSAGE(setupConfig("bad code"), Common::Wasm::WasmException,
                            "Failed to initialize WASM code from <inline>");
}

// Test happy path.
TEST_P(WasmFilterTest, HappyPath) {
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
