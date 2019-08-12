#include <chrono>
#include <memory>

#include "envoy/config/filter/http/buffer/v2/buffer.pb.h"
#include "envoy/event/dispatcher.h"

#include "common/http/header_map_impl.h"
#include "common/runtime/runtime_impl.h"

#include "extensions/filters/http/buffer/buffer_filter.h"
#include "extensions/filters/http/well_known_names.h"

#include "test/mocks/buffer/mocks.h"
#include "test/mocks/http/mocks.h"
#include "test/mocks/init/mocks.h"
#include "test/mocks/local_info/mocks.h"
#include "test/mocks/protobuf/mocks.h"
#include "test/mocks/runtime/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/test_common/printers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::DoAll;
using testing::InSequence;
using testing::NiceMock;
using testing::Return;
using testing::SaveArg;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace BufferFilter {

class BufferFilterTest : public testing::Test {
public:
  BufferFilterConfigSharedPtr setupConfig() {
    envoy::config::filter::http::buffer::v2::Buffer proto_config;
    proto_config.mutable_max_request_bytes()->set_value(1024 * 1024);
    return std::make_shared<BufferFilterConfig>(proto_config);
  }

  BufferFilterTest() : config_(setupConfig()), filter_(config_), api_(Api::createApiForTest()) {
    filter_.setDecoderFilterCallbacks(callbacks_);

    // Create a runtime loader, so that tests can manually manipulate runtime
    // guarded features.
    envoy::config::bootstrap::v2::LayeredRuntime config;
    config.add_layers()->mutable_admin_layer();
    loader_ = std::make_unique<Runtime::ScopedLoaderSingleton>(Runtime::LoaderPtr{
        new Runtime::LoaderImpl(dispatcher_, tls_, config, local_info_, init_manager_, store_,
                                generator_, validation_visitor_, *api_)});
  }

  void routeLocalConfig(const Router::RouteSpecificFilterConfig* route_settings,
                        const Router::RouteSpecificFilterConfig* vhost_settings) {
    ON_CALL(callbacks_.route_->route_entry_, perFilterConfig(HttpFilterNames::get().Buffer))
        .WillByDefault(Return(route_settings));
    ON_CALL(callbacks_.route_->route_entry_.virtual_host_,
            perFilterConfig(HttpFilterNames::get().Buffer))
        .WillByDefault(Return(vhost_settings));
  }

  NiceMock<Http::MockStreamDecoderFilterCallbacks> callbacks_;
  BufferFilterConfigSharedPtr config_;
  BufferFilter filter_;
  Event::MockDispatcher dispatcher_;
  NiceMock<ThreadLocal::MockInstance> tls_;
  Stats::IsolatedStoreImpl store_;
  Runtime::MockRandomGenerator generator_;
  Api::ApiPtr api_;
  NiceMock<LocalInfo::MockLocalInfo> local_info_;
  Init::MockManager init_manager_;
  NiceMock<ProtobufMessage::MockValidationVisitor> validation_visitor_;
  std::unique_ptr<Runtime::ScopedLoaderSingleton> loader_;
};

TEST_F(BufferFilterTest, HeaderOnlyRequest) {
  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_.decodeHeaders(headers, true));
}

TEST_F(BufferFilterTest, TestMetadata) {
  Http::MetadataMap metadata_map{{"metadata", "metadata"}};
  EXPECT_EQ(Http::FilterMetadataStatus::Continue, filter_.decodeMetadata(metadata_map));
}

TEST_F(BufferFilterTest, RequestWithData) {
  InSequence s;

  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_.decodeHeaders(headers, false));

  Buffer::OwnedImpl data1("hello");
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndBuffer, filter_.decodeData(data1, false));

  Buffer::OwnedImpl data2(" world");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_.decodeData(data2, true));
}

