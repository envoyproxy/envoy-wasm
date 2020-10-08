#include "server/server.h"

#include <csignal>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "envoy/admin/v3/config_dump.pb.h"
#include "envoy/common/exception.h"
#include "envoy/config/bootstrap/v2/bootstrap.pb.h"
#include "envoy/config/bootstrap/v2/bootstrap.pb.validate.h"
#include "envoy/config/bootstrap/v3/bootstrap.pb.h"
#include "envoy/config/bootstrap/v3/bootstrap.pb.validate.h"
#include "envoy/event/dispatcher.h"
#include "envoy/event/signal.h"
#include "envoy/event/timer.h"
#include "envoy/network/dns.h"
#include "envoy/registry/registry.h"
#include "envoy/server/bootstrap_extension_config.h"
#include "envoy/server/options.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/api/api_impl.h"
#include "common/api/os_sys_calls_impl.h"
#include "common/common/enum_to_int.h"
#include "common/common/mutex_tracer_impl.h"
#include "common/common/utility.h"
#include "common/config/utility.h"
#include "common/config/version_converter.h"
#include "common/http/codes.h"
#include "common/local_info/local_info_impl.h"
#include "common/memory/stats.h"
#include "common/network/address_impl.h"
#include "common/network/socket_interface.h"
#include "common/network/socket_interface_impl.h"
#include "common/network/tcp_listener_impl.h"
#include "common/protobuf/utility.h"
#include "common/router/rds_impl.h"
#include "common/runtime/runtime_impl.h"
#include "common/singleton/manager_impl.h"
#include "common/stats/thread_local_store.h"
#include "common/stats/timespan_impl.h"
#include "common/upstream/cluster_manager_impl.h"
#include "common/version/version.h"

#include "server/admin/utils.h"
#include "server/configuration_impl.h"
#include "server/connection_handler_impl.h"
#include "server/guarddog_impl.h"
#include "server/listener_hooks.h"
#include "server/ssl_context_manager.h"

