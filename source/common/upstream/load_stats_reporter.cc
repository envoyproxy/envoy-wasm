#include "common/upstream/load_stats_reporter.h"

#include "envoy/service/load_stats/v3alpha/lrs.pb.h"
#include "envoy/stats/scope.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Upstream {

LoadStatsReporter::LoadStatsReporter(const LocalInfo::LocalInfo& local_info,
                                     ClusterManager& cluster_manager, Stats::Scope& scope,
                                     Grpc::RawAsyncClientPtr async_client,
                                     Event::Dispatcher& dispatcher)
    : cm_(cluster_manager), stats_{ALL_LOAD_REPORTER_STATS(
                                POOL_COUNTER_PREFIX(scope, "load_reporter."))},
      async_client_(std::move(async_client)),
      service_method_(*Protobuf::DescriptorPool::generated_pool()->FindMethodByName(
          "envoy.service.load_stats.v2.LoadReportingService.StreamLoadStats")),
      time_source_(dispatcher.timeSource()) {
  request_.mutable_node()->MergeFrom(local_info.node());
  retry_timer_ = dispatcher.createTimer([this]() -> void { establishNewStream(); });
  response_timer_ = dispatcher.createTimer([this]() -> void { sendLoadStatsRequest(); });
  establishNewStream();
}

void LoadStatsReporter::setRetryTimer() {
  retry_timer_->enableTimer(std::chrono::milliseconds(RETRY_DELAY_MS));
}

void LoadStatsReporter::establishNewStream() {
  ENVOY_LOG(debug, "Establishing new gRPC bidi stream for {}", service_method_.DebugString());
  stream_ = async_client_->start(service_method_, *this, Http::AsyncClient::StreamOptions());
  if (stream_ == nullptr) {
    ENVOY_LOG(warn, "Unable to establish new stream");
    handleFailure();
    return;
  }

  request_.mutable_cluster_stats()->Clear();
  sendLoadStatsRequest();
}

void LoadStatsReporter::sendLoadStatsRequest() {
  request_.mutable_cluster_stats()->Clear();
  for (const auto& cluster_name_and_timestamp : clusters_) {
    const std::string& cluster_name = cluster_name_and_timestamp.first;
    auto cluster_info_map = cm_.clusters();
    auto it = cluster_info_map.find(cluster_name);
    if (it == cluster_info_map.end()) {
      ENVOY_LOG(debug, "Cluster {} does not exist", cluster_name);
      continue;
    }
    auto& cluster = it->second.get();
    auto* cluster_stats = request_.add_cluster_stats();
    cluster_stats->set_cluster_name(cluster_name);
    if (cluster.info()->eds_service_name().has_value()) {
      cluster_stats->set_cluster_service_name(cluster.info()->eds_service_name().value());
    }
    for (auto& host_set : cluster.prioritySet().hostSetsPerPriority()) {
      ENVOY_LOG(trace, "Load report locality count {}", host_set->hostsPerLocality().get().size());
      for (auto& hosts : host_set->hostsPerLocality().get()) {
        ASSERT(!hosts.empty());
        uint64_t rq_success = 0;
        uint64_t rq_error = 0;
        uint64_t rq_active = 0;
        uint64_t rq_issued = 0;
        for (const auto& host : hosts) {
          rq_success += host->stats().rq_success_.latch();
          rq_error += host->stats().rq_error_.latch();
          rq_active += host->stats().rq_active_.value();
          rq_issued += host->stats().rq_total_.latch();
        }
        if (rq_success + rq_error + rq_active != 0) {
          auto* locality_stats = cluster_stats->add_upstream_locality_stats();
          locality_stats->mutable_locality()->MergeFrom(hosts[0]->locality());
          locality_stats->set_priority(host_set->priority());
          locality_stats->set_total_successful_requests(rq_success);
          locality_stats->set_total_error_requests(rq_error);
          locality_stats->set_total_requests_in_progress(rq_active);
          locality_stats->set_total_issued_requests(rq_issued);
        }
      }
    }
    cluster_stats->set_total_dropped_requests(
        cluster.info()->loadReportStats().upstream_rq_dropped_.latch());
    const auto now = time_source_.monotonicTime().time_since_epoch();
    const auto measured_interval = now - cluster_name_and_timestamp.second;
    cluster_stats->mutable_load_report_interval()->MergeFrom(
        Protobuf::util::TimeUtil::MicrosecondsToDuration(
            std::chrono::duration_cast<std::chrono::microseconds>(measured_interval).count()));
    clusters_[cluster_name] = now;
  }

  ENVOY_LOG(trace, "Sending LoadStatsRequest: {}", request_.DebugString());
  stream_->sendMessage(request_, false);
  stats_.responses_.inc();
  // When the connection is established, the message has not yet been read so we
  // will not have a load reporting period.
  if (message_) {
    startLoadReportPeriod();
  }
}

