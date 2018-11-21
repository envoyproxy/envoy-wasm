#include "common/buffer/buffer_impl.h"
#include "common/http/message_impl.h"
#include "common/stream_info/stream_info_impl.h"

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

class TestFilter : public Filter {
public:
  using Filter::Filter;

  MOCK_METHOD2(scriptLog, void(spdlog::level::level_enum level, const char* message));
};

class WasmHttpFilterTest : public testing::Test {
public:
  WasmHttpFilterTest() {}

  ~WasmHttpFilterTest() {
    if (filter_) {
      filter_->onDestroy();
    }
  }

  void setup(const std::string& wasm_file) {
    config_.reset(new FilterConfig("envoy.wasm.vm.wavm", wasm_file, "", true, tls_, cluster_manager_));
    setupFilter();
  }

  void setupFilter() { filter_.reset(new TestFilter(config_)); }

  NiceMock<ThreadLocal::MockInstance> tls_;
  Upstream::MockClusterManager cluster_manager_;
  std::shared_ptr<FilterConfig> config_;
  std::unique_ptr<TestFilter> filter_;
  Http::MockStreamDecoderFilterCallbacks decoder_callbacks_;
  Http::MockStreamEncoderFilterCallbacks encoder_callbacks_;
  envoy::api::v2::core::Metadata metadata_;
  NiceMock<Envoy::Ssl::MockConnection> ssl_;
  NiceMock<Envoy::Network::MockConnection> connection_;
  NiceMock<Envoy::StreamInfo::MockStreamInfo> stream_info_;
};

// Bad code in initial config.
TEST_F(WasmHttpFilterTest, BadCode) {
  EXPECT_THROW_WITH_MESSAGE(
      setup(TestEnvironment::substitute(
          "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/bad_code.wasm")),
      Common::Wasm::WasmException, "unable to initialize WASM vm");
}

// Script touching headers only, request that is headers only.
TEST_F(WasmHttpFilterTest, HeadersOnlyRequestHeadersOnly) {
  InSequence s;
  setup(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers.wasm"));
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  // TODO(PiotrSikora): re-enable once fixed.
#if 0
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::debug, StrEq("onCreate 1")));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::info, StrEq("onStart 1")));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::warn, StrEq("onDestroy 1")));

  EXPECT_CALL(*filter_, scriptLog(spdlog::level::trace, StrEq("header path /")));
#endif
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
