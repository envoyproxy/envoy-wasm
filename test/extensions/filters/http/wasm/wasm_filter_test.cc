#include <stdio.h>

#include "extensions/filters/http/wasm/wasm_filter.h"

#include "test/test_common/wasm_base.h"

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

using BufferFunction = std::function<void(::Envoy::Buffer::Instance&)>;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

using envoy::config::core::v3::TrafficDirection;
using Envoy::Extensions::Common::Wasm::CreateContextFn;
using Envoy::Extensions::Common::Wasm::Plugin;
using Envoy::Extensions::Common::Wasm::PluginSharedPtr;
using Envoy::Extensions::Common::Wasm::Wasm;
using Envoy::Extensions::Common::Wasm::WasmHandleSharedPtr;
using proxy_wasm::ContextBase;
using GrpcService = envoy::config::core::v3::GrpcService;
using WasmFilterConfig = envoy::extensions::filters::http::wasm::v3::Wasm;

class TestFilter : public Envoy::Extensions::Common::Wasm::Context {
public:
  TestFilter(Wasm* wasm, uint32_t root_context_id,
             Envoy::Extensions::Common::Wasm::PluginSharedPtr plugin)
      : Envoy::Extensions::Common::Wasm::Context(wasm, root_context_id, plugin) {}

  void log(const Http::RequestHeaderMap* request_headers,
           const Http::ResponseHeaderMap* response_headers,
           const Http::ResponseTrailerMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info) override {
    Envoy::Extensions::Common::Wasm::Context::log(request_headers, response_headers,
                                                  response_trailers, stream_info);
  }
  MOCK_CONTEXT_LOG_;
};

class TestRoot : public Envoy::Extensions::Common::Wasm::Context {
public:
  TestRoot(Wasm* wasm, const std::shared_ptr<Plugin>& plugin) : Context(wasm, plugin) {}
  MOCK_CONTEXT_LOG_;
};

class WasmHttpFilterTest
    : public Common::Wasm::WasmHttpFilterTestBase<testing::TestWithParam<std::string>> {
public:
  WasmHttpFilterTest() = default;
  ~WasmHttpFilterTest() override = default;

  CreateContextFn createContextFn() {
    return [](Wasm* wasm, const std::shared_ptr<Plugin>& plugin) -> ContextBase* {
      return new TestRoot(wasm, plugin);
    };
  }

  void setup(const std::string& code, std::string root_id = "", std::string vm_configuration = "") {
    setupBase(GetParam(), code, createContextFn(), root_id, vm_configuration);
  }
  void setupTest(std::string root_id = "", std::string vm_configuration = "") {
    std::string code =
        GetParam() != "null"
            ? TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(
                  "{{ test_rundir }}/test/extensions/filters/http/wasm/test_data/test_cpp.wasm"))
            : "HttpWasmTestCpp";
    setupBase(GetParam(), code, createContextFn(), root_id, vm_configuration);
  }
  void setupFilter(const std::string root_id = "") { setupFilterBase<TestFilter>(root_id); }

  TestRoot& root_context() { return *static_cast<TestRoot*>(root_context_); }
  TestFilter& filter() { return *static_cast<TestFilter*>(context_.get()); }
};

INSTANTIATE_TEST_SUITE_P(Runtimes, WasmHttpFilterTest, testing::Values("v8", "null"));

// Bad code in initial config.
TEST_P(WasmHttpFilterTest, BadCode) {
  setup("bad code");
  EXPECT_EQ(wasm_, nullptr);
}

// Script touching headers only, request that is headers only.
TEST_P(WasmHttpFilterTest, HeadersOnlyRequestHeadersOnly) {
  setupTest("", "headers");
  setupFilter();
  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2 headers"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}, {"server", "envoy"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  EXPECT_THAT(request_headers.get_("newheader"), Eq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), Eq("envoy-wasm"));
  filter().onDestroy();
}

// Script touching headers only, request that is headers only.
TEST_P(WasmHttpFilterTest, HeadersOnlyRequestHeadersAndBody) {
  setupTest("", "headers");
  setupFilter();
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2 headers"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

TEST_P(WasmHttpFilterTest, HeadersStopAndContinue) {
  setupTest("", "headers");
  setupFilter();
  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2 headers"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}, {"server", "envoy-wasm-pause"}};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter().decodeHeaders(request_headers, true));
  root_context_->onTick(0);
  EXPECT_THAT(request_headers.get_("newheader"), Eq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), Eq("envoy-wasm-continue"));
  filter().onDestroy();
}

