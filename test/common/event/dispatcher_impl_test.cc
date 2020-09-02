#include <functional>

#include "envoy/thread/thread.h"

#include "common/api/api_impl.h"
#include "common/common/lock_guard.h"
#include "common/event/deferred_task.h"
#include "common/event/dispatcher_impl.h"
#include "common/event/timer_impl.h"
#include "common/stats/isolated_store_impl.h"

#include "test/mocks/common.h"
#include "test/mocks/stats/mocks.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/test_runtime.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::InSequence;
using testing::NiceMock;

namespace Envoy {
namespace Event {
namespace {

void onWatcherReady(evwatch*, const evwatch_prepare_cb_info*, void* arg) {
  // `arg` contains the ReadyWatcher passed in from evwatch_prepare_new.
  auto watcher = static_cast<ReadyWatcher*>(arg);
  watcher->ready();
}

class SchedulableCallbackImplTest : public testing::Test {
protected:
  SchedulableCallbackImplTest()
      : api_(Api::createApiForTest()), dispatcher_(api_->allocateDispatcher("test_thread")) {}

  void createCallback(std::function<void()> cb) {
    callbacks_.emplace_back(dispatcher_->createSchedulableCallback(cb));
  }

  Api::ApiPtr api_;
  DispatcherPtr dispatcher_;
  std::vector<SchedulableCallbackPtr> callbacks_;
};

TEST_F(SchedulableCallbackImplTest, ScheduleCurrentAndCancel) {
  ReadyWatcher watcher;

  auto cb = dispatcher_->createSchedulableCallback([&]() { watcher.ready(); });

  // Cancel is a no-op if not scheduled.
  cb->cancel();
  dispatcher_->run(Dispatcher::RunType::Block);

  // Callback is not invoked if cancelled before it executes.
  cb->scheduleCallbackCurrentIteration();
  EXPECT_TRUE(cb->enabled());
  cb->cancel();
  EXPECT_FALSE(cb->enabled());
  dispatcher_->run(Dispatcher::RunType::Block);

  // Scheduled callback executes.
  cb->scheduleCallbackCurrentIteration();
  EXPECT_CALL(watcher, ready());
  dispatcher_->run(Dispatcher::RunType::Block);

  // Callbacks implicitly cancelled if runner is deleted.
  cb->scheduleCallbackCurrentIteration();
  cb.reset();
  dispatcher_->run(Dispatcher::RunType::Block);
}

TEST_F(SchedulableCallbackImplTest, ScheduleNextAndCancel) {
  ReadyWatcher watcher;

  auto cb = dispatcher_->createSchedulableCallback([&]() { watcher.ready(); });

  // Cancel is a no-op if not scheduled.
  cb->cancel();
  dispatcher_->run(Dispatcher::RunType::Block);

  // Callback is not invoked if cancelled before it executes.
  cb->scheduleCallbackNextIteration();
  EXPECT_TRUE(cb->enabled());
  cb->cancel();
  EXPECT_FALSE(cb->enabled());
  dispatcher_->run(Dispatcher::RunType::Block);

  // Scheduled callback executes.
  cb->scheduleCallbackNextIteration();
  EXPECT_CALL(watcher, ready());
  dispatcher_->run(Dispatcher::RunType::Block);

  // Callbacks implicitly cancelled if runner is deleted.
  cb->scheduleCallbackNextIteration();
  cb.reset();
  dispatcher_->run(Dispatcher::RunType::Block);
}

TEST_F(SchedulableCallbackImplTest, ScheduleOrder) {
  ReadyWatcher watcher0;
  createCallback([&]() { watcher0.ready(); });
  ReadyWatcher watcher1;
  createCallback([&]() { watcher1.ready(); });
  ReadyWatcher watcher2;
  createCallback([&]() { watcher2.ready(); });

  // Current iteration callbacks run in the order they are scheduled. Next iteration callbacks run
  // after current iteration callbacks.
  callbacks_[0]->scheduleCallbackNextIteration();
  callbacks_[1]->scheduleCallbackCurrentIteration();
  callbacks_[2]->scheduleCallbackCurrentIteration();
  InSequence s;
  EXPECT_CALL(watcher1, ready());
  EXPECT_CALL(watcher2, ready());
  EXPECT_CALL(watcher0, ready());
  dispatcher_->run(Dispatcher::RunType::Block);
}

TEST_F(SchedulableCallbackImplTest, ScheduleChainingAndCancellation) {
  DispatcherImpl* dispatcher_impl = static_cast<DispatcherImpl*>(dispatcher_.get());
  ReadyWatcher prepare_watcher;
  evwatch_prepare_new(&dispatcher_impl->base(), onWatcherReady, &prepare_watcher);

  ReadyWatcher watcher0;
  createCallback([&]() {
    watcher0.ready();
    callbacks_[1]->scheduleCallbackCurrentIteration();
  });

  ReadyWatcher watcher1;
  createCallback([&]() {
    watcher1.ready();
    callbacks_[2]->scheduleCallbackCurrentIteration();
    callbacks_[3]->scheduleCallbackCurrentIteration();
    callbacks_[4]->scheduleCallbackCurrentIteration();
    callbacks_[5]->scheduleCallbackNextIteration();
  });

  ReadyWatcher watcher2;
  createCallback([&]() {
    watcher2.ready();
    EXPECT_TRUE(callbacks_[3]->enabled());
    callbacks_[3]->cancel();
    EXPECT_TRUE(callbacks_[4]->enabled());
    callbacks_[4].reset();
  });

  ReadyWatcher watcher3;
  createCallback([&]() { watcher3.ready(); });

  ReadyWatcher watcher4;
  createCallback([&]() { watcher4.ready(); });

  ReadyWatcher watcher5;
  createCallback([&]() { watcher5.ready(); });

  // Chained callbacks run in the same event loop iteration, as signaled by a single call to
  // prepare_watcher.ready(). watcher3 and watcher4 are not invoked because cb2 cancels
  // cb3 and deletes cb4 as part of its execution. cb5 runs after a second call to the
  // prepare callback since it's scheduled for the next iteration.
  callbacks_[0]->scheduleCallbackCurrentIteration();
  InSequence s;
  EXPECT_CALL(prepare_watcher, ready());
  EXPECT_CALL(watcher0, ready());
  EXPECT_CALL(watcher1, ready());
  EXPECT_CALL(watcher2, ready());
  EXPECT_CALL(prepare_watcher, ready());
  EXPECT_CALL(watcher5, ready());
  dispatcher_->run(Dispatcher::RunType::Block);
}

class TestDeferredDeletable : public DeferredDeletable {
public:
  TestDeferredDeletable(std::function<void()> on_destroy) : on_destroy_(on_destroy) {}
  ~TestDeferredDeletable() override { on_destroy_(); }

private:
  std::function<void()> on_destroy_;
};

TEST(DeferredDeleteTest, DeferredDelete) {
  InSequence s;
  Api::ApiPtr api = Api::createApiForTest();
  DispatcherPtr dispatcher(api->allocateDispatcher("test_thread"));
  ReadyWatcher watcher1;

  dispatcher->deferredDelete(
      DeferredDeletablePtr{new TestDeferredDeletable([&]() -> void { watcher1.ready(); })});

  // The first one will get deleted inline.
  EXPECT_CALL(watcher1, ready());
  dispatcher->clearDeferredDeleteList();

  // This one does a nested deferred delete. We should need two clear calls to actually get
  // rid of it with the vector swapping. We also test that inline clear() call does nothing.
  ReadyWatcher watcher2;
  ReadyWatcher watcher3;
  dispatcher->deferredDelete(DeferredDeletablePtr{new TestDeferredDeletable([&]() -> void {
    watcher2.ready();
    dispatcher->deferredDelete(
        DeferredDeletablePtr{new TestDeferredDeletable([&]() -> void { watcher3.ready(); })});
    dispatcher->clearDeferredDeleteList();
  })});

  EXPECT_CALL(watcher2, ready());
  dispatcher->clearDeferredDeleteList();

  EXPECT_CALL(watcher3, ready());
  dispatcher->clearDeferredDeleteList();
}

TEST(DeferredTaskTest, DeferredTask) {
  InSequence s;
  Api::ApiPtr api = Api::createApiForTest();
  DispatcherPtr dispatcher(api->allocateDispatcher("test_thread"));
  ReadyWatcher watcher1;

  DeferredTaskUtil::deferredRun(*dispatcher, [&watcher1]() -> void { watcher1.ready(); });
  // The first one will get deleted inline.
  EXPECT_CALL(watcher1, ready());
  dispatcher->clearDeferredDeleteList();

  // Deferred task is scheduled FIFO.
  ReadyWatcher watcher2;
  ReadyWatcher watcher3;
  DeferredTaskUtil::deferredRun(*dispatcher, [&watcher2]() -> void { watcher2.ready(); });
  DeferredTaskUtil::deferredRun(*dispatcher, [&watcher3]() -> void { watcher3.ready(); });
  EXPECT_CALL(watcher2, ready());
  EXPECT_CALL(watcher3, ready());
  dispatcher->clearDeferredDeleteList();
}

class DispatcherImplTest : public testing::Test {
protected:
  DispatcherImplTest()
      : api_(Api::createApiForTest()), dispatcher_(api_->allocateDispatcher("test_thread")) {
    dispatcher_thread_ = api_->threadFactory().createThread([this]() {
      // Must create a keepalive timer to keep the dispatcher from exiting.
      std::chrono::milliseconds time_interval(500);
      keepalive_timer_ = dispatcher_->createTimer(
          [this, time_interval]() { keepalive_timer_->enableTimer(time_interval); });
      keepalive_timer_->enableTimer(time_interval);

      dispatcher_->run(Dispatcher::RunType::Block);
    });
  }

