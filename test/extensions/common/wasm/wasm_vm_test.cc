#include "envoy/registry/registry.h"

#include "common/stats/isolated_store_impl.h"

#include "extensions/common/wasm/wasm_vm.h"

#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "include/proxy-wasm/null_vm_plugin.h"

using proxy_wasm::Cloneable;    // NOLINT
using proxy_wasm::WasmCallVoid; // NOLINT
using proxy_wasm::WasmCallWord; // NOLINT
using proxy_wasm::Word;         // NOLINT
using testing::HasSubstr;       // NOLINT
using testing::Return;          // NOLINT

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace {

class TestNullVmPlugin : public proxy_wasm::NullVmPlugin {
public:
  TestNullVmPlugin() = default;
  ~TestNullVmPlugin() override = default;

  MOCK_METHOD(void, start, ());
};

TestNullVmPlugin* test_null_vm_plugin_ = nullptr;

proxy_wasm::RegisterNullVmPluginFactory register_test_null_vm_plugin("test_null_vm_plugin", []() {
  auto plugin = std::make_unique<TestNullVmPlugin>();
  test_null_vm_plugin_ = plugin.get();
  return plugin;
});

class BaseVmTest : public testing::Test {
public:
  BaseVmTest() : scope_(Stats::ScopeSharedPtr(stats_store.createScope("wasm."))) {}

protected:
  Stats::IsolatedStoreImpl stats_store;
  Stats::ScopeSharedPtr scope_;
};

TEST_F(BaseVmTest, NoRuntime) { EXPECT_EQ(createWasmVm("", scope_), nullptr); }

TEST_F(BaseVmTest, BadRuntime) {
  EXPECT_EQ(createWasmVm("envoy.wasm.runtime.invalid", scope_), nullptr);
}

TEST_F(BaseVmTest, NullVmStartup) {
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.null", scope_);
  EXPECT_TRUE(wasm_vm != nullptr);
  EXPECT_TRUE(wasm_vm->runtime() == "null");
  EXPECT_TRUE(wasm_vm->cloneable() == Cloneable::InstantiatedModule);
  auto wasm_vm_clone = wasm_vm->clone();
  EXPECT_TRUE(wasm_vm_clone != nullptr);
  EXPECT_TRUE(wasm_vm->getCustomSection("user").empty());
  EXPECT_EQ(getEnvoyWasmIntegration(*wasm_vm).runtime(), "envoy.wasm.runtime.null");
  std::function<void()> f;
  EXPECT_FALSE(
      getEnvoyWasmIntegration(*wasm_vm).getNullVmFunction("bad_function", false, 0, nullptr, &f));
}

TEST_F(BaseVmTest, NullVmMemory) {
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.null", scope_);
  EXPECT_EQ(wasm_vm->getMemorySize(), std::numeric_limits<uint64_t>::max());
  std::string d = "data";
  auto m = wasm_vm->getMemory(reinterpret_cast<uint64_t>(d.data()), d.size()).value();
  EXPECT_EQ(m.data(), d.data());
  EXPECT_EQ(m.size(), d.size());
  EXPECT_FALSE(wasm_vm->getMemory(0 /* nullptr */, 1 /* size */).has_value());

  char c;
  char z = 'z';
  EXPECT_TRUE(wasm_vm->setMemory(reinterpret_cast<uint64_t>(&c), 1, &z));
  EXPECT_EQ(c, z);
  EXPECT_TRUE(wasm_vm->setMemory(0 /* nullptr */, 0 /* size */, nullptr));
  EXPECT_FALSE(wasm_vm->setMemory(0 /* nullptr */, 1 /* size */, nullptr));

  Word w(13);
  EXPECT_TRUE(
      wasm_vm->setWord(reinterpret_cast<uint64_t>(&w), std::numeric_limits<uint64_t>::max()));
  EXPECT_EQ(w.u64_, std::numeric_limits<uint64_t>::max());
  EXPECT_FALSE(wasm_vm->setWord(0 /* nullptr */, 1));

  Word w2(0);
  w.u64_ = 7;
  EXPECT_TRUE(wasm_vm->getWord(reinterpret_cast<uint64_t>(&w), &w2));
  EXPECT_EQ(w2.u64_, 7);
  EXPECT_FALSE(wasm_vm->getWord(0 /* nullptr */, &w2));
}

class MockHostFunctions {
public:
  MOCK_METHOD(void, pong, (uint32_t), (const));
  MOCK_METHOD(uint32_t, random, (), (const));
};

#if defined(ENVOY_WASM_V8)
MockHostFunctions* g_host_functions;

void pong(void*, Word value) { g_host_functions->pong(convertWordToUint32(value)); }

Word random(void*) { return {g_host_functions->random()}; }

// pong() with wrong number of arguments.
void bad_pong1(void*) {}

// pong() with wrong return type.
Word bad_pong2(void*, Word) { return 2; }

// pong() with wrong argument type.
double bad_pong3(void*, double) { return 3; }

class WasmVmTest : public testing::TestWithParam<bool> {
public:
  WasmVmTest() : scope_(Stats::ScopeSharedPtr(stats_store.createScope("wasm."))) {}

  void SetUp() override { g_host_functions = new MockHostFunctions(); }
  void TearDown() override { delete g_host_functions; }

protected:
  Stats::IsolatedStoreImpl stats_store;
  Stats::ScopeSharedPtr scope_;
};

INSTANTIATE_TEST_SUITE_P(AllowPrecompiled, WasmVmTest, testing::Values(false, true));

TEST_P(WasmVmTest, V8BadCode) {
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.v8", scope_);
  ASSERT_TRUE(wasm_vm != nullptr);

  EXPECT_FALSE(wasm_vm->load("bad code", GetParam()));
}

TEST_P(WasmVmTest, V8Code) {
#ifndef NDEBUG
  // Do not execute pre-compilation tests in debug mode because V8 will fail to load because the
  // flags do not match. TODO: restore this test when the rust toolchain is integrated.
  if (GetParam() == 1) {
    return;
  }
#endif
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.v8", scope_);
  ASSERT_TRUE(wasm_vm != nullptr);
  EXPECT_TRUE(wasm_vm->runtime() == "v8");

  auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_rust.wasm"));
  EXPECT_TRUE(wasm_vm->load(code, GetParam()));

  // Sanity checks for the expected test file.
  if (!wasm_vm->getPrecompiledSectionName().empty()) {
    EXPECT_TRUE(!wasm_vm->getCustomSection(wasm_vm->getPrecompiledSectionName()).empty());
  }
  EXPECT_THAT(wasm_vm->getCustomSection("producers"), HasSubstr("rustc"));
  EXPECT_TRUE(wasm_vm->getCustomSection("emscripten_metadata").empty());

  EXPECT_TRUE(wasm_vm->cloneable() == Cloneable::CompiledBytecode);
  EXPECT_TRUE(wasm_vm->clone() != nullptr);
}

TEST_P(WasmVmTest, V8BadHostFunctions) {
#ifndef NDEBUG
  // Do not execute pre-compilation tests in debug mode because V8 will fail to load because the
  // flags do not match. TODO: restore this test when the rust toolchain is integrated.
  if (GetParam() == 1) {
    return;
  }
#endif
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.v8", scope_);
  ASSERT_TRUE(wasm_vm != nullptr);

  auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_rust.wasm"));
  EXPECT_TRUE(wasm_vm->load(code, GetParam()));

  wasm_vm->registerCallback("env", "random", &random, CONVERT_FUNCTION_WORD_TO_UINT32(random));
  EXPECT_FALSE(wasm_vm->link("test"));

  wasm_vm->registerCallback("env", "pong", &bad_pong1, CONVERT_FUNCTION_WORD_TO_UINT32(bad_pong1));
  EXPECT_FALSE(wasm_vm->link("test"));

  wasm_vm->registerCallback("env", "pong", &bad_pong2, CONVERT_FUNCTION_WORD_TO_UINT32(bad_pong2));
  EXPECT_FALSE(wasm_vm->link("test"));

  wasm_vm->registerCallback("env", "pong", &bad_pong3, CONVERT_FUNCTION_WORD_TO_UINT32(bad_pong3));
  EXPECT_FALSE(wasm_vm->link("test"));
}

TEST_P(WasmVmTest, V8BadModuleFunctions) {
#ifndef NDEBUG
  // Do not execute pre-compilation tests in debug mode because V8 will fail to load because the
  // flags do not match. TODO: restore this test when the rust toolchain is integrated.
  if (GetParam() == 1) {
    return;
  }
#endif
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.v8", scope_);
  ASSERT_TRUE(wasm_vm != nullptr);

  auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_rust.wasm"));
  EXPECT_TRUE(wasm_vm->load(code, GetParam()));

  wasm_vm->registerCallback("env", "pong", &pong, CONVERT_FUNCTION_WORD_TO_UINT32(pong));
  wasm_vm->registerCallback("env", "random", &random, CONVERT_FUNCTION_WORD_TO_UINT32(random));
  wasm_vm->link("test");

  WasmCallVoid<1> ping;
  WasmCallWord<3> sum;

  wasm_vm->getFunction("nonexistent", &ping);
  EXPECT_TRUE(ping == nullptr);

  wasm_vm->getFunction("nonexistent", &sum);
  EXPECT_TRUE(sum == nullptr);

  wasm_vm->getFunction("ping", &sum);
  EXPECT_TRUE(wasm_vm->isFailed());

  wasm_vm->getFunction("sum", &ping);
  EXPECT_TRUE(wasm_vm->isFailed());
}

TEST_P(WasmVmTest, V8FunctionCalls) {
#ifndef NDEBUG
  // Do not execute pre-compilation tests in debug mode because V8 will fail to load because the
  // flags do not match. TODO: restore this test when the rust toolchain is integrated.
  if (GetParam() == 1) {
    return;
  }
#endif
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.v8", scope_);
  ASSERT_TRUE(wasm_vm != nullptr);

  auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_rust.wasm"));
  EXPECT_TRUE(wasm_vm->load(code, GetParam()));

  wasm_vm->registerCallback("env", "pong", &pong, CONVERT_FUNCTION_WORD_TO_UINT32(pong));
  wasm_vm->registerCallback("env", "random", &random, CONVERT_FUNCTION_WORD_TO_UINT32(random));
  wasm_vm->link("test");

  WasmCallVoid<1> ping;
  wasm_vm->getFunction("ping", &ping);
  EXPECT_CALL(*g_host_functions, pong(42));
  ping(nullptr /* no context */, 42);

  WasmCallWord<1> lucky;
  wasm_vm->getFunction("lucky", &lucky);
  EXPECT_CALL(*g_host_functions, random()).WillRepeatedly(Return(42));
  EXPECT_EQ(0, lucky(nullptr /* no context */, 1).u64_);
  EXPECT_EQ(1, lucky(nullptr /* no context */, 42).u64_);

  WasmCallWord<3> sum;
  wasm_vm->getFunction("sum", &sum);
  EXPECT_EQ(42, sum(nullptr /* no context */, 13, 14, 15).u64_);

  WasmCallWord<2> div;
  wasm_vm->getFunction("div", &div);
  div(nullptr /* no context */, 42, 0);
  EXPECT_TRUE(wasm_vm->isFailed());

  WasmCallVoid<0> abort;
  wasm_vm->getFunction("abort", &abort);
  abort(nullptr /* no context */);
  EXPECT_TRUE(wasm_vm->isFailed());
}

TEST_P(WasmVmTest, V8Memory) {
#ifndef NDEBUG
  // Do not execute pre-compilation tests in debug mode because V8 will fail to load because the
  // flags do not match. TODO: restore this test when the rust toolchain is integrated.
  if (GetParam() == 1) {
    return;
  }
#endif
  auto wasm_vm = createWasmVm("envoy.wasm.runtime.v8", scope_);
  ASSERT_TRUE(wasm_vm != nullptr);

  auto code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/common/wasm/test_data/test_rust.wasm"));
  EXPECT_TRUE(wasm_vm->load(code, GetParam()));

  wasm_vm->registerCallback("env", "pong", &pong, CONVERT_FUNCTION_WORD_TO_UINT32(pong));
  wasm_vm->registerCallback("env", "random", &random, CONVERT_FUNCTION_WORD_TO_UINT32(random));
  wasm_vm->link("test");

  EXPECT_EQ(wasm_vm->getMemorySize(), 65536 /* stack size requested at the build-time */);

  const uint64_t test_addr = 128;

  std::string set = "test";
  EXPECT_TRUE(wasm_vm->setMemory(test_addr, set.size(), set.data()));
  auto got = wasm_vm->getMemory(test_addr, set.size()).value();
  EXPECT_EQ(sizeof("test") - 1, got.size());
  EXPECT_STREQ("test", got.data());

  EXPECT_FALSE(wasm_vm->setMemory(1024 * 1024 /* out of bound */, 1 /* size */, nullptr));
  EXPECT_FALSE(wasm_vm->getMemory(1024 * 1024 /* out of bound */, 1 /* size */).has_value());

  Word word(0);
  EXPECT_TRUE(wasm_vm->setWord(test_addr, std::numeric_limits<uint32_t>::max()));
  EXPECT_TRUE(wasm_vm->getWord(test_addr, &word));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), word.u64_);

  EXPECT_FALSE(wasm_vm->setWord(1024 * 1024 /* out of bound */, 1));
  EXPECT_FALSE(wasm_vm->getWord(1024 * 1024 /* out of bound */, &word));
}
#endif

} // namespace
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
