#include "extensions/filters/http/jwt_authn/filter.h"

#include "common/http/utility.h"

#include "extensions/filters/http/well_known_names.h"

#include "jwt_verify_lib/status.h"

using ::google::jwt_verify::Status;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace JwtAuthn {

struct RcDetailsValues {
  // The jwt_authn filter rejected the request
  const std::string JwtAuthnAccessDenied = "jwt_authn_access_denied";
};
using RcDetails = ConstSingleton<RcDetailsValues>;

Filter::Filter(FilterConfigSharedPtr config)
    : stats_(config->stats()), config_(std::move(config)) {}

void Filter::onDestroy() {
  ENVOY_LOG(debug, "Called Filter : {}", __func__);
  if (context_) {
    context_->cancel();
  }
}

Http::FilterHeadersStatus Filter::decodeHeaders(Http::HeaderMap& headers, bool) {
  ENVOY_LOG(debug, "Called Filter : {}", __func__);

  state_ = Calling;
  stopped_ = false;
  // Verify the JWT token, onComplete() will be called when completed.
  const auto* verifier =
      config_->findVerifier(headers, decoder_callbacks_->streamInfo().filterState());
  if (!verifier) {
    onComplete(Status::Ok);
  } else {
    context_ = Verifier::createContext(headers, decoder_callbacks_->activeSpan(), this);
    verifier->verify(context_);
  }

  if (state_ == Complete) {
    return Http::FilterHeadersStatus::Continue;
  }
  ENVOY_LOG(debug, "Called Filter : {} Stop", __func__);
  stopped_ = true;
  return Http::FilterHeadersStatus::StopIteration;
}

void Filter::setPayload(const ProtobufWkt::Struct& payload) {
  decoder_callbacks_->streamInfo().setDynamicMetadata(HttpFilterNames::get().JwtAuthn, payload);
}

void Filter::onComplete(const Status& status) {
  ENVOY_LOG(debug, "Called Filter : check complete {}",
            ::google::jwt_verify::getStatusString(status));
  // This stream has been reset, abort the callback.
  if (state_ == Responded) {
    return;
  }
  if (Status::Ok != status) {
    stats_.denied_.inc();
    state_ = Responded;
    // verification failed
    Http::Code code =
        status == Status::JwtAudienceNotAllowed ? Http::Code::Forbidden : Http::Code::Unauthorized;
    // return failure reason as message body
    decoder_callbacks_->sendLocalReply(code, ::google::jwt_verify::getStatusString(status), nullptr,
                                       absl::nullopt, RcDetails::get().JwtAuthnAccessDenied);
    return;
  }
  stats_.allowed_.inc();
  state_ = Complete;
  if (stopped_) {
    decoder_callbacks_->continueDecoding();
  }
}

Http::FilterDataStatus Filter::decodeData(Buffer::Instance&, bool) {
  ENVOY_LOG(debug, "Called Filter : {}", __func__);
  if (state_ == Calling) {
    return Http::FilterDataStatus::StopIterationAndWatermark;
  }
  return Http::FilterDataStatus::Continue;
}

Http::FilterTrailersStatus Filter::decodeTrailers(Http::HeaderMap&) {
  ENVOY_LOG(debug, "Called Filter : {}", __func__);
  if (state_ == Calling) {
    return Http::FilterTrailersStatus::StopIteration;
  }
  return Http::FilterTrailersStatus::Continue;
}

void Filter::setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) {
  ENVOY_LOG(debug, "Called Filter : {}", __func__);
  decoder_callbacks_ = &callbacks;
}

} // namespace JwtAuthn
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
