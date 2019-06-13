#include "extensions/common/wasm/null/null.h"

#include <stdint.h>
#include <stdio.h>

#include <atomic>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "envoy/common/exception.h"
#include "envoy/server/wasm.h"

#include "common/common/assert.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/common/wasm/well_known_names.h"

#include "absl/container/node_hash_map.h"
#include "absl/strings/match.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

extern thread_local Envoy::Extensions::Common::Wasm::Context* current_context_;

namespace Null {

namespace {

struct SaveRestoreContext {
  explicit SaveRestoreContext(Context* context) {
    saved_context = current_context_;
    current_context_ = context;
  }
  ~SaveRestoreContext() { current_context_ = saved_context; }
  Context* saved_context;
};

} // namespace

using Plugin::Context;
using Plugin::WasmData;

struct NullVm : public WasmVm {
  NullVm() = default;
  NullVm(const NullVm& other) : plugin_(std::make_unique<NullVmPlugin>(*other.plugin_)) {}
  ~NullVm() override;

  // WasmVm
  absl::string_view vm() override { return WasmVmNames::get().Null; }
  bool clonable() override { return true; };
  std::unique_ptr<WasmVm> clone() override;
  bool load(const std::string& code, bool allow_precompiled) override;
  void link(absl::string_view debug_name, bool needs_emscripten) override;
  void setMemoryLayout(uint64_t, uint64_t, uint64_t) override {}
  void start(Common::Wasm::Context* context) override;
  uint64_t getMemorySize() override;
  absl::string_view getMemory(uint64_t pointer, uint64_t size) override;
  bool getMemoryOffset(void* host_pointer, uint64_t* vm_pointer) override;
  bool setMemory(uint64_t pointer, uint64_t size, void* data) override;
  bool setWord(uint64_t pointer, uint64_t data) override;
  void makeModule(absl::string_view name) override;
  absl::string_view getUserSection(absl::string_view name) override;

  void getFunction(absl::string_view functionName, WasmCall0Void* f) override;
  void getFunction(absl::string_view functionName, WasmCall1Void* f) override;
  void getFunction(absl::string_view functionName, WasmCall2Void* f) override;
  void getFunction(absl::string_view functionName, WasmCall3Void* f) override;
  void getFunction(absl::string_view functionName, WasmCall4Void* f) override;
  void getFunction(absl::string_view functionName, WasmCall5Void* f) override;
  void getFunction(absl::string_view functionName, WasmCall8Void* f) override;
  void getFunction(absl::string_view functionName, WasmCall1Int* f) override;
  void getFunction(absl::string_view functionName, WasmCall3Int* f) override;

  // These are noops for NullVm.
#define _REGISTER_CALLBACK(_type)                                                                  \
  void registerCallback(absl::string_view, absl::string_view, _type,                               \
                        typename ConvertFunctionTypeWordToUint32<_type>::type) override{};
  _REGISTER_CALLBACK(WasmCallback0Void);
  _REGISTER_CALLBACK(WasmCallback1Void);
  _REGISTER_CALLBACK(WasmCallback2Void);
  _REGISTER_CALLBACK(WasmCallback3Void);
  _REGISTER_CALLBACK(WasmCallback4Void);
  _REGISTER_CALLBACK(WasmCallback5Void);
  _REGISTER_CALLBACK(WasmCallback0Int);
  _REGISTER_CALLBACK(WasmCallback1Int);
  _REGISTER_CALLBACK(WasmCallback2Int);
  _REGISTER_CALLBACK(WasmCallback3Int);
  _REGISTER_CALLBACK(WasmCallback4Int);
  _REGISTER_CALLBACK(WasmCallback5Int);
  _REGISTER_CALLBACK(WasmCallback6Int);
  _REGISTER_CALLBACK(WasmCallback7Int);
  _REGISTER_CALLBACK(WasmCallback8Int);
  _REGISTER_CALLBACK(WasmCallback9Int);
  _REGISTER_CALLBACK(WasmCallback_ZWl);
  _REGISTER_CALLBACK(WasmCallback_ZWm);
  _REGISTER_CALLBACK(WasmCallback_m);
  _REGISTER_CALLBACK(WasmCallback_mW);
#undef _REGISTER_CALLBACK

  // NullVm does not advertize code as emscripten so this will not get called.
  std::unique_ptr<Global<double>> makeGlobal(absl::string_view, absl::string_view,
                                             double) override {
    NOT_REACHED_GCOVR_EXCL_LINE;
  };
  std::unique_ptr<Global<Word>> makeGlobal(absl::string_view, absl::string_view, Word) override {
    NOT_REACHED_GCOVR_EXCL_LINE;
  };

