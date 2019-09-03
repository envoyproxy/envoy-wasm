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
  addRequestHeader("newheader", "newheadervalue");
  replaceRequestHeader("server", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus PluginContext::onRequestBody(size_t body_buffer_length, bool /* end_of_stream */) {
  auto body = getRequestBodyBufferBytes(0, body_buffer_length);
  logError(std::string("onRequestBody ") + std::string(body->view()));
  return FilterDataStatus::Continue;
}

void PluginContext::onLog() {
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
    auto responseCode = getSelectorExpression({"response", "code"}).value();
    if (responseCode->size() == sizeof(int64_t)) {
      char buf[sizeof(int64_t)];
      responseCode->view().copy(buf, sizeof(int64_t), 0);
      int64_t code = absl::bit_cast<int64_t>(buf);
      logWarn("response.code: " + absl::StrCat(code));
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
