#include "extensions/common/wasm/wavm/wavm.h"

#include <stdint.h>
#include <stdio.h>

#include <atomic>
#include <fstream>
#include <memory>
#include <utility>
#include <vector>

#include "envoy/common/exception.h"
#include "envoy/server/wasm.h"

#include "common/common/assert.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/common/wasm/well_known_names.h"

#include "WAVM/Emscripten/Emscripten.h"
#include "WAVM/IR/Module.h"
#include "WAVM/IR/Operators.h"
#include "WAVM/IR/Types.h"
#include "WAVM/IR/Validate.h"
#include "WAVM/IR/Value.h"
#include "WAVM/Inline/Assert.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/Errors.h"
#include "WAVM/Inline/Hash.h"
#include "WAVM/Inline/HashMap.h"
#include "WAVM/Inline/IndexMap.h"
#include "WAVM/Inline/IntrusiveSharedPtr.h"
#include "WAVM/Inline/Lock.h"
#include "WAVM/Platform/Mutex.h"
#include "WAVM/Platform/Thread.h"
#include "WAVM/Runtime/Intrinsics.h"
#include "WAVM/Runtime/Linker.h"
#include "WAVM/Runtime/Runtime.h"
#include "WAVM/Runtime/RuntimeData.h"
#include "WAVM/WASM/WASM.h"
#include "WAVM/WASTParse/WASTParse.h"
#include "absl/strings/match.h"

using namespace WAVM;
using namespace WAVM::IR;

DECLARE_INTRINSIC_MODULE(env);

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

extern thread_local Envoy::Extensions::Common::Wasm::Context* current_context_;

namespace Wavm {
namespace {

using Context = Common::Wasm::Context; // Shadowing WAVM::Runtime::Context.

const Logger::Id wasmId = Logger::Id::wasm;

struct SaveRestoreContext {
  explicit SaveRestoreContext(Context* context) {
    saved_context = current_context_;
    current_context_ = context;
  }
  ~SaveRestoreContext() { current_context_ = saved_context; }
  Context* saved_context;
};

#define CALL_WITH_CONTEXT(_x, _context)                                                            \
  do {                                                                                             \
    SaveRestoreContext _saved_context(static_cast<Context*>(_context));                            \
    WAVM::Runtime::catchRuntimeExceptions([&] { _x; },                                             \
                                          [&](WAVM::Runtime::Exception* exception) {               \
                                            auto description = describeException(exception);       \
                                            destroyException(exception);                           \
                                            throw WasmException(description);                      \
                                          });                                                      \
  } while (0)

#define CALL_WITH_CONTEXT_RETURN(_x, _context, _type, _member)                                     \
  do {                                                                                             \
    SaveRestoreContext _saved_context(static_cast<Context*>(_context));                            \
    _type _return_value;                                                                           \
    WAVM::Runtime::catchRuntimeExceptions(                                                         \
        [&] { _return_value = static_cast<_type>(_x[0]._member); },                                \
        [&](WAVM::Runtime::Exception* exception) {                                                 \
          auto description = describeException(exception);                                         \
          destroyException(exception);                                                             \
          throw WasmException(description);                                                        \
        });                                                                                        \
    return _return_value;                                                                          \
  } while (0)

class RootResolver : public WAVM::Runtime::Resolver, public Logger::Loggable<wasmId> {
public:
  RootResolver(WAVM::Runtime::Compartment*) {}

  virtual ~RootResolver() { moduleNameToInstanceMap_.clear(); }

  bool resolve(const std::string& moduleName, const std::string& exportName, ExternType type,
               WAVM::Runtime::Object*& outObject) override {
    if (moduleName == "env") {
      auto envoyInstance = moduleNameToInstanceMap_.get("envoy");
      if (envoyInstance) {
        outObject = getInstanceExport(*envoyInstance, exportName);
        if (outObject && isA(outObject, type)) {
          return resolveInternal("envoy", exportName, type, outObject);
        }
      }
    }
    return resolveInternal(moduleName, exportName, type, outObject);
  }

