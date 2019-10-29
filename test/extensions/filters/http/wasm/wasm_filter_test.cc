#include <stdio.h>

#include "common/buffer/buffer_impl.h"
#include "common/http/message_impl.h"
#include "common/stats/isolated_store_impl.h"
#include "common/stream_info/stream_info_impl.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/common/wasm/wasm_state.h"
#include "extensions/filters/http/wasm/wasm_filter.h"

#include "test/mocks/grpc/mocks.h"
#include "test/mocks/http/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/mocks/ssl/mocks.h"
#include "test/mocks/stream_info/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::AtLeast;
using testing::Eq;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using testing::ReturnPointee;
using testing::ReturnRef;

MATCHER_P(MapEq, rhs, "") {
  const Envoy::ProtobufWkt::Struct& obj = arg;
  EXPECT_TRUE(rhs.size() > 0);
  for (auto const& entry : rhs) {
    EXPECT_EQ(obj.fields().at(entry.first).string_value(), entry.second);
  }
  return true;
}

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

class TestFilter : public Envoy::Extensions::Common::Wasm::Context {
public:
  TestFilter(Wasm* wasm, uint32_t root_context_id,
             Envoy::Extensions::Common::Wasm::PluginSharedPtr plugin)
      : Envoy::Extensions::Common::Wasm::Context(wasm, root_context_id, plugin) {}

  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
};

class TestRoot : public Envoy::Extensions::Common::Wasm::Context {
public:
  TestRoot() {}

  void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {
    scriptLog_(level, message);
  }
  MOCK_METHOD2(scriptLog_, void(spdlog::level::level_enum level, absl::string_view message));
};

class WasmHttpFilterTest : public testing::TestWithParam<std::string> {
public:
  WasmHttpFilterTest() {}
  ~WasmHttpFilterTest() {}

  void setupConfig(const std::string& code) {
    root_context_ = new TestRoot();
    envoy::config::filter::http::wasm::v2::Wasm proto_config;
    proto_config.mutable_config()->mutable_vm_config()->set_vm_id("vm_id");
    proto_config.mutable_config()->mutable_vm_config()->set_runtime(
        absl::StrCat("envoy.wasm.runtime.", GetParam()));
    proto_config.mutable_config()->mutable_vm_config()->mutable_code()->set_inline_bytes(code);
    Api::ApiPtr api = Api::createApiForTest(stats_store_);
    scope_ = Stats::ScopeSharedPtr(stats_store_.createScope("wasm."));
    auto name = "";
    auto root_id = "";
    auto vm_id = "";
    plugin_ = std::make_shared<Extensions::Common::Wasm::Plugin>(
        name, root_id, vm_id, envoy::api::v2::core::TrafficDirection::INBOUND, local_info_,
        &listener_metadata_);
    wasm_ = Extensions::Common::Wasm::createWasmForTesting(
        proto_config.config().vm_config(), plugin_, scope_, cluster_manager_, dispatcher_, *api,
        std::unique_ptr<Envoy::Extensions::Common::Wasm::Context>(root_context_));
  }

  void setupNullConfig(const std::string& name) {
    root_context_ = new TestRoot();
    envoy::config::filter::http::wasm::v2::Wasm proto_config;
    proto_config.mutable_config()->mutable_vm_config()->set_vm_id("vm_id");
    proto_config.mutable_config()->mutable_vm_config()->set_runtime("envoy.wasm.runtime.null");
    proto_config.mutable_config()->mutable_vm_config()->mutable_code()->set_inline_bytes(name);
    Api::ApiPtr api = Api::createApiForTest(stats_store_);
    scope_ = Stats::ScopeSharedPtr(stats_store_.createScope("wasm."));
    auto root_id = "";
    auto vm_id = "";
    plugin_ = std::make_shared<Extensions::Common::Wasm::Plugin>(
        name, root_id, vm_id, envoy::api::v2::core::TrafficDirection::INBOUND, local_info_,
        &listener_metadata_);
    wasm_ = Extensions::Common::Wasm::createWasmForTesting(
        proto_config.config().vm_config(), plugin_, scope_, cluster_manager_, dispatcher_, *api,
        std::unique_ptr<Envoy::Extensions::Common::Wasm::Context>(root_context_));
  }

