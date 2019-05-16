#include <stdio.h>

#include "common/buffer/buffer_impl.h"
#include "common/http/message_impl.h"
#include "common/stats/isolated_store_impl.h"
#include "common/stream_info/stream_info_impl.h"

#include "extensions/common/wasm/wasm.h"
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
#include "test/test_common/test_base.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"

using testing::_;
using testing::AtLeast;
using testing::Eq;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using testing::ReturnPointee;
using testing::ReturnRef;
using testing::StrEq;

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
  TestFilter(Wasm* wasm) : Envoy::Extensions::Common::Wasm::Context(wasm) {}

  MOCK_METHOD2(scriptLog, void(spdlog::level::level_enum level, absl::string_view message));
};

class WasmHttpFilterTest : public TestBaseWithParam<std::string> {
public:
  WasmHttpFilterTest() {}
  ~WasmHttpFilterTest() {}

  void setupConfig(const std::string& code) {
    envoy::config::filter::http::wasm::v2::Wasm proto_config;
    proto_config.mutable_vm_config()->set_vm(absl::StrCat("envoy.wasm.vm.", GetParam()));
    proto_config.mutable_vm_config()->mutable_code()->set_inline_bytes(code);
    Api::ApiPtr api = Api::createApiForTest(stats_store_);
    scope_ = Stats::ScopeSharedPtr(stats_store_.createScope("wasm."));
    wasm_ = Extensions::Common::Wasm::createWasm(proto_config.id(), proto_config.vm_config(),
                                                 cluster_manager_, dispatcher_, *api, *scope_,
                                                 local_info_);
  }

  void setupFilter() {
    filter_ = std::make_shared<TestFilter>(wasm_.get());
    filter_->setDecoderFilterCallbacks(decoder_callbacks_);
    filter_->setEncoderFilterCallbacks(encoder_callbacks_);
    wasm_->setGeneralContext(
        std::static_pointer_cast<Envoy::Extensions::Common::Wasm::Context>(filter_));
  }

