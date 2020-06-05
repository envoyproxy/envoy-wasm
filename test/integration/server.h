#pragma once

#include <chrono>
#include <cstdint>
#include <list>
#include <memory>
#include <string>

#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/server/options.h"
#include "envoy/server/process_context.h"
#include "envoy/stats/stats.h"

#include "common/common/assert.h"
#include "common/common/lock_guard.h"
#include "common/common/logger.h"
#include "common/common/thread.h"

#include "server/drain_manager_impl.h"
#include "server/listener_hooks.h"
#include "server/options_impl.h"
#include "server/server.h"

#include "test/integration/server_stats.h"
#include "test/integration/tcp_dump.h"
#include "test/test_common/test_time_system.h"
#include "test/test_common/utility.h"

#include "absl/synchronization/notification.h"
#include "absl/types/optional.h"

namespace Envoy {
namespace Server {

struct FieldValidationConfig {
  bool allow_unknown_static_fields = false;
  bool reject_unknown_dynamic_fields = false;
  bool ignore_unknown_dynamic_fields = false;
};

// Create OptionsImpl structures suitable for tests. Disables hot restart.
OptionsImpl createTestOptionsImpl(const std::string& config_path, const std::string& config_yaml,
                                  Network::Address::IpVersion ip_version,
                                  FieldValidationConfig validation_config = FieldValidationConfig(),
                                  uint32_t concurrency = 1,
                                  std::chrono::seconds drain_time = std::chrono::seconds(1));

class TestComponentFactory : public ComponentFactory {
public:
  Server::DrainManagerPtr createDrainManager(Server::Instance& server) override {
    return Server::DrainManagerPtr{
        new Server::DrainManagerImpl(server, envoy::config::listener::v3::Listener::MODIFY_ONLY)};
  }
  Runtime::LoaderPtr createRuntime(Server::Instance& server,
                                   Server::Configuration::Initial& config) override {
    return Server::InstanceUtil::createRuntime(server, config);
  }
};

} // namespace Server

namespace Stats {

/**
 * This is a wrapper for Scopes for the TestIsolatedStoreImpl to ensure new scopes do
 * not interact with the store without grabbing the lock from TestIsolatedStoreImpl.
 */
class TestScopeWrapper : public Scope {
public:
  TestScopeWrapper(Thread::MutexBasicLockable& lock, ScopePtr wrapped_scope)
      : lock_(lock), wrapped_scope_(std::move(wrapped_scope)) {}

  ScopePtr createScope(const std::string& name) override {
    Thread::LockGuard lock(lock_);
    return ScopePtr{new TestScopeWrapper(lock_, wrapped_scope_->createScope(name))};
  }

  void deliverHistogramToSinks(const Histogram& histogram, uint64_t value) override {
    Thread::LockGuard lock(lock_);
    wrapped_scope_->deliverHistogramToSinks(histogram, value);
  }

  Counter& counterFromStatNameWithTags(const StatName& name,
                                       StatNameTagVectorOptConstRef tags) override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->counterFromStatNameWithTags(name, tags);
  }

  Gauge& gaugeFromStatNameWithTags(const StatName& name, StatNameTagVectorOptConstRef tags,
                                   Gauge::ImportMode import_mode) override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->gaugeFromStatNameWithTags(name, tags, import_mode);
  }

  Histogram& histogramFromStatNameWithTags(const StatName& name, StatNameTagVectorOptConstRef tags,
                                           Histogram::Unit unit) override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->histogramFromStatNameWithTags(name, tags, unit);
  }

  TextReadout& textReadoutFromStatNameWithTags(const StatName& name,
                                               StatNameTagVectorOptConstRef tags) override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->textReadoutFromStatNameWithTags(name, tags);
  }

  NullGaugeImpl& nullGauge(const std::string& str) override {
    return wrapped_scope_->nullGauge(str);
  }

  Counter& counterFromString(const std::string& name) override {
    StatNameManagedStorage storage(name, symbolTable());
    return counterFromStatName(storage.statName());
  }
  Gauge& gaugeFromString(const std::string& name, Gauge::ImportMode import_mode) override {
    StatNameManagedStorage storage(name, symbolTable());
    return gaugeFromStatName(storage.statName(), import_mode);
  }
  Histogram& histogramFromString(const std::string& name, Histogram::Unit unit) override {
    StatNameManagedStorage storage(name, symbolTable());
    return histogramFromStatName(storage.statName(), unit);
  }
  TextReadout& textReadoutFromString(const std::string& name) override {
    StatNameManagedStorage storage(name, symbolTable());
    return textReadoutFromStatName(storage.statName());
  }

  CounterOptConstRef findCounter(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->findCounter(name);
  }
  GaugeOptConstRef findGauge(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->findGauge(name);
  }
  HistogramOptConstRef findHistogram(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->findHistogram(name);
  }
  TextReadoutOptConstRef findTextReadout(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return wrapped_scope_->findTextReadout(name);
  }

  const SymbolTable& constSymbolTable() const override {
    return wrapped_scope_->constSymbolTable();
  }
  SymbolTable& symbolTable() override { return wrapped_scope_->symbolTable(); }

