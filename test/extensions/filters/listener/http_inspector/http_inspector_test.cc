#include "common/common/hex.h"
#include "common/network/io_socket_handle_impl.h"

#include "extensions/filters/listener/http_inspector/http_inspector.h"

#include "test/mocks/api/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/stats/mocks.h"
#include "test/test_common/threadsafe_singleton_injector.h"

#include "gtest/gtest.h"

using testing::_;
using testing::AtLeast;
using testing::Eq;
using testing::InSequence;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::NiceMock;
using testing::Return;
using testing::ReturnNew;
using testing::ReturnRef;
using testing::SaveArg;

namespace Envoy {
namespace Extensions {
namespace ListenerFilters {
namespace HttpInspector {
namespace {

class HttpInspectorTest : public testing::Test {
public:
  HttpInspectorTest()
      : cfg_(std::make_shared<Config>(store_)),
        io_handle_(std::make_unique<Network::IoSocketHandleImpl>(42)) {}
  ~HttpInspectorTest() { io_handle_->close(); }

  void init() {
    filter_ = std::make_unique<Filter>(cfg_);

    EXPECT_CALL(cb_, socket()).WillRepeatedly(ReturnRef(socket_));
    EXPECT_CALL(socket_, detectedTransportProtocol()).WillRepeatedly(Return("raw_buffer"));
    EXPECT_CALL(cb_, dispatcher()).WillRepeatedly(ReturnRef(dispatcher_));
    EXPECT_CALL(socket_, ioHandle()).WillRepeatedly(ReturnRef(*io_handle_));

    EXPECT_CALL(dispatcher_,
                createFileEvent_(_, _, Event::FileTriggerType::Edge, Event::FileReadyType::Read))
        .WillOnce(
            DoAll(SaveArg<1>(&file_event_callback_), ReturnNew<NiceMock<Event::MockFileEvent>>()));
    filter_->onAccept(cb_);
  }

  NiceMock<Api::MockOsSysCalls> os_sys_calls_;
  TestThreadsafeSingletonInjector<Api::OsSysCallsImpl> os_calls_{&os_sys_calls_};
  Stats::IsolatedStoreImpl store_;
  ConfigSharedPtr cfg_;
  std::unique_ptr<Filter> filter_;
  Network::MockListenerFilterCallbacks cb_;
  Network::MockConnectionSocket socket_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  Event::FileReadyCb file_event_callback_;
  Network::IoHandlePtr io_handle_;
};

TEST_F(HttpInspectorTest, SkipHttpInspectForTLS) {
  filter_ = std::make_unique<Filter>(cfg_);

  EXPECT_CALL(cb_, socket()).WillRepeatedly(ReturnRef(socket_));
  EXPECT_CALL(socket_, detectedTransportProtocol()).WillRepeatedly(Return("TLS"));
  EXPECT_EQ(filter_->onAccept(cb_), Network::FilterStatus::Continue);
}

TEST_F(HttpInspectorTest, InspectHttp10) {
  init();
  absl::string_view header =
      "GET /anything HTTP/1.0\r\nhost: google.com\r\nuser-agent: curl/7.64.0\r\naccept: "
      "*/*\r\nx-forwarded-proto: http\r\nx-request-id: "
      "a52df4a0-ed00-4a19-86a7-80e5049c6c84\r\nx-envoy-expected-rq-timeout-ms: "
      "15000\r\ncontent-length: 0\r\n\r\n";

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&header](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= header.size());
        memcpy(buffer, header.data(), header.size());
        return Api::SysCallSizeResult{ssize_t(header.size()), 0};
      }));

  const std::vector<absl::string_view> alpn_protos{absl::string_view("http/1.0")};

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(alpn_protos));
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http10_found_.value());
}

TEST_F(HttpInspectorTest, InspectHttp11) {
  init();
  absl::string_view header =
      "GET /anything HTTP/1.1\r\nhost: google.com\r\nuser-agent: curl/7.64.0\r\naccept: "
      "*/*\r\nx-forwarded-proto: http\r\nx-request-id: "
      "a52df4a0-ed00-4a19-86a7-80e5049c6c84\r\nx-envoy-expected-rq-timeout-ms: "
      "15000\r\ncontent-length: 0\r\n\r\n";

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&header](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= header.size());
        memcpy(buffer, header.data(), header.size());
        return Api::SysCallSizeResult{ssize_t(header.size()), 0};
      }));

  const std::vector<absl::string_view> alpn_protos{absl::string_view("http/1.1")};

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(alpn_protos));
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http11_found_.value());
}

TEST_F(HttpInspectorTest, InvalidHttpMethod) {
  init();
  absl::string_view header = "BAD /anything HTTP/1.1\r\n";

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&header](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= header.size());
        memcpy(buffer, header.data(), header.size());
        return Api::SysCallSizeResult{ssize_t(header.size()), 0};
      }));

  const std::vector<absl::string_view> alpn_protos{absl::string_view("http/1.1")};

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(alpn_protos));
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http11_found_.value());
}

