// NOLINT(namespace-envoy)
#include <memory>
#include <string>
#include <unordered_map>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics_lite.h"
#else
#include "extensions/common/wasm/ext/envoy_null_plugin.h"
#endif

START_WASM_PLUGIN(HttpWasmTestCpp)

class GrpcStreamContext : public Context {
public:
  explicit GrpcStreamContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t, bool) override;
};

class GrpcStreamRootContext : public RootContext {
public:
  explicit GrpcStreamRootContext(uint32_t id, std::string_view root_id)
      : RootContext(id, root_id) {}
};

static RegisterContextFactory register_GrpcStreamContext(CONTEXT_FACTORY(GrpcStreamContext),
                                                         ROOT_FACTORY(GrpcStreamRootContext),
                                                         "grpc_stream");
class MyGrpcStreamHandler
    : public GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value> {
public:
  MyGrpcStreamHandler() : GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value>() {}
  void onReceiveInitialMetadata(uint32_t) override {
    auto h = getHeaderMapValue(WasmHeaderMapType::GrpcReceiveInitialMetadata, "foo");
    h = getHeaderMapValue(WasmHeaderMapType::HttpCallResponseHeaders, "foo");
    h = getHeaderMapValue(WasmHeaderMapType::HttpCallResponseTrailers, "foo");
    addHeaderMapValue(WasmHeaderMapType::GrpcReceiveInitialMetadata, "foo", "bar");
  }
  void onReceiveTrailingMetadata(uint32_t) override {
    auto h = getHeaderMapValue(WasmHeaderMapType::GrpcReceiveTrailingMetadata, "foo");
    addHeaderMapValue(WasmHeaderMapType::GrpcReceiveTrailingMetadata, "foo", "bar");
  }
  void onReceive(size_t body_size) override {
    auto response = getBufferBytes(WasmBufferType::GrpcReceiveBuffer, 0, body_size);
    logDebug(response->proto<google::protobuf::Value>().string_value());
    google::protobuf::Value message;
    send(message, false);
  }
  void onRemoteClose(GrpcStatus) override {
    auto p = getStatus();
    logDebug(std::string("failure ") + std::string(p.second->view()));
    close();
  }
};

FilterHeadersStatus GrpcStreamContext::onRequestHeaders(uint32_t, bool) {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  HeaderStringPairs initial_metadata;
  root()->grpcStreamHandler(grpc_service_string, "service", "method", initial_metadata,
                            std::unique_ptr<GrpcStreamHandlerBase>(new MyGrpcStreamHandler()));
  return FilterHeadersStatus::StopIteration;
}

END_WASM_PLUGIN
