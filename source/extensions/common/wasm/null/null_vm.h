#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "envoy/registry/registry.h"

#include "common/common/assert.h"

#include "extensions/common/wasm/null/null.h"
#include "extensions/common/wasm/null/null_vm_plugin.h"
#include "extensions/common/wasm/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {

struct NullVm : public WasmVm {
  NullVm() = default;
  NullVm(const NullVm& other) { load(other.plugin_name_, false /* unused */); }
  ~NullVm() override{};

  // WasmVm
  absl::string_view vm() override { return WasmVmNames::get().Null; }
  bool clonable() override { return true; };
  std::unique_ptr<WasmVm> clone() override;
  bool load(const std::string& code, bool allow_precompiled) override;
  void link(absl::string_view debug_name, bool needs_emscripten) override;
  void setMemoryLayout(uint64_t, uint64_t, uint64_t) override {}
  void start(Common::Wasm::Context* context) override;
  uint64_t getMemorySize() override;
  absl::optional<absl::string_view> getMemory(uint64_t pointer, uint64_t size) override;
  bool getMemoryOffset(void* host_pointer, uint64_t* vm_pointer) override;
  bool setMemory(uint64_t pointer, uint64_t size, const void* data) override;
  bool setWord(uint64_t pointer, Word data) override;
  void makeModule(absl::string_view name) override;
  absl::string_view getUserSection(absl::string_view name) override;

#define _FORWARD_GET_FUNCTION(_T)                                                                  \
  void getFunction(absl::string_view functionName, _T* f) override {                               \
    plugin_->getFunction(functionName, f);                                                         \
  }
  _FORWARD_GET_FUNCTION(WasmCall0Void);
  _FORWARD_GET_FUNCTION(WasmCall1Void);
  _FORWARD_GET_FUNCTION(WasmCall2Void);
  _FORWARD_GET_FUNCTION(WasmCall3Void);
  _FORWARD_GET_FUNCTION(WasmCall4Void);
  _FORWARD_GET_FUNCTION(WasmCall5Void);
  _FORWARD_GET_FUNCTION(WasmCall8Void);
  _FORWARD_GET_FUNCTION(WasmCall0Word);
  _FORWARD_GET_FUNCTION(WasmCall1Word);
  _FORWARD_GET_FUNCTION(WasmCall3Word);
#undef _FORWARD_GET_FUNCTION

  // These are noops for NullVm.
#define _REGISTER_CALLBACK(_type)                                                                  \
  void registerCallback(absl::string_view, absl::string_view, _type,                               \
                        typename ConvertFunctionTypeWordToUint32<_type>::type) override{};
  _REGISTER_CALLBACK(WasmCallback0Void);
  _REGISTER_CALLBACK(WasmCallback1Void);
  _REGISTER_CALLBACK(WasmCallback2Void);
  _REGISTER_CALLBACK(WasmCallback3Void);
  _REGISTER_CALLBACK(WasmCallback4Void);
  _REGISTER_CALLBACK(WasmCallback0Word);
  _REGISTER_CALLBACK(WasmCallback1Word);
  _REGISTER_CALLBACK(WasmCallback2Word);
  _REGISTER_CALLBACK(WasmCallback3Word);
  _REGISTER_CALLBACK(WasmCallback4Word);
  _REGISTER_CALLBACK(WasmCallback5Word);
  _REGISTER_CALLBACK(WasmCallback6Word);
  _REGISTER_CALLBACK(WasmCallback7Word);
  _REGISTER_CALLBACK(WasmCallback8Word);
  _REGISTER_CALLBACK(WasmCallback9Word);
  _REGISTER_CALLBACK(WasmCallback_WWl);
  _REGISTER_CALLBACK(WasmCallback_WWm);
#undef _REGISTER_CALLBACK

  // NullVm does not advertize code as emscripten so this will not get called.
  std::unique_ptr<Global<double>> makeGlobal(absl::string_view, absl::string_view,
                                             double) override {
    NOT_REACHED_GCOVR_EXCL_LINE;
  };
  std::unique_ptr<Global<Word>> makeGlobal(absl::string_view, absl::string_view, Word) override {
    NOT_REACHED_GCOVR_EXCL_LINE;
  };

  std::string plugin_name_;
  std::unique_ptr<NullVmPlugin> plugin_;
};

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
