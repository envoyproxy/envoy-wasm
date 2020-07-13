// NOLINT(namespace-envoy)
#include <memory>
#include <string>
#include <unordered_map>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics_lite.h"
#include "source/extensions/common/wasm/ext/envoy_proxy_wasm_api.h"
#include "source/extensions/common/wasm/ext/declare_property.pb.h"
#else
#include "extensions/common/wasm/ext/envoy_null_plugin.h"
#include "absl/base/casts.h"
#endif

START_WASM_PLUGIN(HttpWasmTestCpp)

#include "contrib/proxy_expr.h"

class TestRootContext : public RootContext {
public:
  explicit TestRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  bool onStart(size_t configuration_size) override;
  void onTick() override;
  void onQueueReady(uint32_t token) override;

  std::string test_;
  uint32_t stream_context_id_;
  uint32_t shared_queue_token_;
};

class TestContext : public Context {
public:
  explicit TestContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t, bool) override;
  FilterHeadersStatus onResponseHeaders(uint32_t, bool) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) override;
  void onLog() override;
  void onDone() override;

  // Currently unused.
  FilterHeadersStatus onRequestHeadersSimple(uint32_t);
  FilterHeadersStatus onRequestHeadersStream(uint32_t);

private:
  TestRootContext* root() { return static_cast<TestRootContext*>(Context::root()); }

  static void logBody(WasmBufferType type);
  FilterDataStatus onBody(WasmBufferType type, size_t buffer_length, bool end);

  std::string body_op_;
  int num_chunks_ = 0;
};

static RegisterContextFactory register_TestContext(CONTEXT_FACTORY(TestContext),
                                                   ROOT_FACTORY(TestRootContext));

class MyGrpcCallHandler : public GrpcCallHandler<google::protobuf::Value> {
public:
  MyGrpcCallHandler() : GrpcCallHandler<google::protobuf::Value>() {}
  void onSuccess(size_t body_size) override {
    auto response = getBufferBytes(WasmBufferType::GrpcReceiveBuffer, 0, body_size);
    logDebug(response->proto<google::protobuf::Value>().string_value());
  }
  void onFailure(GrpcStatus status) override {
    auto p = getStatus();
    logDebug(std::string("failure ") + std::to_string(static_cast<int>(status)) +
             std::string(p.second->view()));
  }
};

// Currently unused.
class MyGrpcStreamHandler
    : public GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value> {
public:
  MyGrpcStreamHandler() : GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value>() {}
  void onReceiveInitialMetadata(uint32_t) override {}
  void onReceiveTrailingMetadata(uint32_t) override {}
  void onReceive(size_t body_size) override {
    auto response = getBufferBytes(WasmBufferType::GrpcReceiveBuffer, 0, body_size);
    logDebug(response->proto<google::protobuf::Value>().string_value());
  }
  void onRemoteClose(GrpcStatus status) override {
    auto p = getStatus();
    logDebug(std::string("failure ") + std::to_string(static_cast<int>(status)) +
             std::string(p.second->view()));
    close();
  }
};

bool TestRootContext::onStart(size_t configuration_size) {
  test_ = getBufferBytes(WasmBufferType::VmConfiguration, 0, configuration_size)->toString();
  if (test_ == "shared_queue") {
    CHECK_RESULT(registerSharedQueue("my_shared_queue", &shared_queue_token_));
  }
  return true;
}