TEST_P(WasmHttpFilterTest, HeadersStopAndEndStream) {
  setupTest("", "headers");
  setupFilter();
  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2 headers"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"server", "envoy-wasm-end-stream"}};
  EXPECT_EQ(Http::FilterHeadersStatus::ContinueAndEndStream,
            filter().decodeHeaders(request_headers, true));
  root_context_->onTick(0);
  EXPECT_THAT(request_headers.get_("newheader"), Eq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), Eq("envoy-wasm-continue"));
  filter().onDestroy();
}

TEST_P(WasmHttpFilterTest, HeadersStopAndBuffer) {
  setupTest("", "headers");
  setupFilter();
  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2 headers"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"server", "envoy-wasm-stop-buffer"}};
  EXPECT_EQ(Http::FilterHeadersStatus::StopAllIterationAndBuffer,
            filter().decodeHeaders(request_headers, true));
  root_context_->onTick(0);
  EXPECT_THAT(request_headers.get_("newheader"), Eq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), Eq("envoy-wasm-continue"));
  filter().onDestroy();
}

TEST_P(WasmHttpFilterTest, HeadersStopAndWatermark) {
  setupTest("", "headers");
  setupFilter();
  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2 headers"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"server", "envoy-wasm-stop-watermark"}};
  EXPECT_EQ(Http::FilterHeadersStatus::StopAllIterationAndWatermark,
            filter().decodeHeaders(request_headers, true));
  root_context_->onTick(0);
  EXPECT_THAT(request_headers.get_("newheader"), Eq("newheadervalue"));
  EXPECT_THAT(request_headers.get_("server"), Eq("envoy-wasm-continue"));
  filter().onDestroy();
}

// Script that reads the body.
TEST_P(WasmHttpFilterTest, BodyRequestReadBody) {
  setupTest("", "body");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}, {"x-test-operation", "ReadBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

// Script that prepends and appends to the body.
TEST_P(WasmHttpFilterTest, BodyRequestPrependAndAppendToBody) {
  setupTest("", "body");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::err,
                             Eq(absl::string_view("onRequestBody prepend.hello.append"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"x-test-operation", "PrependAndAppendToBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

// Script that replaces the body.
TEST_P(WasmHttpFilterTest, BodyRequestReplaceBody) {
  setupTest("", "body");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody replace"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"x-test-operation", "ReplaceBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

// Script that removes the body.
TEST_P(WasmHttpFilterTest, BodyRequestRemoveBody) {
  setupTest("", "body");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody "))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"x-test-operation", "RemoveBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

// Script that buffers the body.
TEST_P(WasmHttpFilterTest, BodyRequestBufferBody) {
  setupTest("", "body");
  setupFilter();

  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"x-test-operation", "BufferBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, false));

  Buffer::OwnedImpl bufferedBody;
  EXPECT_CALL(decoder_callbacks_, decodingBuffer()).WillRepeatedly(Return(&bufferedBody));
  EXPECT_CALL(decoder_callbacks_, modifyDecodingBuffer(_))
      .WillRepeatedly(Invoke([&bufferedBody](BufferFunction f) { f(bufferedBody); }));

  Buffer::OwnedImpl data1("hello");
  bufferedBody.add(data1);
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))))
      .Times(1);
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndBuffer, filter().decodeData(data1, false));

  Buffer::OwnedImpl data2(" again ");
  bufferedBody.add(data2);
  EXPECT_CALL(filter(),
              log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello again "))))
      .Times(1);
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndBuffer, filter().decodeData(data2, false));

  EXPECT_CALL(filter(),
              log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello again hello"))))
      .Times(1);
  Buffer::OwnedImpl data3("hello");
  bufferedBody.add(data3);
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data3, true));

  // Verify that the response still works even though we buffered the request.
  Http::TestResponseHeaderMapImpl response_headers{{":status", "200"},
                                                   {"x-test-operation", "ReadBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().encodeHeaders(response_headers, false));
  // Should not buffer this time
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))))
      .Times(2);
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().encodeData(data1, false));
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().encodeData(data1, true));

  filter().onDestroy();
}

