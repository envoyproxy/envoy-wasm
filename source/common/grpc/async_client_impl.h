#pragma once

#include "envoy/grpc/async_client.h"

#include "common/common/linked_object.h"
#include "common/grpc/codec.h"
#include "common/grpc/typed_async_client.h"
#include "common/http/async_client_impl.h"

namespace Envoy {
namespace Grpc {

class AsyncRequestImpl;
class AsyncStreamImpl;

class AsyncClientImpl final : public RawAsyncClient {
public:
  AsyncClientImpl(Upstream::ClusterManager& cm, const envoy::api::v2::core::GrpcService& config,
                  TimeSource& time_source);
  ~AsyncClientImpl() override;

  // Grpc::AsyncClient
  AsyncRequest* sendRaw(absl::string_view service_full_name, absl::string_view method_name,
                        Buffer::InstancePtr&& request, RawAsyncRequestCallbacks& callbacks,
                        Tracing::Span& parent_span,
                        const absl::optional<std::chrono::milliseconds>& timeout) override;
  RawAsyncStream* startRaw(absl::string_view service_full_name, absl::string_view method_name,
                           RawAsyncStreamCallbacks& callbacks) override;

private:
  Upstream::ClusterManager& cm_;
  const std::string remote_cluster_name_;
  const Protobuf::RepeatedPtrField<envoy::api::v2::core::HeaderValue> initial_metadata_;
  std::list<std::unique_ptr<AsyncStreamImpl>> active_streams_;
  TimeSource& time_source_;

  friend class AsyncRequestImpl;
  friend class AsyncStreamImpl;
};

class AsyncStreamImpl : public RawAsyncStream,
                        Http::AsyncClient::StreamCallbacks,
                        public Event::DeferredDeletable,
                        LinkedObject<AsyncStreamImpl> {
public:
  AsyncStreamImpl(AsyncClientImpl& parent, absl::string_view service_full_name,
                  absl::string_view method_name, RawAsyncStreamCallbacks& callbacks,
                  const absl::optional<std::chrono::milliseconds>& timeout);

  virtual void initialize(bool buffer_body_for_retry);

  void sendMessage(const Protobuf::Message& request, bool end_stream);

  // Http::AsyncClient::StreamCallbacks
  void onHeaders(Http::HeaderMapPtr&& headers, bool end_stream) override;
  void onData(Buffer::Instance& data, bool end_stream) override;
  void onTrailers(Http::HeaderMapPtr&& trailers) override;
  void onComplete() override;
  void onReset() override;

  // Grpc::AsyncStream
  void sendMessageRaw(Buffer::InstancePtr&& request, bool end_stream) override;
  void closeStream() override;
  void resetStream() override;

  bool hasResetStream() const { return http_reset_; }

private:
  void streamError(Status::GrpcStatus grpc_status, const std::string& message);
  void streamError(Status::GrpcStatus grpc_status) { streamError(grpc_status, EMPTY_STRING); }

  void cleanup();
  void trailerResponse(absl::optional<Status::GrpcStatus> grpc_status,
                       const std::string& grpc_message);

  Event::Dispatcher* dispatcher_{};
  Http::MessagePtr headers_message_;
  AsyncClientImpl& parent_;
  std::string service_full_name_;
  std::string method_name_;
  RawAsyncStreamCallbacks& callbacks_;
  const absl::optional<std::chrono::milliseconds>& timeout_;
  bool http_reset_{};
  Http::AsyncClient::Stream* stream_{};
  Decoder decoder_;
  // This is a member to avoid reallocation on every onData().
  std::vector<Frame> decoded_frames_;

  friend class AsyncClientImpl;
};

class AsyncRequestImpl : public AsyncRequest, public AsyncStreamImpl, RawAsyncStreamCallbacks {
public:
  AsyncRequestImpl(AsyncClientImpl& parent, absl::string_view service_full_name,
                   absl::string_view method_name, Buffer::InstancePtr&& request,
                   RawAsyncRequestCallbacks& callbacks, Tracing::Span& parent_span,
                   const absl::optional<std::chrono::milliseconds>& timeout);

  void initialize(bool buffer_body_for_retry) override;

  // Grpc::AsyncRequest
  void cancel() override;

private:
  // Grpc::AsyncStreamCallbacks
  void onCreateInitialMetadata(Http::HeaderMap& metadata) override;
  void onReceiveInitialMetadata(Http::HeaderMapPtr&&) override;
  bool onReceiveMessageRaw(Buffer::InstancePtr&& response) override;
  void onReceiveTrailingMetadata(Http::HeaderMapPtr&&) override;
  void onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) override;

  Buffer::InstancePtr request_;
  RawAsyncRequestCallbacks& callbacks_;
  Tracing::SpanPtr current_span_;
  Buffer::InstancePtr response_;
};

} // namespace Grpc
} // namespace Envoy
