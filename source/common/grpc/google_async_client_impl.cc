#include "common/grpc/google_async_client_impl.h"

#include "envoy/stats/scope.h"

#include "common/common/base64.h"
#include "common/common/empty_string.h"
#include "common/common/lock_guard.h"
#include "common/config/datasource.h"
#include "common/grpc/common.h"
#include "common/grpc/google_grpc_creds_impl.h"
#include "common/grpc/google_grpc_utils.h"
#include "common/tracing/http_tracer_impl.h"

#include "grpcpp/support/proto_buffer_reader.h"

namespace Envoy {
namespace Grpc {

GoogleAsyncClientThreadLocal::GoogleAsyncClientThreadLocal(Api::Api& api)
    : completion_thread_(api.threadFactory().createThread([this] { completionThread(); })) {}

GoogleAsyncClientThreadLocal::~GoogleAsyncClientThreadLocal() {
  // Force streams to shutdown and invoke TryCancel() to start the drain of
  // pending op. If we don't do this, Shutdown() below can jam on pending ops.
  // This is also required to satisfy the contract that once Shutdown is called,
  // streams no longer queue any additional tags.
  for (auto it = streams_.begin(); it != streams_.end();) {
    // resetStream() may result in immediate unregisterStream() and erase(),
    // which would invalidate the iterator for the current element, so make sure
    // we point to the next one first.
    (*it++)->resetStream();
  }
  cq_.Shutdown();
  ENVOY_LOG(debug, "Joining completionThread");
  completion_thread_->join();
  ENVOY_LOG(debug, "Joined completionThread");
  // Ensure that we have cleaned up all orphan streams, now that CQ is gone.
  while (!streams_.empty()) {
    (*streams_.begin())->onCompletedOps();
  }
}

void GoogleAsyncClientThreadLocal::completionThread() {
  ENVOY_LOG(debug, "completionThread running");
  void* tag;
  bool ok;
  while (cq_.Next(&tag, &ok)) {
    const auto& google_async_tag = *reinterpret_cast<GoogleAsyncTag*>(tag);
    const GoogleAsyncTag::Operation op = google_async_tag.op_;
    GoogleAsyncStreamImpl& stream = google_async_tag.stream_;
    ENVOY_LOG(trace, "completionThread CQ event {} {}", op, ok);
    Thread::LockGuard lock(stream.completed_ops_lock_);

    // It's an invariant that there must only be one pending post for arbitrary
    // length completed_ops_, otherwise we can race in stream destruction, where
    // we process multiple events in onCompletedOps() but have only partially
    // consumed the posts on the dispatcher.
    // TODO(htuch): This may result in unbounded processing on the silo thread
    // in onCompletedOps() in extreme cases, when we emplace_back() in
    // completionThread() at a high rate, consider bounding the length of such
    // sequences if this behavior becomes an issue.
    if (stream.completed_ops_.empty()) {
      stream.dispatcher_.post([&stream] { stream.onCompletedOps(); });
    }
    stream.completed_ops_.emplace_back(op, ok);
  }
  ENVOY_LOG(debug, "completionThread exiting");
}

GoogleAsyncClientImpl::GoogleAsyncClientImpl(Event::Dispatcher& dispatcher,
                                             GoogleAsyncClientThreadLocal& tls,
                                             GoogleStubFactory& stub_factory,
                                             Stats::ScopeSharedPtr scope,
                                             const envoy::api::v2::core::GrpcService& config,
                                             Api::Api& api)
    : dispatcher_(dispatcher), tls_(tls), stat_prefix_(config.google_grpc().stat_prefix()),
      initial_metadata_(config.initial_metadata()), scope_(scope) {
  // We rebuild the channel each time we construct the channel. It appears that the gRPC library is
  // smart enough to do connection pooling and reuse with identical channel args, so this should
  // have comparable overhead to what we are doing in Grpc::AsyncClientImpl, i.e. no expensive
  // new connection implied.
  std::shared_ptr<grpc::ChannelCredentials> creds = getGoogleGrpcChannelCredentials(config, api);
  std::shared_ptr<grpc::Channel> channel = CreateChannel(config.google_grpc().target_uri(), creds);
  stub_ = stub_factory.createStub(channel);
  // Initialize client stats.
  stats_.streams_total_ = &scope_->counter("streams_total");
  for (uint32_t i = 0; i <= Status::GrpcStatus::MaximumValid; ++i) {
    stats_.streams_closed_[i] = &scope_->counter(fmt::format("streams_closed_{}", i));
  }
}

GoogleAsyncClientImpl::~GoogleAsyncClientImpl() {
  ENVOY_LOG(debug, "Client teardown, resetting streams");
  while (!active_streams_.empty()) {
    active_streams_.front()->resetStream();
  }
}

AsyncRequest* GoogleAsyncClientImpl::sendRaw(absl::string_view service_full_name,
                                             absl::string_view method_name,
                                             Buffer::InstancePtr&& request,
                                             RawAsyncRequestCallbacks& callbacks,
                                             Tracing::Span& parent_span,
                                             const Http::AsyncClient::RequestOptions& options) {
  auto* const async_request = new GoogleAsyncRequestImpl(
      *this, service_full_name, method_name, std::move(request), callbacks, parent_span, options);
  std::unique_ptr<GoogleAsyncStreamImpl> grpc_stream{async_request};

  grpc_stream->initialize(true);
  if (grpc_stream->call_failed()) {
    return nullptr;
  }

  grpc_stream->moveIntoList(std::move(grpc_stream), active_streams_);
  return async_request;
}

RawAsyncStream* GoogleAsyncClientImpl::startRaw(absl::string_view service_full_name,
                                                absl::string_view method_name,
                                                RawAsyncStreamCallbacks& callbacks,
                                                const Http::AsyncClient::StreamOptions& options) {
  auto grpc_stream = std::make_unique<GoogleAsyncStreamImpl>(*this, service_full_name, method_name,
                                                             callbacks, options);

  grpc_stream->initialize(false);
  if (grpc_stream->call_failed()) {
    return nullptr;
  }

  grpc_stream->moveIntoList(std::move(grpc_stream), active_streams_);
  return active_streams_.front().get();
}

GoogleAsyncStreamImpl::GoogleAsyncStreamImpl(GoogleAsyncClientImpl& parent,
                                             absl::string_view service_full_name,
                                             absl::string_view method_name,
                                             RawAsyncStreamCallbacks& callbacks,
                                             const Http::AsyncClient::StreamOptions& options)
    : parent_(parent), tls_(parent_.tls_), dispatcher_(parent_.dispatcher_), stub_(parent_.stub_),
      service_full_name_(service_full_name), method_name_(method_name), callbacks_(callbacks),
      options_(options) {}

GoogleAsyncStreamImpl::~GoogleAsyncStreamImpl() {
  ENVOY_LOG(debug, "GoogleAsyncStreamImpl destruct");
}

GoogleAsyncStreamImpl::PendingMessage::PendingMessage(Buffer::InstancePtr request, bool end_stream)
    : buf_(GoogleGrpcUtils::makeByteBuffer(std::move(request))), end_stream_(end_stream) {}

// TODO(htuch): figure out how to propagate "this request should be buffered for
// retry" bit to Google gRPC library.
void GoogleAsyncStreamImpl::initialize(bool /*buffer_body_for_retry*/) {
  parent_.stats_.streams_total_->inc();
  gpr_timespec abs_deadline =
      options_.timeout
          ? gpr_time_add(gpr_now(GPR_CLOCK_REALTIME),
                         gpr_time_from_millis(options_.timeout.value().count(), GPR_TIMESPAN))
          : gpr_inf_future(GPR_CLOCK_REALTIME);
  ctxt_.set_deadline(abs_deadline);
  // Fill service-wide initial metadata.
  for (const auto& header_value : parent_.initial_metadata_) {
    ctxt_.AddMetadata(header_value.key(), header_value.value());
  }
  // Due to the different HTTP header implementations, we effectively double
  // copy headers here.
  Http::HeaderMapImpl initial_metadata;
  callbacks_.onCreateInitialMetadata(initial_metadata);
  initial_metadata.iterate(
      [](const Http::HeaderEntry& header, void* ctxt) {
        auto* client_context = static_cast<grpc::ClientContext*>(ctxt);
        client_context->AddMetadata(std::string(header.key().getStringView()),
                                    std::string(header.value().getStringView()));
        return Http::HeaderMap::Iterate::Continue;
      },
      &ctxt_);
  // Invoke stub call.
  rw_ = parent_.stub_->PrepareCall(&ctxt_, "/" + service_full_name_ + "/" + method_name_,
                                   &parent_.tls_.completionQueue());
  if (rw_ == nullptr) {
    notifyRemoteClose(Status::GrpcStatus::Unavailable, nullptr, EMPTY_STRING);
    call_failed_ = true;
    return;
  }
  parent_.tls_.registerStream(this);
  rw_->StartCall(&init_tag_);
  ++inflight_tags_;
}

void GoogleAsyncStreamImpl::notifyRemoteClose(Status::GrpcStatus grpc_status,
                                              Http::HeaderMapPtr trailing_metadata,
                                              const std::string& message) {
  if (grpc_status > Status::GrpcStatus::MaximumValid || grpc_status < 0) {
    ENVOY_LOG(error, "notifyRemoteClose invalid gRPC status code {}", grpc_status);
    // Set the grpc_status as InvalidCode but increment the Unknown stream to avoid out-of-range
    // crash..
    grpc_status = Status::GrpcStatus::InvalidCode;
    parent_.stats_.streams_closed_[Status::GrpcStatus::Unknown]->inc();
  } else {
    parent_.stats_.streams_closed_[grpc_status]->inc();
  }
  ENVOY_LOG(debug, "notifyRemoteClose {} {}", grpc_status, message);
  callbacks_.onReceiveTrailingMetadata(trailing_metadata ? std::move(trailing_metadata)
                                                         : std::make_unique<Http::HeaderMapImpl>());
  callbacks_.onRemoteClose(grpc_status, message);
}

void GoogleAsyncStreamImpl::sendMessageRaw(Buffer::InstancePtr&& request, bool end_stream) {
  write_pending_queue_.emplace(std::move(request), end_stream);
  ENVOY_LOG(trace, "Queued message to write ({} bytes)",
            write_pending_queue_.back().buf_.value().Length());
  writeQueued();
}

void GoogleAsyncStreamImpl::closeStream() {
  // Empty EOS write queued.
  write_pending_queue_.emplace();
  writeQueued();
}

void GoogleAsyncStreamImpl::resetStream() {
  ENVOY_LOG(debug, "resetStream");
  cleanup();
}

void GoogleAsyncStreamImpl::writeQueued() {
  if (!call_initialized_ || finish_pending_ || write_pending_ || write_pending_queue_.empty() ||
      draining_cq_) {
    return;
  }
  write_pending_ = true;
  const PendingMessage& msg = write_pending_queue_.front();

  if (!msg.buf_) {
    ASSERT(msg.end_stream_);
    rw_->WritesDone(&write_last_tag_);
    ++inflight_tags_;
  } else if (msg.end_stream_) {
    grpc::WriteOptions write_options;
    rw_->WriteLast(msg.buf_.value(), write_options, &write_last_tag_);
    ++inflight_tags_;
  } else {
    rw_->Write(msg.buf_.value(), &write_tag_);
    ++inflight_tags_;
  }
  ENVOY_LOG(trace, "Write op dispatched");
}

void GoogleAsyncStreamImpl::onCompletedOps() {
  Thread::LockGuard lock(completed_ops_lock_);
  while (!completed_ops_.empty()) {
    GoogleAsyncTag::Operation op;
    bool ok;
    std::tie(op, ok) = completed_ops_.front();
    completed_ops_.pop_front();
    handleOpCompletion(op, ok);
  }
}

void GoogleAsyncStreamImpl::handleOpCompletion(GoogleAsyncTag::Operation op, bool ok) {
  ENVOY_LOG(trace, "handleOpCompletion op={} ok={} inflight={}", op, ok, inflight_tags_);
  ASSERT(inflight_tags_ > 0);
  --inflight_tags_;
  if (draining_cq_) {
    if (inflight_tags_ == 0) {
      deferredDelete();
    }
    // Ignore op completions while draining CQ.
    return;
  }
  // Consider failure cases first.
  if (!ok) {
    // Early fails can be just treated as Internal.
    if (op == GoogleAsyncTag::Operation::Init ||
        op == GoogleAsyncTag::Operation::ReadInitialMetadata) {
      notifyRemoteClose(Status::GrpcStatus::Internal, nullptr, EMPTY_STRING);
      resetStream();
      return;
    }
    // Remote server has closed, we can pick up some meaningful status.
    // TODO(htuch): We're assuming here that a failed Write/WriteLast operation will result in
    // stream termination, and pick up on the failed Read here. Confirm that this assumption is
    // valid.
    if (op == GoogleAsyncTag::Operation::Read) {
      finish_pending_ = true;
      rw_->Finish(&status_, &finish_tag_);
      ++inflight_tags_;
    }
    return;
  }
  switch (op) {
  case GoogleAsyncTag::Operation::Init: {
    ASSERT(ok);
    ASSERT(!call_initialized_);
    call_initialized_ = true;
    rw_->ReadInitialMetadata(&read_initial_metadata_tag_);
    ++inflight_tags_;
    writeQueued();
    break;
  }
  case GoogleAsyncTag::Operation::ReadInitialMetadata: {
    ASSERT(ok);
    ASSERT(call_initialized_);
    rw_->Read(&read_buf_, &read_tag_);
    ++inflight_tags_;
    Http::HeaderMapPtr initial_metadata = std::make_unique<Http::HeaderMapImpl>();
    metadataTranslate(ctxt_.GetServerInitialMetadata(), *initial_metadata);
    callbacks_.onReceiveInitialMetadata(std::move(initial_metadata));
    break;
  }
  case GoogleAsyncTag::Operation::Write: {
    ASSERT(ok);
    write_pending_ = false;
    write_pending_queue_.pop();
    writeQueued();
    break;
  }
  case GoogleAsyncTag::Operation::WriteLast: {
    ASSERT(ok);
    write_pending_ = false;
    break;
  }
  case GoogleAsyncTag::Operation::Read: {
    ASSERT(ok);
    auto buffer = GoogleGrpcUtils::makeBufferInstance(read_buf_);
    if (!buffer || !callbacks_.onReceiveMessageRaw(std::move(buffer))) {
      // This is basically streamError in Grpc::AsyncClientImpl.
      notifyRemoteClose(Status::GrpcStatus::Internal, nullptr, EMPTY_STRING);
      resetStream();
      break;
    }
    rw_->Read(&read_buf_, &read_tag_);
    ++inflight_tags_;
    break;
  }
  case GoogleAsyncTag::Operation::Finish: {
    ASSERT(finish_pending_);
    ENVOY_LOG(debug, "Finish with grpc-status code {}", status_.error_code());
    Http::HeaderMapPtr trailing_metadata = std::make_unique<Http::HeaderMapImpl>();
    metadataTranslate(ctxt_.GetServerTrailingMetadata(), *trailing_metadata);
    notifyRemoteClose(static_cast<Status::GrpcStatus>(status_.error_code()),
                      std::move(trailing_metadata), status_.error_message());
    cleanup();
    break;
  }
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

void GoogleAsyncStreamImpl::metadataTranslate(
    const std::multimap<grpc::string_ref, grpc::string_ref>& grpc_metadata,
    Http::HeaderMap& header_map) {
  // More painful copying, this time due to the mismatch in header
  // representation data structures in Envoy and Google gRPC.
  for (const auto& it : grpc_metadata) {
    auto key = Http::LowerCaseString(std::string(it.first.data(), it.first.size()));
    if (absl::EndsWith(key.get(), "-bin")) {
      auto value = Base64::encode(it.second.data(), it.second.size());
      header_map.addCopy(key, value);
      continue;
    }
    header_map.addCopy(key, std::string(it.second.data(), it.second.size()));
  }
}

void GoogleAsyncStreamImpl::deferredDelete() {
  ENVOY_LOG(debug, "Deferred delete");
  tls_.unregisterStream(this);
  // We only get here following cleanup(), which has performed a
  // remoteFromList(), resulting in self-ownership of the object's memory.
  // Hence, it is safe here to create a unique_ptr to this and transfer
  // ownership to dispatcher_.deferredDelete(). After this call, no further
  // methods may be invoked on this object.
  dispatcher_.deferredDelete(std::unique_ptr<GoogleAsyncStreamImpl>(this));
}

void GoogleAsyncStreamImpl::cleanup() {
  ENVOY_LOG(debug, "Stream cleanup with {} in-flight tags", inflight_tags_);
  // We can get here if the client has already issued resetStream() and, while
  // this is in progress, the destructor runs.
  if (draining_cq_) {
    ENVOY_LOG(debug, "Cleanup already in progress");
    return;
  }
  draining_cq_ = true;
  ctxt_.TryCancel();
  if (LinkedObject<GoogleAsyncStreamImpl>::inserted()) {
    // We take ownership of our own memory at this point.
    LinkedObject<GoogleAsyncStreamImpl>::removeFromList(parent_.active_streams_).release();
    if (inflight_tags_ == 0) {
      deferredDelete();
    }
  }
}

GoogleAsyncRequestImpl::GoogleAsyncRequestImpl(
    GoogleAsyncClientImpl& parent, absl::string_view service_full_name,
    absl::string_view method_name, Buffer::InstancePtr request, RawAsyncRequestCallbacks& callbacks,
    Tracing::Span& parent_span, const Http::AsyncClient::RequestOptions& options)
    : GoogleAsyncStreamImpl(parent, service_full_name, method_name, *this, options),
      request_(std::move(request)), callbacks_(callbacks) {
  current_span_ = parent_span.spawnChild(Tracing::EgressConfig::get(),
                                         "async " + parent.stat_prefix_ + " egress",
                                         parent.timeSource().systemTime());
  current_span_->setTag(Tracing::Tags::get().UpstreamCluster, parent.stat_prefix_);
  current_span_->setTag(Tracing::Tags::get().Component, Tracing::Tags::get().Proxy);
}

void GoogleAsyncRequestImpl::initialize(bool buffer_body_for_retry) {
  GoogleAsyncStreamImpl::initialize(buffer_body_for_retry);
  if (this->call_failed()) {
    return;
  }
  this->sendMessageRaw(std::move(request_), true);
}

void GoogleAsyncRequestImpl::cancel() {
  current_span_->setTag(Tracing::Tags::get().Status, Tracing::Tags::get().Canceled);
  current_span_->finishSpan();
  this->resetStream();
}

void GoogleAsyncRequestImpl::onCreateInitialMetadata(Http::HeaderMap& metadata) {
  current_span_->injectContext(metadata);
  callbacks_.onCreateInitialMetadata(metadata);
}

void GoogleAsyncRequestImpl::onReceiveInitialMetadata(Http::HeaderMapPtr&&) {}

bool GoogleAsyncRequestImpl::onReceiveMessageRaw(Buffer::InstancePtr&& response) {
  response_ = std::move(response);
  return true;
}

void GoogleAsyncRequestImpl::onReceiveTrailingMetadata(Http::HeaderMapPtr&&) {}

void GoogleAsyncRequestImpl::onRemoteClose(Grpc::Status::GrpcStatus status,
                                           const std::string& message) {
  current_span_->setTag(Tracing::Tags::get().GrpcStatusCode, std::to_string(status));

  if (status != Grpc::Status::GrpcStatus::Ok) {
    current_span_->setTag(Tracing::Tags::get().Error, Tracing::Tags::get().True);
    callbacks_.onFailure(status, message, *current_span_);
  } else if (response_ == nullptr) {
    current_span_->setTag(Tracing::Tags::get().Error, Tracing::Tags::get().True);
    callbacks_.onFailure(Status::Internal, EMPTY_STRING, *current_span_);
  } else {
    callbacks_.onSuccessRaw(std::move(response_), *current_span_);
  }

  current_span_->finishSpan();
}

} // namespace Grpc
} // namespace Envoy