FilterHeadersStatus TestContext::onRequestHeaders(uint32_t, bool) {
  root()->stream_context_id_ = id();
  auto test = root()->test_;
  if (test == "headers") {
    logDebug(std::string("onRequestHeaders ") + std::to_string(id()) + std::string(" ") + test);
    auto path = getRequestHeader(":path");
    logInfo(std::string("header path ") + std::string(path->view()));
    std::string protocol;
    addRequestHeader("newheader", "newheadervalue");
    auto server = getRequestHeader("server");
    replaceRequestHeader("server", "envoy-wasm");
    if (server->view() == "envoy-wasm-pause") {
      return FilterHeadersStatus::StopIteration;
    } else if (server->view() == "envoy-wasm-end-stream") {
      return FilterHeadersStatus::ContinueAndEndStream;
    } else if (server->view() == "envoy-wasm-stop-buffer") {
      return FilterHeadersStatus::StopAllIterationAndBuffer;
    } else if (server->view() == "envoy-wasm-stop-watermark") {
      return FilterHeadersStatus::StopAllIterationAndWatermark;
    } else {
      return FilterHeadersStatus::Continue;
    }
  } else if (test == "body") {
    body_op_ = getRequestHeader("x-test-operation")->toString();
    return FilterHeadersStatus::Continue;
  } else if (test == "shared_data") {
    WasmDataPtr value0;
    if (getSharedData("shared_data_key_bad", &value0) == WasmResult::NotFound) {
      logDebug("get of bad key not found");
    }
    CHECK_RESULT(setSharedData("shared_data_key1", "shared_data_value0"));
    CHECK_RESULT(setSharedData("shared_data_key1", "shared_data_value1"));
    CHECK_RESULT(setSharedData("shared_data_key2", "shared_data_value2"));
    uint32_t cas = 0;
    auto value2 = getSharedDataValue("shared_data_key2", &cas);
    if (WasmResult::CasMismatch ==
        setSharedData("shared_data_key2", "shared_data_value3", cas + 1)) { // Bad cas.
      logInfo("set CasMismatch");
    }
    return FilterHeadersStatus::Continue;
  } else if (test == "shared_queue") {
    uint32_t token;
    if (resolveSharedQueue("vm_id", "bad_shared_queue", &token) == WasmResult::NotFound) {
      logWarn("onRequestHeaders not found bad_shared_queue");
    }
    CHECK_RESULT(resolveSharedQueue("vm_id", "my_shared_queue", &token));
    if (enqueueSharedQueue(token, "data1") == WasmResult::Ok) {
      logWarn("onRequestHeaders enqueue Ok");
    }
    return FilterHeadersStatus::Continue;
  } else if (test == "metadata") {
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
  } else if (test == "async_call") {
    auto context_id = id();
    auto callback = [context_id](uint32_t, size_t body_size, uint32_t) {
      auto response_headers = getHeaderMapPairs(WasmHeaderMapType::HttpCallResponseHeaders);
      // Switch context after getting headers, but before getting body to exercise both code paths.
      getContext(context_id)->setEffectiveContext();
      auto body = getBufferBytes(WasmBufferType::HttpCallResponseBody, 0, body_size);
      auto response_trailers = getHeaderMapPairs(WasmHeaderMapType::HttpCallResponseTrailers);
      for (auto& p : response_headers->pairs()) {
        logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
      }
      logDebug(std::string(body->view()));
      for (auto& p : response_trailers->pairs()) {
        logWarn(std::string(p.first) + std::string(" -> ") + std::string(p.second));
      }
    };
    root()->httpCall("cluster", {{":method", "POST"}, {":path", "/"}, {":authority", "foo"}},
                     "hello world", {{"trail", "cow"}}, 1000, callback);
    return FilterHeadersStatus::StopIteration;
  } else if (test == "grpc_call") {
    GrpcService grpc_service;
    grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
    std::string grpc_service_string;
    grpc_service.SerializeToString(&grpc_service_string);
    google::protobuf::Value value;
    value.set_string_value("request");
    HeaderStringPairs initial_metadata;
    root()->grpcCallHandler(grpc_service_string, "service", "method", initial_metadata, value, 1000,
                            std::unique_ptr<GrpcCallHandlerBase>(new MyGrpcCallHandler()));
    return FilterHeadersStatus::StopIteration;
  }
  return FilterHeadersStatus::Continue;
}

