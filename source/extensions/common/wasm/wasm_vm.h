#pragma once

#include <memory>

#include "envoy/common/exception.h"

#include "common/common/logger.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

class Context;

// Represents a WASM-native word-sized datum. On 32-bit VMs, the high bits are always zero.
// The WASM/VM API treats all bits as significant.
struct Word {
  Word(uint64_t w) : u64(w) {}              // Implicit conversion into Word.
  operator uint64_t() const { return u64; } // Implicit conversion into uint64_t.
  // Note: no implicit conversion to uint32_t as it is lossy.
  uint32_t u32() const { return static_cast<uint32_t>(u64); }
  uint64_t u64;
};

template <typename T> struct ConvertWordTypeToUint32 { using type = T; };
template <> struct ConvertWordTypeToUint32<Word> { using type = uint32_t; };

template <typename F> struct ConvertFunctionTypeWordToUint32 {};
template <typename R, typename... Args> struct ConvertFunctionTypeWordToUint32<R (*)(Args...)> {
  using type = typename ConvertWordTypeToUint32<R>::type (*)(
      typename ConvertWordTypeToUint32<Args>::type...);
};

template <typename T> struct Global {
  virtual ~Global() {}
  virtual T get() PURE;
  virtual void set(const T& t) PURE;
};

// Calls into the WASM VM.
// 1st arg is always a pointer to Context (Context*).
using WasmCall0Void = std::function<void(Context*)>;
using WasmCall1Void = std::function<void(Context*, Word)>;
using WasmCall2Void = std::function<void(Context*, Word, Word)>;
using WasmCall3Void = std::function<void(Context*, Word, Word, Word)>;
using WasmCall4Void = std::function<void(Context*, Word, Word, Word, Word)>;
using WasmCall5Void = std::function<void(Context*, Word, Word, Word, Word, Word)>;
using WasmCall6Void = std::function<void(Context*, Word, Word, Word, Word, Word, Word)>;
using WasmCall7Void = std::function<void(Context*, Word, Word, Word, Word, Word, Word, Word)>;
using WasmCall8Void = std::function<void(Context*, Word, Word, Word, Word, Word, Word, Word, Word)>;
using WasmCall1Word = std::function<Word(Context*, Word)>;
using WasmCall2Word = std::function<Word(Context*, Word, Word)>;
using WasmCall3Word = std::function<Word(Context*, Word, Word, Word)>;
using WasmCall4Word = std::function<Word(Context*, Word, Word, Word, Word)>;
using WasmCall5Word = std::function<Word(Context*, Word, Word, Word, Word, Word)>;
using WasmCall6Word = std::function<Word(Context*, Word, Word, Word, Word, Word, Word)>;
using WasmCall7Word = std::function<Word(Context*, Word, Word, Word, Word, Word, Word, Word)>;
using WasmCall8Word = std::function<Word(Context*, Word, Word, Word, Word, Word, Word, Word, Word)>;

// Calls out of the WASM VM.
// 1st arg is always a pointer to raw_context (void*).
using WasmCallback0Void = void (*)(void*);
using WasmCallback1Void = void (*)(void*, Word);
using WasmCallback2Void = void (*)(void*, Word, Word);
using WasmCallback3Void = void (*)(void*, Word, Word, Word);
using WasmCallback4Void = void (*)(void*, Word, Word, Word, Word);
using WasmCallback5Void = void (*)(void*, Word, Word, Word, Word, Word);
using WasmCallback6Void = void (*)(void*, Word, Word, Word, Word, Word, Word);
using WasmCallback7Void = void (*)(void*, Word, Word, Word, Word, Word, Word, Word);
using WasmCallback8Void = void (*)(void*, Word, Word, Word, Word, Word, Word, Word, Word);
using WasmCallback0Word = Word (*)(void*);
using WasmCallback1Word = Word (*)(void*, Word);
using WasmCallback2Word = Word (*)(void*, Word, Word);
using WasmCallback3Word = Word (*)(void*, Word, Word, Word);
using WasmCallback4Word = Word (*)(void*, Word, Word, Word, Word);
using WasmCallback5Word = Word (*)(void*, Word, Word, Word, Word, Word);
using WasmCallback6Word = Word (*)(void*, Word, Word, Word, Word, Word, Word);
using WasmCallback7Word = Word (*)(void*, Word, Word, Word, Word, Word, Word, Word, Word);
using WasmCallback8Word = Word (*)(void*, Word, Word, Word, Word, Word, Word, Word, Word, Word);
using WasmCallback9Word = Word (*)(void*, Word, Word, Word, Word, Word, Word, Word, Word, Word,
                                   Word);
// Using the standard g++/clang mangling algorithm:
// https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-builtin
// Extended with W = Word
// Z = void, j = uint32_t, l = int64_t, m = uint64_t
using WasmCallback_ZWl = void (*)(void*, Word, int64_t);
using WasmCallback_ZWm = void (*)(void*, Word, uint64_t);
using WasmCallback_m = uint64_t (*)(void*);
using WasmCallback_mW = uint64_t (*)(void*, Word);
using WasmCallback_jW = uint32_t (*)(void*, Word);

// Wasm VM instance. Provides the low level WASM interface.
class WasmVm : public Logger::Loggable<Logger::Id::wasm> {
public:
  virtual ~WasmVm() {}
  virtual absl::string_view vm() PURE;