private:
  Thread::MutexBasicLockable& lock_;
  ScopePtr wrapped_scope_;
};

/**
 * This is a variant of the isolated store that has locking across all operations so that it can
 * be used during the integration tests.
 */
class TestIsolatedStoreImpl : public StoreRoot {
public:
  // Stats::Scope
  Counter& counterFromStatNameWithTags(const StatName& name,
                                       StatNameTagVectorOptConstRef tags) override {
    Thread::LockGuard lock(lock_);
    return store_.counterFromStatNameWithTags(name, tags);
  }
  Counter& counterFromString(const std::string& name) override {
    Thread::LockGuard lock(lock_);
    return store_.counterFromString(name);
  }
  ScopePtr createScope(const std::string& name) override {
    Thread::LockGuard lock(lock_);
    return ScopePtr{new TestScopeWrapper(lock_, store_.createScope(name))};
  }
  void deliverHistogramToSinks(const Histogram&, uint64_t) override {}
  Gauge& gaugeFromStatNameWithTags(const StatName& name, StatNameTagVectorOptConstRef tags,
                                   Gauge::ImportMode import_mode) override {
    Thread::LockGuard lock(lock_);
    return store_.gaugeFromStatNameWithTags(name, tags, import_mode);
  }
  Gauge& gaugeFromString(const std::string& name, Gauge::ImportMode import_mode) override {
    Thread::LockGuard lock(lock_);
    return store_.gaugeFromString(name, import_mode);
  }
  Histogram& histogramFromStatNameWithTags(const StatName& name, StatNameTagVectorOptConstRef tags,
                                           Histogram::Unit unit) override {
    Thread::LockGuard lock(lock_);
    return store_.histogramFromStatNameWithTags(name, tags, unit);
  }
  NullGaugeImpl& nullGauge(const std::string& name) override { return store_.nullGauge(name); }
  Histogram& histogramFromString(const std::string& name, Histogram::Unit unit) override {
    Thread::LockGuard lock(lock_);
    return store_.histogramFromString(name, unit);
  }
  TextReadout& textReadoutFromStatNameWithTags(const StatName& name,
                                               StatNameTagVectorOptConstRef tags) override {
    Thread::LockGuard lock(lock_);
    return store_.textReadoutFromStatNameWithTags(name, tags);
  }
  TextReadout& textReadoutFromString(const std::string& name) override {
    Thread::LockGuard lock(lock_);
    return store_.textReadoutFromString(name);
  }
  CounterOptConstRef findCounter(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return store_.findCounter(name);
  }
  GaugeOptConstRef findGauge(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return store_.findGauge(name);
  }
  HistogramOptConstRef findHistogram(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return store_.findHistogram(name);
  }
  TextReadoutOptConstRef findTextReadout(StatName name) const override {
    Thread::LockGuard lock(lock_);
    return store_.findTextReadout(name);
  }
  const SymbolTable& constSymbolTable() const override { return store_.constSymbolTable(); }
  SymbolTable& symbolTable() override { return store_.symbolTable(); }

  // Stats::Store
  std::vector<CounterSharedPtr> counters() const override {
    Thread::LockGuard lock(lock_);
    return store_.counters();
  }
  std::vector<GaugeSharedPtr> gauges() const override {
    Thread::LockGuard lock(lock_);
    return store_.gauges();
  }
  std::vector<ParentHistogramSharedPtr> histograms() const override {
    Thread::LockGuard lock(lock_);
    return store_.histograms();
  }
  std::vector<TextReadoutSharedPtr> textReadouts() const override {
    Thread::LockGuard lock(lock_);
    return store_.textReadouts();
  }

