// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"
#include "proxy_wasm_intrinsics_lite.pb.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeadersSimple();
  FilterHeadersStatus onRequestHeadersStream();
  FilterHeadersStatus onRequestHeaders() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

class MyGrpcCallHandler : public GrpcCallHandler<google::protobuf::Value> {
public:
  MyGrpcCallHandler() : GrpcCallHandler<google::protobuf::Value>() {}
  void onCreateInitialMetadata() override {}
  void onSuccess(google::protobuf::Value&& response) override { logDebug(response.string_value()); }
  void onFailure(GrpcStatus status, std::unique_ptr<WasmData> error_message) override {
    logDebug(std::string("failure ") + std::to_string(static_cast<int>(status)) +
             std::string(error_message->view()));
  }
};

// Currently unused.
class MyGrpcStreamHandler
    : public GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value> {
public:
  MyGrpcStreamHandler() : GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value>() {}
  void onCreateInitialMetadata() override {
    google::protobuf::Value value;
    value.set_string_value("request");
    send(value, false);
  }
  void onReceiveInitialMetadata() override {}
  void onReceiveTrailingMetadata() override {}
  void onReceive(google::protobuf::Value&& response) override { logDebug(response.string_value()); }
  void onRemoteClose(GrpcStatus status, std::unique_ptr<WasmData> error_message) override {
    logDebug(std::string("failure ") + std::to_string(static_cast<int>(status)) +
             std::string(error_message->view()));
    close();
  }
};

// Currently unused.
FilterHeadersStatus ExampleContext::onRequestHeadersSimple() {
  std::function<void(google::protobuf::Value &&)> success_callback =
      [](google::protobuf::Value&& value) { logDebug(value.string_value()); };
  std::function<void(GrpcStatus status, std::string_view error_message)> failure_callback =
      [](GrpcStatus status, std::string_view message) {
        logDebug(std::string("failure ") + std::to_string(static_cast<int>(status)) +
                 std::string(message));
      };
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  google::protobuf::Value value;
  value.set_string_value("request");
  root()->grpcSimpleCall(grpc_service_string, "service", "method", value, 1000, success_callback,
                         failure_callback);
  return FilterHeadersStatus::StopIteration;
}

// Currently unused.
FilterHeadersStatus ExampleContext::onRequestHeadersStream() {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  root()->grpcStreamHandler(grpc_service_string, "service", "method",
                            std::unique_ptr<GrpcStreamHandlerBase>(new MyGrpcStreamHandler()));
  return FilterHeadersStatus::StopIteration;
}

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  google::protobuf::Value value;
  value.set_string_value("request");
  root()->grpcCallHandler(grpc_service_string, "service", "method", value, 1000,
                          std::unique_ptr<GrpcCallHandlerBase>(new MyGrpcCallHandler()));
  return FilterHeadersStatus::StopIteration;
}