  ~DispatcherImplTest() override {
    dispatcher_->exit();
    dispatcher_thread_->join();
  }

  void timerTest(std::function<void(Timer&)> enable_timer_delegate) {
    TimerPtr timer;
    dispatcher_->post([this, &timer]() {
      {
        Thread::LockGuard lock(mu_);
        timer = dispatcher_->createTimer([this]() {
          {
            Thread::LockGuard lock(mu_);
            work_finished_ = true;
          }
          cv_.notifyOne();
        });
        EXPECT_FALSE(timer->enabled());
      }
      cv_.notifyOne();
    });

    Thread::LockGuard lock(mu_);
    while (timer == nullptr) {
      cv_.wait(mu_);
    }
    enable_timer_delegate(*timer);
    while (!work_finished_) {
      cv_.wait(mu_);
    }
  }

  NiceMock<Stats::MockStore> scope_; // Used in InitializeStats, must outlive dispatcher_->exit().
  Api::ApiPtr api_;
  Thread::ThreadPtr dispatcher_thread_;
  DispatcherPtr dispatcher_;
  Thread::MutexBasicLockable mu_;
  Thread::CondVar cv_;

  bool work_finished_{false};
  TimerPtr keepalive_timer_;
};

// TODO(mergeconflict): We also need integration testing to validate that the expected histograms
// are written when `enable_dispatcher_stats` is true. See issue #6582.
TEST_F(DispatcherImplTest, InitializeStats) {
  EXPECT_CALL(scope_,
              histogram("test.dispatcher.loop_duration_us", Stats::Histogram::Unit::Microseconds));
  EXPECT_CALL(scope_,
              histogram("test.dispatcher.poll_delay_us", Stats::Histogram::Unit::Microseconds));
  dispatcher_->initializeStats(scope_, "test.");
}

TEST_F(DispatcherImplTest, Post) {
  dispatcher_->post([this]() {
    {
      Thread::LockGuard lock(mu_);
      work_finished_ = true;
    }
    cv_.notifyOne();
  });

  Thread::LockGuard lock(mu_);
  while (!work_finished_) {
    cv_.wait(mu_);
  }
}

// Ensure that there is no deadlock related to calling a posted callback, or
// destructing a closure when finished calling it.
TEST_F(DispatcherImplTest, RunPostCallbacksLocking) {
  class PostOnDestruct {
  public:
    PostOnDestruct(Dispatcher& dispatcher) : dispatcher_(dispatcher) {}
    ~PostOnDestruct() {
      dispatcher_.post([]() {});
    }
    void method() {}
    Dispatcher& dispatcher_;
  };

  {
    // Block dispatcher first to ensure that both posted events below are handled
    // by a single call to runPostCallbacks().
    //
    // This also ensures that the post_lock_ is not held while callbacks are called,
    // or else this would deadlock.
    Thread::LockGuard lock(mu_);
    dispatcher_->post([this]() { Thread::LockGuard lock(mu_); });

    auto post_on_destruct = std::make_shared<PostOnDestruct>(*dispatcher_);
    dispatcher_->post([=]() { post_on_destruct->method(); });
    dispatcher_->post([this]() {
      {
        Thread::LockGuard lock(mu_);
        work_finished_ = true;
      }
      cv_.notifyOne();
    });
  }

  Thread::LockGuard lock(mu_);
  while (!work_finished_) {
    cv_.wait(mu_);
  }
}

TEST_F(DispatcherImplTest, Timer) {
  timerTest([](Timer& timer) { timer.enableTimer(std::chrono::milliseconds(0)); });
  timerTest([](Timer& timer) { timer.enableTimer(std::chrono::milliseconds(50)); });
  timerTest([](Timer& timer) { timer.enableHRTimer(std::chrono::microseconds(50)); });
}

TEST_F(DispatcherImplTest, TimerWithScope) {
  TimerPtr timer;
  MockScopedTrackedObject scope;
  dispatcher_->post([this, &timer, &scope]() {
    {
      // Expect a call to dumpState. The timer will call onFatalError during
      // the alarm interval, and if the scope is tracked correctly this will
      // result in a dumpState call.
      EXPECT_CALL(scope, dumpState(_, _));
      Thread::LockGuard lock(mu_);
      timer = dispatcher_->createTimer([this]() {
        {
          Thread::LockGuard lock(mu_);
          static_cast<DispatcherImpl*>(dispatcher_.get())->onFatalError(std::cerr);
          work_finished_ = true;
        }
        cv_.notifyOne();
      });
      EXPECT_FALSE(timer->enabled());
    }
    cv_.notifyOne();
  });

  Thread::LockGuard lock(mu_);
  while (timer == nullptr) {
    cv_.wait(mu_);
  }
  timer->enableTimer(std::chrono::milliseconds(50), &scope);

  while (!work_finished_) {
    cv_.wait(mu_);
  }
}

TEST_F(DispatcherImplTest, IsThreadSafe) {
  dispatcher_->post([this]() {
    {
      Thread::LockGuard lock(mu_);
      // Thread safe because it is called within the dispatcher thread's context.
      EXPECT_TRUE(dispatcher_->isThreadSafe());
      work_finished_ = true;
    }
    cv_.notifyOne();
  });

  Thread::LockGuard lock(mu_);
  while (!work_finished_) {
    cv_.wait(mu_);
  }
  // Not thread safe because it is not called within the dispatcher thread's context.
  EXPECT_FALSE(dispatcher_->isThreadSafe());
}

class NotStartedDispatcherImplTest : public testing::Test {
protected:
  NotStartedDispatcherImplTest()
      : api_(Api::createApiForTest()), dispatcher_(api_->allocateDispatcher("test_thread")) {}