  void setupFilter() {
    filter_ = std::make_unique<TestFilter>(wasm_.get(), wasm_->getRootContext("")->id(), plugin_);
    filter_->setDecoderFilterCallbacks(decoder_callbacks_);
    filter_->setEncoderFilterCallbacks(encoder_callbacks_);
  }

  Stats::IsolatedStoreImpl stats_store_;
  Stats::ScopeSharedPtr scope_;
  NiceMock<ThreadLocal::MockInstance> tls_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  NiceMock<Upstream::MockClusterManager> cluster_manager_;
  std::shared_ptr<Wasm> wasm_;
  std::shared_ptr<Common::Wasm::Plugin> plugin_;
  std::unique_ptr<TestFilter> filter_;
  NiceMock<Envoy::Ssl::MockConnectionInfo> ssl_;
  NiceMock<Envoy::Network::MockConnection> connection_;
  NiceMock<Http::MockStreamDecoderFilterCallbacks> decoder_callbacks_;
  NiceMock<Http::MockStreamEncoderFilterCallbacks> encoder_callbacks_;
  NiceMock<Envoy::StreamInfo::MockStreamInfo> request_stream_info_;
  NiceMock<LocalInfo::MockLocalInfo> local_info_;
  envoy::api::v2::core::Metadata listener_metadata_;
  TestRoot* root_context_ = nullptr;
};

#if defined(ENVOY_WASM_V8) || defined(ENVOY_WASM_WAVM)

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmHttpFilterTest,
                         testing::Values(
#if defined(ENVOY_WASM_V8) && defined(ENVOY_WASM_WAVM)
                             "v8", "wavm"
#elif defined(ENVOY_WASM_V8)
                             "v8"
#elif defined(ENVOY_WASM_WAVM)
                             "wavm"
#endif
                             ));

// Bad code in initial config.
TEST_P(WasmHttpFilterTest, BadCode) {
  EXPECT_THROW_WITH_MESSAGE(setupConfig("bad code"), Common::Wasm::WasmException,
                            "Failed to initialize WASM code from <inline>");
}

// Script touching headers only, request that is headers only.
TEST_P(WasmHttpFilterTest, HeadersOnlyRequestHeadersOnly) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm")));
  setupFilter();
  absl::optional<Http::Protocol> protocol = Http::Protocol::Http11;
  EXPECT_CALL(request_stream_info_, protocol()).WillRepeatedly(Return(protocol));
  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info,
                                   Eq(absl::string_view("request protocol response HTTP/1.1"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestHeaderMapImpl request_headers{{":path", "/"}, {"server", "envoy"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  EXPECT_THAT(request_headers.get_("newheader"), Eq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), Eq("envoy-wasm"));
  filter_->onDestroy();
}

// Script touching headers only, request that is headers only.
TEST_P(WasmHttpFilterTest, HeadersOnlyRequestHeadersAndBody) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm")));
  setupFilter();
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, true));
  filter_->onDestroy();
}

// Script testing AccessLog::Instance::log.
TEST_P(WasmHttpFilterTest, AccessLog) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm")));
  setupFilter();
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn, Eq(absl::string_view("onLog 2 /"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));

  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, true));
  filter_->onDestroy();
  StreamInfo::MockStreamInfo log_stream_info;
  filter_->log(&request_headers, nullptr, nullptr, log_stream_info);
}

TEST_P(WasmHttpFilterTest, AsyncCall) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/async_call_cpp.wasm")));
  setupFilter();

  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  Http::MockAsyncClientRequest request(&cluster_manager_.async_client_);
  Http::AsyncClient::Callbacks* callbacks = nullptr;
  EXPECT_CALL(cluster_manager_, get(Eq("cluster")));
  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster"));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::MessagePtr& message, Http::AsyncClient::Callbacks& cb,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            EXPECT_EQ((Http::TestHeaderMapImpl{{":method", "POST"},
                                               {":path", "/"},
                                               {":authority", "foo"},
                                               {"content-length", "11"}}),
                      message->headers());
            EXPECT_EQ((Http::TestHeaderMapImpl{{"trail", "cow"}}), *message->trailers());
            callbacks = &cb;
            return &request;
          }));

  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::debug, Eq("response")));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info, Eq(":status -> 200")));
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter_->decodeHeaders(request_headers, false));

  Http::MessagePtr response_message(new Http::ResponseMessageImpl(
      Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "200"}}}));
  response_message->body().reset(new Buffer::OwnedImpl("response"));

  EXPECT_NE(callbacks, nullptr);
  if (callbacks) {
    callbacks->onSuccess(std::move(response_message));
  }
}

