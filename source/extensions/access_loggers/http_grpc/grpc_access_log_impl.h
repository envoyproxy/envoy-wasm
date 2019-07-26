#pragma once

#include <unordered_map>
#include <vector>

#include "envoy/config/accesslog/v2/als.pb.h"
#include "envoy/config/filter/accesslog/v2/accesslog.pb.h"
#include "envoy/grpc/async_client.h"
#include "envoy/grpc/async_client_manager.h"
#include "envoy/local_info/local_info.h"
#include "envoy/service/accesslog/v2/als.pb.h"
#include "envoy/singleton/instance.h"
#include "envoy/thread_local/thread_local.h"

#include "common/grpc/typed_async_client.h"

#include "extensions/access_loggers/common/access_log_base.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace HttpGrpc {

// TODO(mattklein123): Stats

/**
 * Interface for an access log streamer. The streamer deals with threading and sends access logs
 * on the correct stream.
 */
class GrpcAccessLogStreamer {
public:
  virtual ~GrpcAccessLogStreamer() = default;

  /**
   * Send an access log.
   * @param message supplies the access log to send.
   * @param log_name supplies the name of the log stream to send on.
   */
  virtual void send(envoy::service::accesslog::v2::StreamAccessLogsMessage& message,
                    const std::string& log_name) PURE;
};

using GrpcAccessLogStreamerSharedPtr = std::shared_ptr<GrpcAccessLogStreamer>;

/**
 * Production implementation of GrpcAccessLogStreamer that supports per-thread and per-log
 * streams.
 */
class GrpcAccessLogStreamerImpl : public Singleton::Instance, public GrpcAccessLogStreamer {
public:
  GrpcAccessLogStreamerImpl(Grpc::AsyncClientFactoryPtr&& factory, ThreadLocal::SlotAllocator& tls,
                            const LocalInfo::LocalInfo& local_info);

  // GrpcAccessLogStreamer
  void send(envoy::service::accesslog::v2::StreamAccessLogsMessage& message,
            const std::string& log_name) override {
    tls_slot_->getTyped<ThreadLocalStreamer>().send(message, log_name);
  }

private:
  /**
   * Shared state that is owned by the per-thread streamers. This allows the main streamer/TLS
   * slot to be destroyed while the streamers hold onto the shared state.
   */
  struct SharedState {
    SharedState(Grpc::AsyncClientFactoryPtr&& factory, const LocalInfo::LocalInfo& local_info)
        : factory_(std::move(factory)), local_info_(local_info) {}

    Grpc::AsyncClientFactoryPtr factory_;
    const LocalInfo::LocalInfo& local_info_;
  };

  using SharedStateSharedPtr = std::shared_ptr<SharedState>;

  struct ThreadLocalStreamer;

  /**
   * Per-thread stream state.
   */
  struct ThreadLocalStream
      : public Grpc::AsyncStreamCallbacks<envoy::service::accesslog::v2::StreamAccessLogsResponse> {
    ThreadLocalStream(ThreadLocalStreamer& parent, const std::string& log_name)
        : parent_(parent), log_name_(log_name) {}

    // Grpc::AsyncStreamCallbacks
    void onCreateInitialMetadata(Http::HeaderMap&) override {}
    void onReceiveInitialMetadata(Http::HeaderMapPtr&&) override {}
    void onReceiveMessage(
        std::unique_ptr<envoy::service::accesslog::v2::StreamAccessLogsResponse>&&) override {}
    void onReceiveTrailingMetadata(Http::HeaderMapPtr&&) override {}
    void onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) override;

    ThreadLocalStreamer& parent_;
    const std::string log_name_;
    Grpc::AsyncStream<envoy::service::accesslog::v2::StreamAccessLogsMessage> stream_{};
  };

  /**
   * Per-thread multi-stream state.
   */
  struct ThreadLocalStreamer : public ThreadLocal::ThreadLocalObject {
    ThreadLocalStreamer(const SharedStateSharedPtr& shared_state);
    void send(envoy::service::accesslog::v2::StreamAccessLogsMessage& message,
              const std::string& log_name);

    Grpc::AsyncClient<envoy::service::accesslog::v2::StreamAccessLogsMessage,
                      envoy::service::accesslog::v2::StreamAccessLogsResponse>
        client_;
    std::unordered_map<std::string, ThreadLocalStream> stream_map_;
    SharedStateSharedPtr shared_state_;
  };

  ThreadLocal::SlotPtr tls_slot_;
};

/**
 * Access log Instance that streams HTTP logs over gRPC.
 */
class HttpGrpcAccessLog : public Common::ImplBase {
public:
  HttpGrpcAccessLog(AccessLog::FilterPtr&& filter,
                    const envoy::config::accesslog::v2::HttpGrpcAccessLogConfig& config,
                    GrpcAccessLogStreamerSharedPtr grpc_access_log_streamer);

  static void responseFlagsToAccessLogResponseFlags(
      envoy::data::accesslog::v2::AccessLogCommon& common_access_log,
      const StreamInfo::StreamInfo& stream_info);

private:
  // Common::ImplBase
  void emitLog(const Http::HeaderMap& request_headers, const Http::HeaderMap& response_headers,
               const Http::HeaderMap& response_trailers,
               const StreamInfo::StreamInfo& stream_info) override;

  const envoy::config::accesslog::v2::HttpGrpcAccessLogConfig config_;
  GrpcAccessLogStreamerSharedPtr grpc_access_log_streamer_;
  std::vector<Http::LowerCaseString> request_headers_to_log_;
  std::vector<Http::LowerCaseString> response_headers_to_log_;
  std::vector<Http::LowerCaseString> response_trailers_to_log_;
};

} // namespace HttpGrpc
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
