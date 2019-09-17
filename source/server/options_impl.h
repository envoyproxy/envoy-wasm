#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/config/bootstrap/v2/bootstrap.pb.h"
#include "envoy/server/options.h"

#include "common/common/logger.h"

#include "spdlog/spdlog.h"

namespace Envoy {
/**
 * Implementation of Server::Options.
 */
class OptionsImpl : public Server::Options, protected Logger::Loggable<Logger::Id::config> {
public:
  /**
   * Parameters are max_stat_name_len, hot_restart_enabled
   */
  using HotRestartVersionCb = std::function<std::string(bool)>;

  /**
   * @throw NoServingException if Envoy has already done everything specified by the argv (e.g.
   *        print the hot restart version) and it's time to exit without serving HTTP traffic. The
   *        caller should exit(0) after any necessary cleanup.
   * @throw MalformedArgvException if something is wrong with the arguments (invalid flag or flag
   *        value). The caller should call exit(1) after any necessary cleanup.
   */
  OptionsImpl(int argc, const char* const* argv, const HotRestartVersionCb& hot_restart_version_cb,
              spdlog::level::level_enum default_log_level);

  // Test constructor; creates "reasonable" defaults, but desired values should be set explicitly.
  OptionsImpl(const std::string& service_cluster, const std::string& service_node,
              const std::string& service_zone, spdlog::level::level_enum log_level);

  // Setters for option fields. These are not part of the Options interface.
  void setBaseId(uint64_t base_id) { base_id_ = base_id; };
  void setConcurrency(uint32_t concurrency) { concurrency_ = concurrency; }
  void setConfigPath(const std::string& config_path) { config_path_ = config_path; }
  void setConfigProto(const envoy::config::bootstrap::v2::Bootstrap& config_proto) {
    config_proto_ = config_proto;
  }
  void setConfigYaml(const std::string& config_yaml) { config_yaml_ = config_yaml; }
  void setAdminAddressPath(const std::string& admin_address_path) {
    admin_address_path_ = admin_address_path;
  }
  void setLocalAddressIpVersion(Network::Address::IpVersion local_address_ip_version) {
    local_address_ip_version_ = local_address_ip_version;
  }
  void setDrainTime(std::chrono::seconds drain_time) { drain_time_ = drain_time; }
  void setLogLevel(spdlog::level::level_enum log_level) { log_level_ = log_level; }
  void setLogFormat(const std::string& log_format) { log_format_ = log_format; }
  void setLogPath(const std::string& log_path) { log_path_ = log_path; }
  void setParentShutdownTime(std::chrono::seconds parent_shutdown_time) {
    parent_shutdown_time_ = parent_shutdown_time;
  }
  void setRestartEpoch(uint64_t restart_epoch) { restart_epoch_ = restart_epoch; }
  void setMode(Server::Mode mode) { mode_ = mode; }
  void setFileFlushIntervalMsec(std::chrono::milliseconds file_flush_interval_msec) {
    file_flush_interval_msec_ = file_flush_interval_msec;
  }
  void setServiceClusterName(const std::string& service_cluster) {
    service_cluster_ = service_cluster;
  }
  void setServiceNodeName(const std::string& service_node) { service_node_ = service_node; }
  void setServiceZone(const std::string& service_zone) { service_zone_ = service_zone; }
  void setHotRestartDisabled(bool hot_restart_disabled) {
    hot_restart_disabled_ = hot_restart_disabled;
  }
  void setSignalHandling(bool signal_handling_enabled) {
    signal_handling_enabled_ = signal_handling_enabled;
  }
  void setCpusetThreads(bool cpuset_threads_enabled) { cpuset_threads_ = cpuset_threads_enabled; }
  void setAllowUnkownFields(bool allow_unknown_static_fields) {
    allow_unknown_static_fields_ = allow_unknown_static_fields;
  }
  void setRejectUnknownFieldsDynamic(bool reject_unknown_dynamic_fields) {
    reject_unknown_dynamic_fields_ = reject_unknown_dynamic_fields;
  }
  void setFakeSymbolTableEnabled(bool fake_symbol_table_enabled) {
    fake_symbol_table_enabled_ = fake_symbol_table_enabled;
  }