TEST_P(WasmHttpFilterTest, AsyncCallAfterDestroyed) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/async_call_cpp.wasm")));
  setupFilter();

  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  Http::MockAsyncClientRequest request(&cluster_manager_.async_client_);
  Http::AsyncClient::Callbacks* callbacks = nullptr;
  EXPECT_CALL(cluster_manager_, get(Eq("cluster")));
  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster"));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::MessagePtr& message, Http::AsyncClient::Callbacks& cb,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            EXPECT_EQ((Http::TestHeaderMapImpl{{":method", "POST"},
                                               {":path", "/"},
                                               {":authority", "foo"},
                                               {"content-length", "11"}}),
                      message->headers());
            EXPECT_EQ((Http::TestHeaderMapImpl{{"trail", "cow"}}), *message->trailers());
            callbacks = &cb;
            return &request;
          }));

  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter_->decodeHeaders(request_headers, false));

  EXPECT_CALL(request, cancel()).WillOnce([&]() { callbacks = nullptr; });

  // Destroy the Context, Plugin and VM.
  filter_.reset();
  plugin_.reset();
  wasm_.reset();

  Http::MessagePtr response_message(new Http::ResponseMessageImpl(
      Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "200"}}}));
  response_message->body().reset(new Buffer::OwnedImpl("response"));

  // (Don't) Make the callback on the destroyed VM.
  EXPECT_EQ(callbacks, nullptr);
  if (callbacks) {
    callbacks->onSuccess(std::move(response_message));
  }
}

TEST_P(WasmHttpFilterTest, GrpcCall) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/grpc_call_cpp.wasm")));
  setupFilter();
  Grpc::MockAsyncRequest request;
  Grpc::RawAsyncRequestCallbacks* callbacks = nullptr;
  Grpc::MockAsyncClientManager client_manager;
  auto client_factory = std::make_unique<Grpc::MockAsyncClientFactory>();
  auto async_client = std::make_unique<Grpc::MockAsyncClient>();
  Tracing::Span* parent_span{};
  EXPECT_CALL(*async_client, sendRaw(_, _, _, _, _, _))
      .WillOnce(Invoke([&](absl::string_view service_full_name, absl::string_view method_name,
                           Buffer::InstancePtr&& message, Grpc::RawAsyncRequestCallbacks& cb,
                           Tracing::Span& span, const Http::AsyncClient::RequestOptions& options)
                           -> Grpc::AsyncRequest* {
        EXPECT_EQ(service_full_name, "service");
        EXPECT_EQ(method_name, "method");
        ProtobufWkt::Value value;
        EXPECT_TRUE(value.ParseFromArray(message->linearize(message->length()), message->length()));
        EXPECT_EQ(value.string_value(), "request");
        callbacks = &cb;
        parent_span = &span;
        EXPECT_EQ(options.timeout->count(), 1000);
        return &request;
      }));
  EXPECT_CALL(*client_factory, create).WillOnce(Invoke([&]() -> Grpc::RawAsyncClientPtr {
    return std::move(async_client);
  }));
  EXPECT_CALL(cluster_manager_, grpcAsyncClientManager())
      .WillOnce(Invoke([&]() -> Grpc::AsyncClientManager& { return client_manager; }));
  EXPECT_CALL(client_manager, factoryForGrpcService(_, _, _))
      .WillOnce(
          Invoke([&](const envoy::api::v2::core::GrpcService&, Stats::Scope&,
                     bool) -> Grpc::AsyncClientFactoryPtr { return std::move(client_factory); }));
  EXPECT_CALL(*root_context_, scriptLog_(spdlog::level::debug, Eq("response")));
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter_->decodeHeaders(request_headers, false));

  ProtobufWkt::Value value;
  value.set_string_value("response");
  std::string response_string;
  EXPECT_TRUE(value.SerializeToString(&response_string));
  auto response = std::make_unique<Buffer::OwnedImpl>(response_string);
  EXPECT_NE(callbacks, nullptr);
  NiceMock<Tracing::MockSpan> span;
  if (callbacks) {
    callbacks->onSuccessRaw(std::move(response), span);
  }
}