  Api::ApiPtr api_;
  DispatcherPtr dispatcher_;
};

TEST_F(NotStartedDispatcherImplTest, IsThreadSafe) {
  // Thread safe because the dispatcher has not started.
  // Therefore, no thread id has been assigned.
  EXPECT_TRUE(dispatcher_->isThreadSafe());
}

class DispatcherMonotonicTimeTest : public testing::Test {
protected:
  DispatcherMonotonicTimeTest()
      : api_(Api::createApiForTest()), dispatcher_(api_->allocateDispatcher("test_thread")) {}
  ~DispatcherMonotonicTimeTest() override = default;

  Api::ApiPtr api_;
  DispatcherPtr dispatcher_;
  MonotonicTime time_;
};

TEST_F(DispatcherMonotonicTimeTest, UpdateApproximateMonotonicTime) {
  dispatcher_->post([this]() {
    {
      MonotonicTime time1 = dispatcher_->approximateMonotonicTime();
      dispatcher_->updateApproximateMonotonicTime();
      MonotonicTime time2 = dispatcher_->approximateMonotonicTime();
      EXPECT_LT(time1, time2);
    }
  });

  dispatcher_->run(Dispatcher::RunType::Block);
}

TEST_F(DispatcherMonotonicTimeTest, ApproximateMonotonicTime) {
  // approximateMonotonicTime is constant within one event loop run.
  dispatcher_->post([this]() {
    {
      time_ = dispatcher_->approximateMonotonicTime();
      EXPECT_EQ(time_, dispatcher_->approximateMonotonicTime());
    }
  });

  dispatcher_->run(Dispatcher::RunType::Block);

  // approximateMonotonicTime is increasing between event loop runs.
  dispatcher_->post([this]() {
    { EXPECT_LT(time_, dispatcher_->approximateMonotonicTime()); }
  });

  dispatcher_->run(Dispatcher::RunType::Block);
}

class TimerImplTest : public testing::TestWithParam<bool> {
protected:
  TimerImplTest() {
    Runtime::LoaderSingleton::getExisting()->mergeValues(
        {{"envoy.reloadable_features.activate_timers_next_event_loop",
          activateTimersNextEventLoop() ? "true" : "false"}});
    // Hook into event loop prepare and check events.
    evwatch_prepare_new(&libevent_base_, onWatcherReady, &prepare_watcher_);
    evwatch_check_new(&libevent_base_, onCheck, this);
  }
  ~TimerImplTest() override { ASSERT(check_callbacks_.empty()); }

