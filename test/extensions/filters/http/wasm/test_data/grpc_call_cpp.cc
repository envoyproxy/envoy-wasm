// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"
#include "proxy_wasm_intrinsics_lite.pb.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeadersSimple(uint32_t);
  FilterHeadersStatus onRequestHeadersStream(uint32_t);
  FilterHeadersStatus onRequestHeaders(uint32_t) override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

class MyGrpcCallHandler : public GrpcCallHandler<google::protobuf::Value> {
public:
  MyGrpcCallHandler() : GrpcCallHandler<google::protobuf::Value>() {}
  void onSuccess(size_t body_size) override {
    auto response = getBufferBytes(BufferType::GrpcReceiveBuffer, 0, body_size);
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
    auto response = getBufferBytes(BufferType::GrpcReceiveBuffer, 0, body_size);
    logDebug(response->proto<google::protobuf::Value>().string_value());
  }
  void onRemoteClose(GrpcStatus status) override {
    auto p = getStatus();
    logDebug(std::string("failure ") + std::to_string(static_cast<int>(status)) +
             std::string(p.second->view()));
    close();
  }
};

// Currently unused.
FilterHeadersStatus ExampleContext::onRequestHeadersSimple(uint32_t) {
  std::function<void(size_t body_size)> success_callback = [](size_t body_size) {
    auto response = getBufferBytes(BufferType::GrpcReceiveBuffer, 0, body_size);
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
FilterHeadersStatus ExampleContext::onRequestHeadersStream(uint32_t) {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  HeaderStringPairs initial_metadata;
  root()->grpcStreamHandler(grpc_service_string, "service", "method", initial_metadata,
                            std::unique_ptr<GrpcStreamHandlerBase>(new MyGrpcStreamHandler()));
  return FilterHeadersStatus::StopIteration;
}

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t) {
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