TEST_P(WasmHttpFilterTest, GrpcCallAfterDestroyed) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/grpc_call_cpp.wasm")));
  setupFilter();
  Grpc::MockAsyncRequest request;
  Grpc::RawAsyncRequestCallbacks* callbacks = nullptr;
  Grpc::MockAsyncClientManager client_manager;
  auto client_factory = std::make_unique<Grpc::MockAsyncClientFactory>();
  auto async_client = std::make_unique<Grpc::MockAsyncClient>();
  Tracing::Span* parent_span{};
  EXPECT_CALL(*async_client, sendRaw(_, _, _, _, _, _))
      .WillOnce(Invoke([&](absl::string_view service_full_name, absl::string_view method_name,
                           Buffer::InstancePtr&& message, Grpc::RawAsyncRequestCallbacks& cb,
                           Tracing::Span& span, const Http::AsyncClient::RequestOptions& options)
                           -> Grpc::AsyncRequest* {
        EXPECT_EQ(service_full_name, "service");
        EXPECT_EQ(method_name, "method");
        ProtobufWkt::Value value;
        EXPECT_TRUE(value.ParseFromArray(message->linearize(message->length()), message->length()));
        EXPECT_EQ(value.string_value(), "request");
        callbacks = &cb;
        parent_span = &span;
        EXPECT_EQ(options.timeout->count(), 1000);
        return &request;
      }));
  EXPECT_CALL(*client_factory, create).WillOnce(Invoke([&]() -> Grpc::RawAsyncClientPtr {
    return std::move(async_client);
  }));
  EXPECT_CALL(cluster_manager_, grpcAsyncClientManager())
      .WillOnce(Invoke([&]() -> Grpc::AsyncClientManager& { return client_manager; }));
  EXPECT_CALL(client_manager, factoryForGrpcService(_, _, _))
      .WillOnce(
          Invoke([&](const envoy::api::v2::core::GrpcService&, Stats::Scope&,
                     bool) -> Grpc::AsyncClientFactoryPtr { return std::move(client_factory); }));
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};

  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter_->decodeHeaders(request_headers, false));

  EXPECT_CALL(request, cancel()).WillOnce([&]() { callbacks = nullptr; });

  // Destroy the Context, Plugin and VM.
  filter_.reset();
  plugin_.reset();
  wasm_.reset();

  ProtobufWkt::Value value;
  value.set_string_value("response");
  std::string response_string;
  EXPECT_TRUE(value.SerializeToString(&response_string));
  auto response = std::make_unique<Buffer::OwnedImpl>(response_string);
  EXPECT_EQ(callbacks, nullptr);
  NiceMock<Tracing::MockSpan> span;
  if (callbacks) {
    callbacks->onSuccessRaw(std::move(response), span);
  }
}

