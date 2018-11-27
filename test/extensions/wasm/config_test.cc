#include <stdio.h>

#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/registry/registry.h"

#include "server/wasm_config_impl.h"

#include "extensions/wasm/config.h"

#include "test/mocks/event/mocks.h"
#include "test/test_common/environment.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

TEST(WasmFactoryTest, CreateWasmFromWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromPrecompiledWASM) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wasm"));
  config.set_allow_precompiled(true);
  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromWAT) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_filename(
      TestEnvironment::substitute("{{ test_rundir }}/test/extensions/wasm/test_data/logging.wat"));
  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromInlineWAT) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_inline_string(
      "(module\n"
      "  (type $0 (func (param i32 i32 i32)))\n"
      "  (type $1 (func))\n"
      "  (import \"envoy\" \"log\" (func $envoy_log (param i32 i32 i32)))\n"
      "  (import \"env\" \"memory\" (memory $2 17 1000))\n"
      "  (export \"main\" (func $main))\n"
      "  (data $2 (i32.const 1048576) \"Hello, world!\")\n"
      ""
      " (func $main (type $1)\n"
      "   i32.const 1\n"
      "   i32.const 1048576\n"
      "   i32.const 13\n"
      "   call $envoy_log\n"
      "   )\n"
      " )");

  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromInlineWATWithAlias) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_inline_string(
      "(module\n"
      "  (type $0 (func (param i32 i32 i32)))\n"
      "  (type $1 (func))\n"
      "  (import \"env\" \"envoy_log\" (func $envoy_log (param i32 i32 i32)))\n"
      "  (import \"env\" \"memory\" (memory $2 17 1000))\n"
      "  (export \"main\" (func $main))\n"
      "  (data $2 (i32.const 1048576) \"Hello, world!\")\n"
      ""
      " (func $main (type $1)\n"
      "   i32.const 1\n"
      "   i32.const 1048576\n"
      "   i32.const 13\n"
      "   call $envoy_log\n"
      "   )\n"
      " )");

  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, CreateWasmFromInlineWATWithUnderscoreAlias) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_inline_string(
      "(module\n"
      "  (type $0 (func (param i32 i32 i32)))\n"
      "  (type $1 (func))\n"
      "  (import \"env\" \"_envoy_log\" (func $envoy_log (param i32 i32 i32)))\n"
      "  (import \"env\" \"memory\" (memory $2 17 1000))\n"
      "  (export \"main\" (func $main))\n"
      "  (data $2 (i32.const 1048576) \"Hello, world!\")\n"
      ""
      " (func $main (type $1)\n"
      "   i32.const 1\n"
      "   i32.const 1048576\n"
      "   i32.const 13\n"
      "   call $envoy_log\n"
      "   )\n"
      " )");

  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  auto wasm = factory->createWasm(config, context);
  EXPECT_NE(wasm, nullptr);
}

TEST(WasmFactoryTest, MissingImport) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::WasmFactory>::getFactory("envoy.wasm");
  ASSERT_NE(factory, nullptr);
  envoy::config::wasm::v2::WasmConfig config;
  config.set_vm("envoy.wasm.vm.wavm");
  config.mutable_code()->set_inline_string(
      "(module\n"
      "  (type $0 (func (param i32 i32 i32)))\n"
      "  (type $1 (func))\n"
      "  (import \"env\" \"missing\" (func $missing (param i32 i32 i32)))\n"
      "  (import \"env\" \"memory\" (memory $2 17 1000))\n"
      "  (export \"main\" (func $main))\n"
      "  (data $2 (i32.const 1048576) \"Hello, world!\")\n"
      ""
      " (func $main (type $1)\n"
      "   i32.const 1\n"
      "   i32.const 1048576\n"
      "   i32.const 13\n"
      "   call $missing\n"
      "   )\n"
      " )");

  Event::MockDispatcher dispatcher;
  Server::Configuration::WasmFactoryContextImpl context(dispatcher);
  EXPECT_THROW_WITH_MESSAGE(
      factory->createWasm(config, context), Extensions::Common::Wasm::WasmException,
      "Failed to load WASM module due to a missing import: env.missing func (i32, i32, i32)->()");
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
