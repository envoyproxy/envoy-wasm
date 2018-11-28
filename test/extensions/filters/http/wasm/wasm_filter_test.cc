#include <stdio.h>
#include "common/buffer/buffer_impl.h"
#include "common/http/message_impl.h"
#include "common/stream_info/stream_info_impl.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/wasm/wasm_filter.h"

#include "test/mocks/http/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/ssl/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"

using testing::_;
using testing::AtLeast;
using testing::Eq;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using testing::ReturnPointee;
using testing::ReturnRef;
using testing::StrEq;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

class TestContext : public  Context {
public:
  TestContext(Wasm *wasm) : Context(wasm, nullptr) {}
  ~TestContext() override {}
  MOCK_METHOD2(scriptLog, void(spdlog::level::level_enum level, absl::string_view message));
  MOCK_METHOD2(getHeader, absl::string_view(HeaderType type, absl::string_view key_view));
};

class WasmHttpFilterTest : public testing::Test {
public:
  WasmHttpFilterTest() {}

  ~WasmHttpFilterTest() {
    if (filter_) {
      filter_->onDestroy();
    }
  }

  void setupConfig(const std::string& code) {
    config_.reset(
        new FilterConfig("envoy.wasm.vm.wavm", code, "<test>", true, "", tls_, cluster_manager_));
    setupFilter();
  }

  void setupFilter() { filter_.reset(new Filter(config_)); }

  NiceMock<ThreadLocal::MockInstance> tls_;
  Upstream::MockClusterManager cluster_manager_;
  std::shared_ptr<FilterConfig> config_;
  std::unique_ptr<Filter> filter_;
  Http::MockStreamDecoderFilterCallbacks decoder_callbacks_;
  Http::MockStreamEncoderFilterCallbacks encoder_callbacks_;
  envoy::api::v2::core::Metadata metadata_;
  NiceMock<Envoy::Ssl::MockConnection> ssl_;
  NiceMock<Envoy::Network::MockConnection> connection_;
  NiceMock<Envoy::StreamInfo::MockStreamInfo> stream_info_;
};

// Bad code in initial config.
TEST_F(WasmHttpFilterTest, BadCode) {
  EXPECT_THROW_WITH_MESSAGE(setupConfig("bad code"), Common::Wasm::WasmException,
                            "unable to initialize WASM vm");
}

// Script touching headers only, request that is headers only.
TEST_F(WasmHttpFilterTest, HeadersOnlyRequestHeadersOnly) {
  InSequence s;
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers.wasm")));
  auto context = std::make_unique<TestContext>(config_->wasm());
  EXPECT_CALL(*context, scriptLog(spdlog::level::trace, Eq(absl::string_view("main"))));
  config_->wasm()->setGeneralContext(std::move(context));
  config_->wasm()->start();
  setupFilter();
#if 0
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_CALL(*context, scriptLog(spdlog::level::info, StrEq("onStart 0")));
  EXPECT_CALL(*context, scriptLog(spdlog::level::debug, StrEq("onCreate 0")));
  EXPECT_CALL(*context, scriptLog(spdlog::level::warn, StrEq("onDestroy 0")));
  EXPECT_CALL(*context, getHeader(HeaderType::Header, StrEq("path"))).
    WillOnce(Return("/"));
  EXPECT_CALL(*context, scriptLog(spdlog::level::trace, StrEq("header path /")));
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
#endif
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
