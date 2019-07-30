// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  void onLog() override;
  void onDone() override;
};
class ExampleRootContext : public RootContext {
public:
  explicit ExampleRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}
  void onTick() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext), ROOT_FACTORY(ExampleRootContext));

void ExampleRootContext::onTick() {
  auto metadataString = nodeMetadataValue("wasm_node_get_key").string_value();
  logDebug(std::string("onTick ") + metadataString);
}

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  auto metadataString = getMetadataStringValue(MetadataType::Request, "wasm_request_get_key");
  setMetadataStringValue(MetadataType::Request, "wasm_request_set_key", "wasm_request_set_value");
  auto path = getRequestHeader(":path");
  logInfo(std::string("header path ") + path->toString());
  addRequestHeader("newheader", "newheadervalue");
  replaceRequestHeader("server", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  logError(std::string("onRequestBody ") + nodeMetadataValue("wasm_node_get_key").string_value());
  auto s = requestMetadataStruct();
  auto t = requestMetadataStruct();
  logTrace(std::string("Struct ") + s.fields().at("wasm_request_get_key").string_value() + " " +
           t.fields().at("wasm_request_get_key").string_value());
  return FilterDataStatus::Continue;
}

void ExampleContext::onLog() {
  auto path = getRequestHeader(":path");
  logWarn("onLog " + std::to_string(id()) + " " + path->toString());
}

void ExampleContext::onDone() { logWarn("onDone " + std::to_string(id())); }
