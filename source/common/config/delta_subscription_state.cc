#include "common/config/delta_subscription_state.h"

#include "common/common/assert.h"
#include "common/common/hash.h"

namespace Envoy {
namespace Config {

DeltaSubscriptionState::DeltaSubscriptionState(std::string type_url,
                                               SubscriptionCallbacks& callbacks,
                                               const LocalInfo::LocalInfo& local_info,
                                               std::chrono::milliseconds init_fetch_timeout,
                                               Event::Dispatcher& dispatcher)
    : type_url_(std::move(type_url)), callbacks_(callbacks), local_info_(local_info),
      init_fetch_timeout_(init_fetch_timeout) {
  if (init_fetch_timeout_.count() > 0 && !init_fetch_timeout_timer_) {
    init_fetch_timeout_timer_ = dispatcher.createTimer([this]() -> void {
      ENVOY_LOG(warn, "delta config: initial fetch timed out for {}", type_url_);
      callbacks_.onConfigUpdateFailed(ConfigUpdateFailureReason::FetchTimedout, nullptr);
    });
    init_fetch_timeout_timer_->enableTimer(init_fetch_timeout_);
  }
}

void DeltaSubscriptionState::updateSubscriptionInterest(const std::set<std::string>& cur_added,
                                                        const std::set<std::string>& cur_removed) {
  for (const auto& a : cur_added) {
    setResourceWaitingForServer(a);
    // If interest in a resource is removed-then-added (all before a discovery request
    // can be sent), we must treat it as a "new" addition: our user may have forgotten its
    // copy of the resource after instructing us to remove it, and need to be reminded of it.
    names_removed_.erase(a);
    names_added_.insert(a);
  }
  for (const auto& r : cur_removed) {
    setLostInterestInResource(r);
    // Ideally, when interest in a resource is added-then-removed in between requests,
    // we would avoid putting a superfluous "unsubscribe [resource that was never subscribed]"
    // in the request. However, the removed-then-added case *does* need to go in the request,
    // and due to how we accomplish that, it's difficult to distinguish remove-add-remove from
    // add-remove (because "remove-add" has to be treated as equivalent to just "add").
    names_added_.erase(r);
    names_removed_.insert(r);
  }
}

// Not having sent any requests yet counts as an "update pending" since you're supposed to resend
// the entirety of your interest at the start of a stream, even if nothing has changed.
bool DeltaSubscriptionState::subscriptionUpdatePending() const {
  return !names_added_.empty() || !names_removed_.empty() ||
         !any_request_sent_yet_in_current_stream_;
}

UpdateAck
DeltaSubscriptionState::handleResponse(const envoy::api::v2::DeltaDiscoveryResponse& message) {
  // We *always* copy the response's nonce into the next request, even if we're going to make that
  // request a NACK by setting error_detail.
  UpdateAck ack(message.nonce(), type_url_);
  try {
    handleGoodResponse(message);
  } catch (const EnvoyException& e) {
    handleBadResponse(e, ack);
  }
  return ack;
}

void DeltaSubscriptionState::handleGoodResponse(
    const envoy::api::v2::DeltaDiscoveryResponse& message) {
  disableInitFetchTimeoutTimer();
  absl::flat_hash_set<std::string> names_added_removed;
  for (const auto& resource : message.resources()) {
    if (!names_added_removed.insert(resource.name()).second) {
      throw EnvoyException(
          fmt::format("duplicate name {} found among added/updated resources", resource.name()));
    }
    if (message.type_url() != resource.resource().type_url()) {
      throw EnvoyException(fmt::format("type URL {} embedded in an individual Any does not match "
                                       "the message-wide type URL {} in DeltaDiscoveryResponse {}",
                                       resource.resource().type_url(), message.type_url(),
                                       message.DebugString()));
    }
  }
  for (const auto& name : message.removed_resources()) {
    if (!names_added_removed.insert(name).second) {
      throw EnvoyException(
          fmt::format("duplicate name {} found in the union of added+removed resources", name));
    }
  }
  callbacks_.onConfigUpdate(message.resources(), message.removed_resources(),
                            message.system_version_info());
  for (const auto& resource : message.resources()) {
    setResourceVersion(resource.name(), resource.version());
  }
  // If a resource is gone, there is no longer a meaningful version for it that makes sense to
  // provide to the server upon stream reconnect: either it will continue to not exist, in which
  // case saying nothing is fine, or the server will bring back something new, which we should
  // receive regardless (which is the logic that not specifying a version will get you).
  //
  // So, leave the version map entry present but blank. It will be left out of
  // initial_resource_versions messages, but will remind us to explicitly tell the server "I'm
  // cancelling my subscription" when we lose interest.
  for (const auto& resource_name : message.removed_resources()) {
    if (resource_names_.find(resource_name) != resource_names_.end()) {
      setResourceWaitingForServer(resource_name);
    }
  }
  ENVOY_LOG(debug, "Delta config for {} accepted with {} resources added, {} removed", type_url_,
            message.resources().size(), message.removed_resources().size());
}

void DeltaSubscriptionState::handleBadResponse(const EnvoyException& e, UpdateAck& ack) {
  // Note that error_detail being set is what indicates that a DeltaDiscoveryRequest is a NACK.
  ack.error_detail_.set_code(Grpc::Status::GrpcStatus::Internal);
  ack.error_detail_.set_message(e.what());
  disableInitFetchTimeoutTimer();
  ENVOY_LOG(warn, "delta config for {} rejected: {}", type_url_, e.what());
  callbacks_.onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason::UpdateRejected, &e);
}

void DeltaSubscriptionState::handleEstablishmentFailure() {
  callbacks_.onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason::ConnectionFailure,
                                  nullptr);
}

