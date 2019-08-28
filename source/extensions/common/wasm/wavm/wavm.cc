#include "extensions/common/wasm/wavm/wavm.h"

#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/common/assert.h"
#include "common/common/logger.h"

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
#include "absl/container/node_hash_map.h"
#include "absl/strings/match.h"

using namespace WAVM;
using namespace WAVM::IR;

namespace WAVM {
namespace IR {
template <> constexpr ValueType inferValueType<Envoy::Extensions::Common::Wasm::Word>() {
  return ValueType::i32;
}
} // namespace IR
} // namespace WAVM

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

// Forward declarations.
template <typename R, typename... Args>
void getFunctionWavm(WasmVm* vm, absl::string_view function_name,
                     std::function<R(Context*, Args...)>* function);
template <typename R, typename... Args>
void registerCallbackWavm(WasmVm* vm, absl::string_view module_name,
                          absl::string_view function_name, R (*)(Args...));
template <typename F, typename R, typename... Args>
void registerCallbackWavm(WasmVm* vm, absl::string_view module_name,
                          absl::string_view function_name, F, R (*)(Args...));
template <typename T>
std::unique_ptr<Global<T>> makeGlobalWavm(WasmVm* vm, absl::string_view module_name,
                                          absl::string_view name, T initial_value);

namespace Wavm {

struct Wavm;

namespace {

struct WasmUntaggedValue : public WAVM::IR::UntaggedValue {
  WasmUntaggedValue(I32 inI32) { i32 = inI32; }
  WasmUntaggedValue(I64 inI64) { i64 = inI64; }
  WasmUntaggedValue(U32 inU32) { u32 = inU32; }
  WasmUntaggedValue(Word w) { u32 = static_cast<U32>(w.u64); }
  WasmUntaggedValue(U64 inU64) { u64 = inU64; }
  WasmUntaggedValue(F32 inF32) { f32 = inF32; }
  WasmUntaggedValue(F64 inF64) { f64 = inF64; }
};

using Context = Common::Wasm::Context; // Shadowing WAVM::Runtime::Context.

const Logger::Id wasmId = Logger::Id::wasm;

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

#define CALL_WITH_CONTEXT_RETURN(_x, _context, _T, _member)                                        \
  do {                                                                                             \
    SaveRestoreContext _saved_context(static_cast<Context*>(_context));                            \
    _T _return_value;                                                                              \
    WAVM::Runtime::catchRuntimeExceptions([&] { _return_value = static_cast<_T>(_x[0]._member); }, \
                                          [&](WAVM::Runtime::Exception* exception) {               \
                                            auto description = describeException(exception);       \
                                            destroyException(exception);                           \
                                            throw WasmException(description);                      \
                                          });                                                      \
    return _return_value;                                                                          \
  } while (0)

class RootResolver : public WAVM::Runtime::Resolver, public Logger::Loggable<wasmId> {
public:
  RootResolver(WAVM::Runtime::Compartment*) {}

  virtual ~RootResolver() { module_name_to_instance_map_.clear(); }

  bool resolve(const std::string& module_name, const std::string& export_name, ExternType type,
               WAVM::Runtime::Object*& out_object) override {
    if (module_name == "env") {
      auto envoy_instance = module_name_to_instance_map_.get("envoy");
      if (envoy_instance) {
        out_object = getInstanceExport(*envoy_instance, export_name);
        if (out_object && isA(out_object, type)) {
          return resolveInternal("envoy", export_name, type, out_object);
        }
      }
    }
    return resolveInternal(module_name, export_name, type, out_object);
  }

  bool resolveInternal(const std::string& module_name, const std::string& export_name,
                       ExternType type, WAVM::Runtime::Object*& out_object) {
    auto named_instance = module_name_to_instance_map_.get(module_name);
    if (named_instance) {
      out_object = getInstanceExport(*named_instance, export_name);
      if (out_object) {
        if (isA(out_object, type)) {
          return true;
        } else {
          throw WasmException(fmt::format(
              "Failed to load WASM module due to a type mismatch in an import: {}.{} {}, "
              "but was expecting type: {}",
              module_name, export_name, asString(getObjectType(out_object)), asString(type)));
        }
      }
    }

    throw WasmException(fmt::format("Failed to load WASM module due to a missing import: {}.{} {}",
                                    module_name, export_name, asString(type)));
  }