  bool resolveInternal(const std::string& moduleName, const std::string& exportName,
                       ExternType type, WAVM::Runtime::Object*& outObject) {
    auto namedInstance = moduleNameToInstanceMap_.get(moduleName);
    if (namedInstance) {
      outObject = getInstanceExport(*namedInstance, exportName);
      if (outObject) {
        if (isA(outObject, type)) {
          return true;
        } else {
          throw WasmException(fmt::format(
              "Failed to load WASM module due to a type mismatch in an import: {}.{} {}, "
              "but was expecting type: {}",
              moduleName, exportName, asString(getObjectType(outObject)), asString(type)));
        }
      }
    }

    throw WasmException(fmt::format("Failed to load WASM module due to a missing import: {}.{} {}",
                                    moduleName, exportName, asString(type)));
  }

  HashMap<std::string, WAVM::Runtime::ModuleInstance*>& moduleNameToInstanceMap() {
    return moduleNameToInstanceMap_;
  }

private:
  HashMap<std::string, WAVM::Runtime::ModuleInstance*> moduleNameToInstanceMap_;
};

bool loadModule(const std::string& code, IR::Module& outModule) {
  // If the code starts with the WASM binary magic number, load it as a binary irModule.
  static const uint8_t wasmMagicNumber[4] = {0x00, 0x61, 0x73, 0x6d};
  if (code.size() >= 4 && !memcmp(code.data(), wasmMagicNumber, 4)) {
    return WASM::loadBinaryModule(code.data(), code.size(), outModule);
  } else {
    // Load it as a text irModule.
    std::vector<WAST::Error> parseErrors;
    if (!WAST::parseModule(code.c_str(), code.size() + 1, outModule, parseErrors)) {
      return false;
    }
    return true;
  }
}

} // namespace

struct EnvoyHandlerBase {
  virtual ~EnvoyHandlerBase() {}
};

template <typename F> struct EnvoyHandler : EnvoyHandlerBase {
  ~EnvoyHandler() override {}
  explicit EnvoyHandler(F ahandler) : handler(ahandler) {}
  F handler;
};

template <typename F> EnvoyHandlerBase* MakeEnvoyHandler(F handler) {
  return new EnvoyHandler<F>(handler);
}

class Wavm : public WasmVm {
public:
  Wavm() = default;
  ~Wavm() override;

  // WasmVm
  absl::string_view vm() override { return WasmVmNames::get().Wavm; }
  bool clonable() override { return true; };
  std::unique_ptr<WasmVm> clone() override;
  bool initialize(const std::string& code, absl::string_view name, bool allow_precompiled) override;
  void start(Context* context) override;
  void* allocMemory(uint32_t size, uint32_t* pointer) override;
  absl::string_view getMemory(uint32_t pointer, uint32_t size) override;
  bool setMemory(uint32_t pointer, uint32_t size, void* data) override;

  WAVM::Runtime::Memory* memory() { return memory_; }
  WAVM::Runtime::Context* context() { return context_; }
  WAVM::Runtime::ModuleInstance* moduleInstance() { return moduleInstance_; }
  WAVM::Runtime::ModuleInstance* envoyModuleInstance() { return moduleInstance_; }

  void GetFunctions();
  void RegisterCallbacks();