envoy::api::v2::DeltaDiscoveryRequest DeltaSubscriptionState::getNextRequestAckless() {
  envoy::api::v2::DeltaDiscoveryRequest request;
  if (!any_request_sent_yet_in_current_stream_) {
    any_request_sent_yet_in_current_stream_ = true;
    // initial_resource_versions "must be populated for first request in a stream".
    // Also, since this might be a new server, we must explicitly state *all* of our subscription
    // interest.
    for (auto const& resource : resource_versions_) {
      // Populate initial_resource_versions with the resource versions we currently have.
      // Resources we are interested in, but are still waiting to get any version of from the
      // server, do not belong in initial_resource_versions. (But do belong in new subscriptions!)
      if (!resource.second.waitingForServer()) {
        (*request.mutable_initial_resource_versions())[resource.first] = resource.second.version();
      }
      // As mentioned above, fill resource_names_subscribe with everything, including names we
      // have yet to receive any resource for.
      names_added_.insert(resource.first);
    }
    names_removed_.clear();
  }
  std::copy(names_added_.begin(), names_added_.end(),
            Protobuf::RepeatedFieldBackInserter(request.mutable_resource_names_subscribe()));
  std::copy(names_removed_.begin(), names_removed_.end(),
            Protobuf::RepeatedFieldBackInserter(request.mutable_resource_names_unsubscribe()));
  names_added_.clear();
  names_removed_.clear();

  request.set_type_url(type_url_);
  request.mutable_node()->MergeFrom(local_info_.node());
  return request;
}

envoy::api::v2::DeltaDiscoveryRequest
DeltaSubscriptionState::getNextRequestWithAck(const UpdateAck& ack) {
  envoy::api::v2::DeltaDiscoveryRequest request = getNextRequestAckless();
  request.set_response_nonce(ack.nonce_);
  if (ack.error_detail_.code() != Grpc::Status::GrpcStatus::Ok) {
    // Don't needlessly make the field present-but-empty if status is ok.
    request.mutable_error_detail()->CopyFrom(ack.error_detail_);
  }
  return request;
}

void DeltaSubscriptionState::disableInitFetchTimeoutTimer() {
  if (init_fetch_timeout_timer_) {
    init_fetch_timeout_timer_->disableTimer();
    init_fetch_timeout_timer_.reset();
  }
}

void DeltaSubscriptionState::setResourceVersion(const std::string& resource_name,
                                                const std::string& resource_version) {
  resource_versions_[resource_name] = ResourceVersion(resource_version);
  resource_names_.insert(resource_name);
}

void DeltaSubscriptionState::setResourceWaitingForServer(const std::string& resource_name) {
  resource_versions_[resource_name] = ResourceVersion();
  resource_names_.insert(resource_name);
}

void DeltaSubscriptionState::setLostInterestInResource(const std::string& resource_name) {
  resource_versions_.erase(resource_name);
  resource_names_.erase(resource_name);
}

} // namespace Config
} // namespace Envoy