  bool activateTimersNextEventLoop() { return GetParam(); }

  // Run a callback inside the event loop. The libevent monotonic time used for timer registration
  // is frozen while within this callback, so timers enabled within this callback end up with the
  // requested relative registration times. The callback can invoke advanceLibeventTime() to force
  // the libevent monotonic time forward before libevent determines the list of triggered timers.
  void runInEventLoop(std::function<void()> cb) {
    check_callbacks_.emplace_back(cb);

    // Add a callback to the event loop to force it to run at least once despite there being no
    // registered timers yet.
    auto callback = dispatcher_->createSchedulableCallback([]() {});
    callback->scheduleCallbackCurrentIteration();

    in_event_loop_ = true;
    dispatcher_->run(Dispatcher::RunType::NonBlock);
    in_event_loop_ = false;
  }

  // Advance time forward while updating the libevent's time cache and monotonic time reference.
  // Pushing the monotonic time reference forward eliminates the possibility of time moving
  // backwards and breaking the overly picky TimerImpl tests below.
  void advanceLibeventTime(absl::Duration duration) {
    ASSERT(in_event_loop_);
    requested_advance_ += duration;
    adjustCachedTime();
  }

  // Similar to advanceLibeventTime, but for use in mock callback actions. Monotonic time will be
  // moved forward at the start of the next event loop iteration.
  void advanceLibeventTimeNextIteration(absl::Duration duration) {
    ASSERT(in_event_loop_);
    requested_advance_ += duration;
  }

  Api::ApiPtr api_{Api::createApiForTest()};
  DispatcherPtr dispatcher_{api_->allocateDispatcher("test_thread")};
  event_base& libevent_base_{static_cast<DispatcherImpl&>(*dispatcher_).base()};
  ReadyWatcher prepare_watcher_;
  std::vector<SchedulableCallbackPtr> callbacks_;

private:
  static void onCheck(evwatch*, const evwatch_check_cb_info*, void* arg) {
    // `arg` contains the TimerImplTest passed in from evwatch_check_new.
    auto self = static_cast<TimerImplTest*>(arg);
    auto check_callbacks = self->check_callbacks_;
    self->check_callbacks_.clear();
    for (const auto& cb : check_callbacks) {
      cb();
    }
    self->adjustCachedTime();
  }

  absl::Duration cachedTimeAsDuration() const {
    timeval tv;
    int ret = event_base_gettimeofday_cached(&libevent_base_, &tv);
    RELEASE_ASSERT(ret == 0, "event_base_gettimeofday_cached failed");
    return absl::DurationFromTimeval(tv);
  }