  std::unique_ptr<NullVmPlugin> plugin_;
};

NullVm::~NullVm() {}

std::unique_ptr<WasmVm> NullVm::clone() { return std::make_unique<NullVm>(*this); }

bool NullVm::load(const std::string& name, bool /* allow_precompiled */) {
  auto factory = Registry::FactoryRegistry<NullVmPluginFactory>::getFactory(name);
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

bool NullVm::setMemory(uint64_t pointer, uint64_t size, void* data) {
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

std::unique_ptr<WasmVm> createVm() { return std::make_unique<NullVm>(); }

void NullVm::getFunction(absl::string_view functionName, WasmCall0Void* f) {
  if (functionName == "_proxy_onStart") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context) {
      SaveRestoreContext saved_context(context);
      plugin->onStart();
    };
  } else if (functionName == "_proxy_onTick") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context) {
      SaveRestoreContext saved_context(context);
      plugin->onTick();
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

void NullVm::getFunction(absl::string_view functionName, WasmCall1Void* f) {
  if (functionName == "_free") {
    *f = [](Common::Wasm::Context*, Word ptr) { return ::free(reinterpret_cast<void*>(ptr.u64)); };
  } else if (functionName == "_proxy_onCreate") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onCreate(context_id.u64);
    };
  } else if (functionName == "_proxy_onDone") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onDone(context_id.u64);
    };
  } else if (functionName == "_proxy_onLog") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onLog(context_id.u64);
    };
  } else if (functionName == "_proxy_onDelete") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onDelete(context_id.u64);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

void NullVm::getFunction(absl::string_view functionName, WasmCall2Void* f) {
  if (functionName == "_proxy_onConfigure") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word ptr, Word size) {
      SaveRestoreContext saved_context(context);
      plugin->onConfigure(ptr.u64, size.u64);
    };
  } else if (functionName == "_proxy_onGrpcCreateInitialMetadata") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcCreateInitialMetadata(context_id.u64, token.u64);
    };
  } else if (functionName == "_proxy_onGrpcReceiveInitialMetadata") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceiveInitialMetadata(context_id.u64, token.u64);
    };
  } else if (functionName == "_proxy_onGrpcReceiveTrailingMetadata") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceiveTrailingMetadata(context_id.u64, token.u64);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

void NullVm::getFunction(absl::string_view functionName, WasmCall3Void* /* f */) {
  throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
}

void NullVm::getFunction(absl::string_view functionName, WasmCall4Void* f) {
  if (functionName == "_proxy_onGrpcReceive") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word response_ptr,
                  Word response_size) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceive(context_id.u64, token.u64, response_ptr.u64, response_size.u64);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

void NullVm::getFunction(absl::string_view functionName, WasmCall5Void* f) {
  if (functionName == "_proxy_onGrpcClose") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word status_code,
                  Word status_message_ptr, Word status_message_size) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcClose(context_id.u64, token.u64, status_code.u64, status_message_ptr.u64,
                          status_message_size.u64);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

void NullVm::getFunction(absl::string_view functionName, WasmCall8Void* f) {
  if (functionName == "_proxy_onHttpCallResponse") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token,
                  Word header_pairs_ptr, Word header_pairs_size, Word body_ptr, Word body_size,
                  Word trailer_pairs_ptr, Word trailer_pairs_size) {
      SaveRestoreContext saved_context(context);
      plugin->onHttpCallResponse(context_id.u64, token.u64, header_pairs_ptr.u64,
                                 header_pairs_size.u64, body_ptr.u64, body_size.u64,
                                 trailer_pairs_ptr.u64, trailer_pairs_size.u64);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

void NullVm::getFunction(absl::string_view functionName, WasmCall1Int* f) {
  if (functionName == "_malloc") {
    *f = [](Common::Wasm::Context*, Word size) -> Word {
      return Word(reinterpret_cast<uint64_t>(::malloc(size.u64)));
    };
  } else if (functionName == "_proxy_onRequestHeaders") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestHeaders(context_id.u64));
    };
  } else if (functionName == "_proxy_onRequestTrailers") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestTrailers(context_id.u64));
    };
  } else if (functionName == "_proxy_onRequestMetadata") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestMetadata(context_id.u64));
    };
  } else if (functionName == "_proxy_onResponseHeaders") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseHeaders(context_id.u64));
    };
  } else if (functionName == "_proxy_onResponseTrailers") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseTrailers(context_id.u64));
    };
  } else if (functionName == "_proxy_onResponseMetadata") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseMetadata(context_id.u64));
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

void NullVm::getFunction(absl::string_view functionName, WasmCall3Int* f) {
  if (functionName == "_proxy_onRequestBody") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestBody(context_id.u64, body_buffer_length.u64, end_of_stream.u64));
    };
  } else if (functionName == "_proxy_onResponseBody") {
    auto plugin = plugin_.get();
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onResponseBody(context_id.u64, body_buffer_length.u64, end_of_stream.u64));
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", functionName));
  }
}

