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

using namespace WAVM;
using namespace WAVM::IR;
using namespace WAVM::Runtime;

DECLARE_INTRINSIC_MODULE(env);

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Wavm {
namespace {

const Logger::Id wasmId = Logger::Id::wasm;

std::string readFile(const std::string& filename) {
  std::ifstream file(filename);
  if (file.fail())
    return "";
  std::stringstream file_string_stream;
  file_string_stream << file.rdbuf();
  return file_string_stream.str();
}

class RootResolver : public Resolver, public Logger::Loggable<wasmId> {
public:
  RootResolver(Compartment* compartment) : compartment_(compartment) {}
  virtual ~RootResolver() { moduleNameToInstanceMap_.clear(); }

  bool resolve(const std::string& moduleName, const std::string& exportName, ExternType type,
               Object*& outObject) override {
    auto namedInstance = moduleNameToInstanceMap_.get(moduleName);
    if (namedInstance) {
      outObject = getInstanceExport(*namedInstance, exportName);
      if (outObject) {
        if (isA(outObject, type)) {
          return true;
        } else {
          ENVOY_LOG(warn, "Resolved import {}.{} to a {}, but was expecting {}", moduleName,
                    exportName, asString(getObjectType(outObject)), asString(type));
          return false;
        }
      }
    }

    ENVOY_LOG(error, "Generated stub for missing import {}.{} : {}", moduleName, exportName,
              asString(type));
    outObject = getStubObject(exportName, type);
    return true;
  }

  Object* getStubObject(const std::string& exportName, ExternType type) const {
    switch (type.kind) {
    case IR::ExternKind::function: {
      // Generate a function body that just uses the unreachable op to fault if called.
      Serialization::ArrayOutputStream codeStream;
      OperatorEncoderStream encoder(codeStream);
      encoder.unreachable();
      encoder.end();

      // Generate a module for the stub function.
      IR::Module stubIRModule;
      DisassemblyNames stubModuleNames;
      stubIRModule.types.push_back(asFunctionType(type));
      stubIRModule.functions.defs.push_back({{0}, {}, std::move(codeStream.getBytes()), {}});
      stubIRModule.exports.push_back({"importStub", IR::ExternKind::function, 0});
      stubModuleNames.functions.push_back({"importStub: " + exportName, {}, {}});
      IR::setDisassemblyNames(stubIRModule, stubModuleNames);
      IR::validatePreCodeSections(stubIRModule);
      DeferredCodeValidationState deferredCodeValidationState;
      IR::validatePostCodeSections(stubIRModule, deferredCodeValidationState);

      // Instantiate the module and return the stub function instance.
      auto stubModule = compileModule(stubIRModule);
      auto stubModuleInstance = instantiateModule(compartment_, stubModule, {}, "importStub");
      return getInstanceExport(stubModuleInstance, "importStub");
    }
    case IR::ExternKind::memory:
      return asObject(
          Runtime::createMemory(compartment_, asMemoryType(type), std::string(exportName)));
    case IR::ExternKind::table:
      return asObject(
          Runtime::createTable(compartment_, asTableType(type), std::string(exportName)));
    case IR::ExternKind::global:
      return asObject(
          Runtime::createGlobal(compartment_, asGlobalType(type),
                                IR::Value(asGlobalType(type).valueType, IR::UntaggedValue())));
    case IR::ExternKind::exceptionType:
      return asObject(
          Runtime::createExceptionType(compartment_, asExceptionType(type), "importStub"));
    default:
      Errors::unreachable();
    };
  }

  HashMap<std::string, ModuleInstance*>& moduleNameToInstanceMap() {
    return moduleNameToInstanceMap_;
  }

private:
  Compartment* const compartment_;
  HashMap<std::string, ModuleInstance*> moduleNameToInstanceMap_;
};

static bool loadModule(const std::string& filename, IR::Module& outModule) {
  auto bytes = readFile(filename);
  if (bytes.empty())
    return false;

  // If the file starts with the WASM binary magic number, load it as a binary irModule.
  static const uint8_t wasmMagicNumber[4] = {0x00, 0x61, 0x73, 0x6d};
  if (bytes.size() >= 4 && !memcmp(bytes.c_str(), wasmMagicNumber, 4)) {
    return WASM::loadBinaryModule(bytes.c_str(), bytes.size(), outModule);
  } else {
    // Load it as a text irModule.
    std::vector<WAST::Error> parseErrors;
    if (!WAST::parseModule(bytes.c_str(), bytes.size(), outModule, parseErrors)) {
      return false;
    }
    return true;
  }
}

} // namespace

class WavmCallbacks : public Server::Wasm::WasmCallbacks, public Logger::Loggable<wasmId> {
public:
  ~WavmCallbacks() override {}
  void scriptLog(spdlog::level::level_enum level, const std::string& message) override;
};

class Wavm : public Server::Wasm, public Logger::Loggable<wasmId> {
public:
  Wavm();
  ~Wavm() override;
  // Server::Wasm()
  bool initialize(const std::string& file, bool allow_precompiled) override;
  void configure(const std::string& configuration) override;
  void start(Event::Dispatcher& dispatcher) override;
  void tick() override;

  void setWasmCallbacks(WasmCallbacks& callbacks) override { wasm_callbacks_ = &callbacks; }

  WasmCallbacks& callbacks() { return *wasm_callbacks_; }
  Memory* memory() { return memory_; }

private:
  bool hasInstantiatedModule_ = false;
  IR::Module irModule_;
  GCPointer<ModuleInstance> moduleInstance_;
  Memory* memory_;
  Emscripten::Instance* emscriptenInstance_ = nullptr;
  GCPointer<Compartment> compartment_;
  GCPointer<Context> context_;
  std::vector<WAST::Error> errors_;
  WavmCallbacks callbacks_;
  WasmCallbacks* wasm_callbacks_;
};

void WavmCallbacks::scriptLog(spdlog::level::level_enum level, const std::string& message) {
  switch (level) {
  case spdlog::level::trace:
    ENVOY_LOG(trace, "wasm log: {}", message);
    return;
  case spdlog::level::debug:
    ENVOY_LOG(debug, "wasm log: {}", message);
    return;
  case spdlog::level::info:
    ENVOY_LOG(info, "wasm log: {}", message);
    return;
  case spdlog::level::warn:
    ENVOY_LOG(warn, "wasm log: {}", message);
    return;
  case spdlog::level::err:
    ENVOY_LOG(error, "wasm log: {}", message);
    return;
  case spdlog::level::critical:
    ENVOY_LOG(critical, "wasm log: {}", message);
    return;
  case spdlog::level::off:
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
}

std::unique_ptr<Server::Wasm> createWavm() { return std::make_unique<Wavm>(); }

Wavm::Wavm() : wasm_callbacks_(&callbacks_) {}

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
  ASSERT(tryCollectCompartment(std::move(compartment_)));
}

bool Wavm::initialize(const std::string& wasm_file, bool allow_precompiled) {
  ASSERT(!hasInstantiatedModule_);
  hasInstantiatedModule_ = true;
  compartment_ = Runtime::createCompartment();
  context_ = Runtime::createContext(compartment_);
  setUserData(context_, this);
  if (!loadModule(wasm_file, irModule_)) {
    return false;
  }
  Runtime::ModuleRef module = nullptr;
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
    module = Runtime::compileModule(irModule_);
  } else {
    module = Runtime::loadPrecompiledModule(irModule_, precompiledObjectSection->data);
  }
  RootResolver rootResolver(compartment_);
  // todo make this optional
  if (true) {
    emscriptenInstance_ = Emscripten::instantiate(compartment_, irModule_);
    rootResolver.moduleNameToInstanceMap().set("env", emscriptenInstance_->env);
    rootResolver.moduleNameToInstanceMap().set("asm2wasm", emscriptenInstance_->asm2wasm);
    rootResolver.moduleNameToInstanceMap().set("global", emscriptenInstance_->global);
  }
  LinkResult linkResult = linkModule(irModule_, rootResolver);
  moduleInstance_ = instantiateModule(compartment_, module, std::move(linkResult.resolvedImports),
                                      std::string(wasm_file));
  memory_ = getDefaultMemory(moduleInstance_);
  auto f = getStartFunction(moduleInstance_);
  if (f) {
    invokeFunctionChecked(context_, f, {});
  }
  return true;
}

