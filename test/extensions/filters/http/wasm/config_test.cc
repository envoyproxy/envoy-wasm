#include "envoy/config/filter/http/wasm/v2/wasm.pb.validate.h"

#include "common/common/base64.h"
#include "common/common/hex.h"
#include "common/crypto/utility.h"
#include "common/stats/isolated_store_impl.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/wasm/config.h"

#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::ReturnRef;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

class WasmFilterConfigTest : public testing::TestWithParam<std::string> {
protected:
  WasmFilterConfigTest() : api_(Api::createApiForTest(stats_store_)) {
    ON_CALL(context_, api()).WillByDefault(ReturnRef(*api_));
    ON_CALL(context_, scope()).WillByDefault(ReturnRef(stats_store_));
    ON_CALL(context_, listenerMetadata()).WillByDefault(ReturnRef(listener_metadata_));
    ON_CALL(context_, initManager()).WillByDefault(ReturnRef(init_manager_));
    ON_CALL(context_, clusterManager()).WillByDefault(ReturnRef(cluster_manager_));
  }

  NiceMock<Server::Configuration::MockFactoryContext> context_;
  Stats::IsolatedStoreImpl stats_store_;
  Api::ApiPtr api_;
  envoy::api::v2::core::Metadata listener_metadata_;
  Init::ManagerImpl init_manager_{"init_manager"};
  NiceMock<Upstream::MockClusterManager> cluster_manager_;
  Init::ExpectableWatcherImpl init_watcher_;
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmFilterConfigTest,
                         testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                             "v8", "wavm"
#elif defined(ENVOY_WASM_V8)
                             "v8"
#elif defined(ENVOY_WASM_WAVM)
                             "wavm"
#endif
                             ));
TEST_P(WasmFilterConfigTest, JsonLoadFromFileWASM) {
  const std::string json = TestEnvironment::substitute(absl::StrCat(R"EOF(
  {
  "config" : {
  "vm_config": {
    "runtime": "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF(",
    "code": {
      "local": {
        "filename": "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm"
      }
    },
  }}}
  )EOF"));

  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromJson(json, proto_config);
  WasmFilterConfig factory;
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  context_.initManager().initialize(init_watcher_);
  EXPECT_EQ(context_.initManager().state(), Init::Manager::State::Initialized);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  EXPECT_CALL(filter_callback, addAccessLogHandler(_));
  cb(filter_callback);
}

TEST_P(WasmFilterConfigTest, YamlLoadFromFileWASM) {
  const std::string yaml = TestEnvironment::substitute(absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF("
      code:
        local:
          filename: "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm"
  )EOF"));

  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  context_.initManager().initialize(init_watcher_);
  EXPECT_EQ(context_.initManager().state(), Init::Manager::State::Initialized);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  EXPECT_CALL(filter_callback, addAccessLogHandler(_));
  cb(filter_callback);
}

TEST_P(WasmFilterConfigTest, YamlLoadInlineWASM) {
  const std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm"));
  EXPECT_FALSE(code.empty());
  const std::string yaml = absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                        GetParam(), R"EOF("
      code: 
        local: { inline_bytes: ")EOF",
                                        Base64::encode(code.data(), code.size()), R"EOF(" }
                                        )EOF");
  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;
  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  context_.initManager().initialize(init_watcher_);
  EXPECT_EQ(context_.initManager().state(), Init::Manager::State::Initialized);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  EXPECT_CALL(filter_callback, addAccessLogHandler(_));
  cb(filter_callback);
}

TEST_P(WasmFilterConfigTest, YamlLoadInlineBadCode) {
  const std::string yaml = absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                        GetParam(), R"EOF("
      code:
        local:
          inline_string: "bad code"
  )EOF");

  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;
  EXPECT_THROW_WITH_MESSAGE(factory.createFilterFactoryFromProto(proto_config, "stats", context_),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to initialize WASM code from <inline>");
}

TEST_P(WasmFilterConfigTest, YamlLoadFromRemoteWASM) {
  const std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm"));
  const std::string sha256 = Hex::encode(
      Envoy::Common::Crypto::UtilitySingleton::get().getSha256Digest(Buffer::OwnedImpl(code)));
  const std::string yaml = TestEnvironment::substitute(absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF("
      code:
        remote:
          http_uri:
            uri: https://example.com/data
            cluster: cluster_1
            timeout: 5s
          sha256: )EOF",
                                                                    sha256));
  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;

  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster_1"))
      .WillOnce(ReturnRef(cluster_manager_.async_client_));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::MessagePtr&, Http::AsyncClient::Callbacks& callbacks,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            Http::MessagePtr response(new Http::ResponseMessageImpl(
                Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "200"}}}));
            response->body() = std::make_unique<Buffer::OwnedImpl>(code);
            callbacks.onSuccess(std::move(response));
            return nullptr;
          }));

  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  context_.initManager().initialize(init_watcher_);
  EXPECT_EQ(context_.initManager().state(), Init::Manager::State::Initialized);
  Http::MockFilterChainFactoryCallbacks filter_callback;
  EXPECT_CALL(filter_callback, addStreamFilter(_));
  EXPECT_CALL(filter_callback, addAccessLogHandler(_));
  cb(filter_callback);
}