  // Stats::StoreRoot
  void addSink(Sink&) override {}
  void setTagProducer(TagProducerPtr&&) override {}
  void setStatsMatcher(StatsMatcherPtr&&) override {}
  void initializeThreading(Event::Dispatcher&, ThreadLocal::Instance&) override {}
  void shutdownThreading() override {}
  void mergeHistograms(PostMergeCb) override {}

private:
  mutable Thread::MutexBasicLockable lock_;
  IsolatedStoreImpl store_;
};

} // namespace Stats

class IntegrationTestServer;
using IntegrationTestServerPtr = std::unique_ptr<IntegrationTestServer>;

/**
 * Wrapper for running the real server for the purpose of integration tests.
 * This class is an Abstract Base Class and delegates ownership and management
 * of the actual envoy server to a derived class. See the documentation for
 * createAndRunEnvoyServer().
 */
class IntegrationTestServer : public Logger::Loggable<Logger::Id::testing>,
                              public ListenerHooks,
                              public IntegrationTestServerStats,
                              public Server::ComponentFactory {
public:
  static IntegrationTestServerPtr
  create(const std::string& config_path, const Network::Address::IpVersion version,
         std::function<void(IntegrationTestServer&)> on_server_ready_function,
         std::function<void()> on_server_init_function, bool deterministic,
         Event::TestTimeSystem& time_system, Api::Api& api,
         bool defer_listener_finalization = false,
         ProcessObjectOptRef process_object = absl::nullopt,
         Server::FieldValidationConfig validation_config = Server::FieldValidationConfig(),
         uint32_t concurrency = 1, std::chrono::seconds drain_time = std::chrono::seconds(1));
  // Note that the derived class is responsible for tearing down the server in its
  // destructor.
  ~IntegrationTestServer() override;

  void waitUntilListenersReady();

  Server::DrainManagerImpl& drainManager() { return *drain_manager_; }
  void setOnWorkerListenerAddedCb(std::function<void()> on_worker_listener_added) {
    on_worker_listener_added_cb_ = std::move(on_worker_listener_added);
  }
  void setOnWorkerListenerRemovedCb(std::function<void()> on_worker_listener_removed) {
    on_worker_listener_removed_cb_ = std::move(on_worker_listener_removed);
  }
  void setOnServerReadyCb(std::function<void(IntegrationTestServer&)> on_server_ready) {
    on_server_ready_cb_ = std::move(on_server_ready);
  }
  void onRuntimeCreated() override {}

  void start(const Network::Address::IpVersion version,
             std::function<void()> on_server_init_function, bool deterministic,
             bool defer_listener_finalization, ProcessObjectOptRef process_object,
             Server::FieldValidationConfig validation_config, uint32_t concurrency,
             std::chrono::seconds drain_time);

  void waitForCounterEq(const std::string& name, uint64_t value) override {
    TestUtility::waitForCounterEq(statStore(), name, value, time_system_);
  }

  void waitForCounterGe(const std::string& name, uint64_t value) override {
    TestUtility::waitForCounterGe(statStore(), name, value, time_system_);
  }

  void waitForGaugeGe(const std::string& name, uint64_t value) override {
    TestUtility::waitForGaugeGe(statStore(), name, value, time_system_);
  }

  void waitForGaugeEq(const std::string& name, uint64_t value) override {
    TestUtility::waitForGaugeEq(statStore(), name, value, time_system_);
  }

  Stats::CounterSharedPtr counter(const std::string& name) override {
    // When using the thread local store, only counters() is thread safe. This also allows us
    // to test if a counter exists at all versus just defaulting to zero.
    return TestUtility::findCounter(statStore(), name);
  }

  Stats::GaugeSharedPtr gauge(const std::string& name) override {
    // When using the thread local store, only gauges() is thread safe. This also allows us
    // to test if a counter exists at all versus just defaulting to zero.
    return TestUtility::findGauge(statStore(), name);
  }

  std::vector<Stats::CounterSharedPtr> counters() override { return statStore().counters(); }

  std::vector<Stats::GaugeSharedPtr> gauges() override { return statStore().gauges(); }

  // ListenerHooks
  void onWorkerListenerAdded() override;
  void onWorkerListenerRemoved() override;

  // Server::ComponentFactory
  Server::DrainManagerPtr createDrainManager(Server::Instance& server) override {
    drain_manager_ =
        new Server::DrainManagerImpl(server, envoy::config::listener::v3::Listener::MODIFY_ONLY);
    return Server::DrainManagerPtr{drain_manager_};
  }
  Runtime::LoaderPtr createRuntime(Server::Instance& server,
                                   Server::Configuration::Initial& config) override {
    return Server::InstanceUtil::createRuntime(server, config);
  }

  // Should not be called until createAndRunEnvoyServer() is called.
  virtual Server::Instance& server() PURE;
  virtual Stats::Store& statStore() PURE;
  virtual Network::Address::InstanceConstSharedPtr adminAddress() PURE;
  void useAdminInterfaceToQuit(bool use) { use_admin_interface_to_quit_ = use; }
  bool useAdminInterfaceToQuit() { return use_admin_interface_to_quit_; }

protected:
  IntegrationTestServer(Event::TestTimeSystem& time_system, Api::Api& api,
                        const std::string& config_path)
      : time_system_(time_system), api_(api), config_path_(config_path) {}

  // Create the running envoy server. This function will call serverReady() when the virtual
  // functions server(), statStore(), and adminAddress() may be called, but before the server
  // has been started.
  // The subclass is also responsible for tearing down this server in its destructor.
  virtual void createAndRunEnvoyServer(OptionsImpl& options, Event::TimeSystem& time_system,
                                       Network::Address::InstanceConstSharedPtr local_address,
                                       ListenerHooks& hooks, Thread::BasicLockable& access_log_lock,
                                       Server::ComponentFactory& component_factory,
                                       Runtime::RandomGeneratorPtr&& random_generator,
                                       ProcessObjectOptRef process_object) PURE;

  // Will be called by subclass on server thread when the server is ready to be accessed. The
  // server may not have been run yet, but all server access methods (server(), statStore(),
  // adminAddress()) will be available.
  void serverReady();

private:
  /**
   * Runs the real server on a thread.
   */
  void threadRoutine(const Network::Address::IpVersion version, bool deterministic,
                     ProcessObjectOptRef process_object,
                     Server::FieldValidationConfig validation_config, uint32_t concurrency,
                     std::chrono::seconds drain_time);

  Event::TestTimeSystem& time_system_;
  Api::Api& api_;
  const std::string config_path_;
  Thread::ThreadPtr thread_;
  Thread::CondVar listeners_cv_;
  Thread::MutexBasicLockable listeners_mutex_;
  uint64_t pending_listeners_;
  ConditionalInitializer server_set_;
  Server::DrainManagerImpl* drain_manager_{};
  std::function<void()> on_worker_listener_added_cb_;
  std::function<void()> on_worker_listener_removed_cb_;
  TcpDumpPtr tcp_dump_;
  std::function<void(IntegrationTestServer&)> on_server_ready_cb_;
  bool use_admin_interface_to_quit_{};
};

