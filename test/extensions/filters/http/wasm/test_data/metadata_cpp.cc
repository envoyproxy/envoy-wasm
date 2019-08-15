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
  google::protobuf::Value value;
  if (auto r = nodeMetadataValue("wasm_node_get_key", &value); r != WasmResult::Ok) {
    logDebug(toString(r));
  }
  logDebug(std::string("onTick ") + value.string_value());
}

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  google::protobuf::Value value;
  if (auto r = nodeMetadataValue("wasm_node_get_key", &value); r != WasmResult::Ok) {
    logDebug(toString(r));
  }
  if (auto r = setMetadataStringValue(MetadataType::Request, "wasm_request_set_key", "wasm_request_set_value"); r != WasmResult::Ok) {
    logDebug(toString(r));
  }
  auto path = getRequestHeader(":path");
  logInfo(std::string("header path ") + path->toString());
  addRequestHeader("newheader", "newheadervalue");
  replaceRequestHeader("server", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  google::protobuf::Value value;
  if (auto r = nodeMetadataValue("wasm_node_get_key", &value); r != WasmResult::Ok) {
    logDebug(toString(r));
  }
  logError(std::string("onRequestBody ") + value.string_value());
  google::protobuf::Struct request_struct;
  google::protobuf::Struct request_struct2;
  if (auto r = requestMetadataStruct(&request_struct); r != WasmResult::Ok) { 
    logDebug(toString(r));
  }
  if (auto r = requestMetadataStruct(&request_struct2); r != WasmResult::Ok) { 
    logDebug(toString(r));
  }
  logTrace(std::string("Struct ") + request_struct.fields().at("wasm_request_get_key").string_value() + " " +
           request_struct2.fields().at("wasm_request_get_key").string_value());
  return FilterDataStatus::Continue;
}

void ExampleContext::onLog() {
  auto path = getRequestHeader(":path");
  logWarn("onLog " + std::to_string(id()) + " " + path->toString());
}

void ExampleContext::onDone() { logWarn("onDone " + std::to_string(id())); }
