#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "envoy/api/api.h"
#include "envoy/common/scope_tracker.h"
#include "envoy/common/time.h"
#include "envoy/event/deferred_deletable.h"
#include "envoy/event/dispatcher.h"
#include "envoy/network/connection_handler.h"
#include "envoy/stats/scope.h"

#include "common/common/logger.h"
#include "common/common/thread.h"
#include "common/event/libevent.h"
#include "common/event/libevent_scheduler.h"
#include "common/signal/fatal_error_handler.h"

namespace Envoy {
namespace Event {

/**
 * libevent implementation of Event::Dispatcher.
 */
class DispatcherImpl : Logger::Loggable<Logger::Id::main>,
                       public Dispatcher,
                       public FatalErrorHandlerInterface {
public:
  DispatcherImpl(Api::Api& api, Event::TimeSystem& time_system);
  DispatcherImpl(Buffer::WatermarkFactoryPtr&& factory, Api::Api& api,
                 Event::TimeSystem& time_system);
  ~DispatcherImpl() override;

  /**
   * @return event_base& the libevent base.
   */
  event_base& base() { return base_scheduler_.base(); }

  // Event::Dispatcher
  TimeSource& timeSource() override { return api_.timeSource(); }
  void initializeStats(Stats::Scope& scope, const std::string& prefix) override;
  void clearDeferredDeleteList() override;
  Network::ConnectionPtr
  createServerConnection(Network::ConnectionSocketPtr&& socket,
                         Network::TransportSocketPtr&& transport_socket) override;
  Network::ClientConnectionPtr
  createClientConnection(Network::Address::InstanceConstSharedPtr address,
                         Network::Address::InstanceConstSharedPtr source_address,
                         Network::TransportSocketPtr&& transport_socket,
                         const Network::ConnectionSocket::OptionsSharedPtr& options) override;
  Network::DnsResolverSharedPtr createDnsResolver(
      const std::vector<Network::Address::InstanceConstSharedPtr>& resolvers) override;
  FileEventPtr createFileEvent(int fd, FileReadyCb cb, FileTriggerType trigger,
                               uint32_t events) override;
  Filesystem::WatcherPtr createFilesystemWatcher() override;
  Network::ListenerPtr createListener(Network::Socket& socket, Network::ListenerCallbacks& cb,
                                      bool bind_to_port) override;
  Network::UdpListenerPtr createUdpListener(Network::Socket& socket,
                                            Network::UdpListenerCallbacks& cb) override;
  TimerPtr createTimer(TimerCb cb) override;
  void deferredDelete(DeferredDeletablePtr&& to_delete) override;
  void exit() override;
  SignalEventPtr listenForSignal(int signal_num, SignalCb cb) override;
  void post(std::function<void()> callback) override;
  void run(RunType type) override;
  Buffer::WatermarkFactory& getWatermarkFactory() override { return *buffer_factory_; }
  const ScopeTrackedObject* setTrackedObject(const ScopeTrackedObject* object) override {
    const ScopeTrackedObject* return_object = current_object_;
    current_object_ = object;
    return return_object;
  }
  MonotonicTime approximateMonotonicTime() const override;
  void updateApproximateMonotonicTime() override;

  // FatalErrorInterface
  void onFatalError() const override {
    // Dump the state of the tracked object if it is in the current thread. This generally results
    // in dumping the active state only for the thread which caused the fatal error.
    if (isThreadSafe()) {
      if (current_object_) {
        current_object_->dumpState(std::cerr);
      }
    }
  }

private:
  TimerPtr createTimerInternal(TimerCb cb);
  void runPostCallbacks();

  // Validate that an operation is thread safe, i.e. it's invoked on the same thread that the
  // dispatcher run loop is executing on. We allow run_tid_ to be empty for tests where we don't
  // invoke run().
  bool isThreadSafe() const override {
    return run_tid_.isEmpty() || run_tid_ == api_.threadFactory().currentThreadId();
  }

  Api::Api& api_;
  std::string stats_prefix_;
  std::unique_ptr<DispatcherStats> stats_;
  Thread::ThreadId run_tid_;
  Buffer::WatermarkFactoryPtr buffer_factory_;
  LibeventScheduler base_scheduler_;
  SchedulerPtr scheduler_;
  TimerPtr deferred_delete_timer_;
  TimerPtr post_timer_;
  std::vector<DeferredDeletablePtr> to_delete_1_;
  std::vector<DeferredDeletablePtr> to_delete_2_;
  std::vector<DeferredDeletablePtr>* current_to_delete_;
  Thread::MutexBasicLockable post_lock_;
  std::list<std::function<void()>> post_callbacks_ ABSL_GUARDED_BY(post_lock_);
  const ScopeTrackedObject* current_object_{};
  bool deferred_deleting_{};
  MonotonicTime approximate_monotonic_time_;
};

} // namespace Event
} // namespace Envoy