  bool hasInstantiatedModule_ = false;
  IR::Module irModule_;
  WAVM::Runtime::GCPointer<WAVM::Runtime::ModuleInstance> moduleInstance_;
  WAVM::Runtime::Memory* memory_;
  Emscripten::Instance* emscriptenInstance_ = nullptr;
  WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment_;
  WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context_;
  Intrinsics::Module envoy_module_;
  WAVM::Runtime::GCPointer<WAVM::Runtime::ModuleInstance> envoyModuleInstance_ = nullptr;
  std::vector<std::unique_ptr<Intrinsics::Function>> envoy_functions_;
};

Wavm::~Wavm() {
  moduleInstance_ = nullptr;
  if (emscriptenInstance_) {
    emscriptenInstance_->env = nullptr;
    emscriptenInstance_->asm2wasm = nullptr;
    emscriptenInstance_->global = nullptr;
    emscriptenInstance_->emscriptenMemory = nullptr;
    delete emscriptenInstance_;
  }
  context_ = nullptr;
  envoyModuleInstance_ = nullptr;
  envoy_functions_.clear();
  if (compartment_) {
    ASSERT(tryCollectCompartment(std::move(compartment_)));
  }
}

std::unique_ptr<WasmVm> Wavm::clone() {
  auto wavm = std::make_unique<Wavm>();
  wavm->compartment_ = WAVM::Runtime::cloneCompartment(compartment_);
  wavm->memory_ = WAVM::Runtime::remapToClonedCompartment(memory_, wavm->compartment_);
  wavm->context_ = WAVM::Runtime::createContext(wavm->compartment_);
  wavm->envoyModuleInstance_ =
      WAVM::Runtime::remapToClonedCompartment(envoyModuleInstance_, wavm->compartment_);
  wavm->moduleInstance_ =
      WAVM::Runtime::remapToClonedCompartment(moduleInstance_, wavm->compartment_);
  return wavm;
}

bool Wavm::initialize(const std::string& code, absl::string_view name, bool allow_precompiled) {
  ASSERT(!hasInstantiatedModule_);
  hasInstantiatedModule_ = true;
  compartment_ = WAVM::Runtime::createCompartment();
  context_ = WAVM::Runtime::createContext(compartment_);
  if (!loadModule(code, irModule_)) {
    return false;
  }
  WAVM::Runtime::ModuleRef module = nullptr;
  // todo check percompiled section is permitted
  const UserSection* precompiledObjectSection = nullptr;
  if (allow_precompiled) {
    for (const UserSection& userSection : irModule_.userSections) {
      if (userSection.name == "wavm.precompiled_object") {
        precompiledObjectSection = &userSection;
        break;
      }
    }
  }
  if (!precompiledObjectSection) {
    module = WAVM::Runtime::compileModule(irModule_);
  } else {
    module = WAVM::Runtime::loadPrecompiledModule(irModule_, precompiledObjectSection->data);
  }
  RootResolver rootResolver(compartment_);
  envoyModuleInstance_ = Intrinsics::instantiateModule(compartment_, envoy_module_, "envoy");
  rootResolver.moduleNameToInstanceMap().set("envoy", envoyModuleInstance_);
  // Auto-detect if WASM module needs Emscripten.
  bool needs_emscripten = false;
  for (const auto& func : irModule_.functions.imports) {
    if (func.exportName == "_emscripten_memcpy_big" && func.moduleName == "env") {
      needs_emscripten = true;
      break;
    }
  }
  if (needs_emscripten) {
    emscriptenInstance_ = Emscripten::instantiate(compartment_, irModule_);
    rootResolver.moduleNameToInstanceMap().set("env", emscriptenInstance_->env);
    rootResolver.moduleNameToInstanceMap().set("asm2wasm", emscriptenInstance_->asm2wasm);
    rootResolver.moduleNameToInstanceMap().set("global", emscriptenInstance_->global);
  }
  WAVM::Runtime::LinkResult linkResult = linkModule(irModule_, rootResolver);
  moduleInstance_ = instantiateModule(compartment_, module, std::move(linkResult.resolvedImports),
                                      std::string(name));
  memory_ = getDefaultMemory(moduleInstance_);
  return true;
}

void Wavm::start(Context* context) {
  auto f = getStartFunction(moduleInstance_);
  if (f) {
    CALL_WITH_CONTEXT(invokeFunctionChecked(context_, f, {}), context);
  }
  if (emscriptenInstance_) {
    Emscripten::initializeGlobals(context_, irModule_, moduleInstance_);
  }
  f = asFunctionNullable(getInstanceExport(moduleInstance_, "main"));
  if (!f) {
    f = asFunctionNullable(getInstanceExport(moduleInstance_, "_main"));
  }
  if (f) {
    CALL_WITH_CONTEXT(invokeFunctionChecked(context_, f, {}), context);
  }
}

void* Wavm::allocMemory(uint32_t size, uint32_t* address) {
  auto f = asFunctionNullable(getInstanceExport(moduleInstance(), "_malloc"));
  if (!f)
    return nullptr;
  auto values = invokeFunctionChecked(context_, f, {size});
  ASSERT(values.values.size() == 1);
  auto& v = values[0];
  ASSERT(v.type == ValueType::i32);
  *address = v.u32;
  return reinterpret_cast<char*>(
      WAVM::Runtime::memoryArrayPtr<U8>(memory(), v.u32, static_cast<U64>(size)));
}

absl::string_view Wavm::getMemory(uint32_t pointer, uint32_t size) {
  return {reinterpret_cast<char*>(
              WAVM::Runtime::memoryArrayPtr<U8>(memory(), pointer, static_cast<U64>(size))),
          static_cast<size_t>(size)};
}

bool Wavm::setMemory(uint32_t pointer, uint32_t size, void* data) {
  auto p = reinterpret_cast<char*>(
      WAVM::Runtime::memoryArrayPtr<U8>(memory(), pointer, static_cast<U64>(size)));
  if (p) {
    memcpy(p, data, size);
    return true;
  } else {
    return false;
  }
}

std::unique_ptr<WasmVm> createWavm() { return std::make_unique<Wavm>(); }

} // namespace Wavm

