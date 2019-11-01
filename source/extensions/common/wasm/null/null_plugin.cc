#include "extensions/common/wasm/null/null_plugin.h"

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

#include "extensions/common/wasm/null/null_vm.h"
#include "extensions/common/wasm/wasm.h"
#include "extensions/common/wasm/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {

using Plugin::Context;
using Plugin::WasmData;

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<0>* f) {
  if (function_name == "_start") {
    *f = nullptr;
  } else if (function_name == "__wasm_call_ctors") {
    *f = nullptr;
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<1>* f) {
  if (function_name == "free") {
    *f = [](Common::Wasm::Context*, Word ptr) { return ::free(reinterpret_cast<void*>(ptr.u64_)); };
  } else if (function_name == "proxy_onTick") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onTick(context_id.u64_);
    };
  } else if (function_name == "proxy_onDone") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onDone(context_id.u64_);
    };
  } else if (function_name == "proxy_onLog") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onLog(context_id.u64_);
    };
  } else if (function_name == "proxy_onDelete") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onDelete(context_id.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<2>* f) {
  if (function_name == "proxy_onCreate") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word root_context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onCreate(context_id.u64_, root_context_id.u64_);
    };
  } else if (function_name == "proxy_onDownstreamConnectionClose") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word peer_type) {
      SaveRestoreContext saved_context(context);
      plugin->onDownstreamConnectionClose(context_id.u64_, peer_type.u64_);
    };
  } else if (function_name == "proxy_onUpstreamConnectionClose") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word peer_type) {
      SaveRestoreContext saved_context(context);
      plugin->onUpstreamConnectionClose(context_id.u64_, peer_type.u64_);
    };
  } else if (function_name == "proxy_onGrpcCreateInitialMetadata") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcCreateInitialMetadata(context_id.u64_, token.u64_);
    };
  } else if (function_name == "proxy_onGrpcReceiveInitialMetadata") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceiveInitialMetadata(context_id.u64_, token.u64_);
    };
  } else if (function_name == "proxy_onGrpcReceiveTrailingMetadata") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceiveTrailingMetadata(context_id.u64_, token.u64_);
    };
  } else if (function_name == "proxy_onQueueReady") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onQueueReady(context_id.u64_, token.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<3>* /* f */) {
  throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<4>* f) {
  if (function_name == "proxy_onGrpcReceive") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word response_ptr,
                  Word response_size) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceive(context_id.u64_, token.u64_, response_ptr.u64_, response_size.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<5>* f) {
  if (function_name == "proxy_onGrpcClose") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word status_code,
                  Word status_message_ptr, Word status_message_size) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcClose(context_id.u64_, token.u64_, status_code.u64_, status_message_ptr.u64_,
                          status_message_size.u64_);
    };
  } else if (function_name == "proxy_onStart") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word root_id_ptr,
                  Word root_id_size, Word vm_configuration_ptr, Word vm_configuration_size) {
      SaveRestoreContext saved_context(context);
      plugin->onStart(context_id.u64_, root_id_ptr.u64_, root_id_size.u64_,
                      vm_configuration_ptr.u64_, vm_configuration_size.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<8>* f) {
  if (function_name == "proxy_onHttpCallResponse") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token,
                  Word header_pairs_ptr, Word header_pairs_size, Word body_ptr, Word body_size,
                  Word trailer_pairs_ptr, Word trailer_pairs_size) {
      SaveRestoreContext saved_context(context);
      plugin->onHttpCallResponse(context_id.u64_, token.u64_, header_pairs_ptr.u64_,
                                 header_pairs_size.u64_, body_ptr.u64_, body_size.u64_,
                                 trailer_pairs_ptr.u64_, trailer_pairs_size.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallWord<0>* /* f */) {
  throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallWord<1>* f) {
  if (function_name == "malloc") {
    *f = [](Common::Wasm::Context*, Word size) -> Word {
      return Word(reinterpret_cast<uint64_t>(::malloc(size.u64_)));
    };
  } else if (function_name == "proxy_onNewConnection") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onNewConnection(context_id.u64_));
    };
  } else if (function_name == "proxy_onRequestHeaders") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestHeaders(context_id.u64_));
    };
  } else if (function_name == "proxy_onRequestTrailers") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestTrailers(context_id.u64_));
    };
  } else if (function_name == "proxy_onRequestMetadata") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestMetadata(context_id.u64_));
    };
  } else if (function_name == "proxy_onResponseHeaders") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseHeaders(context_id.u64_));
    };
  } else if (function_name == "proxy_onResponseTrailers") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseTrailers(context_id.u64_));
    };
  } else if (function_name == "proxy_onResponseMetadata") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseMetadata(context_id.u64_));
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallWord<3>* f) {
  if (function_name == "proxy_validateConfiguration") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word ptr, Word size) {
      SaveRestoreContext saved_context(context);
      return Word(plugin->validateConfiguration(context_id.u64_, ptr.u64_, size.u64_));
    };
  } else if (function_name == "proxy_onConfigure") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word ptr, Word size) {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onConfigure(context_id.u64_, ptr.u64_, size.u64_));
    };
  } else if (function_name == "proxy_onDownstreamData") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onDownstreamData(context_id.u64_, body_buffer_length.u64_, end_of_stream.u64_));
    };
  } else if (function_name == "proxy_onUpstreamData") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onUpstreamData(context_id.u64_, body_buffer_length.u64_, end_of_stream.u64_));
    };
  } else if (function_name == "proxy_onRequestBody") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onRequestBody(context_id.u64_, body_buffer_length.u64_, end_of_stream.u64_));
    };
  } else if (function_name == "proxy_onResponseBody") {
    auto plugin = this;
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onResponseBody(context_id.u64_, body_buffer_length.u64_, end_of_stream.u64_));
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