  HashMap<std::string, WAVM::Runtime::ModuleInstance*>& moduleNameToInstanceMap() {
    return module_name_to_instance_map_;
  }

private:
  HashMap<std::string, WAVM::Runtime::ModuleInstance*> module_name_to_instance_map_;
};

const uint64_t WasmPageSize = 1 << 16;

bool loadModule(const std::string& code, IR::Module& out_module) {
  // If the code starts with the WASM binary magic number, load it as a binary IR::Module.
  static const uint8_t WasmMagicNumber[4] = {0x00, 0x61, 0x73, 0x6d};
  if (code.size() >= 4 && !memcmp(code.data(), WasmMagicNumber, 4)) {
    return WASM::loadBinaryModule(code.data(), code.size(), out_module);
  } else {
    // Load it as a text IR::Module.
    std::vector<WAST::Error> parseErrors;
    if (!WAST::parseModule(code.c_str(), code.size() + 1, out_module, parseErrors)) {
      return false;
    }
    return true;
  }
}

} // namespace

struct WavmGlobalBase {
  WAVM::Runtime::Global* global_ = nullptr;
};

template <typename T> struct NativeWord { using type = T; };
template <> struct NativeWord<Word> { using type = uint32_t; };

template <typename T>
struct WavmGlobal : Global<T>,
                    Intrinsics::GenericGlobal<typename NativeWord<T>::type>,
                    WavmGlobalBase {
  WavmGlobal(Common::Wasm::Wavm::Wavm* wavm, Intrinsics::Module& module, const std::string& name,
             T value)
      : Intrinsics::GenericGlobal<typename NativeWord<T>::type>(module, name.c_str(), value),
        wavm_(wavm) {}
  virtual ~WavmGlobal() {}

  T get() override;
  void set(const T& t) override;

  Common::Wasm::Wavm::Wavm* wavm_;
};

struct PairHash {
  template <typename T, typename U> std::size_t operator()(const std::pair<T, U>& x) const {
    return std::hash<T>()(x.first) + std::hash<U>()(x.second);
  }
};

struct Wavm : public WasmVm {
  Wavm() = default;
  ~Wavm() override;

  // WasmVm
  absl::string_view vm() override { return WasmVmNames::get().Wavm; }
  bool clonable() override { return true; };
  std::unique_ptr<WasmVm> clone() override;
  bool load(const std::string& code, bool allow_precompiled) override;
  void setMemoryLayout(uint64_t, uint64_t, uint64_t) override {}
  void link(absl::string_view debug_name, bool needs_emscripten) override;
  void start(Context* context) override;
  uint64_t getMemorySize() override;
  absl::optional<absl::string_view> getMemory(uint64_t pointer, uint64_t size) override;
  bool getMemoryOffset(void* host_pointer, uint64_t* vm_pointer) override;
  bool setMemory(uint64_t pointer, uint64_t size, const void* data) override;
  bool setWord(uint64_t pointer, Word data) override;
  void makeModule(absl::string_view name) override;
  absl::string_view getUserSection(absl::string_view name) override;

  void getInstantiatedGlobals();

#define _GET_FUNCTION(_T)                                                                          \
  void getFunction(absl::string_view function_name, _T* f) override {                              \
    getFunctionWavm(this, function_name, f);                                                       \
  };
  FOR_ALL_WASM_VM_EXPORTS(_GET_FUNCTION)
#undef _GET_FUNCTION

#define _REGISTER_CALLBACK(_T)                                                                     \
  void registerCallback(absl::string_view module_name, absl::string_view function_name, _T,        \
                        typename ConvertFunctionTypeWordToUint32<_T>::type f) override {           \
    registerCallbackWavm(this, module_name, function_name, f);                                     \
  };
  FOR_ALL_WASM_VM_IMPORTS(_REGISTER_CALLBACK)
#undef _REGISTER_CALLBACK

#define _REGISTER_GLOBAL(_T)                                                                       \
  std::unique_ptr<Global<_T>> makeGlobal(absl::string_view module_name, absl::string_view name,    \
                                         _T initial_value) override {                              \
    return makeGlobalWavm(this, module_name, name, initial_value);                                 \
  };
  _REGISTER_GLOBAL(Word);
  _REGISTER_GLOBAL(double);
#undef _REGISTER_GLOBAL

