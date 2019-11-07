// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>
#include "proxy_wasm_intrinsics.h"
#include "proxy_wasm_intrinsics_lite.pb.h"

class ServiceContext : public RootContext {
public:
  explicit ServiceContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}

  bool onStart(size_t /* vm_configuration */) override {
    CHECK_RESULT(
        defineMetric(MetricType::Counter, "test_callout_successes", &callout_success_counter_));
    CHECK_RESULT(
        defineMetric(MetricType::Counter, "test_callout_failures", &callout_failure_counter_));
    return true;
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

class RequestContext : public Context {
public:
  explicit RequestContext(uint32_t id, RootContext* root) : Context(id, root) {}

  ServiceContext* serviceContext() { return static_cast<ServiceContext*>(root()); }

  FilterHeadersStatus onRequestHeaders(uint32_t) override;
};

static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(RequestContext),
                                                      ROOT_FACTORY(ServiceContext));

class CalloutResponseHandler : public GrpcCallHandler<google::protobuf::Value> {
public:
  CalloutResponseHandler(RequestContext* request_context, ServiceContext* service_context)
      : GrpcCallHandler(), request_context_(request_context), service_context_(service_context) {}

  void onCreateInitialMetadata(uint32_t) override {}
  void onSuccess(size_t body_size) override {
    auto response = getBufferBytes(BufferType::GrpcReceiveBuffer, 0, body_size);
    request_context_->setEffectiveContext();
    logDebug(response->proto<google::protobuf::Value>().string_value());

    service_context_->incrementCalloutSuccesses();

    continueRequest();
  }

  void onFailure(GrpcStatus status) override {
    auto p = getStatus();
    request_context_->setEffectiveContext();
    logInfo(std::string("failure ") + std::to_string(static_cast<int>(status)) + " " +
            std::string(p.second->view()));

    service_context_->incrementCalloutFailures();

    sendLocalResponse(502, p.second->view(), "", {}, status);
  }

private:
  RequestContext* const request_context_;
  ServiceContext* const service_context_;
};

FilterHeadersStatus RequestContext::onRequestHeaders(uint32_t) {
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name("callout_cluster");
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);

  google::protobuf::Value value;
  value.set_string_value("request");

  root()->grpcCallHandler(
      grpc_service_string, "service", "method", value, 1000,
      std::unique_ptr<GrpcCallHandlerBase>(new CalloutResponseHandler(this, serviceContext())));
  return FilterHeadersStatus::StopIteration;
}
