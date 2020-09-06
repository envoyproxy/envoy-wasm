#include "extensions/access_loggers/wasm/wasm_access_log_impl.h"

#include "test/mocks/stream_info/mocks.h"
#include "test/test_common/wasm_base.h"

using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace Wasm {
namespace {

using Common::Wasm::Context;
using Common::Wasm::PluginSharedPtr;
using Common::Wasm::Wasm;
using Common::Wasm::WasmTestBase;
using proxy_wasm::ContextBase;

class TestAccessLog : public Context {
public:
  TestAccessLog(Wasm* wasm, const PluginSharedPtr& plugin) : Context(wasm, plugin) {}

  // overridden to avoid overshadowing by MOCK_CONTEXT_LOG_
  void log(const Http::RequestHeaderMap* request_headers,
           const Http::ResponseHeaderMap* response_headers,
           const Http::ResponseTrailerMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info) override {
    Context::log(request_headers, response_headers, response_trailers, stream_info);
  }

  MOCK_CONTEXT_LOG_;
};

class WasmAccessLogTest
    : public WasmTestBase<testing::TestWithParam<std::tuple<std::string, std::string>>> {
public:
  void setupPlugin(const absl::optional<std::string>& vm_configuration = absl::nullopt,
                   const absl::optional<std::string>& vm_code = absl::nullopt) {
    auto code = getTestCode(vm_configuration, vm_code);
    EXPECT_TRUE(!code.empty());
    auto runtime = std::get<0>(GetParam());
    setupBase(runtime, code, [](Wasm* wasm, const PluginSharedPtr& plugin) -> ContextBase* {
      return new TestAccessLog(wasm, plugin); // root context
    });
  }

  TestAccessLog& access_log() { return *static_cast<TestAccessLog*>(rootContext()); }

private:
  std::string getTestCode(const absl::optional<std::string>& vm_configuration = absl::nullopt,
                          const absl::optional<std::string>& vm_code = absl::nullopt) {
    if (vm_code) {
      return *vm_code;
    } else if (std::get<0>(GetParam()) == "null") {
      return "AccessLoggerTestCpp";
    } else if (std::get<1>(GetParam()) == "cpp") {
      return TestEnvironment::readFileToStringForTest(TestEnvironment::runfilesPath(
          "test/extensions/access_loggers/wasm/test_data/test_cpp.wasm"));
    } else if (std::get<1>(GetParam()) == "rust") {
      return TestEnvironment::readFileToStringForTest(TestEnvironment::runfilesPath(
          absl::StrCat("test/extensions/access_loggers/wasm/test_data/",
                       vm_configuration.value_or("") + "_rust.wasm")));
    }
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
};

// Notice there are no tests for C++ since C++ SDK and Null VM don't support
// dispatching `proxy_on_log` callback into the root context.
INSTANTIATE_TEST_SUITE_P(RuntimesAndLanguages, WasmAccessLogTest,
                         testing::Values(std::make_tuple("v8", "rust")
#if defined(ENVOY_WASM_WAVM)
                                             ,
                                         std::make_tuple("wavm", "rust")
#endif
                                             ));

// Bad code in initial config.
TEST_P(WasmAccessLogTest, BadCode) {
  setupPlugin(absl::nullopt, "bad code");
  EXPECT_EQ(wasm_, nullptr);
}

// Test happy path.
TEST_P(WasmAccessLogTest, HappyPath) {
  setupPlugin("basic");

  // No headers when called for a TCP connection.
  {
    NiceMock<StreamInfo::MockStreamInfo> stream_info;

    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: 0 request headers"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: 0 response headers"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: 0 response trailers"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: stream info"))));
    access_log().log(nullptr, nullptr, nullptr, stream_info);
  }

  // All headers when called for an HTTP stream.
  {
    NiceMock<StreamInfo::MockStreamInfo> stream_info;
    stream_info.upstream_transport_failure_reason_ = "TLS error";

    Http::TestRequestHeaderMapImpl request_headers{
        {":scheme", "http"},
        {":authority", "example.org"},
        {":path", "/api"},
        {":method", "POST"},
    };
    Http::TestResponseHeaderMapImpl response_headers{{":status", "200"}};
    Http::TestResponseTrailerMapImpl response_trailers{
        {"grpc-status", "1"},
        {"grpc-message", "UNKNOWN"},
    };

    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: 4 request headers"))));
    EXPECT_CALL(access_log(), log_(spdlog::level::trace, Eq(absl::string_view(" :scheme = http"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view(" :authority = example.org"))));
    EXPECT_CALL(access_log(), log_(spdlog::level::trace, Eq(absl::string_view(" :path = /api"))));
    EXPECT_CALL(access_log(), log_(spdlog::level::trace, Eq(absl::string_view(" :method = POST"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: 1 response headers"))));
    EXPECT_CALL(access_log(), log_(spdlog::level::trace, Eq(absl::string_view(" :status = 200"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: 2 response trailers"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view(" grpc-status = 1"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view(" grpc-message = UNKNOWN"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace, Eq(absl::string_view("onLog 1: stream info"))));
    EXPECT_CALL(access_log(),
                log_(spdlog::level::trace,
                     Eq(absl::string_view(" upstream.transport_failure_reason = TLS error"))));
    access_log().log(&request_headers, &response_headers, &response_trailers, stream_info);
  }
}

} // namespace
} // namespace Wasm
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