// Default implementation of IntegrationTestServer
class IntegrationTestServerImpl : public IntegrationTestServer {
public:
  IntegrationTestServerImpl(Event::TestTimeSystem& time_system, Api::Api& api,
                            const std::string& config_path)
      : IntegrationTestServer(time_system, api, config_path) {}

  ~IntegrationTestServerImpl() override;

  Server::Instance& server() override {
    RELEASE_ASSERT(server_ != nullptr, "");
    return *server_;
  }
  Stats::Store& statStore() override {
    RELEASE_ASSERT(stat_store_ != nullptr, "");
    return *stat_store_;
  }
  Network::Address::InstanceConstSharedPtr adminAddress() override { return admin_address_; }

private:
  void createAndRunEnvoyServer(OptionsImpl& options, Event::TimeSystem& time_system,
                               Network::Address::InstanceConstSharedPtr local_address,
                               ListenerHooks& hooks, Thread::BasicLockable& access_log_lock,
                               Server::ComponentFactory& component_factory,
                               Runtime::RandomGeneratorPtr&& random_generator,
                               ProcessObjectOptRef process_object) override;

  // Owned by this class. An owning pointer is not used because the actual allocation is done
  // on a stack in a non-main thread.
  Server::Instance* server_{};
  Stats::Store* stat_store_{};
  Network::Address::InstanceConstSharedPtr admin_address_;
  absl::Notification server_gone_;
};

} // namespace Envoy