TEST_P(WasmFilterConfigTest, YamlLoadFromRemoteConnectionReset) {
  const std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm"));
  const std::string sha256 = Hex::encode(
      Envoy::Common::Crypto::UtilitySingleton::get().getSha256Digest(Buffer::OwnedImpl(code)));
  const std::string yaml = TestEnvironment::substitute(absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF("
      code:
        remote:
          http_uri:
            uri: https://example.com/data
            cluster: cluster_1
            timeout: 5s
          sha256: )EOF",
                                                                    sha256));
  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;

  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster_1"))
      .WillOnce(ReturnRef(cluster_manager_.async_client_));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::MessagePtr&, Http::AsyncClient::Callbacks& callbacks,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            callbacks.onFailure(Envoy::Http::AsyncClient::FailureReason::Reset);
            return nullptr;
          }));

  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  EXPECT_THROW_WITH_MESSAGE(context_.initManager().initialize(init_watcher_),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to load WASM code from https://example.com/data");
}

TEST_P(WasmFilterConfigTest, YamlLoadFromRemoteSuccessWith503) {
  const std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm"));
  const std::string sha256 = Hex::encode(
      Envoy::Common::Crypto::UtilitySingleton::get().getSha256Digest(Buffer::OwnedImpl(code)));
  const std::string yaml = TestEnvironment::substitute(absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF("
      code:
        remote:
          http_uri:
            uri: https://example.com/data
            cluster: cluster_1
            timeout: 5s
          sha256: )EOF",
                                                                    sha256));
  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;

  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster_1"))
      .WillOnce(ReturnRef(cluster_manager_.async_client_));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::MessagePtr&, Http::AsyncClient::Callbacks& callbacks,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            callbacks.onSuccess(Http::MessagePtr{new Http::ResponseMessageImpl(
                Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "503"}}})});
            return nullptr;
          }));

  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  EXPECT_THROW_WITH_MESSAGE(context_.initManager().initialize(init_watcher_),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to load WASM code from https://example.com/data");
}

TEST_P(WasmFilterConfigTest, YamlLoadFromRemoteSuccessIncorrectSha256) {
  const std::string code = TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm"));
  const std::string sha256 = Hex::encode(
      Envoy::Common::Crypto::UtilitySingleton::get().getSha256Digest(Buffer::OwnedImpl(code)));
  const std::string yaml = TestEnvironment::substitute(absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF("
      code:
        remote:
          http_uri:
            uri: https://example.com/data
            cluster: cluster_1
            timeout: 5s
          sha256: xxxx )EOF"));
  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;

  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster_1"))
      .WillOnce(ReturnRef(cluster_manager_.async_client_));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::MessagePtr&, Http::AsyncClient::Callbacks& callbacks,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            Http::MessagePtr response(new Http::ResponseMessageImpl(
                Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "200"}}}));
            response->body() = std::make_unique<Buffer::OwnedImpl>(code);
            callbacks.onSuccess(std::move(response));
            return nullptr;
          }));

  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  EXPECT_THROW_WITH_MESSAGE(context_.initManager().initialize(init_watcher_),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to load WASM code from https://example.com/data");
}

TEST_P(WasmFilterConfigTest, YamlLoadFromRemoteSuccessBadcode) {
  const std::string code = "foo";
  const std::string sha256 = Hex::encode(
      Envoy::Common::Crypto::UtilitySingleton::get().getSha256Digest(Buffer::OwnedImpl(code)));
  const std::string yaml = TestEnvironment::substitute(absl::StrCat(R"EOF(
  config:
    vm_config:
      runtime: "envoy.wasm.runtime.)EOF",
                                                                    GetParam(), R"EOF("
      code:
        remote:
          http_uri:
            uri: https://example.com/data
            cluster: cluster_1
            timeout: 5s
          sha256: )EOF",
                                                                    sha256));
  envoy::config::filter::http::wasm::v2::Wasm proto_config;
  TestUtility::loadFromYaml(yaml, proto_config);
  WasmFilterConfig factory;

  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster_1"))
      .WillOnce(ReturnRef(cluster_manager_.async_client_));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::MessagePtr&, Http::AsyncClient::Callbacks& callbacks,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            Http::MessagePtr response(new Http::ResponseMessageImpl(
                Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "200"}}}));
            response->body() = std::make_unique<Buffer::OwnedImpl>(code);
            callbacks.onSuccess(std::move(response));
            return nullptr;
          }));

  Http::FilterFactoryCb cb = factory.createFilterFactoryFromProto(proto_config, "stats", context_);
  EXPECT_CALL(init_watcher_, ready());
  EXPECT_THROW_WITH_MESSAGE(context_.initManager().initialize(init_watcher_),
                            Extensions::Common::Wasm::WasmException,
                            "Failed to initialize WASM code from https://example.com/data");
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
