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

class GrpcCallContext : public Context {
public:
  explicit GrpcCallContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t, bool) override;
};

class GrpcCallRootContext : public RootContext {
public:
  explicit GrpcCallRootContext(uint32_t id, std::string_view root_id) : RootContext(id, root_id) {}
};

static RegisterContextFactory register_GrpcCallContext(CONTEXT_FACTORY(GrpcCallContext),
                                                       ROOT_FACTORY(GrpcCallRootContext),
                                                       "grpc_call");

class MyGrpcCallHandler : public GrpcCallHandler<google::protobuf::Value> {
public:
  MyGrpcCallHandler() : GrpcCallHandler<google::protobuf::Value>() {}
  void onSuccess(size_t body_size) override {
    auto response = getBufferBytes(WasmBufferType::GrpcReceiveBuffer, 0, body_size);
    logDebug(response->proto<google::protobuf::Value>().string_value());
    cancel();
  }
  void onFailure(GrpcStatus) override {
    auto p = getStatus();
    logDebug(std::string("failure ") + std::string(p.second->view()));
  }
};

FilterHeadersStatus GrpcCallContext::onRequestHeaders(uint32_t, bool) {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  google::protobuf::Value value;
  value.set_string_value("request");
  HeaderStringPairs initial_metadata;
  root()->grpcCallHandler(grpc_service_string, "service", "method", initial_metadata, value, 1000,
                          std::unique_ptr<GrpcCallHandlerBase>(new MyGrpcCallHandler()));
  if (root()->grpcCallHandler(
          "bogus grpc_service", "service", "method", initial_metadata, value, 1000,
          std::unique_ptr<GrpcCallHandlerBase>(new MyGrpcCallHandler())) == WasmResult::Ok) {
    logError("bogus grpc_service accepted error");
  }
  return FilterHeadersStatus::StopIteration;
}

END_WASM_PLUGIN
