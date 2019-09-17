#include "common/upstream/health_discovery_service.h"

#include "envoy/stats/scope.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Upstream {

HdsDelegate::HdsDelegate(Stats::Scope& scope, Grpc::RawAsyncClientPtr async_client,
                         Event::Dispatcher& dispatcher, Runtime::Loader& runtime,
                         Envoy::Stats::Store& stats, Ssl::ContextManager& ssl_context_manager,
                         Runtime::RandomGenerator& random, ClusterInfoFactory& info_factory,
                         AccessLog::AccessLogManager& access_log_manager, ClusterManager& cm,
                         const LocalInfo::LocalInfo& local_info, Server::Admin& admin,
                         Singleton::Manager& singleton_manager, ThreadLocal::SlotAllocator& tls,
                         ProtobufMessage::ValidationVisitor& validation_visitor, Api::Api& api)
    : stats_{ALL_HDS_STATS(POOL_COUNTER_PREFIX(scope, "hds_delegate."))},
      service_method_(*Protobuf::DescriptorPool::generated_pool()->FindMethodByName(
          "envoy.service.discovery.v2.HealthDiscoveryService.StreamHealthCheck")),
      async_client_(std::move(async_client)), dispatcher_(dispatcher), runtime_(runtime),
      store_stats(stats), ssl_context_manager_(ssl_context_manager), random_(random),
      info_factory_(info_factory), access_log_manager_(access_log_manager), cm_(cm),
      local_info_(local_info), admin_(admin), singleton_manager_(singleton_manager), tls_(tls),
      validation_visitor_(validation_visitor), api_(api) {
  health_check_request_.mutable_health_check_request()->mutable_node()->MergeFrom(
      local_info_.node());
  backoff_strategy_ = std::make_unique<JitteredBackOffStrategy>(RetryInitialDelayMilliseconds,
                                                                RetryMaxDelayMilliseconds, random_);
  hds_retry_timer_ = dispatcher.createTimer([this]() -> void { establishNewStream(); });
  hds_stream_response_timer_ = dispatcher.createTimer([this]() -> void { sendResponse(); });

  // TODO(lilika): Add support for other types of healthchecks
  health_check_request_.mutable_health_check_request()
      ->mutable_capability()
      ->add_health_check_protocols(envoy::service::discovery::v2::Capability::HTTP);
  health_check_request_.mutable_health_check_request()
      ->mutable_capability()
      ->add_health_check_protocols(envoy::service::discovery::v2::Capability::TCP);

  establishNewStream();
}

void HdsDelegate::setHdsRetryTimer() {
  const auto retry_ms = std::chrono::milliseconds(backoff_strategy_->nextBackOffMs());
  ENVOY_LOG(warn, "HdsDelegate stream/connection failure, will retry in {} ms.", retry_ms.count());

  hds_retry_timer_->enableTimer(retry_ms);
}

void HdsDelegate::setHdsStreamResponseTimer() {
  hds_stream_response_timer_->enableTimer(std::chrono::milliseconds(server_response_ms_));
}

void HdsDelegate::establishNewStream() {
  ENVOY_LOG(debug, "Establishing new gRPC bidi stream for {}", service_method_.DebugString());
  stream_ = async_client_->start(service_method_, *this);
  if (stream_ == nullptr) {
    ENVOY_LOG(warn, "Unable to establish new stream");
    handleFailure();
    return;
  }

  ENVOY_LOG(debug, "Sending HealthCheckRequest {} ", health_check_request_.DebugString());
  stream_->sendMessage(health_check_request_, false);
  stats_.responses_.inc();
  backoff_strategy_->reset();
}

void HdsDelegate::handleFailure() {
  stats_.errors_.inc();
  setHdsRetryTimer();
}