void LoadStatsReporter::handleFailure() {
  ENVOY_LOG(warn, "Load reporter stats stream/connection failure, will retry in {} ms.",
            RETRY_DELAY_MS);
  stats_.errors_.inc();
  setRetryTimer();
}

void LoadStatsReporter::onCreateInitialMetadata(Http::HeaderMap& metadata) {
  UNREFERENCED_PARAMETER(metadata);
}

void LoadStatsReporter::onReceiveInitialMetadata(Http::HeaderMapPtr&& metadata) {
  UNREFERENCED_PARAMETER(metadata);
}

void LoadStatsReporter::onReceiveMessage(
    std::unique_ptr<envoy::service::load_stats::v3alpha::LoadStatsResponse>&& message) {
  ENVOY_LOG(debug, "New load report epoch: {}", message->DebugString());
  stats_.requests_.inc();
  message_ = std::move(message);
  startLoadReportPeriod();
}

void LoadStatsReporter::startLoadReportPeriod() {
  // Once a cluster is tracked, we don't want to reset its stats between reports
  // to avoid racing between request/response.
  // TODO(htuch): They key here could be absl::string_view, but this causes
  // problems due to referencing of temporaries in the below loop with Google's
  // internal string type. Consider this optimization when the string types
  // converge.
  std::unordered_map<std::string, std::chrono::steady_clock::duration> existing_clusters;
  for (const std::string& cluster_name : message_->clusters()) {
    if (clusters_.count(cluster_name) > 0) {
      existing_clusters.emplace(cluster_name, clusters_[cluster_name]);
    }
  }
  clusters_.clear();
  // Reset stats for all hosts in clusters we are tracking.
  for (const std::string& cluster_name : message_->clusters()) {
    clusters_.emplace(cluster_name, existing_clusters.count(cluster_name) > 0
                                        ? existing_clusters[cluster_name]
                                        : time_source_.monotonicTime().time_since_epoch());
    auto cluster_info_map = cm_.clusters();
    auto it = cluster_info_map.find(cluster_name);
    if (it == cluster_info_map.end()) {
      continue;
    }
    // Don't reset stats for existing tracked clusters.
    if (existing_clusters.count(cluster_name) > 0) {
      continue;
    }
    auto& cluster = it->second.get();
    for (auto& host_set : cluster.prioritySet().hostSetsPerPriority()) {
      for (const auto& host : host_set->hosts()) {
        host->stats().rq_success_.latch();
        host->stats().rq_error_.latch();
        host->stats().rq_total_.latch();
      }
    }
    cluster.info()->loadReportStats().upstream_rq_dropped_.latch();
  }
  response_timer_->enableTimer(std::chrono::milliseconds(
      DurationUtil::durationToMilliseconds(message_->load_reporting_interval())));
}

void LoadStatsReporter::onReceiveTrailingMetadata(Http::HeaderMapPtr&& metadata) {
  UNREFERENCED_PARAMETER(metadata);
}

void LoadStatsReporter::onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) {
  ENVOY_LOG(warn, "gRPC config stream closed: {}, {}", status, message);
  response_timer_->disableTimer();
  stream_ = nullptr;
  handleFailure();
}

} // namespace Upstream
} // namespace Envoy