// Currently unused.
FilterHeadersStatus TestContext::onRequestHeadersSimple(uint32_t) {
  std::function<void(size_t body_size)> success_callback = [](size_t body_size) {
    auto response = getBufferBytes(WasmBufferType::GrpcReceiveBuffer, 0, body_size);
    logDebug(response->proto<google::protobuf::Value>().string_value());
  };
  std::function<void(GrpcStatus status)> failure_callback = [](GrpcStatus status) {
    auto p = getStatus();
    logDebug(std::string("failure ") + std::to_string(static_cast<int>(status)) +
             std::string(p.second->view()));
  };
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  google::protobuf::Value value;
  value.set_string_value("request");
  HeaderStringPairs initial_metadata;
  root()->grpcSimpleCall(grpc_service_string, "service", "method", initial_metadata, value, 1000,
                         success_callback, failure_callback);
  return FilterHeadersStatus::StopIteration;
}

// Currently unused.
FilterHeadersStatus TestContext::onRequestHeadersStream(uint32_t) {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  HeaderStringPairs initial_metadata;
  root()->grpcStreamHandler(grpc_service_string, "service", "method", initial_metadata,
                            std::unique_ptr<GrpcStreamHandlerBase>(new MyGrpcStreamHandler()));
  return FilterHeadersStatus::StopIteration;
}

FilterHeadersStatus TestContext::onResponseHeaders(uint32_t, bool) {
  auto test = root()->test_;
  if (test == "body") {
    body_op_ = getResponseHeader("x-test-operation")->toString();
  }
  return FilterHeadersStatus::Continue;
}

FilterDataStatus TestContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  auto test = root()->test_;
  if (test == "headers") {
    auto body = getBufferBytes(WasmBufferType::HttpRequestBody, 0, body_buffer_length);
    logError(std::string("onRequestBody ") + std::string(body->view()));
  } else if (test == "body") {
    return onBody(WasmBufferType::HttpRequestBody, body_buffer_length, end_of_stream);
  } else if (test == "metadata") {
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
  return FilterDataStatus::Continue;
}

FilterDataStatus TestContext::onResponseBody(size_t body_buffer_length, bool end_of_stream) {
  auto test = root()->test_;
  if (test == "body") {
    return onBody(WasmBufferType::HttpResponseBody, body_buffer_length, end_of_stream);
  }
  return FilterDataStatus::Continue;
}

void TestContext::logBody(WasmBufferType type) {
  size_t buffered_size;
  uint32_t flags;
  getBufferStatus(type, &buffered_size, &flags);
  auto body = getBufferBytes(type, 0, buffered_size);
  logError(std::string("onRequestBody ") + std::string(body->view()));
}

FilterDataStatus TestContext::onBody(WasmBufferType type, size_t buffer_length,
                                     bool end_of_stream) {
  auto test = root()->test_;
  if (test == "body") {
    size_t size;
    uint32_t flags;
    if (body_op_ == "ReadBody") {
      auto body = getBufferBytes(type, 0, buffer_length);
      logError("onRequestBody " + std::string(body->view()));

    } else if (body_op_ == "PrependAndAppendToBody") {
      setBuffer(WasmBufferType::HttpRequestBody, 0, 0, "prepend.");
      getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
      setBuffer(WasmBufferType::HttpRequestBody, size, 0, ".append");
      getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
      auto updated = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
      logError("onRequestBody " + std::string(updated->view()));

    } else if (body_op_ == "ReplaceBody") {
      setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "replace");
      getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
      auto replaced = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
      logError("onRequestBody " + std::string(replaced->view()));

    } else if (body_op_ == "RemoveBody") {
      setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "");
      getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
      auto erased = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
      logError("onRequestBody " + std::string(erased->view()));

    } else if (body_op_ == "BufferBody") {
      logBody(type);
      return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

    } else if (body_op_ == "PrependAndAppendToBufferedBody") {
      setBuffer(WasmBufferType::HttpRequestBody, 0, 0, "prepend.");
      getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
      setBuffer(WasmBufferType::HttpRequestBody, size, 0, ".append");
      logBody(type);
      return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

    } else if (body_op_ == "ReplaceBufferedBody") {
      setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "replace");
      getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
      auto replaced = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
      logBody(type);
      return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

    } else if (body_op_ == "RemoveBufferedBody") {
      setBuffer(WasmBufferType::HttpRequestBody, 0, buffer_length, "");
      getBufferStatus(WasmBufferType::HttpRequestBody, &size, &flags);
      auto erased = getBufferBytes(WasmBufferType::HttpRequestBody, 0, size);
      logBody(type);
      return end_of_stream ? FilterDataStatus::Continue : FilterDataStatus::StopIterationAndBuffer;

    } else if (body_op_ == "BufferTwoBodies") {
      logBody(type);
      num_chunks_++;
      if (end_of_stream || num_chunks_ > 2) {
        return FilterDataStatus::Continue;
      }
      return FilterDataStatus::StopIterationAndBuffer;

    } else {
      // This is a test and the test was configured incorrectly.
      logError("Invalid body test op " + body_op_);
      abort();
    }
  }
  return FilterDataStatus::Continue;
}

