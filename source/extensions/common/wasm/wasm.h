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

// Basic callbacks supportable by all WASM VMs.  To prevent callbacks, override in Context subclass.
class WasmVmCallbacks {
  public:
    virtual ~WasmVmCallbacks() {}     
    virtual void scriptLog(spdlog::level::level_enum level, absl::string_view message) PURE;
};

class WasmVm;
// A context which will be the target of callbacks for a particular session
// e.g. a handler of a stream.
class Context : public WasmVmCallbacks, public Logger::Loggable<Logger::Id::wasm> {
  public:
    Context(WasmVm *vm) : wasm_vm(vm) {}
    virtual ~Context() {}

    // WasmVmCalblacks.
    void scriptLog(spdlog::level::level_enum level, absl::string_view message) override;

    // Handlers, called from the VM.
    void wasmLogHandler(uint32_t level, uint32_t address, uint32_t size);

    uint32_t id = 0;
    WasmVm* const wasm_vm;
};

class WasmVm : public Logger::Loggable<Logger::Id::wasm> { 
  public:
    virtual ~WasmVm() {}
    virtual absl::string_view vm() PURE;

    // Load the WASM code from a file.  Return true on success.
    virtual bool initialize(absl::string_view file, bool allow_precompiled) PURE;

    // Call the 'start' function or main() if there is no start funcition.
    virtual void start(Context*) PURE;

    // Allocate a block of memory in the VM and return the pointer to use as a call arguments.
    virtual void* allocMemory(uint32_t size, uint32_t *pointer) PURE;
    // Convert a block of memory in the VM to a string_view.
    virtual absl::string_view getMemory(uint32_t pointer, uint32_t size) PURE;

    // Convenience functions.

    // Allocate a null-terminated string in the VM and return the pointer to use as a call arguments.
    uint32_t copyString(absl::string_view s) {
      uint32_t pointer;
      uint8_t *m = static_cast<uint8_t*>(allocMemory((s.size() + 1), &pointer));
      if (s.size() > 0) memcpy(m, s.data(), s.size());
      m[s.size()] = 0;
      return pointer;
    }

};

// Register a function which can be called from WAVM code (e.g. declared extern "C").
// All callbacks must be registered before calling 'initialize'.  // Callbacks will be called
// on member funcions on the current Context (i.e. object of type C),  F must be C::(*)(...);
template<typename F> void registerCallback(WasmVm *vm, absl::string_view functionName, F function);

// Get a function exported by WASM code (e.g. with a declaration of extern "C").
// F should be of the form std::function<R(Args...>).
// Returned by pointer or nullptr if not found.
template<typename F> void getFunction(WasmVm *vm, absl::string_view functionName, F* function);
// Same as above but F should be std::function<R(Context*, Args...>).
template<typename F> void getFunctionWithContext(WasmVm *vm, absl::string_view functionName, F* function);

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
template<typename F> void registerCallbackWavm(WasmVm *vm, absl::string_view functionName, F function);
template<typename F> void getFunctionWavm(WasmVm *vm, absl::string_view functionName, F* function);

template<typename F> void registerCallback(WasmVm *vm, absl::string_view functionName, F function) {
  (void)vm;
  (void)functionName;
  (void)function;
#if 0
  if (vm->vm() == WasmVmNames::get().Wavm) {
    registerCallbackWavm(vm, functionName, [](Context* context, ARGS...) {
        static_cast<C>(context)->function(ARGS...);
        });
  } else {
    throw WasmVmException("unsupoorted wasm vm");
  }
#endif
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