TEST_P(WasmHttpFilterTest, Metadata) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/metadata_cpp.wasm")));
  setupFilter();
  envoy::api::v2::core::Node node_data;
  ProtobufWkt::Value node_val;
  node_val.set_string_value("wasm_node_get_value");
  (*node_data.mutable_metadata()->mutable_fields())["wasm_node_get_key"] = node_val;
  EXPECT_CALL(local_info_, node()).WillRepeatedly(ReturnRef(node_data));
  EXPECT_CALL(*root_context_, scriptLog_(spdlog::level::debug,
                                         Eq(absl::string_view("onTick wasm_node_get_value"))));

  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::err,
                                   Eq(absl::string_view("onRequestBody wasm_node_get_value"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn, Eq(absl::string_view("onLog 2 /"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  EXPECT_CALL(
      *filter_,
      scriptLog_(spdlog::level::trace,
                 Eq(absl::string_view("Struct wasm_request_get_value wasm_request_get_value"))));

  request_stream_info_.metadata_.mutable_filter_metadata()->insert(
      Protobuf::MapPair<std::string, ProtobufWkt::Struct>(
          HttpFilters::HttpFilterNames::get().Wasm,
          MessageUtil::keyValueStruct("wasm_request_get_key", "wasm_request_get_value")));

  wasm_->tickHandler(root_context_->id());

  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, true));
  filter_->onDestroy();
  StreamInfo::MockStreamInfo log_stream_info;
  filter_->log(&request_headers, nullptr, nullptr, log_stream_info);

  const auto& result = request_stream_info_.filterState().getDataReadOnly<Common::Wasm::WasmState>(
      "wasm_request_set_key");
  EXPECT_EQ("wasm_request_set_value", result.value());
}

#endif

// Null VM Plugin, headers only.
TEST_F(WasmHttpFilterTest, NullPluginRequestHeadersOnly) {
  setupNullConfig("null_vm_plugin");
  setupFilter();
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestHeaderMapImpl request_headers{{":path", "/"}, {"server", "envoy"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  EXPECT_THAT(request_headers.get_("newheader"), Eq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), Eq("envoy-wasm"));
  filter_->onDestroy();
}

TEST_F(WasmHttpFilterTest, NullVmResolver) {
  setupNullConfig("null_vm_plugin");
  setupFilter();
  envoy::api::v2::core::Node node_data;
  ProtobufWkt::Value node_val;
  node_val.set_string_value("sample_data");
  (*node_data.mutable_metadata()->mutable_fields())["istio.io/metadata"] = node_val;
  EXPECT_CALL(local_info_, node()).WillRepeatedly(ReturnRef(node_data));

  request_stream_info_.metadata_.mutable_filter_metadata()->insert(
      Protobuf::MapPair<std::string, ProtobufWkt::Struct>(
          HttpFilters::HttpFilterNames::get().Wasm,
          MessageUtil::keyValueStruct("wasm_request_get_key", "wasm_request_get_value")));
  EXPECT_CALL(request_stream_info_, responseCode()).WillRepeatedly(Return(403));
  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::info, Eq(absl::string_view("header path /test_context"))));

  // test outputs should match inputs
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn,
                                   Eq(absl::string_view("request.path: /test_context"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::warn, Eq(absl::string_view("node.metadata: sample_data"))));
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn,
                                   Eq(absl::string_view("metadata: wasm_request_get_value"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::warn, Eq(absl::string_view("response.code: 403"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::warn, Eq(absl::string_view("state: wasm_value"))));

  Http::TestHeaderMapImpl request_headers{{":path", "/test_context"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  StreamInfo::MockStreamInfo log_stream_info;
  filter_->log(&request_headers, nullptr, nullptr, log_stream_info);
}

TEST_P(WasmHttpFilterTest, SharedData) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/shared_cpp.wasm")));
  setupFilter();
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::info, Eq(absl::string_view("set CasMismatch"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::debug, Eq(absl::string_view("get 1 shared_data_value1"))));
  EXPECT_CALL(*filter_,
              scriptLog_(spdlog::level::warn, Eq(absl::string_view("get 2 shared_data_value2"))));

  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  StreamInfo::MockStreamInfo log_stream_info;
  filter_->log(&request_headers, nullptr, nullptr, log_stream_info);
}

TEST_P(WasmHttpFilterTest, SharedQueue) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/queue_cpp.wasm")));
  setupFilter();
  EXPECT_CALL(*filter_, scriptLog_(spdlog::level::warn,
                                   Eq(absl::string_view("onRequestHeaders enqueue Ok"))));
  EXPECT_CALL(*root_context_,
              scriptLog_(spdlog::level::info, Eq(absl::string_view("onQueueReady"))));
  EXPECT_CALL(*root_context_,
              scriptLog_(spdlog::level::debug, Eq(absl::string_view("data data1 Ok"))));

  EXPECT_CALL(dispatcher_, post(_)).WillOnce(Return());
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  auto token = Common::Wasm::resolveQueueForTest("vm_id", "my_shared_queue");
  wasm_->queueReady(root_context_->id(), token);
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