Plugin::Context* NullPlugin::ensureContext(uint64_t context_id, uint64_t root_context_id) {
  auto e = context_map_.insert(std::make_pair(context_id, nullptr));
  if (e.second) {
    auto root_base = context_map_[root_context_id].get();
    Plugin::RootContext* root = root_base ? root_base->asRoot() : nullptr;
    std::string root_id = root ? std::string(root->root_id()) : "";
    auto factory = registry_->context_factories[root_id];
    if (!factory) {
      throw WasmException("no context factory for root_id: " + root_id);
    }
    e.first->second = factory(context_id, root);
  }
  return e.first->second->asContext();
}

Plugin::RootContext* NullPlugin::ensureRootContext(uint64_t context_id,
                                                   std::unique_ptr<WasmData> root_id) {
  auto it = context_map_.find(context_id);
  if (it != context_map_.end()) {
    return it->second->asRoot();
  }
  auto root_id_string = root_id->toString();
  auto factory = registry_->root_factories[root_id_string];
  Plugin::RootContext* root_context;
  if (factory) {
    auto context = factory(context_id, root_id->view());
    root_context = context->asRoot();
    root_context_map_[root_id_string] = root_context;
    context_map_[context_id] = std::move(context);
  } else {
    // Default handlers.
    auto context = std::make_unique<Plugin::RootContext>(context_id, root_id->view());
    root_context = context->asRoot();
    context_map_[context_id] = std::move(context);
  }
  return root_context;
}

Plugin::ContextBase* NullPlugin::getContextBase(uint64_t context_id) {
  auto it = context_map_.find(context_id);
  if (it == context_map_.end() || !(it->second->asContext() || it->second->asRoot())) {
    throw WasmException("no base context context_id: " + std::to_string(context_id));
  }
  return it->second.get();
}

Plugin::Context* NullPlugin::getContext(uint64_t context_id) {
  auto it = context_map_.find(context_id);
  if (it == context_map_.end() || !it->second->asContext()) {
    throw WasmException("no context context_id: " + std::to_string(context_id));
  }
  return it->second->asContext();
}

Plugin::RootContext* NullPlugin::getRootContext(uint64_t context_id) {
  auto it = context_map_.find(context_id);
  if (it == context_map_.end() || !it->second->asRoot()) {
    throw WasmException("no root context_id: " + std::to_string(context_id));
  }
  return it->second->asRoot();
}

Plugin::RootContext* NullPlugin::getRoot(absl::string_view root_id) {
  auto it = root_context_map_.find(std::string(root_id));
  if (it == root_context_map_.end()) {
    return nullptr;
  }
  return it->second;
}

bool NullPlugin::validateConfiguration(uint64_t root_context_id, uint64_t ptr, uint64_t size) {
  return getRootContext(root_context_id)
      ->validateConfiguration(std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size));
}

void NullPlugin::onStart(uint64_t root_context_id, uint64_t root_id_ptr, uint64_t root_id_size,
                         uint64_t vm_configuration_ptr, uint64_t vm_configuration_size) {
  ensureRootContext(root_context_id,
                    std::make_unique<WasmData>(reinterpret_cast<char*>(root_id_ptr), root_id_size))
      ->onStart(std::make_unique<WasmData>(reinterpret_cast<char*>(vm_configuration_ptr),
                                           vm_configuration_size));
}

bool NullPlugin::onConfigure(uint64_t root_context_id, uint64_t ptr, uint64_t size) {
  return getRootContext(root_context_id)
      ->onConfigure(std::make_unique<WasmData>(reinterpret_cast<char*>(ptr), size));
}

void NullPlugin::onTick(uint64_t root_context_id) { getRootContext(root_context_id)->onTick(); }

void NullPlugin::onCreate(uint64_t context_id, uint64_t root_context_id) {
  ensureContext(context_id, root_context_id)->onCreate();
}

uint64_t NullPlugin::onNewConnection(uint64_t context_id) {
  return static_cast<uint64_t>(getContext(context_id)->onNewConnection());
}

uint64_t NullPlugin::onDownstreamData(uint64_t context_id, uint64_t data_length,
                                      uint64_t end_of_stream) {
  return static_cast<uint64_t>(
      getContext(context_id)
          ->onDownstreamData(static_cast<size_t>(data_length), end_of_stream != 0));
}

