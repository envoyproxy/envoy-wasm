#include "test/integration/integration.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "envoy/buffer/buffer.h"
#include "envoy/http/header_map.h"

#include "common/api/api_impl.h"
#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/fmt.h"
#include "common/common/stack_array.h"
#include "common/event/dispatcher_impl.h"
#include "common/event/libevent.h"
#include "common/network/connection_impl.h"
#include "common/network/utility.h"
#include "common/upstream/upstream_impl.h"

#include "extensions/transport_sockets/tls/context_config_impl.h"
#include "extensions/transport_sockets/tls/context_manager_impl.h"
#include "extensions/transport_sockets/tls/ssl_socket.h"

#include "test/integration/autonomous_upstream.h"
#include "test/integration/utility.h"
#include "test/test_common/environment.h"
#include "test/test_common/network_utility.h"

#include "gtest/gtest.h"

using testing::_;
using testing::AnyNumber;
using testing::AssertionFailure;
using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::AtLeast;
using testing::Invoke;
using testing::IsSubstring;
using testing::NiceMock;
using testing::ReturnRef;

namespace Envoy {

IntegrationStreamDecoder::IntegrationStreamDecoder(Event::Dispatcher& dispatcher)
    : dispatcher_(dispatcher) {}

void IntegrationStreamDecoder::waitForContinueHeaders() {
  if (!continue_headers_.get()) {
    waiting_for_continue_headers_ = true;
    dispatcher_.run(Event::Dispatcher::RunType::Block);
  }
}

void IntegrationStreamDecoder::waitForHeaders() {
  if (!headers_.get()) {
    waiting_for_headers_ = true;
    dispatcher_.run(Event::Dispatcher::RunType::Block);
  }
}

void IntegrationStreamDecoder::waitForBodyData(uint64_t size) {
  ASSERT(body_data_waiting_length_ == 0);
  body_data_waiting_length_ = size;
  body_data_waiting_length_ -=
      std::min(body_data_waiting_length_, static_cast<uint64_t>(body_.size()));
  if (body_data_waiting_length_ > 0) {
    dispatcher_.run(Event::Dispatcher::RunType::Block);
  }
}

void IntegrationStreamDecoder::waitForEndStream() {
  if (!saw_end_stream_) {
    waiting_for_end_stream_ = true;
    dispatcher_.run(Event::Dispatcher::RunType::Block);
  }
}

void IntegrationStreamDecoder::waitForReset() {
  if (!saw_reset_) {
    waiting_for_reset_ = true;
    dispatcher_.run(Event::Dispatcher::RunType::Block);
  }
}

void IntegrationStreamDecoder::decode100ContinueHeaders(Http::HeaderMapPtr&& headers) {
  continue_headers_ = std::move(headers);
  if (waiting_for_continue_headers_) {
    dispatcher_.exit();
  }
}

void IntegrationStreamDecoder::decodeHeaders(Http::HeaderMapPtr&& headers, bool end_stream) {
  saw_end_stream_ = end_stream;
  headers_ = std::move(headers);
  if ((end_stream && waiting_for_end_stream_) || waiting_for_headers_) {
    dispatcher_.exit();
  }
}

void IntegrationStreamDecoder::decodeData(Buffer::Instance& data, bool end_stream) {
  saw_end_stream_ = end_stream;
  body_ += data.toString();

  if (end_stream && waiting_for_end_stream_) {
    dispatcher_.exit();
  } else if (body_data_waiting_length_ > 0) {
    body_data_waiting_length_ -= std::min(body_data_waiting_length_, data.length());
    if (body_data_waiting_length_ == 0) {
      dispatcher_.exit();
    }
  }
}

void IntegrationStreamDecoder::decodeTrailers(Http::HeaderMapPtr&& trailers) {
  saw_end_stream_ = true;
  trailers_ = std::move(trailers);
  if (waiting_for_end_stream_) {
    dispatcher_.exit();
  }
}

void IntegrationStreamDecoder::decodeMetadata(Http::MetadataMapPtr&& metadata_map) {
  // Combines newly received metadata with the existing metadata.
  for (const auto& metadata : *metadata_map) {
    duplicated_metadata_key_count_[metadata.first]++;
    metadata_map_->insert(metadata);
  }
}

void IntegrationStreamDecoder::onResetStream(Http::StreamResetReason reason, absl::string_view) {
  saw_reset_ = true;
  reset_reason_ = reason;
  if (waiting_for_reset_) {
    dispatcher_.exit();
  }
}

IntegrationTcpClient::IntegrationTcpClient(Event::Dispatcher& dispatcher,
                                           MockBufferFactory& factory, uint32_t port,
                                           Network::Address::IpVersion version,
                                           bool enable_half_close)
    : payload_reader_(new WaitForPayloadReader(dispatcher)),
      callbacks_(new ConnectionCallbacks(*this)) {
  EXPECT_CALL(factory, create_(_, _))
      .WillOnce(Invoke([&](std::function<void()> below_low,
                           std::function<void()> above_high) -> Buffer::Instance* {
        client_write_buffer_ = new NiceMock<MockWatermarkBuffer>(below_low, above_high);
        return client_write_buffer_;
      }));

  connection_ = dispatcher.createClientConnection(
      Network::Utility::resolveUrl(
          fmt::format("tcp://{}:{}", Network::Test::getLoopbackAddressUrlString(version), port)),
      Network::Address::InstanceConstSharedPtr(), Network::Test::createRawBufferSocket(), nullptr);

  ON_CALL(*client_write_buffer_, drain(_))
      .WillByDefault(testing::Invoke(client_write_buffer_, &MockWatermarkBuffer::baseDrain));
  EXPECT_CALL(*client_write_buffer_, drain(_)).Times(AnyNumber());

  connection_->enableHalfClose(enable_half_close);
  connection_->addConnectionCallbacks(*callbacks_);
  connection_->addReadFilter(payload_reader_);
  connection_->connect();
}

void IntegrationTcpClient::close() { connection_->close(Network::ConnectionCloseType::NoFlush); }

void IntegrationTcpClient::waitForData(const std::string& data, bool exact_match) {
  auto found = payload_reader_->data().find(data);
  if (found == 0 || (!exact_match && found != std::string::npos)) {
    return;
  }

  payload_reader_->set_data_to_wait_for(data, exact_match);
  connection_->dispatcher().run(Event::Dispatcher::RunType::Block);
}

void IntegrationTcpClient::waitForDisconnect(bool ignore_spurious_events) {
  if (ignore_spurious_events) {
    while (!disconnected_) {
      connection_->dispatcher().run(Event::Dispatcher::RunType::Block);
    }
  } else {
    connection_->dispatcher().run(Event::Dispatcher::RunType::Block);
    EXPECT_TRUE(disconnected_);
  }
}

void IntegrationTcpClient::waitForHalfClose() {
  connection_->dispatcher().run(Event::Dispatcher::RunType::Block);
  EXPECT_TRUE(payload_reader_->readLastByte());
}

void IntegrationTcpClient::readDisable(bool disabled) { connection_->readDisable(disabled); }

void IntegrationTcpClient::write(const std::string& data, bool end_stream, bool verify) {
  Buffer::OwnedImpl buffer(data);
  if (verify) {
    EXPECT_CALL(*client_write_buffer_, move(_));
    if (!data.empty()) {
      EXPECT_CALL(*client_write_buffer_, write(_)).Times(AtLeast(1));
    }
  }

  int bytes_expected = client_write_buffer_->bytes_written() + data.size();

  connection_->write(buffer, end_stream);
  do {
    connection_->dispatcher().run(Event::Dispatcher::RunType::NonBlock);
  } while (client_write_buffer_->bytes_written() != bytes_expected && !disconnected_);
  if (verify) {
    // If we disconnect part way through the write, then we should fail, since write() is always
    // expected to succeed.
    EXPECT_TRUE(!disconnected_ || client_write_buffer_->bytes_written() == bytes_expected);
  }
}

void IntegrationTcpClient::ConnectionCallbacks::onEvent(Network::ConnectionEvent event) {
  if (event == Network::ConnectionEvent::RemoteClose) {
    parent_.disconnected_ = true;
    parent_.connection_->dispatcher().exit();
  }
}

BaseIntegrationTest::BaseIntegrationTest(const InstanceConstSharedPtrFn& upstream_address_fn,
                                         Network::Address::IpVersion version,
                                         const std::string& config)
    : api_(Api::createApiForTest(stats_store_)),
      mock_buffer_factory_(new NiceMock<MockBufferFactory>),
      dispatcher_(api_->allocateDispatcher(Buffer::WatermarkFactoryPtr{mock_buffer_factory_})),
      version_(version), upstream_address_fn_(upstream_address_fn),
      config_helper_(version, *api_, config),
      default_log_level_(TestEnvironment::getOptions().logLevel()) {
  // This is a hack, but there are situations where we disconnect fake upstream connections and
  // then we expect the server connection pool to get the disconnect before the next test starts.
  // This does not always happen. This pause should allow the server to pick up the disconnect
  // notification and clear the pool connection if necessary. A real fix would require adding fairly
  // complex test hooks to the server and/or spin waiting on stats, neither of which I think are
  // necessary right now.
  timeSystem().sleep(std::chrono::milliseconds(10));
  ON_CALL(*mock_buffer_factory_, create_(_, _))
      .WillByDefault(Invoke([](std::function<void()> below_low,
                               std::function<void()> above_high) -> Buffer::Instance* {
        return new Buffer::WatermarkBuffer(below_low, above_high);
      }));
  ON_CALL(factory_context_, api()).WillByDefault(ReturnRef(*api_));
}

BaseIntegrationTest::BaseIntegrationTest(Network::Address::IpVersion version,
                                         const std::string& config)
    : BaseIntegrationTest(
          [version](int) {
            return Network::Utility::parseInternetAddress(
                Network::Test::getAnyAddressString(version), 0);
          },
          version, config) {}

Network::ClientConnectionPtr BaseIntegrationTest::makeClientConnection(uint32_t port) {
  Network::ClientConnectionPtr connection(dispatcher_->createClientConnection(
      Network::Utility::resolveUrl(
          fmt::format("tcp://{}:{}", Network::Test::getLoopbackAddressUrlString(version_), port)),
      Network::Address::InstanceConstSharedPtr(), Network::Test::createRawBufferSocket(), nullptr));

  connection->enableHalfClose(enable_half_close_);
  return connection;
}

void BaseIntegrationTest::initialize() {
  RELEASE_ASSERT(!initialized_, "");
  RELEASE_ASSERT(Event::Libevent::Global::initialized(), "");
  initialized_ = true;

  createUpstreams();
  createXdsUpstream();
  createEnvoy();
}

void BaseIntegrationTest::createUpstreams() {
  for (uint32_t i = 0; i < fake_upstreams_count_; ++i) {
    auto endpoint = upstream_address_fn_(i);
    if (autonomous_upstream_) {
      fake_upstreams_.emplace_back(
          new AutonomousUpstream(endpoint, upstream_protocol_, *time_system_));
    } else {
      fake_upstreams_.emplace_back(
          new FakeUpstream(endpoint, upstream_protocol_, *time_system_, enable_half_close_));
    }
  }
}

void BaseIntegrationTest::createEnvoy() {
  for (auto& upstream : fake_upstreams_) {
    if (upstream->localAddress()->ip()) {
      ports_.push_back(upstream->localAddress()->ip()->port());
    }
  }

  if (use_lds_) {
    ENVOY_LOG_MISC(debug, "Setting up file-based LDS");
    // Before finalization, set up a real lds path, replacing the default /dev/null
    std::string lds_path = TestEnvironment::temporaryPath(TestUtility::uniqueFilename());
    config_helper_.addConfigModifier(
        [lds_path](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
          bootstrap.mutable_dynamic_resources()->mutable_lds_config()->set_path(lds_path);
        });
  }

  // Note that finalize assumes that every fake_upstream_ must correspond to a bootstrap config
  // static entry. So, if you want to manually create a fake upstream without specifying it in the
  // config, you will need to do so *after* initialize() (which calls this function) is done.
  config_helper_.finalize(ports_);

  envoy::config::bootstrap::v2::Bootstrap bootstrap = config_helper_.bootstrap();
  if (use_lds_) {
    // After the config has been finalized, write the final listener config to the lds file.
    const std::string lds_path = config_helper_.bootstrap().dynamic_resources().lds_config().path();
    envoy::api::v2::DiscoveryResponse lds;
    lds.set_version_info("0");
    for (auto& listener : config_helper_.bootstrap().static_resources().listeners()) {
      ProtobufWkt::Any* resource = lds.add_resources();
      resource->PackFrom(listener);
    }
    TestEnvironment::writeStringToFileForTest(lds_path, MessageUtil::getJsonStringFromMessage(lds),
                                              true);

    // Now that the listeners have been written to the lds file, remove them from static resources
    // or they will not be reloadable.
    bootstrap.mutable_static_resources()->mutable_listeners()->Clear();
  }
  ENVOY_LOG_MISC(debug, "Running Envoy with configuration:\n{}",
                 MessageUtil::getYamlStringFromMessage(bootstrap));

  const std::string bootstrap_path = TestEnvironment::writeStringToFileForTest(
      "bootstrap.json", MessageUtil::getJsonStringFromMessage(bootstrap));

  std::vector<std::string> named_ports;
  const auto& static_resources = config_helper_.bootstrap().static_resources();
  named_ports.reserve(static_resources.listeners_size());
  for (int i = 0; i < static_resources.listeners_size(); ++i) {
    named_ports.push_back(static_resources.listeners(i).name());
  }
  createGeneratedApiTestServer(bootstrap_path, named_ports);
}

void BaseIntegrationTest::setUpstreamProtocol(FakeHttpConnection::Type protocol) {
  upstream_protocol_ = protocol;
  if (upstream_protocol_ == FakeHttpConnection::Type::HTTP2) {
    config_helper_.addConfigModifier(
        [&](envoy::config::bootstrap::v2::Bootstrap& bootstrap) -> void {
          RELEASE_ASSERT(bootstrap.mutable_static_resources()->clusters_size() >= 1, "");
          auto* cluster = bootstrap.mutable_static_resources()->mutable_clusters(0);
          cluster->mutable_http2_protocol_options();
        });
  } else {
    RELEASE_ASSERT(protocol == FakeHttpConnection::Type::HTTP1, "");
  }
}

IntegrationTcpClientPtr BaseIntegrationTest::makeTcpConnection(uint32_t port) {
  return std::make_unique<IntegrationTcpClient>(*dispatcher_, *mock_buffer_factory_, port, version_,
                                                enable_half_close_);
}

void BaseIntegrationTest::registerPort(const std::string& key, uint32_t port) {
  port_map_[key] = port;
}

uint32_t BaseIntegrationTest::lookupPort(const std::string& key) {
  auto it = port_map_.find(key);
  if (it != port_map_.end()) {
    return it->second;
  }
  RELEASE_ASSERT(
      false,
      fmt::format("lookupPort() called on service type '{}', which has not been added to port_map_",
                  key));
}

void BaseIntegrationTest::setUpstreamAddress(uint32_t upstream_index,
                                             envoy::api::v2::endpoint::LbEndpoint& endpoint) const {
  auto* socket_address = endpoint.mutable_endpoint()->mutable_address()->mutable_socket_address();
  socket_address->set_address(Network::Test::getLoopbackAddressString(version_));
  socket_address->set_port_value(fake_upstreams_[upstream_index]->localAddress()->ip()->port());
}

void BaseIntegrationTest::registerTestServerPorts(const std::vector<std::string>& port_names) {
  auto port_it = port_names.cbegin();
  auto listeners = test_server_->server().listenerManager().listeners();
  auto listener_it = listeners.cbegin();
  for (; port_it != port_names.end() && listener_it != listeners.end(); ++port_it, ++listener_it) {
    const auto listen_addr = listener_it->get().socket().localAddress();
    if (listen_addr->type() == Network::Address::Type::Ip) {
      ENVOY_LOG(debug, "registered '{}' as port {}.", *port_it, listen_addr->ip()->port());
      registerPort(*port_it, listen_addr->ip()->port());
    }
  }
  const auto admin_addr = test_server_->server().admin().socket().localAddress();
  if (admin_addr->type() == Network::Address::Type::Ip) {
    registerPort("admin", admin_addr->ip()->port());
  }
}

void BaseIntegrationTest::createGeneratedApiTestServer(const std::string& bootstrap_path,
                                                       const std::vector<std::string>& port_names) {
  test_server_ = IntegrationTestServer::create(bootstrap_path, version_, on_server_init_function_,
                                               deterministic_, timeSystem(), *api_,
                                               defer_listener_finalization_, process_object_);
  if (config_helper_.bootstrap().static_resources().listeners_size() > 0 &&
      !defer_listener_finalization_) {

    // Wait for listeners to be created before invoking registerTestServerPorts() below, as that
    // needs to know about the bound listener ports.
    auto end_time = time_system_.monotonicTime() + TestUtility::DefaultTimeout;
    const char* success = "listener_manager.listener_create_success";
    const char* failure = "listener_manager.lds.update_rejected";
    while (test_server_->counter(success) == nullptr ||
           test_server_->counter(success)->value() == 0) {
      if (time_system_.monotonicTime() >= end_time) {
        RELEASE_ASSERT(0, "Timed out waiting for listeners.");
      }
      RELEASE_ASSERT(test_server_->counter(failure) == nullptr ||
                         test_server_->counter(failure)->value() == 0,
                     "Lds update failed");
      time_system_.sleep(std::chrono::milliseconds(10));
    }

    registerTestServerPorts(port_names);
  }
}

void BaseIntegrationTest::createApiTestServer(const ApiFilesystemConfig& api_filesystem_config,
                                              const std::vector<std::string>& port_names) {
  const std::string eds_path = TestEnvironment::temporaryFileSubstitute(
      api_filesystem_config.eds_path_, port_map_, version_);
  const std::string cds_path = TestEnvironment::temporaryFileSubstitute(
      api_filesystem_config.cds_path_, {{"eds_json_path", eds_path}}, port_map_, version_);
  const std::string rds_path = TestEnvironment::temporaryFileSubstitute(
      api_filesystem_config.rds_path_, port_map_, version_);
  const std::string lds_path = TestEnvironment::temporaryFileSubstitute(
      api_filesystem_config.lds_path_, {{"rds_json_path", rds_path}}, port_map_, version_);
  createGeneratedApiTestServer(TestEnvironment::temporaryFileSubstitute(
                                   api_filesystem_config.bootstrap_path_,
                                   {{"cds_json_path", cds_path}, {"lds_json_path", lds_path}},
                                   port_map_, version_),
                               port_names);
}

void BaseIntegrationTest::createTestServer(const std::string& json_path,
                                           const std::vector<std::string>& port_names) {
  test_server_ = createIntegrationTestServer(
      TestEnvironment::temporaryFileSubstitute(json_path, port_map_, version_), nullptr,
      timeSystem());
  registerTestServerPorts(port_names);
}

void BaseIntegrationTest::sendRawHttpAndWaitForResponse(int port, const char* raw_http,
                                                        std::string* response,
                                                        bool disconnect_after_headers_complete) {
  Buffer::OwnedImpl buffer(raw_http);
  RawConnectionDriver connection(
      port, buffer,
      [&](Network::ClientConnection& client, const Buffer::Instance& data) -> void {
        response->append(data.toString());
        if (disconnect_after_headers_complete && response->find("\r\n\r\n") != std::string::npos) {
          client.close(Network::ConnectionCloseType::NoFlush);
        }
      },
      version_);

  connection.run();
}

IntegrationTestServerPtr
BaseIntegrationTest::createIntegrationTestServer(const std::string& bootstrap_path,
                                                 std::function<void()> on_server_init_function,
                                                 Event::TestTimeSystem& time_system) {
  return IntegrationTestServer::create(bootstrap_path, version_, on_server_init_function,
                                       deterministic_, time_system, *api_,
                                       defer_listener_finalization_);
}

void BaseIntegrationTest::createXdsUpstream() {
  if (create_xds_upstream_ == false) {
    return;
  }
  if (tls_xds_upstream_ == false) {
    fake_upstreams_.emplace_back(
        new FakeUpstream(0, FakeHttpConnection::Type::HTTP2, version_, timeSystem()));
  } else {
    envoy::api::v2::auth::DownstreamTlsContext tls_context;
    auto* common_tls_context = tls_context.mutable_common_tls_context();
    common_tls_context->add_alpn_protocols("h2");
    auto* tls_cert = common_tls_context->add_tls_certificates();
    tls_cert->mutable_certificate_chain()->set_filename(
        TestEnvironment::runfilesPath("test/config/integration/certs/upstreamcert.pem"));
    tls_cert->mutable_private_key()->set_filename(
        TestEnvironment::runfilesPath("test/config/integration/certs/upstreamkey.pem"));
    auto cfg = std::make_unique<Extensions::TransportSockets::Tls::ServerContextConfigImpl>(
        tls_context, factory_context_);

    static Stats::Scope* upstream_stats_store = new Stats::TestIsolatedStoreImpl();
    auto context = std::make_unique<Extensions::TransportSockets::Tls::ServerSslSocketFactory>(
        std::move(cfg), context_manager_, *upstream_stats_store, std::vector<std::string>{});
    fake_upstreams_.emplace_back(new FakeUpstream(
        std::move(context), 0, FakeHttpConnection::Type::HTTP2, version_, timeSystem()));
  }
  xds_upstream_ = fake_upstreams_[1].get();
  // Don't ASSERT fail if an xDS reconnect ends up unparented.
  xds_upstream_->set_allow_unexpected_disconnects(true);
}

void BaseIntegrationTest::createXdsConnection() {
  AssertionResult result = xds_upstream_->waitForHttpConnection(*dispatcher_, xds_connection_);
  RELEASE_ASSERT(result, result.message());
}

void BaseIntegrationTest::cleanUpXdsConnection() {
  AssertionResult result = xds_connection_->close();
  RELEASE_ASSERT(result, result.message());
  result = xds_connection_->waitForDisconnect();
  RELEASE_ASSERT(result, result.message());
  xds_connection_.reset();
}

AssertionResult BaseIntegrationTest::compareDiscoveryRequest(
    const std::string& expected_type_url, const std::string& expected_version,
    const std::vector<std::string>& expected_resource_names,
    const std::vector<std::string>& expected_resource_names_added,
    const std::vector<std::string>& expected_resource_names_removed,
    const Protobuf::int32 expected_error_code, const std::string& expected_error_message) {
  if (sotw_or_delta_ == Grpc::SotwOrDelta::Sotw) {
    return compareSotwDiscoveryRequest(expected_type_url, expected_version, expected_resource_names,
                                       expected_error_code, expected_error_message);
  } else {
    return compareDeltaDiscoveryRequest(expected_type_url, expected_resource_names_added,
                                        expected_resource_names_removed, expected_error_code,
                                        expected_error_message);
  }
}

AssertionResult BaseIntegrationTest::compareSotwDiscoveryRequest(
    const std::string& expected_type_url, const std::string& expected_version,
    const std::vector<std::string>& expected_resource_names,
    const Protobuf::int32 expected_error_code, const std::string& expected_error_message) {
  envoy::api::v2::DiscoveryRequest discovery_request;
  VERIFY_ASSERTION(xds_stream_->waitForGrpcMessage(*dispatcher_, discovery_request));

  EXPECT_TRUE(discovery_request.has_node());
  EXPECT_FALSE(discovery_request.node().id().empty());
  EXPECT_FALSE(discovery_request.node().cluster().empty());

  if (expected_type_url != discovery_request.type_url()) {
    return AssertionFailure() << fmt::format("type_url {} does not match expected {}",
                                             discovery_request.type_url(), expected_type_url);
  }
  if (!(expected_error_code == discovery_request.error_detail().code())) {
    return AssertionFailure() << fmt::format("error_code {} does not match expected {}",
                                             discovery_request.error_detail().code(),
                                             expected_error_code);
  }
  EXPECT_TRUE(
      IsSubstring("", "", expected_error_message, discovery_request.error_detail().message()));
  const std::vector<std::string> resource_names(discovery_request.resource_names().cbegin(),
                                                discovery_request.resource_names().cend());
  if (expected_resource_names != resource_names) {
    return AssertionFailure() << fmt::format(
               "resources {} do not match expected {} in {}",
               fmt::join(resource_names.begin(), resource_names.end(), ","),
               fmt::join(expected_resource_names.begin(), expected_resource_names.end(), ","),
               discovery_request.DebugString());
  }
  if (expected_version != discovery_request.version_info()) {
    return AssertionFailure() << fmt::format("version {} does not match expected {} in {}",
                                             discovery_request.version_info(), expected_version,
                                             discovery_request.DebugString());
  }
  return AssertionSuccess();
}

AssertionResult BaseIntegrationTest::compareDeltaDiscoveryRequest(
    const std::string& expected_type_url,
    const std::vector<std::string>& expected_resource_subscriptions,
    const std::vector<std::string>& expected_resource_unsubscriptions, FakeStreamPtr& xds_stream,
    const Protobuf::int32 expected_error_code, const std::string& expected_error_message) {
  envoy::api::v2::DeltaDiscoveryRequest request;
  VERIFY_ASSERTION(xds_stream->waitForGrpcMessage(*dispatcher_, request));

  EXPECT_TRUE(request.has_node());
  EXPECT_FALSE(request.node().id().empty());
  EXPECT_FALSE(request.node().cluster().empty());

  if (expected_type_url != request.type_url()) {
    return AssertionFailure() << fmt::format("type_url {} does not match expected {}",
                                             request.type_url(), expected_type_url);
  }
  if (!(expected_error_code == request.error_detail().code())) {
    return AssertionFailure() << fmt::format("error_code {} does not match expected {}",
                                             request.error_detail().code(), expected_error_code);
  }
  EXPECT_TRUE(IsSubstring("", "", expected_error_message, request.error_detail().message()));

  const std::vector<std::string> resource_subscriptions(request.resource_names_subscribe().cbegin(),
                                                        request.resource_names_subscribe().cend());
  if (expected_resource_subscriptions != resource_subscriptions) {
    return AssertionFailure() << fmt::format(
               "newly subscribed resources {} do not match expected {} in {}",
               fmt::join(resource_subscriptions.begin(), resource_subscriptions.end(), ","),
               fmt::join(expected_resource_subscriptions.begin(),
                         expected_resource_subscriptions.end(), ","),
               request.DebugString());
  }
  const std::vector<std::string> resource_unsubscriptions(
      request.resource_names_unsubscribe().cbegin(), request.resource_names_unsubscribe().cend());
  if (expected_resource_unsubscriptions != resource_unsubscriptions) {
    return AssertionFailure() << fmt::format(
               "newly UNsubscribed resources {} do not match expected {} in {}",
               fmt::join(resource_unsubscriptions.begin(), resource_unsubscriptions.end(), ","),
               fmt::join(expected_resource_unsubscriptions.begin(),
                         expected_resource_unsubscriptions.end(), ","),
               request.DebugString());
  }
  return AssertionSuccess();
}

} // namespace Envoy