// TODO(lilika): Add support for the same endpoint in different clusters/ports
envoy::service::discovery::v2::HealthCheckRequestOrEndpointHealthResponse
HdsDelegate::sendResponse() {
  envoy::service::discovery::v2::HealthCheckRequestOrEndpointHealthResponse response;
  for (const auto& cluster : hds_clusters_) {
    for (const auto& hosts : cluster->prioritySet().hostSetsPerPriority()) {
      for (const auto& host : hosts->hosts()) {
        auto* endpoint = response.mutable_endpoint_health_response()->add_endpoints_health();
        Network::Utility::addressToProtobufAddress(
            *host->address(), *endpoint->mutable_endpoint()->mutable_address());
        // TODO(lilika): Add support for more granular options of envoy::api::v2::core::HealthStatus
        if (host->health() == Host::Health::Healthy) {
          endpoint->set_health_status(envoy::api::v2::core::HealthStatus::HEALTHY);
        } else {
          if (host->getActiveHealthFailureType() == Host::ActiveHealthFailureType::TIMEOUT) {
            endpoint->set_health_status(envoy::api::v2::core::HealthStatus::TIMEOUT);
          } else if (host->getActiveHealthFailureType() ==
                     Host::ActiveHealthFailureType::UNHEALTHY) {
            endpoint->set_health_status(envoy::api::v2::core::HealthStatus::UNHEALTHY);
          } else if (host->getActiveHealthFailureType() == Host::ActiveHealthFailureType::UNKNOWN) {
            endpoint->set_health_status(envoy::api::v2::core::HealthStatus::UNHEALTHY);
          } else {
            NOT_REACHED_GCOVR_EXCL_LINE;
          }
        }
      }
    }
  }
  ENVOY_LOG(debug, "Sending EndpointHealthResponse to server {}", response.DebugString());
  stream_->sendMessage(response, false);
  stats_.responses_.inc();
  setHdsStreamResponseTimer();
  return response;
}

void HdsDelegate::onCreateInitialMetadata(Http::HeaderMap& metadata) {
  UNREFERENCED_PARAMETER(metadata);
}

void HdsDelegate::onReceiveInitialMetadata(Http::HeaderMapPtr&& metadata) {
  UNREFERENCED_PARAMETER(metadata);
}

void HdsDelegate::processMessage(
    std::unique_ptr<envoy::service::discovery::v2::HealthCheckSpecifier>&& message) {
  ENVOY_LOG(debug, "New health check response message {} ", message->DebugString());
  ASSERT(message);

  for (const auto& cluster_health_check : message->cluster_health_checks()) {
    // Create HdsCluster config
    static const envoy::api::v2::core::BindConfig bind_config;
    envoy::api::v2::Cluster cluster_config;

    cluster_config.set_name(cluster_health_check.cluster_name());
    cluster_config.mutable_connect_timeout()->set_seconds(ClusterTimeoutSeconds);
    cluster_config.mutable_per_connection_buffer_limit_bytes()->set_value(
        ClusterConnectionBufferLimitBytes);

    // Add endpoints to cluster
    for (const auto& locality_endpoints : cluster_health_check.locality_endpoints()) {
      for (const auto& endpoint : locality_endpoints.endpoints()) {
        cluster_config.add_hosts()->MergeFrom(endpoint.address());
      }
    }

    // TODO(lilika): Add support for optional per-endpoint health checks

    // Add healthchecks to cluster
    for (auto& health_check : cluster_health_check.health_checks()) {
      cluster_config.add_health_checks()->MergeFrom(health_check);
    }

    ENVOY_LOG(debug, "New HdsCluster config {} ", cluster_config.DebugString());

    // Create HdsCluster
    hds_clusters_.emplace_back(new HdsCluster(admin_, runtime_, cluster_config, bind_config,
                                              store_stats, ssl_context_manager_, false,
                                              info_factory_, cm_, local_info_, dispatcher_, random_,
                                              singleton_manager_, tls_, validation_visitor_, api_));

    hds_clusters_.back()->startHealthchecks(access_log_manager_, runtime_, random_, dispatcher_,
                                            api_);
  }
}

// TODO(lilika): Add support for subsequent HealthCheckSpecifier messages that
// might modify the HdsClusters
void HdsDelegate::onReceiveMessage(
    std::unique_ptr<envoy::service::discovery::v2::HealthCheckSpecifier>&& message) {
  stats_.requests_.inc();
  ENVOY_LOG(debug, "New health check response message {} ", message->DebugString());

  // Reset
  hds_clusters_.clear();

  // Set response
  auto server_response_ms = PROTOBUF_GET_MS_REQUIRED(*message, interval);

  // Process the HealthCheckSpecifier message
  processMessage(std::move(message));

  if (server_response_ms_ != server_response_ms) {
    server_response_ms_ = server_response_ms;
    setHdsStreamResponseTimer();
  }
}

void HdsDelegate::onReceiveTrailingMetadata(Http::HeaderMapPtr&& metadata) {
  UNREFERENCED_PARAMETER(metadata);
}