// Script that prepends and appends to the buffered body.
TEST_P(WasmHttpFilterTest, BodyRequestPrependAndAppendToBufferedBody) {
  setupTest("", "body");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::err,
                             Eq(absl::string_view("onRequestBody prepend.hello.append"))));
  Http::TestRequestHeaderMapImpl request_headers{
      {":path", "/"}, {"x-test-operation", "PrependAndAppendToBufferedBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

// Script that replaces the buffered body.
TEST_P(WasmHttpFilterTest, BodyRequestReplaceBufferedBody) {
  setupTest("", "body");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody replace"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"x-test-operation", "ReplaceBufferedBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

// Script that removes the buffered body.
TEST_P(WasmHttpFilterTest, BodyRequestRemoveBufferedBody) {
  setupTest("", "body");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody "))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"},
                                                 {"x-test-operation", "RemoveBufferedBody"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
}

// Script that buffers the first part of the body and streams the rest
TEST_P(WasmHttpFilterTest, BodyRequestBufferThenStreamBody) {
  setupTest("", "body");
  setupFilter();

  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));

  Buffer::OwnedImpl bufferedBody;
  EXPECT_CALL(decoder_callbacks_, decodingBuffer()).WillRepeatedly(Return(&bufferedBody));
  EXPECT_CALL(decoder_callbacks_, modifyDecodingBuffer(_))
      .WillRepeatedly(Invoke([&bufferedBody](BufferFunction f) { f(bufferedBody); }));

  Http::TestResponseHeaderMapImpl response_headers{{":status", "200"},
                                                   {"x-test-operation", "BufferTwoBodies"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().encodeHeaders(response_headers, false));

  Buffer::OwnedImpl data1("hello");
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))))
      .Times(1);
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndBuffer, filter().decodeData(data1, false));
  bufferedBody.add(data1);

  Buffer::OwnedImpl data2(", there, ");
  bufferedBody.add(data2);
  EXPECT_CALL(filter(),
              log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello, there, "))))
      .Times(1);
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndBuffer, filter().decodeData(data2, false));

  // Previous callbacks returned "Buffer" so we have buffered so far
  Buffer::OwnedImpl data3("world!");
  bufferedBody.add(data3);
  EXPECT_CALL(filter(),
              log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello, there, world!"))))
      .Times(1);
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data3, false));

  // Last callback returned "continue" so we just see individual chunks.
  Buffer::OwnedImpl data4("So it's ");
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody So it's "))))
      .Times(1);
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data4, false));

  Buffer::OwnedImpl data5("goodbye, then!");
  EXPECT_CALL(filter(),
              log_(spdlog::level::err, Eq(absl::string_view("onRequestBody goodbye, then!"))))
      .Times(1);
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data5, true));

  filter().onDestroy();
}

// Script testing AccessLog::Instance::log.
TEST_P(WasmHttpFilterTest, AccessLog) {
  setupTest("", "headers");
  setupFilter();
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders 2 headers"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::err, Eq(absl::string_view("onRequestBody hello"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onLog 2 /"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("onDone 2"))));

  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();
  StreamInfo::MockStreamInfo log_stream_info;
  filter().log(&request_headers, nullptr, nullptr, log_stream_info);
}

TEST_P(WasmHttpFilterTest, AsyncCall) {
  setupTest("", "async_call");
  setupFilter();

  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  Http::MockAsyncClientRequest request(&cluster_manager_.async_client_);
  Http::AsyncClient::Callbacks* callbacks = nullptr;
  EXPECT_CALL(cluster_manager_, get(Eq("cluster")));
  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster"));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::RequestMessagePtr& message, Http::AsyncClient::Callbacks& cb,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            EXPECT_EQ((Http::TestRequestHeaderMapImpl{{":method", "POST"},
                                                      {":path", "/"},
                                                      {":authority", "foo"},
                                                      {"content-length", "11"}}),
                      message->headers());
            EXPECT_EQ((Http::TestRequestTrailerMapImpl{{"trail", "cow"}}), *message->trailers());
            callbacks = &cb;
            return &request;
          }));

  EXPECT_CALL(filter(), log_(spdlog::level::debug, Eq("response")));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(":status -> 200")));
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter().decodeHeaders(request_headers, false));

  Http::ResponseMessagePtr response_message(new Http::ResponseMessageImpl(
      Http::ResponseHeaderMapPtr{new Http::TestResponseHeaderMapImpl{{":status", "200"}}}));
  response_message->body().reset(new Buffer::OwnedImpl("response"));

  EXPECT_NE(callbacks, nullptr);
  if (callbacks) {
    callbacks->onSuccess(request, std::move(response_message));
  }
}

