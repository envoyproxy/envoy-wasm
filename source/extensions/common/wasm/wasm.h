#pragma once

#include <memory>
#include <string>
#include <vector>

#include "envoy/common/exception.h"
#include "envoy/server/wasm.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/assert.h"
#include "common/common/c_smart_ptr.h"
#include "common/common/logger.h"
#include "envoy/common/exception.h"

#include "extensions/common/wasm/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

class WasmVm;
// A context which will be the target of callbacks for a particular session
// e.g. a handler of a stream.
class Context : public Logger::Loggable<Logger::Id::wasm> {
  public:
    Context(WasmVm *vm) : wasm_vm(vm) {}
    virtual ~Context() {}

    // Callbacks.
    virtual void scriptLog(spdlog::level::level_enum level, absl::string_view message);

    // Handlers.
    static void wasmLogHandler(void* context, uint32_t level, uint32_t address, uint32_t size);

    uint32_t id = 0;
    WasmVm* const wasm_vm;
};

extern thread_local Envoy::Extensions::Common::Wasm::Context* current_context_;
#define WASM_CONTEXT(_c, _C) (static_cast<_C*>((void)_c,Envoy::Extensions::Common::Wasm::current_context_))

class WasmVm : public Logger::Loggable<Logger::Id::wasm> { 
  public:
    virtual ~WasmVm() {}
    virtual absl::string_view vm() PURE;

    // Load the WASM code from a file.  Return true on success.
    virtual bool initialize(const std::string& code, absl::string_view name,
                            bool allow_precompiled) PURE;

    // Call the 'start' function or main() if there is no start funcition.
    virtual void start(Context*) PURE;

    // Allocate a block of memory in the VM and return the pointer to use as a call arguments.
    virtual void* allocMemory(uint32_t size, uint32_t *pointer) PURE;
    // Convert a block of memory in the VM to a string_view.
    virtual absl::string_view getMemory(uint32_t pointer, uint32_t size) PURE;
    // Set a block of memory in the VM, returns true on success, false if the pointer/size is invalid.
    virtual bool setMemory(uint32_t pointer, uint32_t size, void *data) PURE;

    // Convenience functions.

    // Allocate a null-terminated string in the VM and return the pointer to use as a call arguments.
    uint32_t copyString(absl::string_view s) {
      uint32_t pointer;
      uint8_t *m = static_cast<uint8_t*>(allocMemory((s.size() + 1), &pointer));
      if (s.size() > 0) memcpy(m, s.data(), s.size());
      m[s.size()] = 0;
      return pointer;
    }

    // Copy the data in 's' into the VM along with the pointer-size pair.  Returns true on success.
    bool copyToPointerSize(absl::string_view s, int32_t ptr_ptr, int32_t size_ptr) {
      uint32_t pointer = 0;
      uint32_t size = s.size();
      void *p = nullptr;
      if (size > 0) {
        p = allocMemory(size, &pointer);
        if (!p) return false;
        memcpy(p, s.data(), size);
      }
      if (!setMemory(ptr_ptr, sizeof(int32_t), &pointer)) return false;
      if (!setMemory(size_ptr, sizeof(int32_t), &size)) return false;
      return true;
    }
};

// Create a new WASM VM of the give type (e.g. "envoy.wasm.vm.wavm").
std::unique_ptr<WasmVm> createWasmVm(absl::string_view vm);

class WasmException : public EnvoyException {
  public:
    using EnvoyException::EnvoyException;
};

class WasmVmException : public EnvoyException {
  public:
    using EnvoyException::EnvoyException;
};

// Template implementations.

// Forward declarations for VM implemenations.
template<typename R, typename ...Args> void registerCallbackWavm(WasmVm *vm, absl::string_view functionName, R (*)(Args...));
template<typename R, typename ...Args> void getFunctionWavm(WasmVm *vm, absl::string_view functionName, std::function<R(Context*, Args...)>*);

template<typename R, typename ...Args>
void registerCallback(WasmVm *vm, absl::string_view functionName, R (*f)(Args...)) {
  if (vm->vm() == WasmVmNames::get().Wavm) {
    registerCallbackWavm(vm, functionName, f);
  } else {
    throw WasmVmException("unsupoorted wasm vm");
  }
}

template<typename F> void getFunction(WasmVm *vm, absl::string_view functionName, F* function) {
  if (vm->vm() == WasmVmNames::get().Wavm) {
    getFunctionWavm(vm, functionName, function);
  } else {
    throw WasmVmException("unsupoorted wasm vm");
  }
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
