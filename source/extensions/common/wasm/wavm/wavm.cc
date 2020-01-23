#include "extensions/common/wasm/wavm/wavm.h"

#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/common/assert.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/wasm_vm_base.h"
#include "extensions/common/wasm/well_known_names.h"

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
#include "WAVM/Platform/Mutex.h"
#include "WAVM/Platform/Thread.h"
#include "WAVM/Runtime/Intrinsics.h"
#include "WAVM/Runtime/Linker.h"
#include "WAVM/Runtime/Runtime.h"
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

namespace Wavm {

struct Wavm;

namespace {

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

struct WasmUntaggedValue : public WAVM::IR::UntaggedValue {
  WasmUntaggedValue() = default;
  WasmUntaggedValue(I32 inI32) { i32 = inI32; }
  WasmUntaggedValue(I64 inI64) { i64 = inI64; }
  WasmUntaggedValue(U32 inU32) { u32 = inU32; }
  WasmUntaggedValue(Word w) { u32 = static_cast<U32>(w.u64_); }
  WasmUntaggedValue(U64 inU64) { u64 = inU64; }
  WasmUntaggedValue(F32 inF32) { f32 = inF32; }
  WasmUntaggedValue(F64 inF64) { f64 = inF64; }
};

using Context = Common::Wasm::Context; // Shadowing WAVM::Runtime::Context.

const Logger::Id wasmId = Logger::Id::wasm;

class RootResolver : public WAVM::Runtime::Resolver, public Logger::Loggable<wasmId> {
public:
  RootResolver(WAVM::Runtime::Compartment*) {}

  virtual ~RootResolver() { module_name_to_instance_map_.clear(); }

  bool resolve(const std::string& module_name, const std::string& export_name, ExternType type,
               WAVM::Runtime::Object*& out_object) override {
    auto named_instance = module_name_to_instance_map_.get(module_name);
    if (named_instance) {
      out_object = getInstanceExport(*named_instance, export_name);
      if (out_object) {
        if (isA(out_object, type)) {
          return true;
        } else {
          throw WasmVmException(fmt::format(
              "Failed to load WASM module due to a type mismatch in an import: {}.{} {}, "
              "but was expecting type: {}",
              module_name, export_name, asString(WAVM::Runtime::getExternType(out_object)),
              asString(type)));
        }
      }
    }
    for (auto r : resolvers_) {
      if (r->resolve(module_name, export_name, type, out_object)) {
        return true;
      }
    }
    throw WasmVmException(
        fmt::format("Failed to load WASM module due to a missing import: {}.{} {}", module_name,
                    export_name, asString(type)));
  }

  HashMap<std::string, WAVM::Runtime::ModuleInstance*>& moduleNameToInstanceMap() {
    return module_name_to_instance_map_;
  }