template <typename R, typename... Args>
IR::FunctionType inferEnvoyFunctionType(R (*)(void*, Args...)) {
  return IR::FunctionType(IR::inferResultType<R>(), IR::TypeTuple({IR::inferValueType<Args>()...}));
}

using namespace Wavm;

template <typename R, typename... Args>
void registerCallbackWavm(WasmVm* vm, absl::string_view functionName, R (*f)(Args...)) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  wavm->envoy_functions_.emplace_back(
      new Intrinsics::Function(wavm->envoy_module_, functionName.data(), reinterpret_cast<void*>(f),
                               inferEnvoyFunctionType(f), IR::CallingConvention::intrinsic));
}

template void registerCallbackWavm<void, void*>(WasmVm* vm, absl::string_view functionName,
                                                void (*f)(void*));
template void registerCallbackWavm<void, void*, U32>(WasmVm* vm, absl::string_view functionName,
                                                     void (*f)(void*, U32));
template void registerCallbackWavm<void, void*, U32, U32>(WasmVm* vm,
                                                          absl::string_view functionName,
                                                          void (*f)(void*, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32>(WasmVm* vm,
                                                               absl::string_view functionName,
                                                               void (*f)(void*, U32, U32, U32));
template void
registerCallbackWavm<void, void*, U32, U32, U32, U32>(WasmVm* vm, absl::string_view functionName,
                                                      void (*f)(void*, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName, void (*f)(void*, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName, void (*f)(void*, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32));

template void registerCallbackWavm<U32, void*>(WasmVm* vm, absl::string_view functionName,
                                               U32 (*f)(void*));
template void registerCallbackWavm<U32, void*, U32>(WasmVm* vm, absl::string_view functionName,
                                                    U32 (*f)(void*, U32));
template void registerCallbackWavm<U32, void*, U32, U32>(WasmVm* vm, absl::string_view functionName,
                                                         U32 (*f)(void*, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32>(WasmVm* vm,
                                                              absl::string_view functionName,
                                                              U32 (*f)(void*, U32, U32, U32));
template void
registerCallbackWavm<U32, void*, U32, U32, U32, U32>(WasmVm* vm, absl::string_view functionName,
                                                     U32 (*f)(void*, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName, U32 (*f)(void*, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName, U32 (*f)(void*, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName, U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view functionName,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32));

template <typename R, typename... Args>
IR::FunctionType
inferStdFunctionType(std::function<R(Envoy::Extensions::Common::Wasm::Context*, Args...)>*) {
  return IR::FunctionType(IR::inferResultType<R>(), IR::TypeTuple({IR::inferValueType<Args>()...}));
}

static bool checkFunctionType(WAVM::Runtime::Function* f, IR::FunctionType t) {
  return getFunctionType(f) == t;
}

template <typename R, typename... Args>
void getFunctionWavmReturn(WasmVm* vm, absl::string_view functionName,
                           std::function<R(Context*, Args...)>* function, uint32_t) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  auto f = asFunctionNullable(getInstanceExport(wavm->moduleInstance(), std::string(functionName)));
  if (!f)
    f = asFunctionNullable(
        getInstanceExport(wavm->envoyModuleInstance(), std::string(functionName)));
  if (!f) {
    *function = nullptr;
    return;
  }
  if (!checkFunctionType(f, inferStdFunctionType(function))) {
    throw WasmVmException(fmt::format("Bad function signature for: {}", functionName));
  }
  *function = [wavm, f](Context* context, Args... args) -> R {
    UntaggedValue values[] = {args...};
    CALL_WITH_CONTEXT_RETURN(invokeFunctionUnchecked(wavm->context(), f, &values[0]), context,
                             uint32_t, i32);
  };
}

struct Void {};

template <typename R, typename... Args>
void getFunctionWavmReturn(WasmVm* vm, absl::string_view functionName,
                           std::function<R(Context*, Args...)>* function, Void) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  auto f = asFunctionNullable(getInstanceExport(wavm->moduleInstance(), std::string(functionName)));
  if (!f)
    f = asFunctionNullable(
        getInstanceExport(wavm->envoyModuleInstance(), std::string(functionName)));
  if (!f) {
    *function = nullptr;
    return;
  }
  if (!checkFunctionType(f, inferStdFunctionType(function))) {
    throw WasmVmException(fmt::format("Bad function signature for: {}", functionName));
  }
  *function = [wavm, f](Context* context, Args... args) -> R {
    UntaggedValue values[] = {args...};
    CALL_WITH_CONTEXT(invokeFunctionUnchecked(wavm->context(), f, &values[0]), context);
  };
}

// NB: Unfortunately 'void' is not treated like every othefunction type in C++. In
// particular it is not possible to specialize a template based on 'void'. Instead
// we use 'Void' for template matching. Note that the template implemenation above
// which matchers on 'bool' does not use 'Void' in the implemenation.
template <typename R, typename... Args>
void getFunctionWavm(WasmVm* vm, absl::string_view functionName,
                     std::function<R(Context*, Args...)>* function) {
  typename std::conditional<std::is_void<R>::value, Void, uint32_t>::type x{};
  getFunctionWavmReturn(vm, functionName, function, x);
}

template void getFunctionWavm<void>(WasmVm*, absl::string_view, std::function<void(Context*)>*);
template void getFunctionWavm<void, uint32_t>(WasmVm*, absl::string_view,
                                              std::function<void(Context*, uint32_t)>*);
template void
getFunctionWavm<void, uint32_t, uint32_t>(WasmVm*, absl::string_view,
                                          std::function<void(Context*, uint32_t, uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view, std::function<void(Context*, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<void(Context*, uint32_t, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<void(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<void(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<void(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                       uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<void(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                       uint32_t, uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<void(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                       uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<void, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<void(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                       uint32_t, uint32_t, uint32_t, uint32_t)>*);

template void getFunctionWavm<uint32_t>(WasmVm*, absl::string_view,
                                        std::function<uint32_t(Context*)>*);
template void getFunctionWavm<uint32_t, uint32_t>(WasmVm*, absl::string_view,
                                                  std::function<uint32_t(Context*, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view, std::function<uint32_t(Context*, uint32_t, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view, std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t)>*);
template void getFunctionWavm<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(
    WasmVm*, absl::string_view,
    std::function<uint32_t(Context*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t)>*);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