  bool has_instantiated_module_ = false;
  IR::Module ir_module_;
  WAVM::Runtime::ModuleRef module_ = nullptr;
  WAVM::Runtime::GCPointer<WAVM::Runtime::ModuleInstance> module_instance_;
  WAVM::Runtime::Memory* memory_;
  Emscripten::Instance* emscripten_instance_ = nullptr;
  WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment_;
  WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context_;
  absl::node_hash_map<std::string, Intrinsics::Module> intrinsic_modules_;
  absl::node_hash_map<std::string, WAVM::Runtime::GCPointer<WAVM::Runtime::ModuleInstance>>
      intrinsic_module_instances_;
  std::vector<std::unique_ptr<Intrinsics::Function>> envoyFunctions_;
  // The values of this map are owned by the Wasm owning this Wavm.
  std::unordered_map<std::pair<std::string, std::string>, WavmGlobalBase*, PairHash>
      intrinsic_globals_;
  uint8_t* memory_base_ = nullptr;
};

Wavm::~Wavm() {
  module_instance_ = nullptr;
  if (emscripten_instance_) {
    emscripten_instance_->env = nullptr;
    emscripten_instance_->global = nullptr;
    emscripten_instance_->memory = nullptr;
    delete emscripten_instance_;
  }
  context_ = nullptr;
  intrinsic_module_instances_.clear();
  intrinsic_modules_.clear();
  envoyFunctions_.clear();
  if (compartment_) {
    ASSERT(tryCollectCompartment(std::move(compartment_)));
  }
}

std::unique_ptr<WasmVm> Wavm::clone() {
  auto wavm = std::make_unique<Wavm>();
  wavm->compartment_ = WAVM::Runtime::cloneCompartment(compartment_);
  wavm->memory_ = WAVM::Runtime::remapToClonedCompartment(memory_, wavm->compartment_);
  wavm->memory_base_ = WAVM::Runtime::getMemoryBaseAddress(wavm->memory_);
  wavm->context_ = WAVM::Runtime::createContext(wavm->compartment_);
  for (auto& p : intrinsic_module_instances_) {
    wavm->intrinsic_module_instances_.emplace(
        p.first, WAVM::Runtime::remapToClonedCompartment(p.second, wavm->compartment_));
  }
  wavm->module_instance_ =
      WAVM::Runtime::remapToClonedCompartment(module_instance_, wavm->compartment_);
  return wavm;
}

bool Wavm::load(const std::string& code, bool allow_precompiled) {
  ASSERT(!has_instantiated_module_);
  has_instantiated_module_ = true;
  compartment_ = WAVM::Runtime::createCompartment();
  context_ = WAVM::Runtime::createContext(compartment_);
  if (!loadModule(code, ir_module_)) {
    return false;
  }
  // todo check percompiled section is permitted
  const UserSection* precompiled_object_section = nullptr;
  if (allow_precompiled) {
    for (const UserSection& userSection : ir_module_.userSections) {
      if (userSection.name == "wavm.precompiled_object") {
        precompiled_object_section = &userSection;
        break;
      }
    }
  }
  if (!precompiled_object_section) {
    module_ = WAVM::Runtime::compileModule(ir_module_);
  } else {
    module_ = WAVM::Runtime::loadPrecompiledModule(ir_module_, precompiled_object_section->data);
  }
  makeModule("envoy");
  return true;
}

void Wavm::link(absl::string_view debug_name, bool needs_emscripten) {
  RootResolver rootResolver(compartment_);
  for (auto& p : intrinsic_modules_) {
    auto instance = Intrinsics::instantiateModule(compartment_, intrinsic_modules_[p.first],
                                                  std::string(p.first));
    intrinsic_module_instances_.emplace(p.first, instance);
    rootResolver.moduleNameToInstanceMap().set(p.first, instance);
  }
  if (needs_emscripten) {
    emscripten_instance_ = Emscripten::instantiate(compartment_, ir_module_);
    rootResolver.moduleNameToInstanceMap().set("env", emscripten_instance_->env);
  }
  WAVM::Runtime::LinkResult link_result = linkModule(ir_module_, rootResolver);
  module_instance_ = instantiateModule(
      compartment_, module_, std::move(link_result.resolvedImports), std::string(debug_name));
  memory_ = getDefaultMemory(module_instance_);
  memory_base_ = WAVM::Runtime::getMemoryBaseAddress(memory_);
  getInstantiatedGlobals();
}

void Wavm::getInstantiatedGlobals() {
  for (auto& p : intrinsic_globals_) {
    auto o = WAVM::Runtime::getInstanceExport(intrinsic_module_instances_[p.first.first],
                                              p.first.second);
    auto g = WAVM::Runtime::as<WAVM::Runtime::Global>(o);
    if (!g) {
      throw WasmVmException(
          fmt::format("Unable to resolve intrinsic global {} {}", p.first.first, p.first.second));
    }
    p.second->global_ = g;
  }
}

void Wavm::makeModule(absl::string_view name) {
  intrinsic_modules_.emplace(std::piecewise_construct, std::make_tuple(name), std::make_tuple());
}

void Wavm::start(Context* context) {
  try {
    auto f = getStartFunction(module_instance_);
    if (f) {
      CALL_WITH_CONTEXT(invokeFunctionChecked(context_, f, {}), context);
    }

    if (emscripten_instance_) {
      CALL_WITH_CONTEXT(Emscripten::initializeGlobals(emscripten_instance_, context_, ir_module_,
                                                      module_instance_),
                        context);
    }

    f = asFunctionNullable(getInstanceExport(module_instance_, "__post_instantiate"));
    if (f) {
      CALL_WITH_CONTEXT(invokeFunctionChecked(context_, f, {}), context);
    }

    f = asFunctionNullable(getInstanceExport(module_instance_, "main"));
    if (!f) {
      f = asFunctionNullable(getInstanceExport(module_instance_, "_main"));
    }
    if (f) {
      CALL_WITH_CONTEXT(invokeFunctionChecked(context_, f, {}), context);
    }
  } catch (const std::exception& e) {
    std::cerr << "Caught exception \"" << e.what() << "\" in WASM\n";
    throw;
  }
}

uint64_t Wavm::getMemorySize() { return WAVM::Runtime::getMemoryNumPages(memory_) * WasmPageSize; }

absl::optional<absl::string_view> Wavm::getMemory(uint64_t pointer, uint64_t size) {
  auto memory_num_bytes = WAVM::Runtime::getMemoryNumPages(memory_) * WasmPageSize;
  if (pointer + size > memory_num_bytes) {
    return absl::nullopt;
  }
  return absl::string_view(reinterpret_cast<char*>(memory_base_ + pointer), size);
}

bool Wavm::getMemoryOffset(void* host_pointer, uint64_t* vm_pointer) {
  intptr_t offset = (static_cast<uint8_t*>(host_pointer) - memory_base_);
  if (offset < 0) {
    return false;
  }
  if (static_cast<size_t>(offset) > WAVM::Runtime::getMemoryNumPages(memory_) * WasmPageSize) {
    return false;
  }
  *vm_pointer = static_cast<uint64_t>(offset);
  return true;
}

bool Wavm::setMemory(uint64_t pointer, uint64_t size, const void* data) {
  auto memory_num_bytes = WAVM::Runtime::getMemoryNumPages(memory_) * WasmPageSize;
  if (pointer + size > memory_num_bytes) {
    return false;
  }
  auto p = reinterpret_cast<char*>(memory_base_ + pointer);
  memcpy(p, data, size);
  return true;
}

bool Wavm::setWord(uint64_t pointer, Word data) {
  uint32_t data32 = data.u32();
  return setMemory(pointer, sizeof(uint32_t), &data32);
}

absl::string_view Wavm::getUserSection(absl::string_view name) {
  for (auto& section : ir_module_.userSections) {
    if (section.name == name) {
      return {reinterpret_cast<char*>(section.data.data()), section.data.size()};
    }
  }
  return {};
}

std::unique_ptr<WasmVm> createVm() { return std::make_unique<Wavm>(); }

} // namespace Wavm