  void addResolver(WAVM::Runtime::Resolver* r) { resolvers_.push_back(r); }

private:
  HashMap<std::string, WAVM::Runtime::ModuleInstance*> module_name_to_instance_map_;
  std::vector<WAVM::Runtime::Resolver*> resolvers_;
};

const uint64_t WasmPageSize = 1 << 16;

bool loadModule(const std::string& code, IR::Module& out_module) {
  // If the code starts with the WASM binary magic number, load it as a binary IR::Module.
  static const uint8_t WasmMagicNumber[4] = {0x00, 0x61, 0x73, 0x6d};
  if (code.size() >= 4 && !memcmp(code.data(), WasmMagicNumber, 4)) {
    return WASM::loadBinaryModule(reinterpret_cast<const unsigned char*>(code.data()), code.size(),
                                  out_module);
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

template <typename T> struct NativeWord { using type = T; };
template <> struct NativeWord<Word> { using type = uint32_t; };

template <typename T> typename NativeWord<T>::type ToNative(const T& t) { return t; }
template <> typename NativeWord<Word>::type ToNative(const Word& t) { return t.u32(); }

struct PairHash {
  template <typename T, typename U> std::size_t operator()(const std::pair<T, U>& x) const {
    return std::hash<T>()(x.first) + std::hash<U>()(x.second);
  }
};

struct Wavm : public WasmVmBase {
  Wavm(Stats::ScopeSharedPtr scope) : WasmVmBase(scope, WasmRuntimeNames::get().Wavm) {}
  ~Wavm() override;

  // WasmVm
  absl::string_view runtime() override { return WasmRuntimeNames::get().Wavm; }
  Cloneable cloneable() override { return Cloneable::InstantiatedModule; };
  std::unique_ptr<WasmVm> clone() override;
  bool load(const std::string& code, bool allow_precompiled) override;
  void link(absl::string_view debug_name) override;
  uint64_t getMemorySize() override;
  absl::optional<absl::string_view> getMemory(uint64_t pointer, uint64_t size) override;
  bool setMemory(uint64_t pointer, uint64_t size, const void* data) override;
  bool getWord(uint64_t pointer, Word* data) override;
  bool setWord(uint64_t pointer, Word data) override;
  absl::string_view getCustomSection(absl::string_view name) override;
  absl::string_view getPrecompiledSectionName() override;

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

  bool has_instantiated_module_ = false;
  IR::Module ir_module_;
  WAVM::Runtime::ModuleRef module_ = nullptr;
  WAVM::Runtime::GCPointer<WAVM::Runtime::ModuleInstance> module_instance_;
  WAVM::Runtime::Memory* memory_;
  WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment_;
  WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context_;
  absl::node_hash_map<std::string, Intrinsics::Module> intrinsic_modules_;
  absl::node_hash_map<std::string, WAVM::Runtime::GCPointer<WAVM::Runtime::ModuleInstance>>
      intrinsic_module_instances_;
  std::vector<std::unique_ptr<Intrinsics::Function>> envoyFunctions_;
  uint8_t* memory_base_ = nullptr;
};

Wavm::~Wavm() {
  module_instance_ = nullptr;
  context_ = nullptr;
  intrinsic_module_instances_.clear();
  intrinsic_modules_.clear();
  envoyFunctions_.clear();
  if (compartment_) {
    ASSERT(tryCollectCompartment(std::move(compartment_)));
  }
}

std::unique_ptr<WasmVm> Wavm::clone() {
  auto wavm = std::make_unique<Wavm>(scope_);
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
  const CustomSection* precompiled_object_section = nullptr;
  if (allow_precompiled) {
    for (const CustomSection& customSection : ir_module_.customSections) {
      if (customSection.name == getPrecompiledSectionName()) {
        precompiled_object_section = &customSection;
        break;
      }
    }
  }
  if (!precompiled_object_section) {
    module_ = WAVM::Runtime::compileModule(ir_module_);
  } else {
    module_ = WAVM::Runtime::loadPrecompiledModule(ir_module_, precompiled_object_section->data);
  }
  return true;
}

void Wavm::link(absl::string_view debug_name) {
  RootResolver rootResolver(compartment_);
  for (auto& p : intrinsic_modules_) {
    auto instance = Intrinsics::instantiateModule(compartment_, {&intrinsic_modules_[p.first]},
                                                  std::string(p.first));
    intrinsic_module_instances_.emplace(p.first, instance);
    rootResolver.moduleNameToInstanceMap().set(p.first, instance);
  }
  WAVM::Runtime::LinkResult link_result = linkModule(ir_module_, rootResolver);
  module_instance_ = instantiateModule(
      compartment_, module_, std::move(link_result.resolvedImports), std::string(debug_name));
  memory_ = getDefaultMemory(module_instance_);
  memory_base_ = WAVM::Runtime::getMemoryBaseAddress(memory_);
}

uint64_t Wavm::getMemorySize() { return WAVM::Runtime::getMemoryNumPages(memory_) * WasmPageSize; }

absl::optional<absl::string_view> Wavm::getMemory(uint64_t pointer, uint64_t size) {
  auto memory_num_bytes = WAVM::Runtime::getMemoryNumPages(memory_) * WasmPageSize;
  if (pointer + size > memory_num_bytes) {
    return absl::nullopt;
  }
  return absl::string_view(reinterpret_cast<char*>(memory_base_ + pointer), size);
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

bool Wavm::getWord(uint64_t pointer, Word* data) {
  auto memory_num_bytes = WAVM::Runtime::getMemoryNumPages(memory_) * WasmPageSize;
  if (pointer + sizeof(uint32_t) > memory_num_bytes) {
    return false;
  }
  auto p = reinterpret_cast<char*>(memory_base_ + pointer);
  uint32_t data32;
  memcpy(&data32, p, sizeof(uint32_t));
  data->u64_ = data32;
  return true;
}

bool Wavm::setWord(uint64_t pointer, Word data) {
  uint32_t data32 = data.u32();
  return setMemory(pointer, sizeof(uint32_t), &data32);
}

absl::string_view Wavm::getCustomSection(absl::string_view name) {
  for (auto& section : ir_module_.customSections) {
    if (section.name == name) {
      return {reinterpret_cast<char*>(section.data.data()), section.data.size()};
    }
  }
  return {};
}

absl::string_view Wavm::getPrecompiledSectionName() { return "wavm.precompiled_object"; }

std::unique_ptr<WasmVm> createVm(Stats::ScopeSharedPtr scope) {
  return std::make_unique<Wavm>(scope);
}

} // namespace Wavm

template <typename R, typename... Args>
IR::FunctionType inferEnvoyFunctionType(R (*)(void*, Args...)) {
  return IR::FunctionType(IR::inferResultType<R>(), IR::TypeTuple({IR::inferValueType<Args>()...}),
                          IR::CallingConvention::intrinsic);
}

using namespace Wavm;

template <typename R, typename... Args>
void registerCallbackWavm(WasmVm* vm, absl::string_view module_name,
                          absl::string_view function_name, R (*f)(Args...)) {
  auto wavm = static_cast<Common::Wasm::Wavm::Wavm*>(vm);
  wavm->envoyFunctions_.emplace_back(
      new Intrinsics::Function(&wavm->intrinsic_modules_[module_name], function_name.data(),
                               reinterpret_cast<void*>(f), inferEnvoyFunctionType(f)));
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
  *function = [wavm, f, function_name](Context* context, Args... args) -> R {
    WasmUntaggedValue values[] = {args...};
    WasmUntaggedValue return_value;
    try {
      CALL_WITH_CONTEXT(
          invokeFunction(wavm->context_, f, getFunctionType(f), &values[0], &return_value),
          context);
      return static_cast<uint32_t>(return_value.i32);
    } catch (const std::exception& e) {
      throw WasmException(fmt::format("Function: {} failed: {}", function_name, e.what()));
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
  *function = [wavm, f, function_name](Context* context, Args... args) -> R {
    WasmUntaggedValue values[] = {args...};
    try {
      CALL_WITH_CONTEXT(invokeFunction(wavm->context_, f, getFunctionType(f), &values[0]), context);
    } catch (const std::exception& e) {
      throw WasmException(fmt::format("Function: {} failed: {}", function_name, e.what()));
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

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