  Stats::IsolatedStoreImpl stats_store_;
  Stats::ScopeSharedPtr scope_;
  NiceMock<ThreadLocal::MockInstance> tls_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  Upstream::MockClusterManager cluster_manager_;
  std::shared_ptr<Wasm> wasm_;
  std::shared_ptr<TestFilter> filter_;
  NiceMock<Envoy::Ssl::MockConnection> ssl_;
  NiceMock<Envoy::Network::MockConnection> connection_;
  NiceMock<Http::MockStreamDecoderFilterCallbacks> decoder_callbacks_;
  NiceMock<Http::MockStreamEncoderFilterCallbacks> encoder_callbacks_;
  NiceMock<Envoy::StreamInfo::MockStreamInfo> request_stream_info_;
  NiceMock<LocalInfo::MockLocalInfo> local_info_;
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmHttpFilterTest, testing::Values("wavm", "v8"));

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
  EXPECT_CALL(*filter_,
              scriptLog(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 1"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::warn, Eq(absl::string_view("onDone 1"))));
  wasm_->start();
  Http::TestHeaderMapImpl request_headers{{":path", "/"}, {"server", "envoy"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  EXPECT_THAT(request_headers.get_("newheader"), StrEq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), StrEq("envoy-wasm"));
  filter_->onDestroy();
}

// Script touching headers only, request that is headers only.
TEST_P(WasmHttpFilterTest, HeadersOnlyRequestHeadersAndBody) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/headers_cpp.wasm")));
  setupFilter();
  EXPECT_CALL(*filter_,
              scriptLog(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 1"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_,
              scriptLog(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::warn, Eq(absl::string_view("onDone 1"))));
  wasm_->start();
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
              scriptLog(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 1"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_,
              scriptLog(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::warn, Eq(absl::string_view("onLog 1 /"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::warn, Eq(absl::string_view("onDone 1"))));

  wasm_->start();
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
  wasm_->start();

  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  Http::MockAsyncClientRequest request(&cluster_manager_.async_client_);
  Http::AsyncClient::Callbacks* callbacks = nullptr;
  EXPECT_CALL(cluster_manager_, get("cluster"));
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

  EXPECT_CALL(*filter_, scriptLog(spdlog::level::debug, StrEq("response")));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::info, StrEq(":status -> 200")));
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

TEST_P(WasmHttpFilterTest, GrpcCall) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/grpc_call_cpp.wasm")));
  setupFilter();
  wasm_->start();

  Grpc::MockAsyncRequest request;
  Grpc::RawAsyncRequestCallbacks* callbacks = nullptr;
  Grpc::MockAsyncClientManager client_manager;
  auto client_factory = std::make_unique<Grpc::MockAsyncClientFactory>();
  auto async_client = std::make_unique<Grpc::MockAsyncClient>();
  Tracing::Span* parent_span{};
  EXPECT_CALL(*async_client, sendRaw_(_, _, _, _, _, _))
      .WillOnce(Invoke(
          [&](absl::string_view service_full_name, absl::string_view method_name,
              Buffer::Instance& message, Grpc::RawAsyncRequestCallbacks& cb, Tracing::Span& span,
              const absl::optional<std::chrono::milliseconds>& timeout) -> Grpc::AsyncRequest* {
            EXPECT_EQ(service_full_name, "service");
            EXPECT_EQ(method_name, "method");
            ProtobufWkt::Value value;
            EXPECT_TRUE(
                value.ParseFromArray(message.linearize(message.length()), message.length()));
            EXPECT_EQ(value.string_value(), "request");
            callbacks = &cb;
            parent_span = &span;
            EXPECT_EQ(timeout->count(), 1000);
            return &request;
          }));
  EXPECT_CALL(*client_factory, create).WillOnce(Invoke([&]() -> Grpc::AsyncClientPtr {
    return std::move(async_client);
  }));
  EXPECT_CALL(cluster_manager_, grpcAsyncClientManager())
      .WillOnce(Invoke([&]() -> Grpc::AsyncClientManager& { return client_manager; }));
  EXPECT_CALL(client_manager, factoryForGrpcService(_, _, _))
      .WillOnce(
          Invoke([&](const envoy::api::v2::core::GrpcService&, Stats::Scope&,
                     bool) -> Grpc::AsyncClientFactoryPtr { return std::move(client_factory); }));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::debug, StrEq("response")));
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

TEST_P(WasmHttpFilterTest, Metadata) {
  setupConfig(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/metadata_cpp.wasm")));
  setupFilter();
  envoy::api::v2::core::Node node_data;
  ProtobufWkt::Value node_val;
  node_val.set_string_value("wasm_node_get_value");
  (*node_data.mutable_metadata()->mutable_fields())["wasm_node_get_key"] = node_val;
  EXPECT_CALL(local_info_, node()).WillOnce(ReturnRef(node_data));
  EXPECT_CALL(*filter_,
              scriptLog(spdlog::level::debug,
                        Eq(absl::string_view("onRequestHeaders 1 wasm_request_get_value"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::err,
                                  Eq(absl::string_view("onRequestBody wasm_node_get_value"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::warn, Eq(absl::string_view("onLog 1 /"))));
  EXPECT_CALL(*filter_, scriptLog(spdlog::level::warn, Eq(absl::string_view("onDone 1"))));
  EXPECT_CALL(
      *filter_,
      scriptLog(spdlog::level::trace,
                Eq(absl::string_view("Struct wasm_request_get_value wasm_request_get_value"))));

  request_stream_info_.metadata_.mutable_filter_metadata()->insert(
      Protobuf::MapPair<Envoy::ProtobufTypes::String, ProtobufWkt::Struct>(
          HttpFilters::HttpFilterNames::get().Wasm,
          MessageUtil::keyValueStruct("wasm_request_get_key", "wasm_request_get_value")));

  EXPECT_CALL(decoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  std::string serialized_value;
  ProtobufWkt::Value value;
  value.set_string_value("wasm_request_set_value");
  EXPECT_TRUE(value.SerializeToString(&serialized_value));
  EXPECT_CALL(request_stream_info_,
              setDynamicMetadata(HttpFilters::HttpFilterNames::get().Wasm,
                                 MapEq(std::map<std::string, std::string>{
                                     {"wasm_request_set_key", serialized_value}})));

  wasm_->start();
  Http::TestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_->decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_->decodeData(data, true));
  filter_->onDestroy();
  StreamInfo::MockStreamInfo log_stream_info;
  filter_->log(&request_headers, nullptr, nullptr, log_stream_info);
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