  void adjustCachedTime() {
    auto start = cachedTimeAsDuration();
    // Sanity check: ensure that cache time is in use.
    EXPECT_EQ(start, cachedTimeAsDuration());

    while (cachedTimeAsDuration() - start < requested_advance_) {
      absl::SleepFor(absl::Milliseconds(1));
      event_base_update_cache_time(&libevent_base_);
    }
    requested_advance_ = absl::ZeroDuration();
  }

  TestScopedRuntime scoped_runtime_;
  absl::Duration requested_advance_ = absl::ZeroDuration();
  std::vector<std::function<void()>> check_callbacks_;
  bool in_event_loop_{};
};

INSTANTIATE_TEST_SUITE_P(DelayActivation, TimerImplTest, testing::Bool());

TEST_P(TimerImplTest, TimerEnabledDisabled) {
  InSequence s;

  Event::TimerPtr timer = dispatcher_->createTimer([] {});
  EXPECT_FALSE(timer->enabled());
  timer->enableTimer(std::chrono::milliseconds(0));
  EXPECT_TRUE(timer->enabled());
  EXPECT_CALL(prepare_watcher_, ready());
  dispatcher_->run(Dispatcher::RunType::NonBlock);
  EXPECT_FALSE(timer->enabled());
  timer->enableHRTimer(std::chrono::milliseconds(0));
  EXPECT_TRUE(timer->enabled());
  EXPECT_CALL(prepare_watcher_, ready());
  dispatcher_->run(Dispatcher::RunType::NonBlock);
  EXPECT_FALSE(timer->enabled());
}

TEST_P(TimerImplTest, ChangeTimerBackwardsBeforeRun) {
  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] { watcher1.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  ReadyWatcher watcher3;
  Event::TimerPtr timer3 = dispatcher_->createTimer([&] { watcher3.ready(); });

  // Expect watcher3 to trigger first because the deadlines for timers 1 and 2 was moved backwards.
  InSequence s;
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher3, ready());
  EXPECT_CALL(watcher2, ready());
  EXPECT_CALL(watcher1, ready());
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::milliseconds(0));
    timer2->enableTimer(std::chrono::milliseconds(1));
    timer3->enableTimer(std::chrono::milliseconds(2));
    timer2->enableTimer(std::chrono::milliseconds(3));
    timer1->enableTimer(std::chrono::milliseconds(4));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));
  });
}

TEST_P(TimerImplTest, ChangeTimerForwardsToZeroBeforeRun) {
  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] { watcher1.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  if (activateTimersNextEventLoop()) {
    // Expect watcher1 to trigger first because timer1's deadline was moved forward.
    InSequence s;
    EXPECT_CALL(prepare_watcher_, ready());
    EXPECT_CALL(watcher1, ready());
    EXPECT_CALL(watcher2, ready());
  } else {
    // Timers execute in the wrong order.
    InSequence s;
    EXPECT_CALL(prepare_watcher_, ready());
    EXPECT_CALL(watcher2, ready());
    EXPECT_CALL(watcher1, ready());
  }
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::milliseconds(2));
    timer2->enableTimer(std::chrono::milliseconds(1));
    timer1->enableTimer(std::chrono::milliseconds(0));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));
  });
}

TEST_P(TimerImplTest, ChangeTimerForwardsToNonZeroBeforeRun) {
  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] { watcher1.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  // Expect watcher1 to trigger first because timer1's deadline was moved forward.
  InSequence s;
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher1, ready());
  EXPECT_CALL(watcher2, ready());
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::milliseconds(3));
    timer2->enableTimer(std::chrono::milliseconds(2));
    timer1->enableTimer(std::chrono::milliseconds(1));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));
  });
}

TEST_P(TimerImplTest, ChangeLargeTimerForwardToZeroBeforeRun) {
  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] { watcher1.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  // Expect watcher1 to trigger because timer1's deadline was moved forward.
  InSequence s;
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher1, ready());
  EXPECT_CALL(prepare_watcher_, ready());
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::seconds(2000));
    timer2->enableTimer(std::chrono::seconds(1000));
    timer1->enableTimer(std::chrono::seconds(0));
  });
}

TEST_P(TimerImplTest, ChangeLargeTimerForwardToNonZeroBeforeRun) {
  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] { watcher1.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  // Expect watcher1 to trigger because timer1's deadline was moved forward.
  InSequence s;
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher1, ready());
  EXPECT_CALL(prepare_watcher_, ready());
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::seconds(2000));
    timer2->enableTimer(std::chrono::seconds(1000));
    timer1->enableTimer(std::chrono::milliseconds(1));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));
  });
}