void TestContext::onLog() {
  auto test = root()->test_;
  if (test == "headers") {
    auto path = getRequestHeader(":path");
    logWarn("onLog " + std::to_string(id()) + " " + std::string(path->view()));
  } else if (test == "shared_data") {
    WasmDataPtr value0;
    if (getSharedData("shared_data_key_bad", &value0) == WasmResult::NotFound) {
      logDebug("second get of bad key not found");
    }
    auto value1 = getSharedDataValue("shared_data_key1");
    logDebug("get 1 " + value1->toString());
    auto value2 = getSharedDataValue("shared_data_key2");
    logWarn("get 2 " + value2->toString());
  } else if (test == "property") {
    setFilterState("wasm_state", "wasm_value");
    auto path = getRequestHeader(":path");
    if (path->view() == "/test_context") {
      logWarn("request.path: " + getProperty({"request", "path"}).value()->toString());
      logWarn("node.metadata: " +
              getProperty({"node", "metadata", "istio.io/metadata"}).value()->toString());
      logWarn("metadata: " + getProperty({"metadata", "filter_metadata", "envoy.filters.http.wasm",
                                          "wasm_request_get_key"})
                                 .value()
                                 ->toString());
      int64_t responseCode;
      if (getValue({"response", "code"}, &responseCode)) {
        logWarn("response.code: " + std::to_string(responseCode));
      }
      logWarn("state: " + getProperty({"wasm_state"}).value()->toString());
    } else {
      logWarn("onLog " + std::to_string(id()) + " " + std::string(path->view()));
    }

    // Wasm state property set and read validation for {i: 1337}
    // Generated using the following input.json:
    // {
    //   "i": 1337
    // }
    // flatc -b schema.fbs input.json
    {
      static const char data[24] = {0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
                                    0x0c, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
                                    0x39, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      if (WasmResult::Ok != setFilterState("structured_state", StringView(data, 24))) {
        logWarn("setProperty(structured_state) failed");
      }
      int64_t value = 0;
      if (!getValue({"structured_state", "i"}, &value)) {
        logWarn("getProperty(structured_state) failed");
      }
      if (value != 1337) {
        logWarn("getProperty(structured_state) returned " + std::to_string(value));
      }
    }
  }
}

void TestRootContext::onQueueReady(uint32_t token) {
  if (token == shared_queue_token_) {
    logInfo("onQueueReady");
  }
  std::unique_ptr<WasmData> data;
  if (dequeueSharedQueue(9999999 /* bad token */, &data) == WasmResult::NotFound) {
    logWarn("onQueueReady bad token not found");
  }
  if (dequeueSharedQueue(token, &data) == WasmResult::Ok) {
    logDebug("data " + data->toString() + " Ok");
  }
  if (dequeueSharedQueue(token, &data) == WasmResult::Empty) {
    logWarn("onQueueReady extra data not found");
  }
}

void TestContext::onDone() {
  auto test = root()->test_;
  if (test == "headers") {
    logWarn("onDone " + std::to_string(id()));
  }
}

void TestRootContext::onTick() {
  if (test_ == "headers") {
    getContext(stream_context_id_)->setEffectiveContext();
    replaceRequestHeader("server", "envoy-wasm-continue");
    continueRequest();
  } else if (test_ == "metadata") {
    std::string value;
    if (!getValue({"node", "metadata", "wasm_node_get_key"}, &value)) {
      logDebug("missing node metadata");
    }
    logDebug(std::string("onTick ") + value);
  } else if (test_ == "property") {
    uint64_t t;
    if (WasmResult::Ok != proxy_get_current_time_nanoseconds(&t)) {
      logError(std::string("bad proxy_get_current_time_nanoseconds result"));
    }
    {
      std::string function = "declare_property";
      envoy::source::extensions::common::wasm::DeclarePropertyArguments args;
      args.set_name("structured_state");
      args.set_type(envoy::source::extensions::common::wasm::WasmType::FlatBuffers);
      args.set_span(envoy::source::extensions::common::wasm::LifeSpan::DownstreamConnection);
      // Reflection flatbuffer for a simple table {i : int64}.
      // Generated using the following schema.fbs:
      //
      // namespace Wasm.Common;
      // table T {
      //   i: int64;
      // }
      // root_type T;
      //
      // flatc --cpp --bfbs-gen-embed schema.fbs
      static const char bfbsData[192] = {
          0x18, 0x00, 0x00, 0x00, 0x42, 0x46, 0x42, 0x53, 0x10, 0x00, 0x1C, 0x00, 0x04, 0x00, 0x08,
          0x00, 0x0C, 0x00, 0x10, 0x00, 0x14, 0x00, 0x18, 0x00, 0x10, 0x00, 0x00, 0x00, 0x30, 0x00,
          0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x34,
          0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x10, 0x00, 0x04, 0x00,
          0x08, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x08,
          0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
          0x0D, 0x00, 0x00, 0x00, 0x57, 0x61, 0x73, 0x6D, 0x2E, 0x43, 0x6F, 0x6D, 0x6D, 0x6F, 0x6E,
          0x2E, 0x54, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x12, 0x00, 0x08, 0x00, 0x0C, 0x00, 0x00, 0x00,
          0x06, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0C,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x07, 0x00, 0x06, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00};
      args.set_schema(bfbsData, 192);
      std::string in;
      args.SerializeToString(&in);
      char* out = nullptr;
      size_t out_size = 0;
      if (WasmResult::Ok != proxy_call_foreign_function(function.data(), function.size(), in.data(),
                                                        in.size(), &out, &out_size)) {
        logError("declare_property failed");
      }
      ::free(out);
    }
  }
}

class Context1 : public Context {
public:
  Context1(uint32_t id, RootContext* root) : Context(id, root) {}
  FilterHeadersStatus onRequestHeaders(uint32_t, bool) override;
};

class Context2 : public Context {
public:
  Context2(uint32_t id, RootContext* root) : Context(id, root) {}
  FilterHeadersStatus onRequestHeaders(uint32_t, bool) override;
};

static RegisterContextFactory register_Context1(CONTEXT_FACTORY(Context1), "context1");
static RegisterContextFactory register_Contxt2(CONTEXT_FACTORY(Context2), "context2");

FilterHeadersStatus Context1::onRequestHeaders(uint32_t, bool) {
  logDebug(std::string("onRequestHeaders1 ") + std::to_string(id()));
  return FilterHeadersStatus::Continue;
}

FilterHeadersStatus Context2::onRequestHeaders(uint32_t, bool) {
  logDebug(std::string("onRequestHeaders2 ") + std::to_string(id()));
  return FilterHeadersStatus::Continue;
}

END_WASM_PLUGIN
