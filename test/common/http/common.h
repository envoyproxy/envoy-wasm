#pragma once

#include <functional>

#include "envoy/http/conn_pool.h"

#include "common/http/codec_client.h"

#include "test/mocks/common.h"
#include "test/mocks/event/mocks.h"

namespace Envoy {
/**
 * A fake CodecClient that 1) allows a mock codec to be passed in and 2) Allows for a destroy
 * callback.
 */
class CodecClientForTest : public Http::CodecClient {
public:
  using DestroyCb = std::function<void(CodecClient*)>;
  CodecClientForTest(CodecClient::Type type, Network::ClientConnectionPtr&& connection,
                     Http::ClientConnection* codec, DestroyCb destroy_cb,
                     Upstream::HostDescriptionConstSharedPtr host, Event::Dispatcher& dispatcher)
      : CodecClient(type, std::move(connection), host, dispatcher), destroy_cb_(destroy_cb) {
    codec_.reset(codec);
  }
  ~CodecClientForTest() override {
    if (destroy_cb_) {
      destroy_cb_(this);
    }
  }
  void raiseGoAway() { onGoAway(); }
  Event::Timer* idleTimer() { return idle_timer_.get(); }

  DestroyCb destroy_cb_;
};

/**
 * Mock callbacks used for conn pool testing.
 */
struct ConnPoolCallbacks : public Http::ConnectionPool::Callbacks {
  void onPoolReady(Http::StreamEncoder& encoder, Upstream::HostDescriptionConstSharedPtr host,
                   const StreamInfo::StreamInfo&) override {
    outer_encoder_ = &encoder;
    host_ = host;
    pool_ready_.ready();
  }

  void onPoolFailure(Http::ConnectionPool::PoolFailureReason, absl::string_view,
                     Upstream::HostDescriptionConstSharedPtr host) override {
    host_ = host;
    pool_failure_.ready();
  }

  ReadyWatcher pool_failure_;
  ReadyWatcher pool_ready_;
  Http::StreamEncoder* outer_encoder_{};
  Upstream::HostDescriptionConstSharedPtr host_;
};

/**
 * Common utility functions for HTTP tests.
 */
class HttpTestUtility {
public:
  static void addDefaultHeaders(Http::HeaderMap& headers, const std::string default_method = "GET");
};
} // namespace Envoy