uint64_t NullPlugin::onUpstreamData(uint64_t context_id, uint64_t data_length,
                                    uint64_t end_of_stream) {
  return static_cast<uint64_t>(
      getContext(context_id)->onUpstreamData(static_cast<size_t>(data_length), end_of_stream != 0));
}

void NullPlugin::onDownstreamConnectionClose(uint64_t context_id, uint64_t peer_type) {
  getContext(context_id)->onDownstreamConnectionClose(static_cast<Plugin::PeerType>(peer_type));
}

void NullPlugin::onUpstreamConnectionClose(uint64_t context_id, uint64_t peer_type) {
  getContext(context_id)->onUpstreamConnectionClose(static_cast<Plugin::PeerType>(peer_type));
}

uint64_t NullPlugin::onRequestHeaders(uint64_t context_id) {
  return static_cast<uint64_t>(getContext(context_id)->onRequestHeaders());
}

uint64_t NullPlugin::onRequestBody(uint64_t context_id, uint64_t body_buffer_length,
                                   uint64_t end_of_stream) {
  return static_cast<uint64_t>(
      getContext(context_id)
          ->onRequestBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0));
}

uint64_t NullPlugin::onRequestTrailers(uint64_t context_id) {
  return static_cast<uint64_t>(getContext(context_id)->onRequestTrailers());
}

uint64_t NullPlugin::onRequestMetadata(uint64_t context_id) {
  return static_cast<uint64_t>(getContext(context_id)->onRequestMetadata());
}

uint64_t NullPlugin::onResponseHeaders(uint64_t context_id) {
  return static_cast<uint64_t>(getContext(context_id)->onResponseHeaders());
}

uint64_t NullPlugin::onResponseBody(uint64_t context_id, uint64_t body_buffer_length,
                                    uint64_t end_of_stream) {
  return static_cast<uint64_t>(
      getContext(context_id)
          ->onResponseBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0));
}

uint64_t NullPlugin::onResponseTrailers(uint64_t context_id) {
  return static_cast<uint64_t>(getContext(context_id)->onResponseTrailers());
}

uint64_t NullPlugin::onResponseMetadata(uint64_t context_id) {
  return static_cast<uint64_t>(getContext(context_id)->onResponseMetadata());
}

void NullPlugin::onHttpCallResponse(uint64_t context_id, uint64_t token, uint64_t header_pairs_ptr,
                                    uint64_t header_pairs_size, uint64_t body_ptr,
                                    uint64_t body_size, uint64_t trailer_pairs_ptr,
                                    uint64_t trailer_pairs_size) {
  getRootContext(context_id)
      ->onHttpCallResponse(
          token,
          std::make_unique<WasmData>(reinterpret_cast<char*>(header_pairs_ptr), header_pairs_size),
          std::make_unique<WasmData>(reinterpret_cast<char*>(body_ptr), body_size),
          std::make_unique<WasmData>(reinterpret_cast<char*>(trailer_pairs_ptr),
                                     trailer_pairs_size));
}

void NullPlugin::onGrpcReceive(uint64_t context_id, uint64_t token, uint64_t response_ptr,
                               uint64_t response_size) {
  getRootContext(context_id)
      ->onGrpcReceive(
          token, std::make_unique<WasmData>(reinterpret_cast<char*>(response_ptr), response_size));
}

void NullPlugin::onGrpcClose(uint64_t context_id, uint64_t token, uint64_t status_code,
                             uint64_t status_message_ptr, uint64_t status_message_size) {
  getRootContext(context_id)
      ->onGrpcClose(token, static_cast<Plugin::GrpcStatus>(status_code),
                    std::make_unique<WasmData>(reinterpret_cast<char*>(status_message_ptr),
                                               status_message_size));
}

void NullPlugin::onGrpcCreateInitialMetadata(uint64_t context_id, uint64_t token) {
  getRootContext(context_id)->onGrpcCreateInitialMetadata(token);
}

void NullPlugin::onGrpcReceiveInitialMetadata(uint64_t context_id, uint64_t token) {
  getRootContext(context_id)->onGrpcReceiveInitialMetadata(token);
}

void NullPlugin::onGrpcReceiveTrailingMetadata(uint64_t context_id, uint64_t token) {
  getRootContext(context_id)->onGrpcReceiveTrailingMetadata(token);
}

void NullPlugin::onQueueReady(uint64_t context_id, uint64_t token) {
  getRootContext(context_id)->onQueueReady(token);
}

void NullPlugin::onLog(uint64_t context_id) { getContext(context_id)->onLog(); }

void NullPlugin::onDone(uint64_t context_id) { getContext(context_id)->onDone(); }

void NullPlugin::onDelete(uint64_t context_id) {
  getContext(context_id)->onDelete();
  context_map_.erase(context_id);
}

Plugin::RootContext* nullVmGetRoot(absl::string_view root_id) {
  auto null_vm = static_cast<NullVm*>(current_context_->wasm()->wasmVm());
  return static_cast<NullPlugin*>(null_vm->plugin_.get())->getRoot(root_id);
}

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