TEST_P(WasmHttpFilterTest, AsyncCallAfterDestroyed) {
  setupTest("", "async_call");
  setupFilter();

  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  Http::MockAsyncClientRequest request(&cluster_manager_.async_client_);
  Http::AsyncClient::Callbacks* callbacks = nullptr;
  EXPECT_CALL(cluster_manager_, get(Eq("cluster")));
  EXPECT_CALL(cluster_manager_, httpAsyncClientForCluster("cluster"));
  EXPECT_CALL(cluster_manager_.async_client_, send_(_, _, _))
      .WillOnce(
          Invoke([&](Http::RequestMessagePtr& message, Http::AsyncClient::Callbacks& cb,
                     const Http::AsyncClient::RequestOptions&) -> Http::AsyncClient::Request* {
            EXPECT_EQ((Http::TestRequestHeaderMapImpl{{":method", "POST"},
                                                      {":path", "/"},
                                                      {":authority", "foo"},
                                                      {"content-length", "11"}}),
                      message->headers());
            EXPECT_EQ((Http::TestRequestTrailerMapImpl{{"trail", "cow"}}), *message->trailers());
            callbacks = &cb;
            return &request;
          }));

  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter().decodeHeaders(request_headers, false));

  EXPECT_CALL(request, cancel()).WillOnce([&]() { callbacks = nullptr; });

  // Destroy the Context, Plugin and VM.
  context_.reset();
  plugin_.reset();
  wasm_.reset();

  Http::ResponseMessagePtr response_message(new Http::ResponseMessageImpl(
      Http::ResponseHeaderMapPtr{new Http::TestResponseHeaderMapImpl{{":status", "200"}}}));
  response_message->body().reset(new Buffer::OwnedImpl("response"));

  // (Don't) Make the callback on the destroyed VM.
  EXPECT_EQ(callbacks, nullptr);
  if (callbacks) {
    callbacks->onSuccess(request, std::move(response_message));
  }
}