  // Whether or not the VM implementation supports cloning.
  virtual bool clonable() PURE;
  // Make a thread-specific copy. This may not be supported by the underlying VM system in which
  // case it will return nullptr and the caller will need to create a new VM from scratch.
  virtual std::unique_ptr<WasmVm> clone() PURE;

  // Load the WASM code from a file. Return true on success.
  virtual bool load(const std::string& code, bool allow_precompiled) PURE;
  // Link to registered function.
  virtual void link(absl::string_view debug_name, bool needs_emscripten) PURE;

  // Set memory layout (start of dynamic heap base, etc.) in the VM.
  virtual void setMemoryLayout(uint64_t stack_base, uint64_t heap_base,
                               uint64_t heap_base_pointer) PURE;

  // Call the 'start' function and initialize globals.
  virtual void start(Context*) PURE;

  // Get size of the currently allocated memory in the VM.
  virtual uint64_t getMemorySize() PURE;
  // Convert a block of memory in the VM to a string_view.
  virtual absl::string_view getMemory(uint64_t pointer, uint64_t size) PURE;
  // Convert a host pointer to memory in the VM into a VM "pointer" (an offset into the Memory).
  virtual bool getMemoryOffset(void* host_pointer, uint64_t* vm_pointer) PURE;
  // Set a block of memory in the VM, returns true on success, false if the pointer/size is invalid.
  virtual bool setMemory(uint64_t pointer, uint64_t size, const void* data) PURE;
  // Set a Word in the VM, returns true on success, false if the pointer is invalid.
  virtual bool setWord(uint64_t pointer, uint64_t data) PURE;
  // Make a new intrinsic module (e.g. for Emscripten support).
  virtual void makeModule(absl::string_view name) PURE;

  // Get the contents of the user section with the given name or "" if it does not exist.
  virtual absl::string_view getUserSection(absl::string_view name) PURE;

  // Get typed function exported by the WASM module.
  virtual void getFunction(absl::string_view functionName, WasmCall0Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall1Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall2Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall3Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall4Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall5Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall8Void* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall1Word* f) PURE;
  virtual void getFunction(absl::string_view functionName, WasmCall3Word* f) PURE;

  // Register typed callbacks exported by the host environment.
#define REGISTER_CALLBACK(_t)                                                                      \
  virtual void registerCallback(absl::string_view moduleName, absl::string_view functionName,      \
                                _t f, typename ConvertFunctionTypeWordToUint32<_t>::type) PURE;
  REGISTER_CALLBACK(WasmCallback0Void);
  REGISTER_CALLBACK(WasmCallback1Void);
  REGISTER_CALLBACK(WasmCallback2Void);
  REGISTER_CALLBACK(WasmCallback3Void);
  REGISTER_CALLBACK(WasmCallback4Void);
  REGISTER_CALLBACK(WasmCallback5Void);
  REGISTER_CALLBACK(WasmCallback8Void);
  REGISTER_CALLBACK(WasmCallback0Word);
  REGISTER_CALLBACK(WasmCallback1Word);
  REGISTER_CALLBACK(WasmCallback2Word);
  REGISTER_CALLBACK(WasmCallback3Word);
  REGISTER_CALLBACK(WasmCallback4Word);
  REGISTER_CALLBACK(WasmCallback5Word);
  REGISTER_CALLBACK(WasmCallback6Word);
  REGISTER_CALLBACK(WasmCallback7Word);
  REGISTER_CALLBACK(WasmCallback8Word);
  REGISTER_CALLBACK(WasmCallback9Word);
  REGISTER_CALLBACK(WasmCallback_ZWl);
  REGISTER_CALLBACK(WasmCallback_ZWm);
  REGISTER_CALLBACK(WasmCallback_m);
  REGISTER_CALLBACK(WasmCallback_mW);
  REGISTER_CALLBACK(WasmCallback_jW);
#undef REGISTER_CALLBACK

  // Register typed value exported by the host environment.
  virtual std::unique_ptr<Global<Word>> makeGlobal(absl::string_view module_name,
                                                   absl::string_view name, Word initial_value) PURE;
  virtual std::unique_ptr<Global<double>>
  makeGlobal(absl::string_view module_name, absl::string_view name, double initial_value) PURE;
};

// Exceptions for issues with the WasmVm.
class WasmVmException : public EnvoyException {
public:
  using EnvoyException::EnvoyException;
};

// Exceptions for issues with the WebAssembly code.
class WasmException : public EnvoyException {
public:
  using EnvoyException::EnvoyException;
};

extern thread_local Envoy::Extensions::Common::Wasm::Context* current_context_;
extern thread_local uint32_t effective_context_id_;

struct SaveRestoreContext {
  explicit SaveRestoreContext(Context* context) {
    saved_context = current_context_;
    saved_effective_context_id_ = effective_context_id_;
    current_context_ = context;
    effective_context_id_ = 0; // No effective context id.
  }
  ~SaveRestoreContext() {
    current_context_ = saved_context;
    effective_context_id_ = saved_effective_context_id_;
  }
  Context* saved_context;
  uint32_t saved_effective_context_id_;
};

// Create a new low-level WASM VM of the give type (e.g. "envoy.wasm.vm.wavm").
std::unique_ptr<WasmVm> createWasmVm(absl::string_view vm);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