void HdsDelegate::onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) {
  ENVOY_LOG(warn, "gRPC config stream closed: {}, {}", status, message);
  hds_stream_response_timer_->disableTimer();
  stream_ = nullptr;
  server_response_ms_ = 0;
  handleFailure();
}

HdsCluster::HdsCluster(Server::Admin& admin, Runtime::Loader& runtime,
                       const envoy::api::v2::Cluster& cluster,
                       const envoy::api::v2::core::BindConfig& bind_config, Stats::Store& stats,
                       Ssl::ContextManager& ssl_context_manager, bool added_via_api,
                       ClusterInfoFactory& info_factory, ClusterManager& cm,
                       const LocalInfo::LocalInfo& local_info, Event::Dispatcher& dispatcher,
                       Runtime::RandomGenerator& random, Singleton::Manager& singleton_manager,
                       ThreadLocal::SlotAllocator& tls,
                       ProtobufMessage::ValidationVisitor& validation_visitor, Api::Api& api)
    : runtime_(runtime), cluster_(cluster), bind_config_(bind_config), stats_(stats),
      ssl_context_manager_(ssl_context_manager), added_via_api_(added_via_api),
      initial_hosts_(new HostVector()), validation_visitor_(validation_visitor) {
  ENVOY_LOG(debug, "Creating an HdsCluster");
  priority_set_.getOrCreateHostSet(0);

  info_ = info_factory.createClusterInfo(
      {admin, runtime_, cluster_, bind_config_, stats_, ssl_context_manager_, added_via_api_, cm,
       local_info, dispatcher, random, singleton_manager, tls, validation_visitor, api});

  for (const auto& host : cluster.hosts()) {
    initial_hosts_->emplace_back(
        new HostImpl(info_, "", Network::Address::resolveProtoAddress(host),
                     envoy::api::v2::core::Metadata::default_instance(), 1,
                     envoy::api::v2::core::Locality().default_instance(),
                     envoy::api::v2::endpoint::Endpoint::HealthCheckConfig().default_instance(), 0,
                     envoy::api::v2::core::HealthStatus::UNKNOWN));
  }
  initialize([] {});
}

ClusterSharedPtr HdsCluster::create() { NOT_IMPLEMENTED_GCOVR_EXCL_LINE; }

ClusterInfoConstSharedPtr
ProdClusterInfoFactory::createClusterInfo(const CreateClusterInfoParams& params) {
  Envoy::Stats::ScopePtr scope =
      params.stats_.createScope(fmt::format("cluster.{}.", params.cluster_.name()));

  Envoy::Server::Configuration::TransportSocketFactoryContextImpl factory_context(
      params.admin_, params.ssl_context_manager_, *scope, params.cm_, params.local_info_,
      params.dispatcher_, params.random_, params.stats_, params.singleton_manager_, params.tls_,
      params.validation_visitor_, params.api_);

  // TODO(JimmyCYJ): Support SDS for HDS cluster.
  Network::TransportSocketFactoryPtr socket_factory =
      Upstream::createTransportSocketFactory(params.cluster_, factory_context);

  return std::make_unique<ClusterInfoImpl>(
      params.cluster_, params.bind_config_, params.runtime_, std::move(socket_factory),
      std::move(scope), params.added_via_api_, params.validation_visitor_, factory_context);
}

void HdsCluster::startHealthchecks(AccessLog::AccessLogManager& access_log_manager,
                                   Runtime::Loader& runtime, Runtime::RandomGenerator& random,
                                   Event::Dispatcher& dispatcher, Api::Api& api) {
  for (auto& health_check : cluster_.health_checks()) {
    health_checkers_.push_back(
        Upstream::HealthCheckerFactory::create(health_check, *this, runtime, random, dispatcher,
                                               access_log_manager, validation_visitor_, api));
    health_checkers_.back()->start();
  }
}

void HdsCluster::initialize(std::function<void()> callback) {
  initialization_complete_callback_ = callback;
  for (const auto& host : *initial_hosts_) {
    host->healthFlagSet(Host::HealthFlag::FAILED_ACTIVE_HC);
  }

  priority_set_.updateHosts(
      0, HostSetImpl::partitionHosts(initial_hosts_, HostsPerLocalityImpl::empty()), {},
      *initial_hosts_, {}, absl::nullopt);
}

void HdsCluster::setOutlierDetector(const Outlier::DetectorSharedPtr&) {
  NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
}

} // namespace Upstream
} // namespace Envoy
