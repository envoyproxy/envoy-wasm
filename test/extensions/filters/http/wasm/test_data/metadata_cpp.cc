// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"
#include "proxy_wasm_intrinsics_lite.pb.h"
#include "contrib/proxy_expr.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  void onLog() override;
  void onDone() override;
};
class ExampleRootContext : public RootContext {
public:
  explicit ExampleRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}
  void onTick() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext),
                                                      ROOT_FACTORY(ExampleRootContext));

void ExampleRootContext::onTick() {
  std::string value;
  if (!getValue({"node", "metadata", "wasm_node_get_key"}, &value)) {
    logDebug("missing node metadata");
  }
  logDebug(std::string("onTick ") + value);
}

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
  std::string value;
  if (!getValue({"node", "metadata", "wasm_node_get_key"}, &value)) {
    logDebug("missing node metadata");
  }
  auto r = setFilterStateStringValue("wasm_request_set_key", "wasm_request_set_value");
  if (r != WasmResult::Ok) {
    logDebug(toString(r));
  }
  auto path = getRequestHeader(":path");
  logInfo(std::string("header path ") + path->toString());
  addRequestHeader("newheader", "newheadervalue");
  replaceRequestHeader("server", "envoy-wasm");

  {
    const std::string expr = R"("server is " + request.headers["server"])";
    uint32_t token = 0;
    if (WasmResult::Ok != createExpression(expr, &token)) {
      logError("expr_create error");
    } else {
      std::string eval_result;
      if (!evaluateExpression(token, &eval_result)) {
        logError("expr_eval error");
      } else {
        logInfo(eval_result);
      }
      if (WasmResult::Ok != exprDelete(token)) {
        logError("failed to delete an expression");
      }
    }
  }

  {
    const std::string expr = R"(
envoy.api.v2.core.GrpcService{
  envoy_grpc: envoy.api.v2.core.GrpcService.EnvoyGrpc {
    cluster_name: "test"
  }
})";
    uint32_t token = 0;
    if (WasmResult::Ok != createExpression(expr, &token)) {
      logError("expr_create error");
    } else {
      GrpcService eval_result;
      if (!evaluateMessage(token, &eval_result)) {
        logError("expr_eval error");
      } else {
        logInfo("grpc service: " + eval_result.envoy_grpc().cluster_name());
      }
      if (WasmResult::Ok != exprDelete(token)) {
        logError("failed to delete an expression");
      }
    }
  }

  int64_t dur;
  if (getValue({"request", "duration"}, &dur)) {
    logInfo("duration is " + std::to_string(dur));
  } else {
    logError("failed to get request duration");
  }

  return FilterHeadersStatus::Continue;
}

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  std::string value;
  if (!getValue({"node", "metadata", "wasm_node_get_key"}, &value)) {
    logDebug("missing node metadata");
  }
  logError(std::string("onRequestBody ") + value);
  std::string request_string;
  std::string request_string2;
  if (!getValue(
          {"metadata", "filter_metadata", "envoy.filters.http.wasm", "wasm_request_get_key"},
          &request_string)) {
    logDebug("missing request metadata");
  }
  if (!getValue(
          {"metadata", "filter_metadata", "envoy.filters.http.wasm", "wasm_request_get_key"},
          &request_string2)) {
    logDebug("missing request metadata");
  }
  logTrace(std::string("Struct ") + request_string + " " + request_string2);
  return FilterDataStatus::Continue;
}

void ExampleContext::onLog() {
  auto path = getRequestHeader(":path");
  logWarn("onLog " + std::to_string(id()) + " " + path->toString());
}

void ExampleContext::onDone() { logWarn("onDone " + std::to_string(id())); }