void Wavm::configure(const std::string& configuration_file) {
  if (configuration_file.empty())
    return;
  auto f = asFunctionNullable(getInstanceExport(moduleInstance_, "configure"));
  if (f) {
    auto configuration = readFile(configuration_file);
    (void)configuration;
    invokeFunctionChecked(context_, f, {});
  }
}

void Wavm::start(Event::Dispatcher&) {
  auto f = asFunctionNullable(getInstanceExport(moduleInstance_, "main"));
  if (!f)
    f = asFunctionNullable(getInstanceExport(moduleInstance_, "_main"));
  if (f) {
    invokeFunctionChecked(context_, f, {});
  }
}

void Wavm::tick() {
  auto f = asFunctionNullable(getInstanceExport(moduleInstance_, "tick"));
  if (f) {
    invokeFunctionChecked(context_, f, {});
  }
}

DEFINE_INTRINSIC_FUNCTION(env, "_wasmLog", void, _wasmLog, U32 logLevel, U32 logMessage,
                          U32 messageSize) {
  auto wavm = static_cast<Wavm*>(getUserData(getContextFromRuntimeData(contextRuntimeData)));
  wavm->callbacks().scriptLog(
      static_cast<spdlog::level::level_enum>(logLevel),
      std::string(reinterpret_cast<char*>(memoryArrayPtr<U8>(wavm->memory(), logMessage,
                                                             static_cast<U64>(messageSize))),
                  static_cast<size_t>(messageSize)));
}

} // namespace Wavm
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