TEST_P(WasmHttpFilterTest, GrpcCall) {
  setupTest("", "grpc_call");
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
      .WillOnce(Invoke([&](const GrpcService&, Stats::Scope&, bool) -> Grpc::AsyncClientFactoryPtr {
        return std::move(client_factory);
      }));
  EXPECT_CALL(root_context(), log_(spdlog::level::debug, Eq("response")));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter().decodeHeaders(request_headers, false));

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
  setupTest("", "grpc_call");
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
      .WillOnce(Invoke([&](const GrpcService&, Stats::Scope&, bool) -> Grpc::AsyncClientFactoryPtr {
        return std::move(client_factory);
      }));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};

  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter().decodeHeaders(request_headers, false));

  EXPECT_CALL(request, cancel()).WillOnce([&]() { callbacks = nullptr; });

  // Destroy the Context, Plugin and VM.
  context_.reset();
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
  setupTest("", "metadata");
  setupFilter();
  envoy::config::core::v3::Node node_data;
  ProtobufWkt::Value node_val;
  node_val.set_string_value("wasm_node_get_value");
  (*node_data.mutable_metadata()->mutable_fields())["wasm_node_get_key"] = node_val;
  EXPECT_CALL(local_info_, node()).WillRepeatedly(ReturnRef(node_data));
  EXPECT_CALL(root_context(),
              log_(spdlog::level::debug, Eq(absl::string_view("onTick wasm_node_get_value"))));

  EXPECT_CALL(filter(),
              log_(spdlog::level::err, Eq(absl::string_view("onRequestBody wasm_node_get_value"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("header path /"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::trace,
                   Eq(absl::string_view("Struct wasm_request_get_value wasm_request_get_value"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("server is envoy-wasm"))));

  request_stream_info_.metadata_.mutable_filter_metadata()->insert(
      Protobuf::MapPair<std::string, ProtobufWkt::Struct>(
          HttpFilters::HttpFilterNames::get().Wasm,
          MessageUtil::keyValueStruct("wasm_request_get_key", "wasm_request_get_value")));

  root_context().onTick(0);

  EXPECT_CALL(encoder_callbacks_, streamInfo()).WillRepeatedly(ReturnRef(request_stream_info_));
  absl::optional<std::chrono::nanoseconds> dur = std::chrono::nanoseconds(15000000);
  EXPECT_CALL(request_stream_info_, requestComplete()).WillRepeatedly(Return(dur));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("duration is 15000000"))));
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("grpc service: test"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  Buffer::OwnedImpl data("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter().decodeData(data, true));
  filter().onDestroy();

  StreamInfo::MockStreamInfo log_stream_info;
  filter().log(&request_headers, nullptr, nullptr, log_stream_info);

  const auto& result = request_stream_info_.filterState()->getDataReadOnly<Common::Wasm::WasmState>(
      "wasm.wasm_request_set_key");
  EXPECT_EQ("wasm_request_set_value", result.value());
}

TEST_P(WasmHttpFilterTest, Property) {
  setupTest("", "property");
  setupFilter();
  envoy::config::core::v3::Node node_data;
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

  // test outputs should match inputs
  EXPECT_CALL(filter(),
              log_(spdlog::level::warn, Eq(absl::string_view("request.path: /test_context"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::warn, Eq(absl::string_view("node.metadata: sample_data"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::warn, Eq(absl::string_view("metadata: wasm_request_get_value"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("response.code: 403"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn, Eq(absl::string_view("state: wasm_value"))));

  root_context_->onTick(0);
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/test_context"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  StreamInfo::MockStreamInfo log_stream_info;
  filter().log(&request_headers, nullptr, nullptr, log_stream_info);
}

TEST_P(WasmHttpFilterTest, SharedData) {
  setupTest("", "shared_data");
  setupFilter();
  EXPECT_CALL(filter(), log_(spdlog::level::info, Eq(absl::string_view("set CasMismatch"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("get 1 shared_data_value1"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::warn, Eq(absl::string_view("get 2 shared_data_value2"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("get of bad key not found"))));
  EXPECT_CALL(filter(),
              log_(spdlog::level::debug, Eq(absl::string_view("second get of bad key not found"))));

  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  StreamInfo::MockStreamInfo log_stream_info;
  filter().log(&request_headers, nullptr, nullptr, log_stream_info);
}

TEST_P(WasmHttpFilterTest, SharedQueue) {
  setupTest("", "shared_queue");
  setupFilter();
  EXPECT_CALL(filter(),
              log_(spdlog::level::warn, Eq(absl::string_view("onRequestHeaders enqueue Ok"))));
  EXPECT_CALL(filter(), log_(spdlog::level::warn,
                             Eq(absl::string_view("onRequestHeaders not found bad_shared_queue"))));
  EXPECT_CALL(root_context(),
              log_(spdlog::level::warn, Eq(absl::string_view("onQueueReady bad token not found"))));
  EXPECT_CALL(root_context(), log_(spdlog::level::warn,
                                   Eq(absl::string_view("onQueueReady extra data not found"))));
  EXPECT_CALL(root_context(), log_(spdlog::level::info, Eq(absl::string_view("onQueueReady"))));
  EXPECT_CALL(root_context(), log_(spdlog::level::debug, Eq(absl::string_view("data data1 Ok"))));
  Http::TestRequestHeaderMapImpl request_headers{{":path", "/"}};
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
  auto token = proxy_wasm::resolveQueueForTest("vm_id", "my_shared_queue");
  root_context_->onQueueReady(token);
}

// Script using a root_id which is not registered.
TEST_P(WasmHttpFilterTest, RootIdNotRegistered) {
  setupTest();
  setupFilter();
  Http::TestRequestHeaderMapImpl request_headers;
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
}

// Script using an explicit root_id which is registered.
TEST_P(WasmHttpFilterTest, RootId1) {
  setupTest("context1");
  setupFilter("context1");
  EXPECT_CALL(filter(), log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders1 2"))));
  Http::TestRequestHeaderMapImpl request_headers;
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
}

// Script using an explicit root_id which is registered.
TEST_P(WasmHttpFilterTest, RootId2) {
  setupTest("context2");
  setupFilter("context2");
  EXPECT_CALL(filter(), log_(spdlog::level::debug, Eq(absl::string_view("onRequestHeaders2 2"))));
  Http::TestRequestHeaderMapImpl request_headers;
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter().decodeHeaders(request_headers, true));
}

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
