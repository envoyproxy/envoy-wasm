#include <string>
#include <unordered_map>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics.h"
#else

#include "extensions/common/wasm/null/null_plugin.h"
#include "absl/base/casts.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {
namespace Null {
namespace Plugin {
namespace ExamplePlugin {
NULL_PLUGIN_ROOT_REGISTRY;
#endif

class PluginContext : public Context {
public:
  explicit PluginContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  void onLog() override;
  void onDone() override;
};
static RegisterContextFactory register_PluginContext(CONTEXT_FACTORY(PluginContext));

FilterHeadersStatus PluginContext::onRequestHeaders() {
  logDebug(std::string("onRequestHeaders ") + std::to_string(id()));
  auto path = getRequestHeader(":path");
  logInfo(std::string("header path ") + std::string(path->view()));
  if (addRequestHeader("newheader", "newheadervalue") != WasmResult::Ok) {
    logInfo("addRequestHeader failed");
  }
  replaceRequestHeader("server", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus PluginContext::onRequestBody(size_t body_buffer_length, bool /* end_of_stream */) {
  auto body = getRequestBodyBufferBytes(0, body_buffer_length);
  logError(std::string("onRequestBody ") + std::string(body->view()));
  return FilterDataStatus::Continue;
}

void PluginContext::onLog() {
  setFilterStateStringValue("wasm_state", "wasm_value");
  auto path = getRequestHeader(":path");
  if (path->view() == "/test_context") {
    logWarn("request.path: " + getSelectorExpression({"request", "path"}).value()->toString());
    logWarn("node.metadata: " +
            getSelectorExpression({"node", "metadata", "istio.io/metadata"}).value()->toString());
    logWarn("metadata: " +
            getSelectorExpression(
                {"metadata", "filter_metadata", "envoy.filters.http.wasm", "wasm_request_get_key"})
                .value()
                ->toString());
    int64_t responseCode;
    if (getValue({"response", "code"}, &responseCode)) {
      logWarn("response.code: " + absl::StrCat(responseCode));
    }
    logWarn("state: " + getSelectorExpression({"filter_state", "wasm_state"}).value()->toString());

    // exercise struct state roundtrip
    ProtobufWkt::Value struct_obj;
    ProtobufWkt::Value val;
    val.set_string_value("wasm_struct_value");
    (*struct_obj.mutable_struct_value()->mutable_fields())["wasm_struct_key"] = val;
    setFilterStateValue("wasm_struct", struct_obj);

    ProtobufWkt::Value got;
    if (getStructValue({"filter_state", "wasm_struct"}, &got)) {
      logWarn("struct state: " + got.struct_value().fields().at("wasm_struct_key").string_value());
    }
  } else {
    logWarn("onLog " + std::to_string(id()) + " " + std::string(path->view()));
  }
}

void PluginContext::onDone() { logWarn("onDone " + std::to_string(id())); }

#ifdef NULL_PLUGIN
} // namespace ExamplePlugin
} // namespace Plugin
} // namespace Null
} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
#endif