  // Server::Options
  uint64_t baseId() const override { return base_id_; }
  uint32_t concurrency() const override { return concurrency_; }
  const std::string& configPath() const override { return config_path_; }
  const envoy::config::bootstrap::v2::Bootstrap& configProto() const override {
    return config_proto_;
  }
  const std::string& configYaml() const override { return config_yaml_; }
  bool allowUnknownStaticFields() const override { return allow_unknown_static_fields_; }
  bool rejectUnknownDynamicFields() const override { return reject_unknown_dynamic_fields_; }
  const std::string& adminAddressPath() const override { return admin_address_path_; }
  Network::Address::IpVersion localAddressIpVersion() const override {
    return local_address_ip_version_;
  }
  std::chrono::seconds drainTime() const override { return drain_time_; }
  spdlog::level::level_enum logLevel() const override { return log_level_; }
  const std::vector<std::pair<std::string, spdlog::level::level_enum>>&
  componentLogLevels() const override {
    return component_log_levels_;
  }
  const std::string& logFormat() const override { return log_format_; }
  const std::string& logPath() const override { return log_path_; }
  std::chrono::seconds parentShutdownTime() const override { return parent_shutdown_time_; }
  uint64_t restartEpoch() const override { return restart_epoch_; }
  Server::Mode mode() const override { return mode_; }
  std::chrono::milliseconds fileFlushIntervalMsec() const override {
    return file_flush_interval_msec_;
  }
  const std::string& serviceClusterName() const override { return service_cluster_; }
  const std::string& serviceNodeName() const override { return service_node_; }
  const std::string& serviceZone() const override { return service_zone_; }
  bool hotRestartDisabled() const override { return hot_restart_disabled_; }
  bool signalHandlingEnabled() const override { return signal_handling_enabled_; }
  bool mutexTracingEnabled() const override { return mutex_tracing_enabled_; }
  bool libeventBufferEnabled() const override { return libevent_buffer_enabled_; }
  bool fakeSymbolTableEnabled() const override { return fake_symbol_table_enabled_; }
  Server::CommandLineOptionsPtr toCommandLineOptions() const override;
  void parseComponentLogLevels(const std::string& component_log_levels);
  bool cpusetThreadsEnabled() const override { return cpuset_threads_; }
  uint32_t count() const;

private:
  void logError(const std::string& error) const;

  uint64_t base_id_;
  uint32_t concurrency_;
  std::string config_path_;
  envoy::config::bootstrap::v2::Bootstrap config_proto_;
  std::string config_yaml_;
  bool allow_unknown_static_fields_{false};
  bool reject_unknown_dynamic_fields_{false};
  std::string admin_address_path_;
  Network::Address::IpVersion local_address_ip_version_;
  spdlog::level::level_enum log_level_;
  std::vector<std::pair<std::string, spdlog::level::level_enum>> component_log_levels_;
  std::string component_log_level_str_;
  std::string log_format_;
  std::string log_path_;
  uint64_t restart_epoch_;
  std::string service_cluster_;
  std::string service_node_;
  std::string service_zone_;
  std::chrono::milliseconds file_flush_interval_msec_;
  std::chrono::seconds drain_time_;
  std::chrono::seconds parent_shutdown_time_;
  Server::Mode mode_;
  bool hot_restart_disabled_;
  bool signal_handling_enabled_;
  bool mutex_tracing_enabled_;
  bool cpuset_threads_;
  bool libevent_buffer_enabled_;
  bool fake_symbol_table_enabled_;
  uint32_t count_;
};

/**
 * Thrown when an OptionsImpl was not constructed because all of Envoy's work is done (for example,
 * it was started with --help and it's already printed a help message) so all that's left to do is
 * exit successfully.
 */
class NoServingException : public EnvoyException {
public:
  NoServingException() : EnvoyException("NoServingException") {}
};

/**
 * Thrown when an OptionsImpl was not constructed because the argv was invalid.
 */
class MalformedArgvException : public EnvoyException {
public:
  MalformedArgvException(const std::string& what) : EnvoyException(what) {}
};

} // namespace Envoy