// Timers scheduled at different times execute in order.
TEST_P(TimerImplTest, TimerOrdering) {
  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] { watcher1.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  ReadyWatcher watcher3;
  Event::TimerPtr timer3 = dispatcher_->createTimer([&] { watcher3.ready(); });

  // Expect watcher calls to happen in order since timers have different times.
  InSequence s;
  if (activateTimersNextEventLoop()) {
    EXPECT_CALL(prepare_watcher_, ready());
    EXPECT_CALL(watcher1, ready());
    EXPECT_CALL(watcher2, ready());
    EXPECT_CALL(watcher3, ready());
  } else {
    EXPECT_CALL(prepare_watcher_, ready());
    EXPECT_CALL(watcher1, ready());
    EXPECT_CALL(watcher2, ready());
    EXPECT_CALL(watcher3, ready());
  }

  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::milliseconds(0));
    timer2->enableTimer(std::chrono::milliseconds(1));
    timer3->enableTimer(std::chrono::milliseconds(2));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));

    EXPECT_TRUE(timer1->enabled());
    EXPECT_TRUE(timer2->enabled());
    EXPECT_TRUE(timer3->enabled());
  });
}

// Alarms that are scheduled to execute and are cancelled do not trigger.
TEST_P(TimerImplTest, TimerOrderAndDisableAlarm) {
  ReadyWatcher watcher3;
  Event::TimerPtr timer3 = dispatcher_->createTimer([&] { watcher3.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] {
    timer2->disableTimer();
    watcher1.ready();
  });

  // Expect watcher calls to happen in order since timers have different times.
  InSequence s;
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher1, ready());
  EXPECT_CALL(watcher3, ready());
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::milliseconds(0));
    timer2->enableTimer(std::chrono::milliseconds(1));
    timer3->enableTimer(std::chrono::milliseconds(2));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));

    EXPECT_TRUE(timer1->enabled());
    EXPECT_TRUE(timer2->enabled());
    EXPECT_TRUE(timer3->enabled());
  });
}

// Change the registration time for a timer that is already activated by disabling and re-enabling
// the timer. Verify that execution is delayed.
TEST_P(TimerImplTest, TimerOrderDisableAndReschedule) {
  ReadyWatcher watcher4;
  Event::TimerPtr timer4 = dispatcher_->createTimer([&] { watcher4.ready(); });

  ReadyWatcher watcher3;
  Event::TimerPtr timer3 = dispatcher_->createTimer([&] { watcher3.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] {
    timer2->disableTimer();
    timer2->enableTimer(std::chrono::milliseconds(0));
    timer3->disableTimer();
    timer3->enableTimer(std::chrono::milliseconds(1));
    watcher1.ready();
  });

  // timer1 is expected to run first and reschedule timers 2 and 3. timer4 should fire before
  // timer2 and timer3 since timer4's registration is unaffected.
  InSequence s;
  if (activateTimersNextEventLoop()) {
    EXPECT_CALL(prepare_watcher_, ready());
    EXPECT_CALL(watcher1, ready());
    EXPECT_CALL(watcher4, ready());
    // Sleep during prepare to ensure that enough time has elapsed before timer evaluation to ensure
    // that timers 2 and 3 are picked up by the same loop iteration. Without the sleep the two
    // timers could execute in different loop iterations.
    EXPECT_CALL(prepare_watcher_, ready()).WillOnce(testing::InvokeWithoutArgs([&]() {
      advanceLibeventTimeNextIteration(absl::Milliseconds(10));
    }));
    EXPECT_CALL(watcher2, ready());
    EXPECT_CALL(watcher3, ready());
  } else {
    EXPECT_CALL(prepare_watcher_, ready());
    EXPECT_CALL(watcher1, ready());
    EXPECT_CALL(watcher4, ready());
    EXPECT_CALL(watcher2, ready());
    // Sleep in prepare cb to avoid flakiness if epoll_wait returns before the timer timeout.
    EXPECT_CALL(prepare_watcher_, ready()).WillOnce(testing::InvokeWithoutArgs([&]() {
      advanceLibeventTimeNextIteration(absl::Milliseconds(10));
    }));
    EXPECT_CALL(watcher3, ready());
  }
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::milliseconds(0));
    timer2->enableTimer(std::chrono::milliseconds(1));
    timer3->enableTimer(std::chrono::milliseconds(2));
    timer4->enableTimer(std::chrono::milliseconds(3));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));

    EXPECT_TRUE(timer1->enabled());
    EXPECT_TRUE(timer2->enabled());
    EXPECT_TRUE(timer3->enabled());
    EXPECT_TRUE(timer4->enabled());
  });
}

