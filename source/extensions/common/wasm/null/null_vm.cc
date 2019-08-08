#include "extensions/common/wasm/null/null_vm.h"

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

std::unique_ptr<WasmVm> NullVm::clone() { return std::make_unique<NullVm>(*this); }

bool NullVm::load(const std::string& name, bool /* allow_precompiled */) {
  auto factory = Registry::FactoryRegistry<NullPluginFactory>::getFactory(name);
  if (!factory) {
    return false;
  }
  plugin_ = factory->create();
  return true;
}

void NullVm::link(absl::string_view /* name */, bool /* needs_emscripten */) {}

void NullVm::makeModule(absl::string_view /* name */) {
  // NullVm does not advertize code as emscripten so this will not get called.
  NOT_REACHED_GCOVR_EXCL_LINE;
}

void NullVm::start(Common::Wasm::Context* context) {
  SaveRestoreContext saved_context(context);
  plugin_->start();
}

uint64_t NullVm::getMemorySize() { return std::numeric_limits<uint64_t>::max(); }

absl::string_view NullVm::getMemory(uint64_t pointer, uint64_t size) {
  return {reinterpret_cast<char*>(pointer), static_cast<size_t>(size)};
}

bool NullVm::getMemoryOffset(void* host_pointer, uint64_t* vm_pointer) {
  *vm_pointer = reinterpret_cast<uint64_t>(host_pointer);
  return true;
}

bool NullVm::setMemory(uint64_t pointer, uint64_t size, const void* data) {
  auto p = reinterpret_cast<char*>(pointer);
  memcpy(p, data, size);
  return true;
}

bool NullVm::setWord(uint64_t pointer, uint64_t data) {
  auto p = reinterpret_cast<char*>(pointer);
  memcpy(p, &data, sizeof(data));
  return true;
}

absl::string_view NullVm::getUserSection(absl::string_view /* name */) {
  // Return nothing: there is no WASM file.
  return {};
}

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