TEST_F(HttpInspectorTest, InvalidHttpRequestLine) {
  init();
  absl::string_view header = "BAD /anything HTTP/1.1";

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&header](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= header.size());
        memcpy(buffer, header.data(), header.size());
        return Api::SysCallSizeResult{ssize_t(header.size()), 0};
      }));

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(_)).Times(0);
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http_not_found_.value());
}

TEST_F(HttpInspectorTest, UnsupportedHttpProtocol) {
  init();
  absl::string_view header = "GET /anything HTTP/0.9\r\n";

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&header](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= header.size());
        memcpy(buffer, header.data(), header.size());
        return Api::SysCallSizeResult{ssize_t(header.size()), 0};
      }));

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(_)).Times(0);
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http_not_found_.value());
}

TEST_F(HttpInspectorTest, InvalidRequestLine) {
  init();
  absl::string_view header = "GET /anything HTTP/1.1 BadRequestLine\r\n";

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&header](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= header.size());
        memcpy(buffer, header.data(), header.size());
        return Api::SysCallSizeResult{ssize_t(header.size()), 0};
      }));

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(_)).Times(0);
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http_not_found_.value());
}

TEST_F(HttpInspectorTest, InspectHttp2) {
  init();

  std::string header =
      "505249202a20485454502f322e300d0a0d0a534d0d0a0d0a00000c04000000000000041000000000020000000000"
      "00040800000000000fff000100007d010500000001418aa0e41d139d09b8f0000f048860757a4ce6aa660582867a"
      "8825b650c3abb8d2e053032a2f2a408df2b4a7b3c0ec90b22d5d8749ff839d29af4089f2b585ed6950958d279a18"
      "9e03f1ca5582265f59a75b0ac3111959c7e49004908db6e83f4096f2b16aee7f4b17cd65224b22d6765926a4a7b5"
      "2b528f840b60003f";
  std::vector<uint8_t> data = Hex::decode(header);

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&data](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= data.size());
        memcpy(buffer, data.data(), data.size());
        return Api::SysCallSizeResult{ssize_t(data.size()), 0};
      }));

  const std::vector<absl::string_view> alpn_protos{absl::string_view("h2")};

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(alpn_protos));
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http2_found_.value());
}

TEST_F(HttpInspectorTest, InvalidConnectionPreface) {
  init();

  std::string header = "505249202a20485454502f322e300d0a";
  std::vector<uint8_t> data = Hex::decode(header);

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
      .WillOnce(Invoke([&data](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
        ASSERT(length >= data.size());
        memcpy(buffer, data.data(), data.size());
        return Api::SysCallSizeResult{ssize_t(data.size()), 0};
      }));

  EXPECT_CALL(socket_, setRequestedApplicationProtocols(_)).Times(0);
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().http_not_found_.value());
}

TEST_F(HttpInspectorTest, ReadError) {
  init();

  EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK)).WillOnce(InvokeWithoutArgs([]() {
    return Api::SysCallSizeResult{ssize_t(-1), ENOTSUP};
  }));
  EXPECT_CALL(cb_, continueFilterChain(true));
  file_event_callback_(Event::FileReadyType::Read);
  EXPECT_EQ(1, cfg_->stats().read_error_.value());
}

TEST_F(HttpInspectorTest, MultipleReads) {

  init();
  const std::vector<absl::string_view> alpn_protos = {absl::string_view("h2")};

  const std::string header =
      "505249202a20485454502f322e300d0a0d0a534d0d0a0d0a00000c04000000000000041000000000020000000000"
      "00040800000000000fff000100007d010500000001418aa0e41d139d09b8f0000f048860757a4ce6aa660582867a"
      "8825b650c3abb8d2e053032a2f2a408df2b4a7b3c0ec90b22d5d8749ff839d29af4089f2b585ed6950958d279a18"
      "9e03f1ca5582265f59a75b0ac3111959c7e49004908db6e83f4096f2b16aee7f4b17cd65224b22d6765926a4a7b5"
      "2b528f840b60003f";
  const std::vector<uint8_t> data = Hex::decode(header);
  {
    InSequence s;

    EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK)).WillOnce(InvokeWithoutArgs([]() {
      return Api::SysCallSizeResult{ssize_t(-1), EAGAIN};
    }));
    EXPECT_CALL(os_sys_calls_, recv(42, _, _, MSG_PEEK))
        .WillOnce(Invoke([&data](int, void* buffer, size_t length, int) -> Api::SysCallSizeResult {
          ASSERT(length >= data.size());
          memcpy(buffer, data.data(), data.size());
          return Api::SysCallSizeResult{ssize_t(data.size()), 0};
        }));
  }

  bool got_continue = false;
  EXPECT_CALL(socket_, setRequestedApplicationProtocols(alpn_protos));
  EXPECT_CALL(cb_, continueFilterChain(true)).WillOnce(InvokeWithoutArgs([&got_continue]() {
    got_continue = true;
  }));
  while (!got_continue) {
    file_event_callback_(Event::FileReadyType::Read);
  }
  EXPECT_EQ(1, cfg_->stats().http2_found_.value());
}

} // namespace
} // namespace HttpInspector
} // namespace ListenerFilters
} // namespace Extensions
} // namespace Envoy
