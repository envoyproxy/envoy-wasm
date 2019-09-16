#pragma once

#include <memory>

#include "envoy/common/exception.h"

#include "common/common/logger.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

class Context;

// Represents a WASM-native word-sized datum. On 32-bit VMs, the high bits are always zero.
// The WASM/VM API treats all bits as significant.
struct Word {
  Word(uint64_t w) : u64_(w) {} // Implicit conversion into Word.
  uint32_t u32() const { return static_cast<uint32_t>(u64_); }
  uint64_t u64_;
};

// Convert Word type for use by 32-bit VMs.
template <typename T> struct ConvertWordTypeToUint32 { using type = T; };
template <> struct ConvertWordTypeToUint32<Word> { using type = uint32_t; };

// Convert Word-based function types for 32-bit VMs.
template <typename F> struct ConvertFunctionTypeWordToUint32 {};
template <typename R, typename... Args> struct ConvertFunctionTypeWordToUint32<R (*)(Args...)> {
  using type = typename ConvertWordTypeToUint32<R>::type (*)(
      typename ConvertWordTypeToUint32<Args>::type...);
};

// A wrapper for a global variable within the VM.
template <typename T> struct Global {
  virtual ~Global() = default;
  virtual T get() PURE;
  virtual void set(const T& t) PURE;
};

// These are templates and its helper for constructing signatures of functions calling into and out
// of WASM VMs.
// - WasmFuncTypeHelper is a helper for WasmFuncType and shouldn't be used anywhere else than
// WasmFuncType definition.
// - WasmFuncType takes 4 template parameter which are number of argument, return type, context type
// and param type respectively, resolve to a function type.
//   For example `WasmFuncType<3, void, Context*, Word>` resolves to `void(Context*, Word, Word,
//   Word)`
template <size_t N, class ReturnType, class ContextType, class ParamType,
          class FuncBase = ReturnType(ContextType)>
struct WasmFuncTypeHelper {};

template <size_t N, class ReturnType, class ContextType, class ParamType, class... Args>
struct WasmFuncTypeHelper<N, ReturnType, ContextType, ParamType, ReturnType(ContextType, Args...)> {
  using type = typename WasmFuncTypeHelper<N - 1, ReturnType, ContextType, ParamType,
                                           ReturnType(ContextType, Args..., ParamType)>::type;
};

template <class ReturnType, class ContextType, class ParamType, class... Args>
struct WasmFuncTypeHelper<0, ReturnType, ContextType, ParamType, ReturnType(ContextType, Args...)> {
  using type = ReturnType(ContextType, Args...);
};

template <size_t N, class ReturnType, class ContextType, class ParamType>
using WasmFuncType = typename WasmFuncTypeHelper<N, ReturnType, ContextType, ParamType>::type;

// Calls into the WASM VM.
// 1st arg is always a pointer to Context (Context*).
template <size_t N> using WasmCallVoid = std::function<WasmFuncType<N, void, Context*, Word>>;
template <size_t N> using WasmCallWord = std::function<WasmFuncType<N, Word, Context*, Word>>;

#define FOR_ALL_WASM_VM_EXPORTS(_f)                                                                \
  _f(WasmCallVoid<0>) _f(WasmCallVoid<1>) _f(WasmCallVoid<2>) _f(WasmCallVoid<3>)                  \
      _f(WasmCallVoid<4>) _f(WasmCallVoid<5>) _f(WasmCallVoid<8>) _f(WasmCallWord<0>)              \
          _f(WasmCallWord<1>) _f(WasmCallWord<3>)

// Calls out of the WASM VM.
// 1st arg is always a pointer to raw_context (void*).
template <size_t N> using WasmCallbackVoid = WasmFuncType<N, void, void*, Word>*;
template <size_t N> using WasmCallbackWord = WasmFuncType<N, Word, void*, Word>*;

// Using the standard g++/clang mangling algorithm:
// https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-builtin
// Extended with W = Word
// Z = void, j = uint32_t, l = int64_t, m = uint64_t
using WasmCallback_WWl = Word (*)(void*, Word, int64_t);
using WasmCallback_WWm = Word (*)(void*, Word, uint64_t);

#define FOR_ALL_WASM_VM_IMPORTS(_f)                                                                \
  _f(WasmCallbackVoid<0>) _f(WasmCallbackVoid<1>) _f(WasmCallbackVoid<2>) _f(WasmCallbackVoid<3>)  \
      _f(WasmCallbackVoid<4>) _f(WasmCallbackWord<0>) _f(WasmCallbackWord<1>)                      \
          _f(WasmCallbackWord<2>) _f(WasmCallbackWord<3>) _f(WasmCallbackWord<4>)                  \
              _f(WasmCallbackWord<5>) _f(WasmCallbackWord<6>) _f(WasmCallbackWord<7>)              \
                  _f(WasmCallbackWord<8>) _f(WasmCallbackWord<9>) _f(WasmCallback_WWl)             \
                      _f(WasmCallback_WWm)