// Change the registration time for a timer that is already activated by re-enabling the timer
// without calling disableTimer first.
TEST_P(TimerImplTest, TimerOrderAndReschedule) {
  ReadyWatcher watcher4;
  Event::TimerPtr timer4 = dispatcher_->createTimer([&] { watcher4.ready(); });

  ReadyWatcher watcher3;
  Event::TimerPtr timer3 = dispatcher_->createTimer([&] { watcher3.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] { watcher2.ready(); });

  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] {
    timer2->enableTimer(std::chrono::milliseconds(0));
    timer3->enableTimer(std::chrono::milliseconds(1));
    watcher1.ready();
  });

  // Rescheduling timers that are already scheduled to run in the current event loop iteration has
  // no effect if the time delta is 0. Expect timers 1, 2 and 4 to execute in the original order.
  // Timer 3 is delayed since it is rescheduled with a non-zero delta.
  InSequence s;
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher1, ready());
  if (activateTimersNextEventLoop()) {
    EXPECT_CALL(watcher4, ready());
    // Sleep during prepare to ensure that enough time has elapsed before timer evaluation to ensure
    // that timers 2 and 3 are picked up by the same loop iteration. Without the sleep the two
    // timers could execute in different loop iterations.
    EXPECT_CALL(prepare_watcher_, ready()).WillOnce(testing::InvokeWithoutArgs([&]() {
      advanceLibeventTimeNextIteration(absl::Milliseconds(10));
    }));
    EXPECT_CALL(watcher2, ready());
    EXPECT_CALL(watcher3, ready());
  } else {
    EXPECT_CALL(watcher2, ready());
    EXPECT_CALL(watcher4, ready());
    // Sleep in prepare cb to avoid flakiness if epoll_wait returns before the timer timeout.
    EXPECT_CALL(prepare_watcher_, ready()).WillOnce(testing::InvokeWithoutArgs([&]() {
      advanceLibeventTimeNextIteration(absl::Milliseconds(10));
    }));
    EXPECT_CALL(watcher3, ready());
  }
  runInEventLoop([&]() {
    timer1->enableTimer(std::chrono::milliseconds(0));
    timer2->enableTimer(std::chrono::milliseconds(1));
    timer3->enableTimer(std::chrono::milliseconds(2));
    timer4->enableTimer(std::chrono::milliseconds(3));

    // Advance time by 10ms so timers above all trigger in the same loop iteration.
    advanceLibeventTime(absl::Milliseconds(10));

    EXPECT_TRUE(timer1->enabled());
    EXPECT_TRUE(timer2->enabled());
    EXPECT_TRUE(timer3->enabled());
    EXPECT_TRUE(timer4->enabled());
  });
}

TEST_P(TimerImplTest, TimerChaining) {
  ReadyWatcher watcher1;
  Event::TimerPtr timer1 = dispatcher_->createTimer([&] { watcher1.ready(); });

  ReadyWatcher watcher2;
  Event::TimerPtr timer2 = dispatcher_->createTimer([&] {
    watcher2.ready();
    timer1->enableTimer(std::chrono::milliseconds(0));
  });

  ReadyWatcher watcher3;
  Event::TimerPtr timer3 = dispatcher_->createTimer([&] {
    watcher3.ready();
    timer2->enableTimer(std::chrono::milliseconds(0));
  });

  ReadyWatcher watcher4;
  Event::TimerPtr timer4 = dispatcher_->createTimer([&] {
    watcher4.ready();
    timer3->enableTimer(std::chrono::milliseconds(0));
  });

  timer4->enableTimer(std::chrono::milliseconds(0));

  EXPECT_FALSE(timer1->enabled());
  EXPECT_FALSE(timer2->enabled());
  EXPECT_FALSE(timer3->enabled());
  EXPECT_TRUE(timer4->enabled());
  InSequence s;
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher4, ready());
  if (activateTimersNextEventLoop()) {
    EXPECT_CALL(prepare_watcher_, ready());
  }
  EXPECT_CALL(watcher3, ready());
  if (activateTimersNextEventLoop()) {
    EXPECT_CALL(prepare_watcher_, ready());
  }
  EXPECT_CALL(watcher2, ready());
  if (activateTimersNextEventLoop()) {
    EXPECT_CALL(prepare_watcher_, ready());
  }
  EXPECT_CALL(watcher1, ready());
  dispatcher_->run(Dispatcher::RunType::NonBlock);

  EXPECT_FALSE(timer1->enabled());
  EXPECT_FALSE(timer2->enabled());
  EXPECT_FALSE(timer3->enabled());
  EXPECT_FALSE(timer4->enabled());
}

TEST_P(TimerImplTest, TimerChainDisable) {
  ReadyWatcher watcher;
  Event::TimerPtr timer1;
  Event::TimerPtr timer2;
  Event::TimerPtr timer3;

  auto timer_cb = [&] {
    watcher.ready();
    timer1->disableTimer();
    timer2->disableTimer();
    timer3->disableTimer();
  };

  timer1 = dispatcher_->createTimer(timer_cb);
  timer2 = dispatcher_->createTimer(timer_cb);
  timer3 = dispatcher_->createTimer(timer_cb);

  timer3->enableTimer(std::chrono::milliseconds(0));
  timer2->enableTimer(std::chrono::milliseconds(0));
  timer1->enableTimer(std::chrono::milliseconds(0));

  EXPECT_TRUE(timer1->enabled());
  EXPECT_TRUE(timer2->enabled());
  EXPECT_TRUE(timer3->enabled());
  InSequence s;
  // Only 1 call to watcher ready since the other 2 timers were disabled by the first timer.
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher, ready());
  dispatcher_->run(Dispatcher::RunType::NonBlock);
}

