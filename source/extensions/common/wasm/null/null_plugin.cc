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
  auto plugin = this;
  if (function_name == "free") {
    *f = [](Common::Wasm::Context*, Word ptr) { return ::free(reinterpret_cast<void*>(ptr.u64_)); };
  } else if (function_name == "proxy_onTick") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onTick(context_id.u64_);
    };
  } else if (function_name == "proxy_onDone") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onDone(context_id.u64_);
    };
  } else if (function_name == "proxy_onLog") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onLog(context_id.u64_);
    };
  } else if (function_name == "proxy_onDelete") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onDelete(context_id.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<2>* f) {
  auto plugin = this;
  if (function_name == "proxy_onCreate") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word root_context_id) {
      SaveRestoreContext saved_context(context);
      plugin->onCreate(context_id.u64_, root_context_id.u64_);
    };
  } else if (function_name == "proxy_onDownstreamConnectionClose") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word peer_type) {
      SaveRestoreContext saved_context(context);
      plugin->onDownstreamConnectionClose(context_id.u64_, peer_type.u64_);
    };
  } else if (function_name == "proxy_onUpstreamConnectionClose") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word peer_type) {
      SaveRestoreContext saved_context(context);
      plugin->onUpstreamConnectionClose(context_id.u64_, peer_type.u64_);
    };
  } else if (function_name == "proxy_onQueueReady") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token) {
      SaveRestoreContext saved_context(context);
      plugin->onQueueReady(context_id.u64_, token.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<3>* f) {
  auto plugin = this;
  if (function_name == "proxy_onGrpcClose") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word status_code) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcClose(context_id.u64_, token.u64_, status_code.u64_);
    };
  } else if (function_name == "proxy_onGrpcReceive") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word body_size) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceive(context_id.u64_, token.u64_, body_size.u64_);
    };
  } else if (function_name == "proxy_onGrpcCreateInitialMetadata") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word headers) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcCreateInitialMetadata(context_id.u64_, token.u64_, headers.u64_);
    };
  } else if (function_name == "proxy_onGrpcReceiveInitialMetadata") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word headers) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceiveInitialMetadata(context_id.u64_, token.u64_, headers.u64_);
    };
  } else if (function_name == "proxy_onGrpcReceiveTrailingMetadata") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word trailers) {
      SaveRestoreContext saved_context(context);
      plugin->onGrpcReceiveTrailingMetadata(context_id.u64_, token.u64_, trailers.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallVoid<5>* f) {
  auto plugin = this;
  if (function_name == "proxy_onHttpCallResponse") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word token, Word headers,
                  Word body_size, Word trailers) {
      SaveRestoreContext saved_context(context);
      plugin->onHttpCallResponse(context_id.u64_, token.u64_, headers.u64_, body_size.u64_,
                                 trailers.u64_);
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallWord<1>* f) {
  auto plugin = this;
  if (function_name == "malloc") {
    *f = [](Common::Wasm::Context*, Word size) -> Word {
      return Word(reinterpret_cast<uint64_t>(::malloc(size.u64_)));
    };
  } else if (function_name == "proxy_onNewConnection") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onNewConnection(context_id.u64_));
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallWord<2>* f) {
  auto plugin = this;
  if (function_name == "proxy_onStart") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word configuration_size) {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onStart(context_id.u64_, configuration_size.u64_));
    };
  } else if (function_name == "proxy_onConfigure") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word configuration_size) {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onConfigure(context_id.u64_, configuration_size.u64_));
    };
  } else if (function_name == "proxy_validateConfiguration") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word configuration_size) {
      SaveRestoreContext saved_context(context);
      return Word(plugin->validateConfiguration(context_id.u64_, configuration_size.u64_));
    };
  } else if (function_name == "proxy_onRequestHeaders") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word headers) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestHeaders(context_id.u64_, headers.u64_));
    };
  } else if (function_name == "proxy_onRequestTrailers") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word trailers) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestTrailers(context_id.u64_, trailers.u64_));
    };
  } else if (function_name == "proxy_onRequestMetadata") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word elements) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onRequestMetadata(context_id.u64_, elements.u64_));
    };
  } else if (function_name == "proxy_onResponseHeaders") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word headers) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseHeaders(context_id.u64_, headers.u64_));
    };
  } else if (function_name == "proxy_onResponseTrailers") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word trailers) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseTrailers(context_id.u64_, trailers.u64_));
    };
  } else if (function_name == "proxy_onResponseMetadata") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word elements) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(plugin->onResponseMetadata(context_id.u64_, elements.u64_));
    };
  } else {
    throw WasmVmException(fmt::format("Missing getFunction for: {}", function_name));
  }
}