template <typename R, typename... Args>
IR::FunctionType inferEnvoyFunctionType(R (*)(void*, Args...)) {
  return IR::FunctionType(IR::inferResultType<R>(), IR::TypeTuple({IR::inferValueType<Args>()...}));
}

using namespace Wavm;

template <typename R, typename... Args>
void registerCallbackWavm(WasmVm* vm, absl::string_view module_name,
                          absl::string_view function_name, R (*f)(Args...)) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  wavm->envoyFunctions_.emplace_back(new Intrinsics::Function(
      wavm->intrinsic_modules_[module_name], function_name.data(), reinterpret_cast<void*>(f),
      inferEnvoyFunctionType(f), IR::CallingConvention::intrinsic));
}

template void registerCallbackWavm<void, void*>(WasmVm* vm, absl::string_view module_name,
                                                absl::string_view function_name, void (*f)(void*));
template void registerCallbackWavm<void, void*, U32>(WasmVm* vm, absl::string_view module_name,
                                                     absl::string_view function_name,
                                                     void (*f)(void*, U32));
template void registerCallbackWavm<void, void*, U32, U32>(WasmVm* vm, absl::string_view module_name,
                                                          absl::string_view function_name,
                                                          void (*f)(void*, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32>(WasmVm* vm,
                                                               absl::string_view module_name,
                                                               absl::string_view function_name,
                                                               void (*f)(void*, U32, U32, U32));
template void
registerCallbackWavm<void, void*, U32, U32, U32, U32>(WasmVm* vm, absl::string_view module_name,
                                                      absl::string_view function_name,
                                                      void (*f)(void*, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    void (*f)(void*, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    void (*f)(void*, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<void, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    void (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32));

template void registerCallbackWavm<U32, void*>(WasmVm* vm, absl::string_view module_name,
                                               absl::string_view function_name, U32 (*f)(void*));
template void registerCallbackWavm<U32, void*, U32>(WasmVm* vm, absl::string_view module_name,
                                                    absl::string_view function_name,
                                                    U32 (*f)(void*, U32));
template void registerCallbackWavm<U32, void*, U32, U32>(WasmVm* vm, absl::string_view module_name,
                                                         absl::string_view function_name,
                                                         U32 (*f)(void*, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32>(WasmVm* vm,
                                                              absl::string_view module_name,
                                                              absl::string_view function_name,
                                                              U32 (*f)(void*, U32, U32, U32));
template void
registerCallbackWavm<U32, void*, U32, U32, U32, U32>(WasmVm* vm, absl::string_view module_name,
                                                     absl::string_view function_name,
                                                     U32 (*f)(void*, U32, U32, U32, U32));
template void
registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32>(WasmVm* vm, absl::string_view module_name,
                                                          absl::string_view function_name,
                                                          U32 (*f)(void*, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32));
template void registerCallbackWavm<U32, void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32>(
    WasmVm* vm, absl::string_view module_name, absl::string_view function_name,
    U32 (*f)(void*, U32, U32, U32, U32, U32, U32, U32, U32, U32, U32));

template void registerCallbackWavm<U64, void*, U32>(WasmVm* vm, absl::string_view module_name,
                                                    absl::string_view function_name,
                                                    U64 (*f)(void*, U32));
template void registerCallbackWavm<void, void*, U32, I64>(WasmVm* vm, absl::string_view module_name,
                                                          absl::string_view function_name,
                                                          void (*f)(void*, U32, I64));
template void registerCallbackWavm<void, void*, U32, U64>(WasmVm* vm, absl::string_view module_name,
                                                          absl::string_view function_name,
                                                          void (*f)(void*, U32, U64));

template <typename R, typename... Args>
IR::FunctionType
inferStdFunctionType(std::function<R(Envoy::Extensions::Common::Wasm::Context*, Args...)>*) {
  return IR::FunctionType(IR::inferResultType<R>(), IR::TypeTuple({IR::inferValueType<Args>()...}));
}

static bool checkFunctionType(WAVM::Runtime::Function* f, IR::FunctionType t) {
  return getFunctionType(f) == t;
}

template <typename R, typename... Args>
void getFunctionWavmReturn(WasmVm* vm, absl::string_view function_name,
                           std::function<R(Context*, Args...)>* function, uint32_t) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  auto f =
      asFunctionNullable(getInstanceExport(wavm->module_instance_, std::string(function_name)));
  if (!f)
    f = asFunctionNullable(getInstanceExport(wavm->module_instance_, std::string(function_name)));
  if (!f) {
    *function = nullptr;
    return;
  }
  if (!checkFunctionType(f, inferStdFunctionType(function))) {
    throw WasmVmException(fmt::format("Bad function signature for: {}", function_name));
  }
  *function = [wavm, f](Context* context, Args... args) -> R {
    WasmUntaggedValue values[] = {args...};
    try {
      CALL_WITH_CONTEXT_RETURN(invokeFunctionUnchecked(wavm->context_, f, &values[0]), context,
                               uint32_t, i32);
    } catch (const std::exception& e) {
      std::cerr << "Caught exception \"" << e.what() << "\" in WASM\n";
      throw;
    }
  };
}

struct Void {};

template <typename R, typename... Args>
void getFunctionWavmReturn(WasmVm* vm, absl::string_view function_name,
                           std::function<R(Context*, Args...)>* function, Void) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  auto f =
      asFunctionNullable(getInstanceExport(wavm->module_instance_, std::string(function_name)));
  if (!f)
    f = asFunctionNullable(getInstanceExport(wavm->module_instance_, std::string(function_name)));
  if (!f) {
    *function = nullptr;
    return;
  }
  if (!checkFunctionType(f, inferStdFunctionType(function))) {
    throw WasmVmException(fmt::format("Bad function signature for: {}", function_name));
  }
  *function = [wavm, f](Context* context, Args... args) -> R {
    WasmUntaggedValue values[] = {args...};
    try {
      CALL_WITH_CONTEXT(invokeFunctionUnchecked(wavm->context_, f, &values[0]), context);
    } catch (const std::exception& e) {
      std::cerr << "Caught exception \"" << e.what() << "\" in WASM\n";
      throw;
    }
  };
}

// NB: Unfortunately 'void' is not treated like every other function type in C++. In
// particular it is not possible to specialize a template based on 'void'. Instead
// we use 'Void' for template matching. Note that the template implementation above
// which matchers on 'bool' does not use 'Void' in the implemenation.
template <typename R, typename... Args>
void getFunctionWavm(WasmVm* vm, absl::string_view function_name,
                     std::function<R(Context*, Args...)>* function) {
  typename std::conditional<std::is_void<R>::value, Void, uint32_t>::type x{};
  getFunctionWavmReturn(vm, function_name, function, x);
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

template <typename T> T getValue(IR::Value) {}
template <> Word getValue(IR::Value v) { return v.u32; }
template <> int32_t getValue(IR::Value v) { return v.i32; }
template <> uint32_t getValue(IR::Value v) { return v.u32; }
template <> int64_t getValue(IR::Value v) { return v.i64; }
template <> uint64_t getValue(IR::Value v) { return v.u64; }
template <> float getValue(IR::Value v) { return v.f32; }
template <> double getValue(IR::Value v) { return v.f64; }

template <typename T> T WavmGlobal<T>::get() {
  return getValue<T>(getGlobalValue(wavm_->context_, global_));
}

template <typename T> void WavmGlobal<T>::set(const T& t) {
  setGlobalValue(wavm_->context_, global_, IR::Value(t));
}

template <typename T>
std::unique_ptr<Global<T>> makeGlobalWavm(WasmVm* vm, absl::string_view module_name,
                                          absl::string_view name, T initial_value) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  auto g = std::make_unique<WavmGlobal<T>>(wavm, wavm->intrinsic_modules_[module_name],
                                           std::string(name), initial_value);
  wavm->intrinsic_globals_[std::make_pair(std::string(module_name), std::string(name))] = g.get();
  return g;
}

template std::unique_ptr<Global<double>> makeGlobalWavm(WasmVm* vm, absl::string_view module_name,
                                                        absl::string_view name,
                                                        double initial_value);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