TEST_P(TimerImplTest, TimerChainDelete) {
  ReadyWatcher watcher;
  Event::TimerPtr timer1;
  Event::TimerPtr timer2;
  Event::TimerPtr timer3;

  auto timer_cb = [&] {
    watcher.ready();
    timer1.reset();
    timer2.reset();
    timer3.reset();
  };

  timer1 = dispatcher_->createTimer(timer_cb);
  timer2 = dispatcher_->createTimer(timer_cb);
  timer3 = dispatcher_->createTimer(timer_cb);

  timer3->enableTimer(std::chrono::milliseconds(0));
  timer2->enableTimer(std::chrono::milliseconds(0));
  timer1->enableTimer(std::chrono::milliseconds(0));

  EXPECT_TRUE(timer1->enabled());
  EXPECT_TRUE(timer2->enabled());
  EXPECT_TRUE(timer3->enabled());
  InSequence s;
  // Only 1 call to watcher ready since the other 2 timers were deleted by the first timer.
  EXPECT_CALL(prepare_watcher_, ready());
  EXPECT_CALL(watcher, ready());
  dispatcher_->run(Dispatcher::RunType::NonBlock);
}

class TimerImplTimingTest : public testing::Test {
public:
  std::chrono::nanoseconds getTimerTiming(Event::SimulatedTimeSystem& time_system,
                                          Dispatcher& dispatcher, Event::Timer& timer) {
    const auto start = time_system.monotonicTime();
    EXPECT_TRUE(timer.enabled());
    while (true) {
      dispatcher.run(Dispatcher::RunType::NonBlock);
#ifdef WIN32
      // The event loop runs for a single iteration in NonBlock mode on Windows. A few iterations
      // are required to ensure that next iteration callbacks have a chance to run before time
      // advances once again.
      dispatcher.run(Dispatcher::RunType::NonBlock);
      dispatcher.run(Dispatcher::RunType::NonBlock);
#endif
      if (timer.enabled()) {
        time_system.advanceTimeAsync(std::chrono::microseconds(1));
      } else {
        break;
      }
    }
    return time_system.monotonicTime() - start;
  }
};

// Test the timer with a series of timings and measure they fire accurately
// using simulated time. enableTimer() should be precise at the millisecond
// level, whereas enableHRTimer should be precise at the microsecond level.
// For good measure, also check that '0'/immediate does what it says on the tin.
TEST_F(TimerImplTimingTest, TheoreticalTimerTiming) {
  Event::SimulatedTimeSystem time_system;
  Api::ApiPtr api = Api::createApiForTest(time_system);
  DispatcherPtr dispatcher(api->allocateDispatcher("test_thread"));
  Event::TimerPtr timer = dispatcher->createTimer([&dispatcher] { dispatcher->exit(); });

  const uint64_t timings[] = {0, 10, 50, 1234};
  for (const uint64_t timing : timings) {
    std::chrono::milliseconds ms(timing);
    timer->enableTimer(ms);
    EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(
                  getTimerTiming(time_system, *dispatcher, *timer))
                  .count(),
              timing);

    std::chrono::microseconds us(timing);
    timer->enableHRTimer(us);
    EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(
                  getTimerTiming(time_system, *dispatcher, *timer))
                  .count(),
              timing);
  }
}

class TimerUtilsTest : public testing::Test {
public:
  template <typename Duration>
  void checkConversion(const Duration& duration, const uint64_t expected_secs,
                       const uint64_t expected_usecs) {
    timeval tv;
    TimerUtils::durationToTimeval(duration, tv);
    EXPECT_EQ(tv.tv_sec, expected_secs);
    EXPECT_EQ(tv.tv_usec, expected_usecs);
  }
};

TEST_F(TimerUtilsTest, TimerNegativeValueThrows) {
  timeval tv;
  const int negative_sample = -1;
  EXPECT_THROW_WITH_MESSAGE(
      TimerUtils::durationToTimeval(std::chrono::seconds(negative_sample), tv), EnvoyException,
      fmt::format("Negative duration passed to durationToTimeval(): {}", negative_sample));
}

TEST_F(TimerUtilsTest, TimerValueConversion) {
  // Check input is bounded.
  checkConversion(std::chrono::nanoseconds::duration::max(), INT32_MAX, 0);
  checkConversion(std::chrono::microseconds::duration::max(), INT32_MAX, 0);
  checkConversion(std::chrono::milliseconds::duration::max(), INT32_MAX, 0);
  checkConversion(std::chrono::seconds::duration::max(), INT32_MAX, 0);

  // Test the clipping boundary
  checkConversion(std::chrono::seconds(INT32_MAX) - std::chrono::seconds(1), INT32_MAX - 1, 0);
  checkConversion(std::chrono::seconds(INT32_MAX) - std::chrono::nanoseconds(1), INT32_MAX - 1,
                  999999);

  // Basic test with zero milliseconds.
  checkConversion(std::chrono::milliseconds(0), 0, 0);

  // 2050 milliseconds is 2 seconds and 50000 microseconds.
  checkConversion(std::chrono::milliseconds(2050), 2, 50000);

  // Some arbitrary tests for good measure.
  checkConversion(std::chrono::microseconds(233), 0, 233);

  // Some arbitrary tests for good measure.
  checkConversion(std::chrono::milliseconds(600014), 600, 14000);
}

} // namespace
} // namespace Event
} // namespace Envoy
