// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>
#include "proxy_wasm_intrinsics.h"
#include "proxy_wasm_intrinsics_lite.pb.h"

class RequestContext : public Context {
 public:
  explicit RequestContext(uint32_t id, RootContext *root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
};

class ServiceContext : public RootContext {
 public:
  explicit ServiceContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  void onStart() override {
    callout_success_counter_ = defineMetric(MetricType::Counter, "test_callout_successes");
    callout_failure_counter_ = defineMetric(MetricType::Counter, "test_callout_failures");
  }

  void incrementCalloutSuccesses(uint32_t inc_amount = 1U) {
    incrementMetric(callout_success_counter_, inc_amount);
  }

  void incrementCalloutFailures(uint32_t inc_amount = 1U) {
    incrementMetric(callout_failure_counter_, inc_amount);
  }

 private:
  uint32_t callout_success_counter_;
  uint32_t callout_failure_counter_;
};

static RegisterContextFactory
    register_ExampleContext(CONTEXT_FACTORY(RequestContext), ROOT_FACTORY(ServiceContext));

class CalloutResponseHandler : public GrpcCallHandler<google::protobuf::Value> {
 public:
  CalloutResponseHandler(RequestContext *context)
      : GrpcCallHandler<google::protobuf::Value>(context), context_(context) {}
  void onCreateInitialMetadata() override {}
  void onSuccess(google::protobuf::Value &&response) override {
    logDebug(response.string_value());

    serviceContext()->incrementCalloutSuccesses();

    continueRequest();
  }
  void onFailure(GrpcStatus status,
                 std::unique_ptr<WasmData> error_message) override {
    logInfo(std::string("failure ") + std::to_string(static_cast<int>(status)) +
            std::string(error_message->view()));

    serviceContext()->incrementCalloutFailures();

    // TODO wasm engine must support fail closed: expose abortRequest() or similar
    continueRequest();
  }

 private:
  ServiceContext *serviceContext() {
    return static_cast<ServiceContext *>(context_->root());
  }

  RequestContext *context_;
};

FilterHeadersStatus RequestContext::onRequestHeaders() {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("callout_cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);

  google::protobuf::Value value;
  value.set_string_value("request");

  grpcCallHandler(grpc_service_string, "service", "method", value, 1000,
                  std::unique_ptr<GrpcCallHandlerBase>(new CalloutResponseHandler(this)));
  return FilterHeadersStatus::StopIteration;
}
