#include "server/overload_manager_impl.h"

#include "envoy/common/exception.h"
#include "envoy/config/overload/v3/overload.pb.h"
#include "envoy/stats/scope.h"

#include "common/common/fmt.h"
#include "common/config/utility.h"
#include "common/protobuf/utility.h"
#include "common/stats/symbol_table_impl.h"

#include "server/resource_monitor_config_impl.h"

#include "absl/container/node_hash_map.h"
#include "absl/strings/str_cat.h"

namespace Envoy {
namespace Server {

namespace {

class ThresholdTriggerImpl final : public OverloadAction::Trigger {
public:
  ThresholdTriggerImpl(const envoy::config::overload::v3::ThresholdTrigger& config)
      : threshold_(config.value()), state_(OverloadActionState::inactive()) {}

  bool updateValue(double value) override {
    const OverloadActionState state = actionState();
    state_ =
        value >= threshold_ ? OverloadActionState::saturated() : OverloadActionState::inactive();
    return state.value() != actionState().value();
  }

  OverloadActionState actionState() const override { return state_; }

private:
  const double threshold_;
  OverloadActionState state_;
};

class ScaledTriggerImpl final : public OverloadAction::Trigger {
public:
  ScaledTriggerImpl(const envoy::config::overload::v3::ScaledTrigger& config)
      : scaling_threshold_(config.scaling_threshold()),
        saturated_threshold_(config.saturation_threshold()),
        state_(OverloadActionState::inactive()) {
    if (scaling_threshold_ >= saturated_threshold_) {
      throw EnvoyException("scaling_threshold must be less than saturation_threshold");
    }
  }

  bool updateValue(double value) override {
    const OverloadActionState old_state = actionState();
    if (value <= scaling_threshold_) {
      state_ = OverloadActionState::inactive();
    } else if (value >= saturated_threshold_) {
      state_ = OverloadActionState::saturated();
    } else {
      state_ = OverloadActionState((value - scaling_threshold_) /
                                   (saturated_threshold_ - scaling_threshold_));
    }
    return state_.value() != old_state.value();
  }

  OverloadActionState actionState() const override { return state_; }

private:
  const double scaling_threshold_;
  const double saturated_threshold_;
  OverloadActionState state_;
};

/**
 * Thread-local copy of the state of each configured overload action.
 */
class ThreadLocalOverloadStateImpl : public ThreadLocalOverloadState {
public:
  const OverloadActionState& getState(const std::string& action) override {
    auto it = actions_.find(action);
    if (it == actions_.end()) {
      it = actions_.insert(std::make_pair(action, OverloadActionState::inactive())).first;
    }
    return it->second;
  }

