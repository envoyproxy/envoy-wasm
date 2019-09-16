#pragma once

#include "envoy/grpc/async_client.h"
#include "envoy/local_info/local_info.h"
#include "envoy/network/connection.h"
#include "envoy/service/metrics/v2/metrics_service.pb.h"
#include "envoy/service/metrics/v2/metrics_service.pb.validate.h"
#include "envoy/singleton/instance.h"
#include "envoy/stats/histogram.h"
#include "envoy/stats/sink.h"
#include "envoy/stats/stats.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/buffer/buffer_impl.h"
#include "common/grpc/typed_async_client.h"

namespace Envoy {
namespace Extensions {
namespace StatSinks {
namespace MetricsService {

/**
 * Interface for metrics streamer.
 */
class GrpcMetricsStreamer
    : public Grpc::AsyncStreamCallbacks<envoy::service::metrics::v2::StreamMetricsResponse> {
public:
  ~GrpcMetricsStreamer() override = default;

  /**
   * Send Metrics Message.
   * @param message supplies the metrics to send.
   */
  virtual void send(envoy::service::metrics::v2::StreamMetricsMessage& message) PURE;

  // Grpc::AsyncStreamCallbacks
  void onCreateInitialMetadata(Http::HeaderMap&) override {}
  void onReceiveInitialMetadata(Http::HeaderMapPtr&&) override {}
  void
  onReceiveMessage(std::unique_ptr<envoy::service::metrics::v2::StreamMetricsResponse>&&) override {
  }
  void onReceiveTrailingMetadata(Http::HeaderMapPtr&&) override {}
  void onRemoteClose(Grpc::Status::GrpcStatus, const std::string&) override{};
};

using GrpcMetricsStreamerSharedPtr = std::shared_ptr<GrpcMetricsStreamer>;

/**
 * Production implementation of GrpcMetricsStreamer
 */
class GrpcMetricsStreamerImpl : public Singleton::Instance, public GrpcMetricsStreamer {
public:
  GrpcMetricsStreamerImpl(Grpc::AsyncClientFactoryPtr&& factory,
                          const LocalInfo::LocalInfo& local_info);

  // GrpcMetricsStreamer
  void send(envoy::service::metrics::v2::StreamMetricsMessage& message) override;

  // Grpc::AsyncStreamCallbacks
  void onRemoteClose(Grpc::Status::GrpcStatus, const std::string&) override { stream_ = nullptr; }

private:
  Grpc::AsyncStream<envoy::service::metrics::v2::StreamMetricsMessage> stream_{};
  Grpc::AsyncClient<envoy::service::metrics::v2::StreamMetricsMessage,
                    envoy::service::metrics::v2::StreamMetricsResponse>
      client_;
  const LocalInfo::LocalInfo& local_info_;
};

/**
 * Stat Sink implementation of Metrics Service.
 */
class MetricsServiceSink : public Stats::Sink {
public:
  // MetricsService::Sink
  MetricsServiceSink(const GrpcMetricsStreamerSharedPtr& grpc_metrics_streamer,
                     TimeSource& time_system);
  void flush(Stats::MetricSnapshot& snapshot) override;
  void onHistogramComplete(const Stats::Histogram&, uint64_t) override {}

  void flushCounter(const Stats::Counter& counter);
  void flushGauge(const Stats::Gauge& gauge);
  void flushHistogram(const Stats::ParentHistogram& envoy_histogram);

private:
  GrpcMetricsStreamerSharedPtr grpc_metrics_streamer_;
  envoy::service::metrics::v2::StreamMetricsMessage message_;
  TimeSource& time_source_;
};

} // namespace MetricsService
} // namespace StatSinks
} // namespace Extensions
} // namespace Envoy
