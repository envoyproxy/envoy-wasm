#include <iostream>
#include <unordered_map>

#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/config/listener/v3/listener_components.pb.h"
#include "envoy/network/connection.h"
#include "envoy/network/listen_socket.h"
#include "envoy/protobuf/message_validator.h"

#include "server/filter_chain_manager_impl.h"

#include "extensions/transport_sockets/well_known_names.h"

#include "test/mocks/network/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Server {

namespace {
class MockFilterChainFactoryBuilder : public FilterChainFactoryBuilder {
  std::shared_ptr<Network::DrainableFilterChain>
  buildFilterChain(const envoy::config::listener::v3::FilterChain&,
                   FilterChainFactoryContextCreator&) const override {
    // A place holder to be found
    return std::make_shared<Network::MockFilterChain>();
  }
};

class MockConnectionSocket : public Network::ConnectionSocket {
public:
  MockConnectionSocket() = default;
  static std::unique_ptr<MockConnectionSocket>
  createMockConnectionSocket(uint16_t destination_port, const std::string& destination_address,
                             const std::string& server_name, const std::string& transport_protocol,
                             const std::vector<std::string>& application_protocols,
                             const std::string& source_address, uint16_t source_port) {
    auto res = std::make_unique<MockConnectionSocket>();

    if (absl::StartsWith(destination_address, "/")) {
      res->local_address_ = std::make_shared<Network::Address::PipeInstance>(destination_address);
    } else {
      res->local_address_ =
          Network::Utility::parseInternetAddress(destination_address, destination_port);
    }
    if (absl::StartsWith(source_address, "/")) {
      res->remote_address_ = std::make_shared<Network::Address::PipeInstance>(source_address);
    } else {
      res->remote_address_ = Network::Utility::parseInternetAddress(source_address, source_port);
    }
    res->server_name_ = server_name;
    res->transport_protocol_ = transport_protocol;
    res->application_protocols_ = application_protocols;
    return res;
  }

  const Network::Address::InstanceConstSharedPtr& remoteAddress() const override {
    return remote_address_;
  }

  const Network::Address::InstanceConstSharedPtr& directRemoteAddress() const override {
    return remote_address_;
  }

  const Network::Address::InstanceConstSharedPtr& localAddress() const override {
    return local_address_;
  }

  absl::string_view detectedTransportProtocol() const override { return transport_protocol_; }

  absl::string_view requestedServerName() const override { return server_name_; }
  const std::vector<std::string>& requestedApplicationProtocols() const override {
    return application_protocols_;
  }

  // Wont call
  Network::IoHandle& ioHandle() override { return *io_handle_; }
  const Network::IoHandle& ioHandle() const override { return *io_handle_; }