Context* NullVmPlugin::ensureContext(uint64_t context_id) {
  auto e = context_map_.insert(std::make_pair(context_id, nullptr));
  if (e.second) {
    e.first->second = newContext(context_id);
  }
  return e.first->second.get();
}

Context* NullVmPlugin::getContext(uint64_t context_id) {
  auto it = context_map_.find(context_id);
  if (it == context_map_.end()) {
    return nullptr;
  }
  return it->second.get();
}

void NullVmPlugin::onStart() { ensureContext(0)->onStart(); }

void NullVmPlugin::onConfigure(uint64_t ptr, uint64_t size) {
  ensureContext(0)->onConfigure(std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size));
}

void NullVmPlugin::onTick() { ensureContext(0)->onTick(); }

void NullVmPlugin::onCreate(uint64_t context_id) { ensureContext(context_id)->onCreate(); }

uint64_t NullVmPlugin::onRequestHeaders(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterHeadersStatus::Continue);
  }
  return static_cast<uint64_t>(c->onRequestHeaders());
}

uint64_t NullVmPlugin::onRequestBody(uint64_t context_id, uint64_t body_buffer_length,
                                     uint64_t end_of_stream) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterDataStatus::Continue);
  }
  return static_cast<uint64_t>(
      c->onRequestBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0));
}

uint64_t NullVmPlugin::onRequestTrailers(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterTrailersStatus::Continue);
  }
  return static_cast<uint64_t>(c->onRequestTrailers());
}

uint64_t NullVmPlugin::onRequestMetadata(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterMetadataStatus::Continue);
  }
  return static_cast<uint64_t>(c->onRequestMetadata());
}

uint64_t NullVmPlugin::onResponseHeaders(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterHeadersStatus::Continue);
  }
  return static_cast<uint64_t>(c->onResponseHeaders());
}

uint64_t NullVmPlugin::onResponseBody(uint64_t context_id, uint64_t body_buffer_length,
                                      uint64_t end_of_stream) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterDataStatus::Continue);
  }
  return static_cast<uint64_t>(
      c->onResponseBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0));
}

uint64_t NullVmPlugin::onResponseTrailers(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterTrailersStatus::Continue);
  }
  return static_cast<uint64_t>(c->onResponseTrailers());
}

uint64_t NullVmPlugin::onResponseMetadata(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return static_cast<uint64_t>(Plugin::FilterMetadataStatus::Continue);
  }
  return static_cast<uint64_t>(c->onResponseMetadata());
}

void NullVmPlugin::onHttpCallResponse(uint64_t context_id, uint64_t token,
                                      uint64_t header_pairs_ptr, uint64_t header_pairs_size,
                                      uint64_t body_ptr, uint64_t body_size,
                                      uint64_t trailer_pairs_ptr, uint64_t trailer_pairs_size) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onHttpCallResponse(
      token,
      std::make_unique<WasmData>(reinterpret_cast<char*>(header_pairs_ptr), header_pairs_size),
      std::make_unique<WasmData>(reinterpret_cast<char*>(body_ptr), body_size),
      std::make_unique<WasmData>(reinterpret_cast<char*>(trailer_pairs_ptr), trailer_pairs_size));
}

void NullVmPlugin::onGrpcReceive(uint64_t context_id, uint64_t token, uint64_t response_ptr,
                                 uint64_t response_size) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onGrpcReceive(
      token, std::make_unique<WasmData>(reinterpret_cast<char*>(response_ptr), response_size));
}

void NullVmPlugin::onGrpcClose(uint64_t context_id, uint64_t token, uint64_t status_code,
                               uint64_t status_message_ptr, uint64_t status_message_size) {
  auto c = getContext(context_id);
  if (!c)
    return;
  c->onGrpcClose(
      token, static_cast<Plugin::GrpcStatus>(status_code),
      std::make_unique<WasmData>(reinterpret_cast<char*>(status_message_ptr), status_message_size));
}

void NullVmPlugin::onGrpcCreateInitialMetadata(uint64_t context_id, uint64_t token) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onGrpcCreateInitialMetadata(token);
}

void NullVmPlugin::onGrpcReceiveInitialMetadata(uint64_t context_id, uint64_t token) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onGrpcReceiveInitialMetadata(token);
}

void NullVmPlugin::onGrpcReceiveTrailingMetadata(uint64_t context_id, uint64_t token) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onGrpcReceiveTrailingMetadata(token);
}

void NullVmPlugin::onLog(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onLog();
}

void NullVmPlugin::onDone(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onDone();
}

void NullVmPlugin::onDelete(uint64_t context_id) {
  auto c = getContext(context_id);
  if (!c) {
    return;
  }
  c->onDelete();
  context_map_.erase(context_id);
}

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