  void setState(const std::string& action, OverloadActionState state) {
    actions_.insert_or_assign(action, state);
  }

private:
  absl::node_hash_map<std::string, OverloadActionState> actions_;
};

Stats::Counter& makeCounter(Stats::Scope& scope, absl::string_view a, absl::string_view b) {
  Stats::StatNameManagedStorage stat_name(absl::StrCat("overload.", a, ".", b),
                                          scope.symbolTable());
  return scope.counterFromStatName(stat_name.statName());
}

Stats::Gauge& makeGauge(Stats::Scope& scope, absl::string_view a, absl::string_view b,
                        Stats::Gauge::ImportMode import_mode) {
  Stats::StatNameManagedStorage stat_name(absl::StrCat("overload.", a, ".", b),
                                          scope.symbolTable());
  return scope.gaugeFromStatName(stat_name.statName(), import_mode);
}

} // namespace

OverloadAction::OverloadAction(const envoy::config::overload::v3::OverloadAction& config,
                               Stats::Scope& stats_scope)
    : state_(OverloadActionState::inactive()),
      active_gauge_(
          makeGauge(stats_scope, config.name(), "active", Stats::Gauge::ImportMode::Accumulate)),
      scale_percent_gauge_(makeGauge(stats_scope, config.name(), "scale_percent",
                                     Stats::Gauge::ImportMode::Accumulate)) {
  for (const auto& trigger_config : config.triggers()) {
    TriggerPtr trigger;

    switch (trigger_config.trigger_oneof_case()) {
    case envoy::config::overload::v3::Trigger::TriggerOneofCase::kThreshold:
      trigger = std::make_unique<ThresholdTriggerImpl>(trigger_config.threshold());
      break;
    case envoy::config::overload::v3::Trigger::TriggerOneofCase::kScaled:
      trigger = std::make_unique<ScaledTriggerImpl>(trigger_config.scaled());
      break;
    default:
      NOT_REACHED_GCOVR_EXCL_LINE;
    }

    if (!triggers_.try_emplace(trigger_config.name(), std::move(trigger)).second) {
      throw EnvoyException(
          absl::StrCat("Duplicate trigger resource for overload action ", config.name()));
    }
  }

  active_gauge_.set(0);
  scale_percent_gauge_.set(0);
}

bool OverloadAction::updateResourcePressure(const std::string& name, double pressure) {
  const OverloadActionState old_state = getState();

  auto it = triggers_.find(name);
  ASSERT(it != triggers_.end());
  if (!it->second->updateValue(pressure)) {
    return false;
  }
  const auto trigger_new_state = it->second->actionState();
  active_gauge_.set(trigger_new_state.isSaturated() ? 1 : 0);
  scale_percent_gauge_.set(trigger_new_state.value() * 100);

  {
    // Compute the new state as the maximum over all trigger states.
    OverloadActionState new_state = OverloadActionState::inactive();
    for (auto& trigger : triggers_) {
      const auto trigger_state = trigger.second->actionState();
      if (trigger_state.value() > new_state.value()) {
        new_state = trigger_state;
      }
    }
    state_ = new_state;
  }

  return state_.value() != old_state.value();
}

OverloadActionState OverloadAction::getState() const { return state_; }

OverloadManagerImpl::OverloadManagerImpl(Event::Dispatcher& dispatcher, Stats::Scope& stats_scope,
                                         ThreadLocal::SlotAllocator& slot_allocator,
                                         const envoy::config::overload::v3::OverloadManager& config,
                                         ProtobufMessage::ValidationVisitor& validation_visitor,
                                         Api::Api& api)
    : started_(false), dispatcher_(dispatcher), tls_(slot_allocator.allocateSlot()),
      refresh_interval_(
          std::chrono::milliseconds(PROTOBUF_GET_MS_OR_DEFAULT(config, refresh_interval, 1000))) {
  Configuration::ResourceMonitorFactoryContextImpl context(dispatcher, api, validation_visitor);
  for (const auto& resource : config.resource_monitors()) {
    const auto& name = resource.name();
    ENVOY_LOG(debug, "Adding resource monitor for {}", name);
    auto& factory =
        Config::Utility::getAndCheckFactory<Configuration::ResourceMonitorFactory>(resource);
    auto config = Config::Utility::translateToFactoryConfig(resource, validation_visitor, factory);
    auto monitor = factory.createResourceMonitor(*config, context);

    auto result = resources_.try_emplace(name, name, std::move(monitor), *this, stats_scope);
    if (!result.second) {
      throw EnvoyException(absl::StrCat("Duplicate resource monitor ", name));
    }
  }

  for (const auto& action : config.actions()) {
    const auto& name = action.name();
    ENVOY_LOG(debug, "Adding overload action {}", name);
    // TODO: use in place construction once https://github.com/abseil/abseil-cpp/issues/388 is
    // addressed
    // We cannot currently use in place construction as the OverloadAction constructor may throw,
    // causing an inconsistent internal state of the actions_ map, which on destruction results in
    // an invalid free.
    auto result = actions_.try_emplace(name, OverloadAction(action, stats_scope));
    if (!result.second) {
      throw EnvoyException(absl::StrCat("Duplicate overload action ", name));
    }

    for (const auto& trigger : action.triggers()) {
      const std::string& resource = trigger.name();

      if (resources_.find(resource) == resources_.end()) {
        throw EnvoyException(
            fmt::format("Unknown trigger resource {} for overload action {}", resource, name));
      }

      resource_to_actions_.insert(std::make_pair(resource, name));
    }
  }
}

void OverloadManagerImpl::start() {
  ASSERT(!started_);
  started_ = true;

  tls_->set([](Event::Dispatcher&) -> ThreadLocal::ThreadLocalObjectSharedPtr {
    return std::make_shared<ThreadLocalOverloadStateImpl>();
  });

  if (resources_.empty()) {
    return;
  }

  timer_ = dispatcher_.createTimer([this]() -> void {
    // Guarantee that all resource updates get flushed after no more than one refresh_interval_.
    flushResourceUpdates();

    // Start a new flush epoch. If all resource updates complete before this callback runs, the last
    // resource update will call flushResourceUpdates to flush the whole batch early.
    ++flush_epoch_;
    flush_awaiting_updates_ = resources_.size();

    for (auto& resource : resources_) {
      resource.second.update(flush_epoch_);
    }

    timer_->enableTimer(refresh_interval_);
  });
  timer_->enableTimer(refresh_interval_);
}

void OverloadManagerImpl::stop() {
  // Disable any pending timeouts.
  if (timer_) {
    timer_->disableTimer();
  }

  // Clear the resource map to block on any pending updates.
  resources_.clear();
}

bool OverloadManagerImpl::registerForAction(const std::string& action,
                                            Event::Dispatcher& dispatcher,
                                            OverloadActionCb callback) {
  ASSERT(!started_);

  if (actions_.find(action) == actions_.end()) {
    ENVOY_LOG(debug, "No overload action is configured for {}.", action);
    return false;
  }

  action_to_callbacks_.emplace(std::piecewise_construct, std::forward_as_tuple(action),
                               std::forward_as_tuple(dispatcher, callback));
  return true;
}

ThreadLocalOverloadState& OverloadManagerImpl::getThreadLocalOverloadState() {
  return tls_->getTyped<ThreadLocalOverloadStateImpl>();
}

void OverloadManagerImpl::updateResourcePressure(const std::string& resource, double pressure,
                                                 FlushEpochId flush_epoch) {
  auto [start, end] = resource_to_actions_.equal_range(resource);

  std::for_each(start, end, [&](ResourceToActionMap::value_type& entry) {
    const std::string& action = entry.second;
    auto action_it = actions_.find(action);
    ASSERT(action_it != actions_.end());
    const OverloadActionState old_state = action_it->second.getState();
    if (action_it->second.updateResourcePressure(resource, pressure)) {
      const auto state = action_it->second.getState();

      if (old_state.isSaturated() != state.isSaturated()) {
        ENVOY_LOG(debug, "Overload action {} became {}", action,
                  (state.isSaturated() ? "saturated" : "scaling"));
      }

      // Record the updated value to be sent to workers on the next thread-local-state flush, along
      // with any update callbacks. This might overwrite a previous action state change caused by a
      // pressure update for a different resource that hasn't been flushed yet. That's okay because
      // the state recorded here includes the information from all previous resource updates. So
      // even if resource 1 causes an action to have value A, and a later update to resource 2
      // causes the action to have value B, B would have been the result for whichever order the
      // updates to resources 1 and 2 came in.
      state_updates_to_flush_.insert_or_assign(action, state);
      auto [callbacks_start, callbacks_end] = action_to_callbacks_.equal_range(action);
      std::for_each(callbacks_start, callbacks_end, [&](ActionToCallbackMap::value_type& cb_entry) {
        callbacks_to_flush_.insert_or_assign(&cb_entry.second, state);
      });
    }
  });

  // Eagerly flush updates if this is the last call to updateResourcePressure expected for the
  // current epoch. This assert is always valid because flush_awaiting_updates_ is initialized
  // before each batch of updates, and even if a resource monitor performs a double update, or a
  // previous update callback is late, the logic in OverloadManager::Resource::update() will prevent
  // unexpected calls to this function.
  ASSERT(flush_awaiting_updates_ > 0);
  --flush_awaiting_updates_;
  if (flush_epoch == flush_epoch_ && flush_awaiting_updates_ == 0) {
    flushResourceUpdates();
  }
}

void OverloadManagerImpl::flushResourceUpdates() {
  if (!state_updates_to_flush_.empty()) {
    auto shared_updates = std::make_shared<absl::flat_hash_map<std::string, OverloadActionState>>();
    std::swap(*shared_updates, state_updates_to_flush_);

    tls_->runOnAllThreads([this, updates = std::move(shared_updates)] {
      for (const auto& [action, state] : *updates) {
        tls_->getTyped<ThreadLocalOverloadStateImpl>().setState(action, state);
      }
    });
  }

  for (const auto& [cb, state] : callbacks_to_flush_) {
    cb->dispatcher_.post([cb = cb, state = state]() { cb->callback_(state); });
  }
  callbacks_to_flush_.clear();
}

OverloadManagerImpl::Resource::Resource(const std::string& name, ResourceMonitorPtr monitor,
                                        OverloadManagerImpl& manager, Stats::Scope& stats_scope)
    : name_(name), monitor_(std::move(monitor)), manager_(manager), pending_update_(false),
      pressure_gauge_(
          makeGauge(stats_scope, name, "pressure", Stats::Gauge::ImportMode::NeverImport)),
      failed_updates_counter_(makeCounter(stats_scope, name, "failed_updates")),
      skipped_updates_counter_(makeCounter(stats_scope, name, "skipped_updates")) {}

void OverloadManagerImpl::Resource::update(FlushEpochId flush_epoch) {
  if (!pending_update_) {
    pending_update_ = true;
    flush_epoch_ = flush_epoch;
    monitor_->updateResourceUsage(*this);
    return;
  }
  ENVOY_LOG(debug, "Skipping update for resource {} which has pending update", name_);
  skipped_updates_counter_.inc();
}

void OverloadManagerImpl::Resource::onSuccess(const ResourceUsage& usage) {
  pending_update_ = false;
  manager_.updateResourcePressure(name_, usage.resource_pressure_, flush_epoch_);
  pressure_gauge_.set(usage.resource_pressure_ * 100); // convert to percent
}

void OverloadManagerImpl::Resource::onFailure(const EnvoyException& error) {
  pending_update_ = false;
  ENVOY_LOG(info, "Failed to update resource {}: {}", name_, error.what());
  failed_updates_counter_.inc();
}

} // namespace Server
} // namespace Envoy