// Wasm VM instance. Provides the low level WASM interface.
class WasmVm : public Logger::Loggable<Logger::Id::wasm> {
public:
  using WasmVmPtr = std::unique_ptr<WasmVm>;

  virtual ~WasmVm() = default;
  /**
   * Return the VM identifier.
   * @return one of WasmVmValues from well_known_names.h e.g. "envoy.wasm.vm.null".
   */
  virtual absl::string_view vm() PURE;

  /**
   * Whether or not the VM implementation supports cloning. Cloning is VM system dependent.
   * When a VM is configured a single VM is instantiated to check that the .wasm file is valid and
   * to do VM system specific initialization. In the case of WAVM this is potentially ahead-of-time
   * compilation. Then, if cloning is supported, we clone that VM for each worker, potentially
   * copying and sharing the initialized data structures for efficiency. Otherwise we create an new
   * VM from scratch for each worker.
   * @return true if the VM is cloneable.
   */
  virtual bool cloneable() PURE;

  /**
   * Make a worker/thread-specific copy if supported by the underlying VM system (see cloneable()
   * above). If not supported, the caller will need to create a new VM from scratch. If supported,
   * the clone may share compiled code and other read-only data with the source VM.
   * @return a clone of 'this' (e.g. for a different worker/thread).
   */
  virtual WasmVmPtr clone() PURE;

  /**
   * Load the WASM code from a file. Return true on success. Once the module is loaded it can be
   * queried, e.g. to see which version of emscripten support is required. After loading, the
   * appropriate ABI callbacks can be registered and then the module can be link()ed (see below).
   * @param code the WASM binary code (or registered NullVm plugin name).
   * @param allow_precompiled if true, allows supporting VMs (e.g. WAVM) to load the binary
   * machine code from a user-defined section of the WASM file. Because that code is not verified by
   * the envoy process it is up to the user to ensure that the code is both safe and is built for
   * the linked in version of WAVM.
   * @return whether or not the load was successful.
   */
  virtual bool load(const std::string& code, bool allow_precompiled) PURE;

  /**
   * Link the WASM code to the host-provided functions and globals, e.g. the ABI. Prior to linking,
   * the module should be loaded and the ABI callbacks registered (see above). Linking should be
   * done once between load() and start().
   * @param debug_name user-provided name for use in log and error messages.
   * @param needs_emscripten whether emscripten support should be provided (e.g.
   * _emscripten_memcpy_bigHandler). Emscripten (http://https://emscripten.org/) is
   * a C++ WebAssembly tool chain.
   */
  virtual void link(absl::string_view debug_name, bool needs_emscripten) PURE;

  /**
   * Set memory layout (start of dynamic heap base, etc.) in the VM.
   * @param stack_base the location in VM memory of the stack.
   * @param heap_base the location in VM memory of the heap.
   * @param heap_base_ptr the location in VM memory of a location to store the heap pointer.
   */
  virtual void setMemoryLayout(uint64_t stack_base, uint64_t heap_base,
                               uint64_t heap_base_pointer) PURE;

  /**
   * Initialize globals (including calling global constructors) and call the 'start' function.
   * Prior to calling start() the module should be load()ed, ABI callbacks should be registered
   * (registerCallback), the module link()ed, and any exported functions should be gotten
   * (getFunction).
   * @param vm_context a context which represents the caller: in this case Envoy itself.
   */
  virtual void start(Context* vm_context) PURE;

  /**
   * Get size of the currently allocated memory in the VM.
   * @return the size of memory in bytes.
   */
  virtual uint64_t getMemorySize() PURE;

  /**
   * Convert a block of memory in the VM to a string_view.
   * @param pointer the offset into VM memory of the requested VM memory block.
   * @param size the size of the requested VM memory block.
   * @return if std::nullopt then the pointer/size pair were invalid, otherwise returns
   * a host string_view pointing to the pointer/size pair in VM memory.
   */
  virtual absl::optional<absl::string_view> getMemory(uint64_t pointer, uint64_t size) PURE;

  /**
   * Convert a host pointer to memory in the VM into a VM "pointer" (an offset into the Memory).
   * @param host_pointer a pointer to host memory to be converted into a VM offset (pointer).
   * @param vm_pointer a pointer to an uint64_t to be filled with the offset in VM memory
   * corresponding to 'host_pointer'.
   * @return whether or not the host_pointer was a valid VM memory offset.
   */
  virtual bool getMemoryOffset(void* host_pointer, uint64_t* vm_pointer) PURE;