void NullPlugin::getFunction(absl::string_view function_name, WasmCallWord<3>* f) {
  auto plugin = this;
  if (function_name == "proxy_onDownstreamData") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onDownstreamData(context_id.u64_, body_buffer_length.u64_, end_of_stream.u64_));
    };
  } else if (function_name == "proxy_onUpstreamData") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onUpstreamData(context_id.u64_, body_buffer_length.u64_, end_of_stream.u64_));
    };
  } else if (function_name == "proxy_onRequestBody") {
    *f = [plugin](Common::Wasm::Context* context, Word context_id, Word body_buffer_length,
                  Word end_of_stream) -> Word {
      SaveRestoreContext saved_context(context);
      return Word(
          plugin->onRequestBody(context_id.u64_, body_buffer_length.u64_, end_of_stream.u64_));
    };
  } else if (function_name == "proxy_onResponseBody") {
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

Plugin::RootContext* NullPlugin::ensureRootContext(uint64_t context_id) {
  auto root_id_opt = Plugin::getProperty({"plugin_root_id"});
  if (!root_id_opt) {
    throw WasmException("unable to get root_id");
  }
  auto root_id = std::move(root_id_opt.value());
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

bool NullPlugin::validateConfiguration(uint64_t root_context_id, uint64_t configuration_size) {
  return getRootContext(root_context_id)->validateConfiguration(configuration_size);
}

bool NullPlugin::onStart(uint64_t root_context_id, uint64_t vm_configuration_size) {
  auto context = ensureRootContext(root_context_id);
  if (!context) {
    return false;
  }
  return context->onStart(vm_configuration_size) != 0;
}

bool NullPlugin::onConfigure(uint64_t root_context_id, uint64_t plugin_configuration_size) {
  return getRootContext(root_context_id)->onConfigure(plugin_configuration_size);
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

uint64_t NullPlugin::onRequestHeaders(uint64_t context_id, uint64_t headers) {
  return static_cast<uint64_t>(getContext(context_id)->onRequestHeaders(headers));
}

uint64_t NullPlugin::onRequestBody(uint64_t context_id, uint64_t body_buffer_length,
                                   uint64_t end_of_stream) {
  return static_cast<uint64_t>(
      getContext(context_id)
          ->onRequestBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0));
}

uint64_t NullPlugin::onRequestTrailers(uint64_t context_id, uint64_t trailers) {
  return static_cast<uint64_t>(getContext(context_id)->onRequestTrailers(trailers));
}

uint64_t NullPlugin::onRequestMetadata(uint64_t context_id, uint64_t elements) {
  return static_cast<uint64_t>(getContext(context_id)->onRequestMetadata(elements));
}

uint64_t NullPlugin::onResponseHeaders(uint64_t context_id, uint64_t headers) {
  return static_cast<uint64_t>(getContext(context_id)->onResponseHeaders(headers));
}

uint64_t NullPlugin::onResponseBody(uint64_t context_id, uint64_t body_buffer_length,
                                    uint64_t end_of_stream) {
  return static_cast<uint64_t>(
      getContext(context_id)
          ->onResponseBody(static_cast<size_t>(body_buffer_length), end_of_stream != 0));
}

uint64_t NullPlugin::onResponseTrailers(uint64_t context_id, uint64_t trailers) {
  return static_cast<uint64_t>(getContext(context_id)->onResponseTrailers(trailers));
}

uint64_t NullPlugin::onResponseMetadata(uint64_t context_id, uint64_t elements) {
  return static_cast<uint64_t>(getContext(context_id)->onResponseMetadata(elements));
}

void NullPlugin::onHttpCallResponse(uint64_t context_id, uint64_t token, uint64_t headers,
                                    uint64_t body_size, uint64_t trailers) {
  getRootContext(context_id)->onHttpCallResponse(token, headers, body_size, trailers);
}

void NullPlugin::onGrpcReceive(uint64_t context_id, uint64_t token, size_t body_size) {
  getRootContext(context_id)->onGrpcReceive(token, body_size);
}

void NullPlugin::onGrpcClose(uint64_t context_id, uint64_t token, uint64_t status_code) {
  getRootContext(context_id)->onGrpcClose(token, static_cast<Plugin::GrpcStatus>(status_code));
}

void NullPlugin::onGrpcCreateInitialMetadata(uint64_t context_id, uint64_t token,
                                             uint64_t headers) {
  getRootContext(context_id)->onGrpcCreateInitialMetadata(token, headers);
}

void NullPlugin::onGrpcReceiveInitialMetadata(uint64_t context_id, uint64_t token,
                                              uint64_t headers) {
  getRootContext(context_id)->onGrpcReceiveInitialMetadata(token, headers);
}

void NullPlugin::onGrpcReceiveTrailingMetadata(uint64_t context_id, uint64_t token,
                                               uint64_t trailers) {
  getRootContext(context_id)->onGrpcReceiveTrailingMetadata(token, trailers);
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
  auto null_vm = static_cast<NullVm*>(current_context_->wasmVm());
  return static_cast<NullPlugin*>(null_vm->plugin_.get())->getRoot(root_id);
}

} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
