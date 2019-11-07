/**
 * Simple WASM speed test.
 *
 * Run with:
 * TEST_SRCDIR=`pwd` TEST_WORKSPACE=bazel-$(basename `pwd`) bazel run --define wasm=enabled
 * --config=libc++ -c opt //test/extensions/wasm:wasm_speed_test
 * Note: "--linkopt -fuse-ld=ldd" may be required as well depending on the build environment.
 */
#include <stdio.h>

#include "common/event/dispatcher_impl.h"
#include "common/stats/isolated_store_impl.h"

#include "extensions/common/wasm/wasm.h"

#include "test/mocks/server/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "absl/types/optional.h"
#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;
using testing::Eq;

namespace Envoy {
namespace Extensions {
namespace Wasm {

class TestRoot : public Envoy::Extensions::Common::Wasm::Context {
public:
  TestRoot() {}

  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
};

static void BM_WasmSimpleCallSpeedTest(benchmark::State& state, std::string vm) {
  Envoy::Logger::Registry::getLog(Logger::Id::wasm).set_level(spdlog::level::off);
  Stats::IsolatedStoreImpl stats_store;
  Api::ApiPtr api = Api::createApiForTest(stats_store);
  Upstream::MockClusterManager cluster_manager;
  Event::DispatcherPtr dispatcher(api->allocateDispatcher());
  auto scope = Stats::ScopeSharedPtr(stats_store.createScope("wasm."));
  NiceMock<LocalInfo::MockLocalInfo> local_info;
  auto root_context = new TestRoot();
  auto name = "";
  auto root_id = "";
  auto vm_id = "";
  auto vm_configuration = "";
  auto plugin = std::make_shared<Extensions::Common::Wasm::Plugin>(
      name, root_id, vm_id, envoy::api::v2::core::TrafficDirection::UNSPECIFIED, local_info,
      nullptr);
  auto wasm = std::make_unique<Extensions::Common::Wasm::Wasm>(
      absl::StrCat("envoy.wasm.runtime.", vm), vm_id, vm_configuration, scope, cluster_manager,
      *dispatcher);
  std::string code;
  if (vm == "null") {
    code = "null_vm_plugin";
  } else {
    code = TestEnvironment::readFileToStringForTest(
        TestEnvironment::runfilesPath("test/extensions/wasm/test_data/speed_cpp.wasm"));
  }
  EXPECT_FALSE(code.empty());
  EXPECT_TRUE(wasm->initialize(code, false));
  wasm->setContext(root_context);
  wasm->startForTesting(std::unique_ptr<Common::Wasm::Context>(root_context), plugin);
  EXPECT_NE(wasm, nullptr);
  for (auto _ : state) {
    wasm->tickHandler(root_context->id());
  }
}

#if defined(ENVOY_WASM_V8)
BENCHMARK_CAPTURE(BM_WasmSimpleCallSpeedTest, V8SpeedTest, std::string("v8"));
#endif
#if defined(ENVOY_WASM_WAVM)
BENCHMARK_CAPTURE(BM_WasmSimpleCallSpeedTest, WavmSpeedTest, std::string("wavm"));
#endif
BENCHMARK_CAPTURE(BM_WasmSimpleCallSpeedTest, NullSpeedTest, std::string("null"));

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy

int main(int argc, char** argv) {
  ::benchmark::Initialize(&argc, argv);
  Envoy::TestEnvironment::initializeOptions(argc, argv);
  // Create a Runfiles object for runfiles lookup.
  // https://github.com/bazelbuild/bazel/blob/master/tools/cpp/runfiles/runfiles_src.h#L32
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(argv[0], &error));
  RELEASE_ASSERT(Envoy::TestEnvironment::getOptionalEnvVar("NORUNFILES").has_value() ||
                     runfiles != nullptr,
                 error);
  Envoy::TestEnvironment::setRunfiles(runfiles.get());
  Envoy::TestEnvironment::setEnvVar("ENVOY_IP_TEST_VERSIONS", "all", 0);
  Envoy::Event::Libevent::Global::initialize();
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }
  ::benchmark::RunSpecifiedBenchmarks();
  return 0;
}
