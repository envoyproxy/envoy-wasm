#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <list>

#include "envoy/common/time.h"
#include "envoy/event/deferred_deletable.h"
#include "envoy/event/dispatcher.h"
#include "envoy/event/file_event.h"
#include "envoy/event/signal.h"
#include "envoy/network/connection.h"
#include "envoy/network/connection_handler.h"
#include "envoy/network/dns.h"
#include "envoy/network/listener.h"
#include "envoy/network/transport_socket.h"
#include "envoy/ssl/context.h"

#include "test/mocks/buffer/mocks.h"
#include "test/test_common/test_time.h"

#include "gmock/gmock.h"

namespace Envoy {
namespace Event {

class MockDispatcher : public Dispatcher {
public:
  MockDispatcher();
  ~MockDispatcher() override;

  // Dispatcher
  TimeSource& timeSource() override { return time_system_; }
  Network::ConnectionPtr
  createServerConnection(Network::ConnectionSocketPtr&& socket,
                         Network::TransportSocketPtr&& transport_socket) override {
    return Network::ConnectionPtr{createServerConnection_(socket.get(), transport_socket.get())};
  }

  Network::ClientConnectionPtr
  createClientConnection(Network::Address::InstanceConstSharedPtr address,
                         Network::Address::InstanceConstSharedPtr source_address,
                         Network::TransportSocketPtr&& transport_socket,
                         const Network::ConnectionSocket::OptionsSharedPtr& options) override {
    return Network::ClientConnectionPtr{
        createClientConnection_(address, source_address, transport_socket, options)};
  }

  FileEventPtr createFileEvent(int fd, FileReadyCb cb, FileTriggerType trigger,
                               uint32_t events) override {
    return FileEventPtr{createFileEvent_(fd, cb, trigger, events)};
  }

  Filesystem::WatcherPtr createFilesystemWatcher() override {
    return Filesystem::WatcherPtr{createFilesystemWatcher_()};
  }

  Network::ListenerPtr createListener(Network::Socket& socket, Network::ListenerCallbacks& cb,
                                      bool bind_to_port,
                                      bool hand_off_restored_destination_connections) override {
    return Network::ListenerPtr{
        createListener_(socket, cb, bind_to_port, hand_off_restored_destination_connections)};
  }

  Network::ListenerPtr createUdpListener(Network::Socket& socket,
                                         Network::UdpListenerCallbacks& cb) override {
    return Network::ListenerPtr{createUdpListener_(socket, cb)};
  }

  Event::TimerPtr createTimer(Event::TimerCb cb) override {
    return Event::TimerPtr{createTimer_(cb)};
  }

  void deferredDelete(DeferredDeletablePtr&& to_delete) override {
    deferredDelete_(to_delete.get());
    if (to_delete) {
      to_delete_.push_back(std::move(to_delete));
    }
  }

  SignalEventPtr listenForSignal(int signal_num, SignalCb cb) override {
    return SignalEventPtr{listenForSignal_(signal_num, cb)};
  }

  // Event::Dispatcher
  MOCK_METHOD2(initializeStats, void(Stats::Scope&, const std::string&));
  MOCK_METHOD0(clearDeferredDeleteList, void());
  MOCK_METHOD2(createServerConnection_,
               Network::Connection*(Network::ConnectionSocket* socket,
                                    Network::TransportSocket* transport_socket));
  MOCK_METHOD4(
      createClientConnection_,
      Network::ClientConnection*(Network::Address::InstanceConstSharedPtr address,
                                 Network::Address::InstanceConstSharedPtr source_address,
                                 Network::TransportSocketPtr& transport_socket,
                                 const Network::ConnectionSocket::OptionsSharedPtr& options));
  MOCK_METHOD1(createDnsResolver,
               Network::DnsResolverSharedPtr(
                   const std::vector<Network::Address::InstanceConstSharedPtr>& resolvers));
  MOCK_METHOD4(createFileEvent_,
               FileEvent*(int fd, FileReadyCb cb, FileTriggerType trigger, uint32_t events));
  MOCK_METHOD0(createFilesystemWatcher_, Filesystem::Watcher*());
  MOCK_METHOD4(createListener_,
               Network::Listener*(Network::Socket& socket, Network::ListenerCallbacks& cb,
                                  bool bind_to_port,
                                  bool hand_off_restored_destination_connections));
  MOCK_METHOD2(createUdpListener_,
               Network::Listener*(Network::Socket& socket, Network::UdpListenerCallbacks& cb));
  MOCK_METHOD1(createTimer_, Timer*(Event::TimerCb cb));
  MOCK_METHOD1(deferredDelete_, void(DeferredDeletable* to_delete));
  MOCK_METHOD0(exit, void());
  MOCK_METHOD2(listenForSignal_, SignalEvent*(int signal_num, SignalCb cb));
  MOCK_METHOD1(post, void(std::function<void()> callback));
  MOCK_METHOD1(run, void(RunType type));
  MOCK_METHOD1(setTrackedObject, const ScopeTrackedObject*(const ScopeTrackedObject* object));
  MOCK_CONST_METHOD0(isThreadSafe, bool());
  Buffer::WatermarkFactory& getWatermarkFactory() override { return buffer_factory_; }

  GlobalTimeSystem time_system_;
  std::list<DeferredDeletablePtr> to_delete_;
  MockBufferFactory buffer_factory_;
};

class MockTimer : public Timer {
public:
  MockTimer();
  MockTimer(MockDispatcher* dispatcher);
  ~MockTimer() override;

  void invokeCallback() {
    EXPECT_TRUE(enabled_);
    enabled_ = false;
    callback_();
  }

  // Timer
  MOCK_METHOD0(disableTimer, void());
  MOCK_METHOD1(enableTimer, void(const std::chrono::milliseconds&));
  MOCK_METHOD0(enabled, bool());

  bool enabled_{};
  Event::TimerCb callback_; // TODO(mattklein123): This should be private and only called via
                            // invoke callback to clear enabled_, but that will break too many
                            // tests and can be done later.
};

class MockSignalEvent : public SignalEvent {
public:
  MockSignalEvent(MockDispatcher* dispatcher);
  ~MockSignalEvent() override;

  SignalCb callback_;
};

class MockFileEvent : public FileEvent {
public:
  MockFileEvent();
  ~MockFileEvent() override;

  MOCK_METHOD1(activate, void(uint32_t events));
  MOCK_METHOD1(setEnabled, void(uint32_t events));
};

} // namespace Event
} // namespace Envoy