  /**
   * Set a block of memory in the VM, returns true on success, false if the pointer/size is invalid.
   * @param pointer the offset into VM memory describing the start of a region of VM memory.
   * @param size the size of the region of VM memory.
   * @return whether or not the pointer/size pair was a valid VM memory block.
   */
  virtual bool setMemory(uint64_t pointer, uint64_t size, const void* data) PURE;

  /**
   * Get a VM native Word (e.g. sizeof(void*) or sizeof(size_t)) from VM memory, returns true on
   * success, false if the pointer is invalid. WASM-32 VMs have 32-bit native words and WASM-64 VMs
   * (not yet supported) will have 64-bit words as does the Null VM (compiled into 64-bit Envoy).
   * This function can be used to chase pointers in VM memory.
   * @param pointer the offset into VM memory describing the start of VM native word size block.
   * @param data a pointer to a Word whose contents will be filled from the VM native word at
   * 'pointer'.
   * @return whether or not the pointer was to a valid VM memory block of VM native word size.
   */
  virtual bool getWord(uint64_t pointer, Word* data) PURE;

  /**
   * Set a Word in the VM, returns true on success, false if the pointer is invalid.
   * See getWord above for details. This function can be used (for example) to set indirect pointer
   * return values (e.g. proxy_getHeaderHapValue(... const char** value_ptr, size_t* value_size).
   * @param pointer the offset into VM memory describing the start of VM native word size block.
   * @param data a Word whose contents will be written in VM native word size at 'pointer'.
   * @return whether or not the pointer was to a valid VM memory block of VM native word size.
   */
  virtual bool setWord(uint64_t pointer, Word data) PURE;

  /**
   * Make a new intrinsic module (e.g. for Emscripten support).
   * @param name the name of the module to make.
   */
  virtual void makeModule(absl::string_view name) PURE;

  /**
   * Get the contents of the user section with the given name or "" if it does not exist.
   * @param name the name of the user section to get.
   * @return the contents of the user section (if any). The result will be empty() if there
   * is no such section.
   */
  virtual absl::string_view getUserSection(absl::string_view name) PURE;

  /**
   * Get typed function exported by the WASM module.
   */
#define _GET_FUNCTION(_T) virtual void getFunction(absl::string_view function_name, _T* f) PURE;
  FOR_ALL_WASM_VM_EXPORTS(_GET_FUNCTION)
#undef _GET_FUNCTION

  /**
   * Register typed callbacks exported by the host environment.
   */
#define _REGISTER_CALLBACK(_T)                                                                     \
  virtual void registerCallback(absl::string_view moduleName, absl::string_view function_name,     \
                                _T f, typename ConvertFunctionTypeWordToUint32<_T>::type) PURE;
  FOR_ALL_WASM_VM_IMPORTS(_REGISTER_CALLBACK)
#undef _REGISTER_CALLBACK

  /**
   * Register typed value exported by the host environment.
   * @param module_name the name of the module to which to export the global.
   * @param name the name of the global variable to export.
   * @param initial_value the initial value of the global.
   * @return a Global object which can be used to access the exported global.
   */
  virtual std::unique_ptr<Global<Word>> makeGlobal(absl::string_view module_name,
                                                   absl::string_view name, Word initial_value) PURE;

  /**
   * Register typed value exported by the host environment.
   * @param module_name the name of the module to which to export the global.
   * @param name the name of the global variable to export.
   * @param initial_value the initial value of the global.
   * @return a Global object which can be used to access the exported global.
   */
  virtual std::unique_ptr<Global<double>>
  makeGlobal(absl::string_view module_name, absl::string_view name, double initial_value) PURE;
};
using WasmVmPtr = std::unique_ptr<WasmVm>;

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

// Thread local state set during a call into a WASM VM so that calls coming out of the
// VM can be attributed correctly to calling Filter. We use thread_local instead of ThreadLocal
// because this state is live only during the calls and does not need to be initialized consistently
// over all workers as with ThreadLocal data.
extern thread_local Envoy::Extensions::Common::Wasm::Context* current_context_;

// Requested effective context set by code within the VM to request that the calls coming out of the
// VM be attributed to another filter, for example if a control plane gRPC comes back to the
// RootContext which effects some set of waiting filters.
extern thread_local uint32_t effective_context_id_;

// Helper to save and restore thread local VM call context information to support reentrant calls.
// NB: this happens for example when a call from the VM invokes a handler which needs to _malloc
// memory in the VM.
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
WasmVmPtr createWasmVm(absl::string_view vm);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