TEST_F(BufferFilterTest, TxResetAfterEndStream) {
  InSequence s;

  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_.decodeHeaders(headers, false));

  Buffer::OwnedImpl data1("hello");
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndBuffer, filter_.decodeData(data1, false));

  Buffer::OwnedImpl data2(" world");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_.decodeData(data2, true));

  // It's possible that the stream will be reset on the TX side even after RX end stream. Mimic
  // that here.
  filter_.onDestroy();
}

TEST_F(BufferFilterTest, ContentLengthPopulation) {
  InSequence s;

  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_.decodeHeaders(headers, false));

  Buffer::OwnedImpl data1("hello");
  EXPECT_EQ(Http::FilterDataStatus::StopIterationAndBuffer, filter_.decodeData(data1, false));

  Buffer::OwnedImpl data2(" world");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_.decodeData(data2, true));
  ASSERT_NE(headers.ContentLength(), nullptr);
  EXPECT_EQ(headers.ContentLength()->value().getStringView(), "11");
}

TEST_F(BufferFilterTest, ContentLengthPopulationAlreadyPresent) {
  InSequence s;

  Http::TestHeaderMapImpl headers{{"content-length", "3"}};
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_.decodeHeaders(headers, false));

  Buffer::OwnedImpl data("foo");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_.decodeData(data, true));
  ASSERT_NE(headers.ContentLength(), nullptr);
  EXPECT_EQ(headers.ContentLength()->value().getStringView(), "3");
}

TEST_F(BufferFilterTest, ContentLengthPopulationRuntimeGuard) {
  InSequence s;
  Runtime::LoaderSingleton::getExisting()->mergeValues(
      {{"envoy.reloadable_features.buffer_filter_populate_content_length", "false"}});

  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_.decodeHeaders(headers, false));

  Buffer::OwnedImpl data("foo");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_.decodeData(data, true));
  EXPECT_EQ(headers.ContentLength(), nullptr);
}

TEST_F(BufferFilterTest, RouteConfigOverride) {
  envoy::config::filter::http::buffer::v2::BufferPerRoute route_cfg;
  auto* buf = route_cfg.mutable_buffer();
  buf->mutable_max_request_bytes()->set_value(123);
  envoy::config::filter::http::buffer::v2::BufferPerRoute vhost_cfg;
  vhost_cfg.set_disabled(true);
  BufferFilterSettings route_settings(route_cfg);
  BufferFilterSettings vhost_settings(vhost_cfg);
  routeLocalConfig(&route_settings, &vhost_settings);

  EXPECT_CALL(callbacks_, setDecoderBufferLimit(123ULL));

  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_.decodeHeaders(headers, false));

  filter_.onDestroy();
}

TEST_F(BufferFilterTest, VHostConfigOverride) {
  envoy::config::filter::http::buffer::v2::BufferPerRoute vhost_cfg;
  auto* buf = vhost_cfg.mutable_buffer();
  buf->mutable_max_request_bytes()->set_value(789);
  BufferFilterSettings vhost_settings(vhost_cfg);
  routeLocalConfig(nullptr, &vhost_settings);

  EXPECT_CALL(callbacks_, setDecoderBufferLimit(789ULL));

  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration, filter_.decodeHeaders(headers, false));
  filter_.onDestroy();
}

TEST_F(BufferFilterTest, RouteDisabledConfigOverride) {
  envoy::config::filter::http::buffer::v2::BufferPerRoute vhost_cfg;
  vhost_cfg.set_disabled(true);
  BufferFilterSettings vhost_settings(vhost_cfg);
  routeLocalConfig(nullptr, &vhost_settings);

  Http::TestHeaderMapImpl headers;
  EXPECT_EQ(Http::FilterHeadersStatus::Continue, filter_.decodeHeaders(headers, false));
  Buffer::OwnedImpl data1("hello");
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_.decodeData(data1, false));
  EXPECT_EQ(Http::FilterDataStatus::Continue, filter_.decodeData(data1, true));
}

} // namespace BufferFilter
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