  // Dummy method
  void close() override {}
  bool isOpen() const override { return false; }
  Network::Address::SocketType socketType() const override {
    return Network::Address::SocketType::Stream;
  }
  Network::Address::Type addressType() const override { return local_address_->type(); }
  void setLocalAddress(const Network::Address::InstanceConstSharedPtr&) override {}
  void restoreLocalAddress(const Network::Address::InstanceConstSharedPtr&) override {}
  void setRemoteAddress(const Network::Address::InstanceConstSharedPtr&) override {}
  bool localAddressRestored() const override { return true; }
  void setDetectedTransportProtocol(absl::string_view) override {}
  void setRequestedApplicationProtocols(const std::vector<absl::string_view>&) override {}
  void addOption(const OptionConstSharedPtr&) override {}
  void addOptions(const OptionsSharedPtr&) override {}
  const OptionsSharedPtr& options() const override { return options_; }
  void setRequestedServerName(absl::string_view) override {}
  Api::SysCallIntResult bind(Network::Address::InstanceConstSharedPtr) override { return {0, 0}; }
  Api::SysCallIntResult listen(int) override { return {0, 0}; }
  Api::SysCallIntResult connect(const Network::Address::InstanceConstSharedPtr) override {
    return {0, 0};
  }
  Api::SysCallIntResult setSocketOption(int, int, const void*, socklen_t) override {
    return {0, 0};
  }
  Api::SysCallIntResult getSocketOption(int, int, void*, socklen_t*) override { return {0, 0}; }
  Api::SysCallIntResult setBlockingForTest(bool) override { return {0, 0}; }

private:
  Network::IoHandlePtr io_handle_;
  OptionsSharedPtr options_;
  Network::Address::InstanceConstSharedPtr local_address_;
  Network::Address::InstanceConstSharedPtr remote_address_;
  std::string server_name_;
  std::string transport_protocol_;
  std::vector<std::string> application_protocols_;
};
const char YamlHeader[] = R"EOF(
    address:
      socket_address: { address: 127.0.0.1, port_value: 1234 }
    listener_filters:
    - name: "envoy.filters.listener.tls_inspector"
      config: {}
    filter_chains:
    - filter_chain_match:
        # empty
      transport_socket:
        name: tls
        typed_config:
          "@type": type.googleapis.com/envoy.api.v2.auth.DownstreamTlsContext
          common_tls_context:
            tls_certificates:
              - certificate_chain: { filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_cert.pem" }
                private_key: { filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_uri_key.pem" }
          session_ticket_keys:
            keys:
            - filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a")EOF";
const char YamlSingleServer[] = R"EOF(
    - filter_chain_match:
        server_names: "server1.example.com"
        transport_protocol: "tls"
      transport_socket:
        name: tls
        typed_config:
          "@type": type.googleapis.com/envoy.api.v2.auth.DownstreamTlsContext
          common_tls_context:
            tls_certificates:
              - certificate_chain: { filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_cert.pem" }
                private_key: { filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_dns_key.pem" }
          session_ticket_keys:
            keys:
            - filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a")EOF";
const char YamlSingleDstPortTop[] = R"EOF(
    - filter_chain_match:
        destination_port: )EOF";
const char YamlSingleDstPortBottom[] = R"EOF(
      transport_socket:
        name: tls
        typed_config:
          "@type": type.googleapis.com/envoy.api.v2.auth.DownstreamTlsContext
          common_tls_context:
            tls_certificates:
              - certificate_chain: { filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_multiple_dns_cert.pem" }
                private_key: { filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/san_multiple_dns_key.pem" }
          session_ticket_keys:
            keys:
            - filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ticket_key_a")EOF";
} // namespace

class FilterChainBenchmarkFixture : public benchmark::Fixture {
public:
  using benchmark::Fixture::SetUp;

  void SetUp(::benchmark::State& state) override {
    int64_t input_size = state.range(0);
    std::vector<std::string> port_chains;
    port_chains.reserve(input_size);
    for (int i = 0; i < input_size; i++) {
      port_chains.push_back(absl::StrCat(YamlSingleDstPortTop, 10000 + i, YamlSingleDstPortBottom));
    }
    listener_yaml_config_ = TestEnvironment::substitute(
        absl::StrCat(YamlHeader, YamlSingleServer, absl::StrJoin(port_chains, "")),
        Network::Address::IpVersion::v4);
    TestUtility::loadFromYaml(listener_yaml_config_, listener_config_);
    filter_chains_ = listener_config_.filter_chains();
  }
  std::string listener_yaml_config_;
  envoy::config::listener::v3::Listener listener_config_;
  absl::Span<const envoy::config::listener::v3::FilterChain* const> filter_chains_;
  MockFilterChainFactoryBuilder dummy_builder_;
  Init::ManagerImpl init_manager_{"fcm_benchmark"};
};

// NOLINTNEXTLINE(readability-redundant-member-init)
BENCHMARK_DEFINE_F(FilterChainBenchmarkFixture, FilterChainManagerBuildTest)
(::benchmark::State& state) {
  NiceMock<Server::Configuration::MockFactoryContext> factory_context;
  for (auto _ : state) {
    FilterChainManagerImpl filter_chain_manager{
        std::make_shared<Network::Address::Ipv4Instance>("127.0.0.1", 1234), factory_context,
        init_manager_};
    filter_chain_manager.addFilterChain(filter_chains_, dummy_builder_, filter_chain_manager);
  }
}

BENCHMARK_DEFINE_F(FilterChainBenchmarkFixture, FilterChainFindTest)
(::benchmark::State& state) {
  std::vector<MockConnectionSocket> sockets;
  sockets.reserve(state.range(0));
  for (int i = 0; i < state.range(0); i++) {
    sockets.push_back(std::move(*MockConnectionSocket::createMockConnectionSocket(
        10000 + i, "127.0.0.1", "", "tls", {}, "8.8.8.8", 111)));
  }
  NiceMock<Server::Configuration::MockFactoryContext> factory_context;
  FilterChainManagerImpl filter_chain_manager{
      std::make_shared<Network::Address::Ipv4Instance>("127.0.0.1", 1234), factory_context,
      init_manager_};

  filter_chain_manager.addFilterChain(filter_chains_, dummy_builder_, filter_chain_manager);
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); i++) {
      filter_chain_manager.findFilterChain(sockets[i]);
    }
  }
}
BENCHMARK_REGISTER_F(FilterChainBenchmarkFixture, FilterChainManagerBuildTest)
    ->Ranges({
        // scale of the chains
        {1, 4096},
    });
BENCHMARK_REGISTER_F(FilterChainBenchmarkFixture, FilterChainFindTest)
    ->Ranges({
        // scale of the chains
        {1, 4096},
    });

/*
clang-format off

Run on (32 X 2200 MHz CPU s)
CPU Caches:
  L1 Data 32K (x16)
  L1 Instruction 32K (x16)
  L2 Unified 256K (x16)
  L3 Unified 56320K (x1)
Load Average: 19.05, 9.89, 3.92
-------------------------------------------------------------------------------------------------------
Benchmark                                                             Time             CPU   Iterations
-------------------------------------------------------------------------------------------------------
FilterChainBenchmarkFixture/FilterChainManagerBuildTest/1        136994 ns       134510 ns         5183
FilterChainBenchmarkFixture/FilterChainManagerBuildTest/8        583649 ns       574596 ns         1207
FilterChainBenchmarkFixture/FilterChainManagerBuildTest/64      4483799 ns      4419618 ns          157
FilterChainBenchmarkFixture/FilterChainManagerBuildTest/512    38864048 ns     38340468 ns           19
FilterChainBenchmarkFixture/FilterChainManagerBuildTest/4096  318686843 ns    318568578 ns            2
FilterChainBenchmarkFixture/FilterChainFindTest/1                   201 ns          201 ns      3494470
FilterChainBenchmarkFixture/FilterChainFindTest/8                  1592 ns         1592 ns       435045
FilterChainBenchmarkFixture/FilterChainFindTest/64                16057 ns        16053 ns        44275
FilterChainBenchmarkFixture/FilterChainFindTest/512              172423 ns       172269 ns         4253
FilterChainBenchmarkFixture/FilterChainFindTest/4096            2676478 ns      2676167 ns          254

clang-format on
*/
} // namespace Server
} // namespace Envoy