namespace Envoy {
namespace Server {

InstanceImpl::InstanceImpl(
    Init::Manager& init_manager, const Options& options, Event::TimeSystem& time_system,
    Network::Address::InstanceConstSharedPtr local_address, ListenerHooks& hooks,
    HotRestart& restarter, Stats::StoreRoot& store, Thread::BasicLockable& access_log_lock,
    ComponentFactory& component_factory, Random::RandomGeneratorPtr&& random_generator,
    ThreadLocal::Instance& tls, Thread::ThreadFactory& thread_factory,
    Filesystem::Instance& file_system, std::unique_ptr<ProcessContext> process_context)
    : init_manager_(init_manager), workers_started_(false), live_(false), shutdown_(false),
      options_(options), validation_context_(options_.allowUnknownStaticFields(),
                                             !options.rejectUnknownDynamicFields(),
                                             options.ignoreUnknownDynamicFields()),
      time_source_(time_system), restarter_(restarter), start_time_(time(nullptr)),
      original_start_time_(start_time_), stats_store_(store), thread_local_(tls),
      random_generator_(std::move(random_generator)),
      api_(new Api::Impl(thread_factory, store, time_system, file_system, *random_generator_,
                         process_context ? ProcessContextOptRef(std::ref(*process_context))
                                         : absl::nullopt)),
      dispatcher_(api_->allocateDispatcher("main_thread")),
      singleton_manager_(new Singleton::ManagerImpl(api_->threadFactory())),
      handler_(new ConnectionHandlerImpl(*dispatcher_, absl::nullopt)),
      listener_component_factory_(*this), worker_factory_(thread_local_, *api_, hooks),
      access_log_manager_(options.fileFlushIntervalMsec(), *api_, *dispatcher_, access_log_lock,
                          store),
      terminated_(false),
      mutex_tracer_(options.mutexTracingEnabled() ? &Envoy::MutexTracerImpl::getOrCreateTracer()
                                                  : nullptr),
      grpc_context_(store.symbolTable()), http_context_(store.symbolTable()),
      process_context_(std::move(process_context)), main_thread_id_(std::this_thread::get_id()),
      server_contexts_(*this) {
  try {
    if (!options.logPath().empty()) {
      try {
        file_logger_ = std::make_unique<Logger::FileSinkDelegate>(
            options.logPath(), access_log_manager_, Logger::Registry::getSink());
      } catch (const EnvoyException& e) {
        throw EnvoyException(
            fmt::format("Failed to open log-file '{}'. e.what(): {}", options.logPath(), e.what()));
      }
    }

    restarter_.initialize(*dispatcher_, *this);
    drain_manager_ = component_factory.createDrainManager(*this);
    initialize(options, std::move(local_address), component_factory, hooks);
  } catch (const EnvoyException& e) {
    ENVOY_LOG(critical, "error initializing configuration '{}': {}", options.configPath(),
              e.what());
    terminate();
    throw;
  } catch (const std::exception& e) {
    ENVOY_LOG(critical, "error initializing due to unexpected exception: {}", e.what());
    terminate();
    throw;
  } catch (...) {
    ENVOY_LOG(critical, "error initializing due to unknown exception");
    terminate();
    throw;
  }
}

InstanceImpl::~InstanceImpl() {
  terminate();

  // Stop logging to file before all the AccessLogManager and its dependencies are
  // destructed to avoid crashing at shutdown.
  file_logger_.reset();

  // Destruct the ListenerManager explicitly, before InstanceImpl's local init_manager_ is
  // destructed.
  //
  // The ListenerManager's DestinationPortsMap contains FilterChainSharedPtrs. There is a rare race
  // condition where one of these FilterChains contains an HttpConnectionManager, which contains an
  // RdsRouteConfigProvider, which contains an RdsRouteConfigSubscriptionSharedPtr. Since
  // RdsRouteConfigSubscription is an Init::Target, ~RdsRouteConfigSubscription triggers a callback
  // set at initialization, which goes to unregister it from the top-level InitManager, which has
  // already been destructed (use-after-free) causing a segfault.
  ENVOY_LOG(debug, "destroying listener manager");
  listener_manager_.reset();
  ENVOY_LOG(debug, "destroyed listener manager");
}

Upstream::ClusterManager& InstanceImpl::clusterManager() { return *config_.clusterManager(); }

void InstanceImpl::drainListeners() {
  ENVOY_LOG(info, "closing and draining listeners");
  listener_manager_->stopListeners(ListenerManager::StopListenersType::All);
  drain_manager_->startDrainSequence([] {});
}

void InstanceImpl::failHealthcheck(bool fail) {
  live_.store(!fail);
  server_stats_->live_.set(live_.load());
}

MetricSnapshotImpl::MetricSnapshotImpl(Stats::Store& store) {
  snapped_counters_ = store.counters();
  counters_.reserve(snapped_counters_.size());
  for (const auto& counter : snapped_counters_) {
    counters_.push_back({counter->latch(), *counter});
  }

  snapped_gauges_ = store.gauges();
  gauges_.reserve(snapped_gauges_.size());
  for (const auto& gauge : snapped_gauges_) {
    ASSERT(gauge->importMode() != Stats::Gauge::ImportMode::Uninitialized);
    gauges_.push_back(*gauge);
  }

  snapped_histograms_ = store.histograms();
  histograms_.reserve(snapped_histograms_.size());
  for (const auto& histogram : snapped_histograms_) {
    histograms_.push_back(*histogram);
  }

  snapped_text_readouts_ = store.textReadouts();
  text_readouts_.reserve(snapped_text_readouts_.size());
  for (const auto& text_readout : snapped_text_readouts_) {
    text_readouts_.push_back(*text_readout);
  }
}

void InstanceUtil::flushMetricsToSinks(const std::list<Stats::SinkPtr>& sinks,
                                       Stats::Store& store) {
  // Create a snapshot and flush to all sinks.
  // NOTE: Even if there are no sinks, creating the snapshot has the important property that it
  //       latches all counters on a periodic basis. The hot restart code assumes this is being
  //       done so this should not be removed.
  MetricSnapshotImpl snapshot(store);
  for (const auto& sink : sinks) {
    sink->flush(snapshot);
  }
}

void InstanceImpl::flushStats() {
  ENVOY_LOG(debug, "flushing stats");
  // If Envoy is not fully initialized, workers will not be started and mergeHistograms
  // completion callback is not called immediately. As a result of this server stats will
  // not be updated and flushed to stat sinks. So skip mergeHistograms call if workers are
  // not started yet.
  if (initManager().state() == Init::Manager::State::Initialized) {
    // A shutdown initiated before this callback may prevent this from being called as per
    // the semantics documented in ThreadLocal's runOnAllThreads method.
    stats_store_.mergeHistograms([this]() -> void { flushStatsInternal(); });
  } else {
    ENVOY_LOG(debug, "Envoy is not fully initialized, skipping histogram merge and flushing stats");
    flushStatsInternal();
  }
}

void InstanceImpl::updateServerStats() {
  // mergeParentStatsIfAny() does nothing and returns a struct of 0s if there is no parent.
  HotRestart::ServerStatsFromParent parent_stats = restarter_.mergeParentStatsIfAny(stats_store_);

  server_stats_->uptime_.set(time(nullptr) - original_start_time_);
  server_stats_->memory_allocated_.set(Memory::Stats::totalCurrentlyAllocated() +
                                       parent_stats.parent_memory_allocated_);
  server_stats_->memory_heap_size_.set(Memory::Stats::totalCurrentlyReserved());
  server_stats_->memory_physical_size_.set(Memory::Stats::totalPhysicalBytes());
  server_stats_->parent_connections_.set(parent_stats.parent_connections_);
  server_stats_->total_connections_.set(listener_manager_->numConnections() +
                                        parent_stats.parent_connections_);
  server_stats_->days_until_first_cert_expiring_.set(
      sslContextManager().daysUntilFirstCertExpires());

  auto secs_until_ocsp_response_expires =
      sslContextManager().secondsUntilFirstOcspResponseExpires();
  if (secs_until_ocsp_response_expires) {
    server_stats_->seconds_until_first_ocsp_response_expiring_.set(
        secs_until_ocsp_response_expires.value());
  }
  server_stats_->state_.set(
      enumToInt(Utility::serverState(initManager().state(), healthCheckFailed())));
  server_stats_->stats_recent_lookups_.set(
      stats_store_.symbolTable().getRecentLookups([](absl::string_view, uint64_t) {}));
}

void InstanceImpl::flushStatsInternal() {
  updateServerStats();
  InstanceUtil::flushMetricsToSinks(config_.statsSinks(), stats_store_);
  // TODO(ramaraochavali): consider adding different flush interval for histograms.
  if (stat_flush_timer_ != nullptr) {
    stat_flush_timer_->enableTimer(config_.statsFlushInterval());
  }
}

bool InstanceImpl::healthCheckFailed() { return !live_.load(); }

namespace {
// Loads a bootstrap object, potentially at a specific version (upgrading if necessary).
void loadBootstrap(absl::optional<uint32_t> bootstrap_version,
                   envoy::config::bootstrap::v3::Bootstrap& bootstrap,
                   std::function<void(Protobuf::Message&, bool)> load_function) {

  if (!bootstrap_version.has_value()) {
    load_function(bootstrap, true);
  } else if (*bootstrap_version == 3) {
    load_function(bootstrap, false);
  } else if (*bootstrap_version == 2) {
    envoy::config::bootstrap::v2::Bootstrap bootstrap_v2;
    load_function(bootstrap_v2, false);
    Config::VersionConverter::upgrade(bootstrap_v2, bootstrap);
    MessageUtil::onVersionUpgradeWarn("v2 bootstrap");
  } else {
    throw EnvoyException(fmt::format("Unknown bootstrap version {}.", *bootstrap_version));
  }
}
} // namespace

void InstanceUtil::loadBootstrapConfig(envoy::config::bootstrap::v3::Bootstrap& bootstrap,
                                       const Options& options,
                                       ProtobufMessage::ValidationVisitor& validation_visitor,
                                       Api::Api& api) {
  const std::string& config_path = options.configPath();
  const std::string& config_yaml = options.configYaml();
  const envoy::config::bootstrap::v3::Bootstrap& config_proto = options.configProto();

  // Exactly one of config_path and config_yaml should be specified.
  if (config_path.empty() && config_yaml.empty() && config_proto.ByteSize() == 0) {
    throw EnvoyException("At least one of --config-path or --config-yaml or Options::configProto() "
                         "should be non-empty");
  }

  if (!config_path.empty()) {
    loadBootstrap(
        options.bootstrapVersion(), bootstrap,
        [&config_path, &validation_visitor, &api](Protobuf::Message& message, bool do_boosting) {
          MessageUtil::loadFromFile(config_path, message, validation_visitor, api, do_boosting);
        });
  }
  if (!config_yaml.empty()) {
    envoy::config::bootstrap::v3::Bootstrap bootstrap_override;
    loadBootstrap(
        options.bootstrapVersion(), bootstrap_override,
        [&config_yaml, &validation_visitor](Protobuf::Message& message, bool do_boosting) {
          MessageUtil::loadFromYaml(config_yaml, message, validation_visitor, do_boosting);
        });
    // TODO(snowp): The fact that we do a merge here doesn't seem to be covered under test.
    bootstrap.MergeFrom(bootstrap_override);
  }
  if (config_proto.ByteSize() != 0) {
    bootstrap.MergeFrom(config_proto);
  }
  MessageUtil::validate(bootstrap, validation_visitor);
}

void InstanceImpl::initialize(const Options& options,
                              Network::Address::InstanceConstSharedPtr local_address,
                              ComponentFactory& component_factory, ListenerHooks& hooks) {
  ENVOY_LOG(info, "initializing epoch {} (base id={}, hot restart version={})",
            options.restartEpoch(), restarter_.baseId(), restarter_.version());

  ENVOY_LOG(info, "statically linked extensions:");
  for (const auto& ext : Envoy::Registry::FactoryCategoryRegistry::registeredFactories()) {
    ENVOY_LOG(info, "  {}: {}", ext.first, absl::StrJoin(ext.second->registeredNames(), ", "));
  }

  // Handle configuration that needs to take place prior to the main configuration load.
  InstanceUtil::loadBootstrapConfig(bootstrap_, options,
                                    messageValidationContext().staticValidationVisitor(), *api_);
  bootstrap_config_update_time_ = time_source_.systemTime();

  // Immediate after the bootstrap has been loaded, override the header prefix, if configured to
  // do so. This must be set before any other code block references the HeaderValues ConstSingleton.
  if (!bootstrap_.header_prefix().empty()) {
    // setPrefix has a release assert verifying that setPrefix() is not called after prefix()
    ThreadSafeSingleton<Http::PrefixValue>::get().setPrefix(bootstrap_.header_prefix().c_str());
  }
  // TODO(mattklein123): Custom O(1) headers can be registered at this point for creating/finalizing
  // any header maps.
  ENVOY_LOG(info, "HTTP header map info:");
  for (const auto& info : Http::HeaderMapImplUtility::getAllHeaderMapImplInfo()) {
    ENVOY_LOG(info, "  {}: {} bytes: {}", info.name_, info.size_,
              absl::StrJoin(info.registered_headers_, ","));
  }

  // Needs to happen as early as possible in the instantiation to preempt the objects that require
  // stats.
  stats_store_.setTagProducer(Config::Utility::createTagProducer(bootstrap_));
  stats_store_.setStatsMatcher(Config::Utility::createStatsMatcher(bootstrap_));
  stats_store_.setHistogramSettings(Config::Utility::createHistogramSettings(bootstrap_));

  const std::string server_stats_prefix = "server.";
  server_stats_ = std::make_unique<ServerStats>(
      ServerStats{ALL_SERVER_STATS(POOL_COUNTER_PREFIX(stats_store_, server_stats_prefix),
                                   POOL_GAUGE_PREFIX(stats_store_, server_stats_prefix),
                                   POOL_HISTOGRAM_PREFIX(stats_store_, server_stats_prefix))});
  validation_context_.staticWarningValidationVisitor().setUnknownCounter(
      server_stats_->static_unknown_fields_);
  validation_context_.dynamicWarningValidationVisitor().setUnknownCounter(
      server_stats_->dynamic_unknown_fields_);

  initialization_timer_ = std::make_unique<Stats::HistogramCompletableTimespanImpl>(
      server_stats_->initialization_time_ms_, timeSource());
  server_stats_->concurrency_.set(options_.concurrency());
  server_stats_->hot_restart_epoch_.set(options_.restartEpoch());

  assert_action_registration_ = Assert::setDebugAssertionFailureRecordAction(
      [this]() { server_stats_->debug_assertion_failures_.inc(); });
  envoy_bug_action_registration_ = Assert::setEnvoyBugFailureRecordAction(
      [this]() { server_stats_->envoy_bug_failures_.inc(); });

  InstanceImpl::failHealthcheck(false);

  // Check if bootstrap has server version override set, if yes, we should use that as
  // 'server.version' stat.
  uint64_t version_int;
  if (bootstrap_.stats_server_version_override().value() > 0) {
    version_int = bootstrap_.stats_server_version_override().value();
  } else {
    if (!StringUtil::atoull(VersionInfo::revision().substr(0, 6).c_str(), version_int, 16)) {
      throw EnvoyException("compiled GIT SHA is invalid. Invalid build.");
    }
  }
  server_stats_->version_.set(version_int);

  bootstrap_.mutable_node()->set_hidden_envoy_deprecated_build_version(VersionInfo::version());
  bootstrap_.mutable_node()->set_user_agent_name("envoy");
  *bootstrap_.mutable_node()->mutable_user_agent_build_version() = VersionInfo::buildVersion();
  for (const auto& ext : Envoy::Registry::FactoryCategoryRegistry::registeredFactories()) {
    for (const auto& name : ext.second->allRegisteredNames()) {
      auto* extension = bootstrap_.mutable_node()->add_extensions();
      extension->set_name(std::string(name));
      extension->set_category(ext.first);
      auto const version = ext.second->getFactoryVersion(name);
      if (version) {
        *extension->mutable_version() = version.value();
      }
      extension->set_disabled(ext.second->isFactoryDisabled(name));
    }
  }

  local_info_ = std::make_unique<LocalInfo::LocalInfoImpl>(
      bootstrap_.node(), local_address, options.serviceZone(), options.serviceClusterName(),
      options.serviceNodeName());

  Configuration::InitialImpl initial_config(bootstrap_);

  // Learn original_start_time_ if our parent is still around to inform us of it.
  restarter_.sendParentAdminShutdownRequest(original_start_time_);
  admin_ = std::make_unique<AdminImpl>(initial_config.admin().profilePath(), *this);

  loadServerFlags(initial_config.flagsPath());

  secret_manager_ = std::make_unique<Secret::SecretManagerImpl>(admin_->getConfigTracker());

  // Initialize the overload manager early so other modules can register for actions.
  overload_manager_ = std::make_unique<OverloadManagerImpl>(
      *dispatcher_, stats_store_, thread_local_, bootstrap_.overload_manager(),
      messageValidationContext().staticValidationVisitor(), *api_);

  heap_shrinker_ =
      std::make_unique<Memory::HeapShrinker>(*dispatcher_, *overload_manager_, stats_store_);

  for (const auto& bootstrap_extension : bootstrap_.bootstrap_extensions()) {
    auto& factory = Config::Utility::getAndCheckFactory<Configuration::BootstrapExtensionFactory>(
        bootstrap_extension);
    auto config = Config::Utility::translateAnyToFactoryConfig(
        bootstrap_extension.typed_config(), messageValidationContext().staticValidationVisitor(),
        factory);
    bootstrap_extensions_.push_back(
        factory.createBootstrapExtension(*config, serverFactoryContext()));
  }

  if (!bootstrap_.default_socket_interface().empty()) {
    auto& sock_name = bootstrap_.default_socket_interface();
    auto sock = const_cast<Network::SocketInterface*>(Network::socketInterface(sock_name));
    if (sock != nullptr) {
      Network::SocketInterfaceSingleton::clear();
      Network::SocketInterfaceSingleton::initialize(sock);
    }
  }

  // Workers get created first so they register for thread local updates.
  listener_manager_ = std::make_unique<ListenerManagerImpl>(
      *this, listener_component_factory_, worker_factory_, bootstrap_.enable_dispatcher_stats());

  // The main thread is also registered for thread local updates so that code that does not care
  // whether it runs on the main thread or on workers can still use TLS.
  thread_local_.registerThread(*dispatcher_, true);

  // We can now initialize stats for threading.
  stats_store_.initializeThreading(*dispatcher_, thread_local_);

  // It's now safe to start writing stats from the main thread's dispatcher.
  if (bootstrap_.enable_dispatcher_stats()) {
    dispatcher_->initializeStats(stats_store_, "server.");
  }

  if (initial_config.admin().address()) {
    if (initial_config.admin().accessLogPath().empty()) {
      throw EnvoyException("An admin access log path is required for a listening server.");
    }
    ENVOY_LOG(info, "admin address: {}", initial_config.admin().address()->asString());
    admin_->startHttpListener(initial_config.admin().accessLogPath(), options.adminAddressPath(),
                              initial_config.admin().address(),
                              initial_config.admin().socketOptions(),
                              stats_store_.createScope("listener.admin."));
  } else {
    ENVOY_LOG(warn, "No admin address given, so no admin HTTP server started.");
  }
  config_tracker_entry_ =
      admin_->getConfigTracker().add("bootstrap", [this] { return dumpBootstrapConfig(); });
  if (initial_config.admin().address()) {
    admin_->addListenerToHandler(handler_.get());
  }

  // The broad order of initialization from this point on is the following:
  // 1. Statically provisioned configuration (bootstrap) are loaded.
  // 2. Cluster manager is created and all primary clusters (i.e. with endpoint assignments
  //    provisioned statically in bootstrap, discovered through DNS or file based CDS) are
  //    initialized.
  // 3. Various services are initialized and configured using the bootstrap config.
  // 4. RTDS is initialized using primary clusters. This  allows runtime overrides to be fully
  //    configured before the rest of xDS configuration is provisioned.
  // 5. Secondary clusters (with endpoint assignments provisioned by xDS servers) are initialized.
  // 6. The rest of the dynamic configuration is provisioned.
  //
  // Please note: this order requires that RTDS is provisioned using a primary cluster. If RTDS is
  // provisioned through ADS then ADS must use primary cluster as well. This invariant is enforced
  // during RTDS initialization and invalid configuration will be rejected.

  // Runtime gets initialized before the main configuration since during main configuration
  // load things may grab a reference to the loader for later use.
  runtime_singleton_ = std::make_unique<Runtime::ScopedLoaderSingleton>(
      component_factory.createRuntime(*this, initial_config));
  hooks.onRuntimeCreated();

  // Once we have runtime we can initialize the SSL context manager.
  ssl_context_manager_ = createContextManager("ssl_context_manager", time_source_);

  const bool use_tcp_for_dns_lookups = bootstrap_.use_tcp_for_dns_lookups();
  dns_resolver_ = dispatcher_->createDnsResolver({}, use_tcp_for_dns_lookups);

  cluster_manager_factory_ = std::make_unique<Upstream::ProdClusterManagerFactory>(
      *admin_, Runtime::LoaderSingleton::get(), stats_store_, thread_local_, dns_resolver_,
      *ssl_context_manager_, *dispatcher_, *local_info_, *secret_manager_,
      messageValidationContext(), *api_, http_context_, grpc_context_, access_log_manager_,
      *singleton_manager_);

  // Now the configuration gets parsed. The configuration may start setting
  // thread local data per above. See MainImpl::initialize() for why ConfigImpl
  // is constructed as part of the InstanceImpl and then populated once
  // cluster_manager_factory_ is available.
  config_.initialize(bootstrap_, *this, *cluster_manager_factory_);

  // Instruct the listener manager to create the LDS provider if needed. This must be done later
  // because various items do not yet exist when the listener manager is created.
  if (bootstrap_.dynamic_resources().has_lds_config() ||
      bootstrap_.dynamic_resources().has_lds_resources_locator()) {
    listener_manager_->createLdsApi(bootstrap_.dynamic_resources().lds_config(),
                                    bootstrap_.dynamic_resources().has_lds_resources_locator()
                                        ? &bootstrap_.dynamic_resources().lds_resources_locator()
                                        : nullptr);
  }

  // We have to defer RTDS initialization until after the cluster manager is
  // instantiated (which in turn relies on runtime...).
  Runtime::LoaderSingleton::get().initialize(clusterManager());

  clusterManager().setPrimaryClustersInitializedCb(
      [this]() { onClusterManagerPrimaryInitializationComplete(); });

  for (Stats::SinkPtr& sink : config_.statsSinks()) {
    stats_store_.addSink(*sink);
  }

  // Some of the stat sinks may need dispatcher support so don't flush until the main loop starts.
  // Just setup the timer.
  stat_flush_timer_ = dispatcher_->createTimer([this]() -> void { flushStats(); });
  stat_flush_timer_->enableTimer(config_.statsFlushInterval());

  // GuardDog (deadlock detection) object and thread setup before workers are
  // started and before our own run() loop runs.
  main_thread_guard_dog_ = std::make_unique<Server::GuardDogImpl>(
      stats_store_, config_.mainThreadWatchdogConfig(), *api_, "main_thread");
  worker_guard_dog_ = std::make_unique<Server::GuardDogImpl>(
      stats_store_, config_.workerWatchdogConfig(), *api_, "workers");
}

void InstanceImpl::onClusterManagerPrimaryInitializationComplete() {
  // If RTDS was not configured the `onRuntimeReady` callback is immediately invoked.
  Runtime::LoaderSingleton::get().startRtdsSubscriptions([this]() { onRuntimeReady(); });
}

void InstanceImpl::onRuntimeReady() {
  // Begin initializing secondary clusters after RTDS configuration has been applied.
  clusterManager().initializeSecondaryClusters(bootstrap_);

  if (bootstrap_.has_hds_config()) {
    const auto& hds_config = bootstrap_.hds_config();
    async_client_manager_ = std::make_unique<Grpc::AsyncClientManagerImpl>(
        *config_.clusterManager(), thread_local_, time_source_, *api_, grpc_context_.statNames());
    hds_delegate_ = std::make_unique<Upstream::HdsDelegate>(
        stats_store_,
        Config::Utility::factoryForGrpcApiConfigSource(*async_client_manager_, hds_config,
                                                       stats_store_, false)
            ->create(),
        hds_config.transport_api_version(), *dispatcher_, Runtime::LoaderSingleton::get(),
        stats_store_, *ssl_context_manager_, info_factory_, access_log_manager_,
        *config_.clusterManager(), *local_info_, *admin_, *singleton_manager_, thread_local_,
        messageValidationContext().dynamicValidationVisitor(), *api_);
  }

  // If there is no global limit to the number of active connections, warn on startup.
  // TODO (tonya11en): Move this functionality into the overload manager.
  if (!runtime().snapshot().get(Network::TcpListenerImpl::GlobalMaxCxRuntimeKey)) {
    ENVOY_LOG(warn,
              "there is no configured limit to the number of allowed active connections. Set a "
              "limit via the runtime key {}",
              Network::TcpListenerImpl::GlobalMaxCxRuntimeKey);
  }
}

void InstanceImpl::startWorkers() {
  listener_manager_->startWorkers(*worker_guard_dog_);
  initialization_timer_->complete();
  // Update server stats as soon as initialization is done.
  updateServerStats();
  workers_started_ = true;
  // At this point we are ready to take traffic and all listening ports are up. Notify our parent
  // if applicable that they can stop listening and drain.
  restarter_.drainParentListeners();
  drain_manager_->startParentShutdownSequence();
}

Runtime::LoaderPtr InstanceUtil::createRuntime(Instance& server,
                                               Server::Configuration::Initial& config) {
  ENVOY_LOG(info, "runtime: {}", MessageUtil::getYamlStringFromMessage(config.runtime()));
  return std::make_unique<Runtime::LoaderImpl>(
      server.dispatcher(), server.threadLocal(), config.runtime(), server.localInfo(),
      server.stats(), server.api().randomGenerator(),
      server.messageValidationContext().dynamicValidationVisitor(), server.api());
}

void InstanceImpl::loadServerFlags(const absl::optional<std::string>& flags_path) {
  if (!flags_path) {
    return;
  }

  ENVOY_LOG(info, "server flags path: {}", flags_path.value());
  if (api_->fileSystem().fileExists(flags_path.value() + "/drain")) {
    ENVOY_LOG(info, "starting server in drain mode");
    InstanceImpl::failHealthcheck(true);
  }
}

RunHelper::RunHelper(Instance& instance, const Options& options, Event::Dispatcher& dispatcher,
                     Upstream::ClusterManager& cm, AccessLog::AccessLogManager& access_log_manager,
                     Init::Manager& init_manager, OverloadManager& overload_manager,
                     std::function<void()> post_init_cb)
    : init_watcher_("RunHelper", [&instance, post_init_cb]() {
        if (!instance.isShutdown()) {
          post_init_cb();
        }
      }) {
  // Setup signals.
  if (options.signalHandlingEnabled()) {
// TODO(Pivotal): Figure out solution to graceful shutdown on Windows. None of these signals exist
// on Windows.
#ifndef WIN32
    sigterm_ = dispatcher.listenForSignal(SIGTERM, [&instance]() {
      ENVOY_LOG(warn, "caught SIGTERM");
      instance.shutdown();
    });

    sigint_ = dispatcher.listenForSignal(SIGINT, [&instance]() {
      ENVOY_LOG(warn, "caught SIGINT");
      instance.shutdown();
    });

    sig_usr_1_ = dispatcher.listenForSignal(SIGUSR1, [&access_log_manager]() {
      ENVOY_LOG(info, "caught SIGUSR1. Reopening access logs.");
      access_log_manager.reopen();
    });

    sig_hup_ = dispatcher.listenForSignal(SIGHUP, []() {
      ENVOY_LOG(warn, "caught and eating SIGHUP. See documentation for how to hot restart.");
    });
#endif
  }

  // Start overload manager before workers.
  overload_manager.start();

  // Register for cluster manager init notification. We don't start serving worker traffic until
  // upstream clusters are initialized which may involve running the event loop. Note however that
  // this can fire immediately if all clusters have already initialized. Also note that we need
  // to guard against shutdown at two different levels since SIGTERM can come in once the run loop
  // starts.
  cm.setInitializedCb([&instance, &init_manager, &cm, this]() {
    if (instance.isShutdown()) {
      return;
    }

    const auto type_urls =
        Config::getAllVersionTypeUrls<envoy::config::route::v3::RouteConfiguration>();
    // Pause RDS to ensure that we don't send any requests until we've
    // subscribed to all the RDS resources. The subscriptions happen in the init callbacks,
    // so we pause RDS until we've completed all the callbacks.
    Config::ScopedResume maybe_resume_rds;
    if (cm.adsMux()) {
      maybe_resume_rds = cm.adsMux()->pause(type_urls);
    }

    ENVOY_LOG(info, "all clusters initialized. initializing init manager");
    init_manager.initialize(init_watcher_);

    // Now that we're execute all the init callbacks we can resume RDS
    // as we've subscribed to all the statically defined RDS resources.
    // This is done by tearing down the maybe_resume_rds Cleanup object.
  });
}

void InstanceImpl::run() {
  // RunHelper exists primarily to facilitate testing of how we respond to early shutdown during
  // startup (see RunHelperTest in server_test.cc).
  const auto run_helper = RunHelper(*this, options_, *dispatcher_, clusterManager(),
                                    access_log_manager_, init_manager_, overloadManager(), [this] {
                                      notifyCallbacksForStage(Stage::PostInit);
                                      startWorkers();
                                    });

  // Run the main dispatch loop waiting to exit.
  ENVOY_LOG(info, "starting main dispatch loop");
  auto watchdog = main_thread_guard_dog_->createWatchDog(api_->threadFactory().currentThreadId(),
                                                         "main_thread");
  watchdog->startWatchdog(*dispatcher_);
  dispatcher_->post([this] { notifyCallbacksForStage(Stage::Startup); });
  dispatcher_->run(Event::Dispatcher::RunType::Block);
  ENVOY_LOG(info, "main dispatch loop exited");
  main_thread_guard_dog_->stopWatching(watchdog);
  watchdog.reset();

  terminate();
}

void InstanceImpl::terminate() {
  if (terminated_) {
    return;
  }
  terminated_ = true;

  // Before starting to shutdown anything else, stop slot destruction updates.
  thread_local_.shutdownGlobalThreading();

  // Before the workers start exiting we should disable stat threading.
  stats_store_.shutdownThreading();

  if (overload_manager_) {
    overload_manager_->stop();
  }

  // Shutdown all the workers now that the main dispatch loop is done.
  if (listener_manager_ != nullptr) {
    // Also shutdown the listener manager's ApiListener, if there is one, which runs on the main
    // thread. This needs to happen ahead of calling thread_local_.shutdown() below to prevent any
    // objects in the ApiListener destructor to reference any objects in thread local storage.
    if (listener_manager_->apiListener().has_value()) {
      listener_manager_->apiListener()->get().shutdown();
    }

    listener_manager_->stopWorkers();
  }

  // Only flush if we have not been hot restarted.
  if (stat_flush_timer_) {
    flushStats();
  }

  if (config_.clusterManager() != nullptr) {
    config_.clusterManager()->shutdown();
  }
  handler_.reset();
  thread_local_.shutdownThread();
  restarter_.shutdown();
  ENVOY_LOG(info, "exiting");
  ENVOY_FLUSH_LOG();
}

Runtime::Loader& InstanceImpl::runtime() { return Runtime::LoaderSingleton::get(); }

void InstanceImpl::shutdown() {
  ENVOY_LOG(info, "shutting down server instance");
  shutdown_ = true;
  restarter_.sendParentTerminateRequest();
  notifyCallbacksForStage(Stage::ShutdownExit, [this] { dispatcher_->exit(); });
}

void InstanceImpl::shutdownAdmin() {
  ENVOY_LOG(warn, "shutting down admin due to child startup");
  stat_flush_timer_.reset();
  handler_->stopListeners();
  admin_->closeSocket();

  // If we still have a parent, it should be terminated now that we have a child.
  ENVOY_LOG(warn, "terminating parent process");
  restarter_.sendParentTerminateRequest();
}

ServerLifecycleNotifier::HandlePtr InstanceImpl::registerCallback(Stage stage,
                                                                  StageCallback callback) {
  auto& callbacks = stage_callbacks_[stage];
  return std::make_unique<LifecycleCallbackHandle<StageCallback>>(callbacks, callback);
}

ServerLifecycleNotifier::HandlePtr
InstanceImpl::registerCallback(Stage stage, StageCallbackWithCompletion callback) {
  ASSERT(stage == Stage::ShutdownExit);
  auto& callbacks = stage_completable_callbacks_[stage];
  return std::make_unique<LifecycleCallbackHandle<StageCallbackWithCompletion>>(callbacks,
                                                                                callback);
}

void InstanceImpl::notifyCallbacksForStage(Stage stage, Event::PostCb completion_cb) {
  ASSERT(std::this_thread::get_id() == main_thread_id_);
  const auto it = stage_callbacks_.find(stage);
  if (it != stage_callbacks_.end()) {
    for (const StageCallback& callback : it->second) {
      callback();
    }
  }

  // Wrap completion_cb so that it only gets invoked when all callbacks for this stage
  // have finished their work.
  std::shared_ptr<void> cb_guard(
      new Cleanup([this, completion_cb]() { dispatcher_->post(completion_cb); }));

  // Registrations which take a completion callback are typically implemented by executing a
  // callback on all worker threads using Slot::runOnAllThreads which will hang indefinitely if
  // worker threads have not been started so we need to skip notifications if envoy is shutdown
  // early before workers have started.
  if (workers_started_) {
    const auto it2 = stage_completable_callbacks_.find(stage);
    if (it2 != stage_completable_callbacks_.end()) {
      ENVOY_LOG(info, "Notifying {} callback(s) with completion.", it2->second.size());
      for (const StageCallbackWithCompletion& callback : it2->second) {
        callback([cb_guard] {});
      }
    }
  }
}

ProtobufTypes::MessagePtr InstanceImpl::dumpBootstrapConfig() {
  auto config_dump = std::make_unique<envoy::admin::v3::BootstrapConfigDump>();
  config_dump->mutable_bootstrap()->MergeFrom(bootstrap_);
  TimestampUtil::systemClockToTimestamp(bootstrap_config_update_time_,
                                        *(config_dump->mutable_last_updated()));
  return config_dump;
}

} // namespace Server
} // namespace Envoy
