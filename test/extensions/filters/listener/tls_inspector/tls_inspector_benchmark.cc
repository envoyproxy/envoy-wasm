#include <vector>

#include "common/network/io_socket_handle_impl.h"
#include "common/network/listen_socket_impl.h"

#include "extensions/filters/listener/tls_inspector/tls_inspector.h"

#include "test/extensions/filters/listener/tls_inspector/tls_utility.h"
#include "test/mocks/api/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/stats/mocks.h"
#include "test/test_common/threadsafe_singleton_injector.h"

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"
#include "openssl/ssl.h"

using testing::NiceMock;

namespace Envoy {
namespace Extensions {
namespace ListenerFilters {
namespace TlsInspector {

class FastMockListenerFilterCallbacks : public Network::MockListenerFilterCallbacks {
public:
  FastMockListenerFilterCallbacks(Network::ConnectionSocket& socket, Event::Dispatcher& dispatcher)
      : socket_(socket), dispatcher_(dispatcher) {}
  Network::ConnectionSocket& socket() override { return socket_; }
  Event::Dispatcher& dispatcher() override { return dispatcher_; }
  void continueFilterChain(bool success) override { RELEASE_ASSERT(success, ""); }

  Network::ConnectionSocket& socket_;
  Event::Dispatcher& dispatcher_;
};

// Don't inherit from the mock implementation at all, because this is instantiated
// in the hot loop.
class FastMockFileEvent : public Event::FileEvent {
  void activate(uint32_t) override {}
  void setEnabled(uint32_t) override {}
};

class FastMockDispatcher : public Event::MockDispatcher {
public:
  Event::FileEventPtr createFileEvent(int, Event::FileReadyCb cb, Event::FileTriggerType,
                                      uint32_t) override {
    file_event_callback_ = cb;
    return std::make_unique<FastMockFileEvent>();
  }

  Event::FileReadyCb file_event_callback_;
};

class FastMockOsSysCalls : public Api::MockOsSysCalls {
public:
  FastMockOsSysCalls(const std::vector<uint8_t>& client_hello) : client_hello_(client_hello) {}

  Api::SysCallSizeResult recv(int, void* buffer, size_t length, int) override {
    RELEASE_ASSERT(length >= client_hello_.size(), "");
    memcpy(buffer, client_hello_.data(), client_hello_.size());
    return Api::SysCallSizeResult{ssize_t(client_hello_.size()), 0};
  }

  const std::vector<uint8_t> client_hello_;
};

static void BM_TlsInspector(benchmark::State& state) {
  NiceMock<FastMockOsSysCalls> os_sys_calls(
      Tls::Test::generateClientHello("example.com", "\x02h2\x08http/1.1"));
  TestThreadsafeSingletonInjector<Api::OsSysCallsImpl> os_calls{&os_sys_calls};
  NiceMock<Stats::MockStore> store;
  ConfigSharedPtr cfg(std::make_shared<Config>(store));
  Network::IoHandlePtr io_handle = std::make_unique<Network::IoSocketHandleImpl>();
  Network::ConnectionSocketImpl socket(std::move(io_handle), nullptr, nullptr);
  NiceMock<FastMockDispatcher> dispatcher;
  FastMockListenerFilterCallbacks cb(socket, dispatcher);

  for (auto _ : state) {
    Filter filter(cfg);
    filter.onAccept(cb);
    dispatcher.file_event_callback_(Event::FileReadyType::Read);
    RELEASE_ASSERT(socket.detectedTransportProtocol() == "tls", "");
    RELEASE_ASSERT(socket.requestedServerName() == "example.com", "");
    RELEASE_ASSERT(socket.requestedApplicationProtocols().size() == 2 &&
                       socket.requestedApplicationProtocols().front() == "h2",
                   "");
    socket.setDetectedTransportProtocol("");
    socket.setRequestedServerName("");
    socket.setRequestedApplicationProtocols({});
  }
}

BENCHMARK(BM_TlsInspector)->Unit(benchmark::kMicrosecond);

} // namespace TlsInspector
} // namespace ListenerFilters
} // namespace Extensions
} // namespace Envoy

// Boilerplate main(), which discovers benchmarks in the same file and runs them.
int main(int argc, char** argv) {
  Envoy::Thread::MutexBasicLockable lock;
  Envoy::Logger::Context logging_context(spdlog::level::warn,
                                         Envoy::Logger::Logger::DEFAULT_LOG_FORMAT, lock);

  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }
  benchmark::RunSpecifiedBenchmarks();
}
