#include "server/hot_restarting_parent.h"

#include "envoy/server/instance.h"

#include "common/memory/stats.h"
#include "common/network/utility.h"

namespace Envoy {
namespace Server {

using HotRestartMessage = envoy::HotRestartMessage;

HotRestartingParent::HotRestartingParent(int base_id, int restart_epoch)
    : HotRestartingBase(base_id), restart_epoch_(restart_epoch) {
  child_address_ = createDomainSocketAddress(restart_epoch_ + 1, "child");
  bindDomainSocket(restart_epoch_, "parent");
}

void HotRestartingParent::initialize(Event::Dispatcher& dispatcher, Server::Instance& server) {
  socket_event_ = dispatcher.createFileEvent(
      myDomainSocket(),
      [this](uint32_t events) -> void {
        ASSERT(events == Event::FileReadyType::Read);
        onSocketEvent();
      },
      Event::FileTriggerType::Edge, Event::FileReadyType::Read);
  internal_ = std::make_unique<Internal>(&server);
}

void HotRestartingParent::onSocketEvent() {
  std::unique_ptr<HotRestartMessage> wrapped_request;
  while ((wrapped_request = receiveHotRestartMessage(Blocking::No))) {
    if (wrapped_request->requestreply_case() == HotRestartMessage::kReply) {
      ENVOY_LOG(error, "child sent us a HotRestartMessage reply (we want requests); ignoring.");
      HotRestartMessage wrapped_reply;
      wrapped_reply.set_didnt_recognize_your_last_message(true);
      sendHotRestartMessage(child_address_, wrapped_reply);
      continue;
    }
    switch (wrapped_request->request().request_case()) {
    case HotRestartMessage::Request::kShutdownAdmin: {
      sendHotRestartMessage(child_address_, internal_->shutdownAdmin());
      break;
    }

    case HotRestartMessage::Request::kPassListenSocket: {
      sendHotRestartMessage(child_address_,
                            internal_->getListenSocketsForChild(wrapped_request->request()));
      break;
    }

    case HotRestartMessage::Request::kStats: {
      HotRestartMessage wrapped_reply;
      internal_->exportStatsToChild(wrapped_reply.mutable_reply()->mutable_stats());
      sendHotRestartMessage(child_address_, wrapped_reply);
      break;
    }

    case HotRestartMessage::Request::kDrainListeners: {
      internal_->drainListeners();
      break;
    }

    case HotRestartMessage::Request::kTerminate: {
      ENVOY_LOG(info, "shutting down due to child request");
      kill(getpid(), SIGTERM);
      break;
    }

    default: {
      ENVOY_LOG(error, "child sent us an unfamiliar type of HotRestartMessage; ignoring.");
      HotRestartMessage wrapped_reply;
      wrapped_reply.set_didnt_recognize_your_last_message(true);
      sendHotRestartMessage(child_address_, wrapped_reply);
      break;
    }
    }
  }
}

void HotRestartingParent::shutdown() { socket_event_.reset(); }

HotRestartMessage HotRestartingParent::Internal::shutdownAdmin() {
  server_->shutdownAdmin();
  HotRestartMessage wrapped_reply;
  wrapped_reply.mutable_reply()->mutable_shutdown_admin()->set_original_start_time_unix_seconds(
      server_->startTimeFirstEpoch());
  return wrapped_reply;
}

HotRestartMessage
HotRestartingParent::Internal::getListenSocketsForChild(const HotRestartMessage::Request& request) {
  HotRestartMessage wrapped_reply;
  wrapped_reply.mutable_reply()->mutable_pass_listen_socket()->set_fd(-1);
  Network::Address::InstanceConstSharedPtr addr =
      Network::Utility::resolveUrl(request.pass_listen_socket().address());
  for (const auto& listener : server_->listenerManager().listeners()) {
    if (*listener.get().socket().localAddress() == *addr && listener.get().bindToPort()) {
      wrapped_reply.mutable_reply()->mutable_pass_listen_socket()->set_fd(
          listener.get().socket().ioHandle().fd());
      break;
    }
  }
  return wrapped_reply;
}

// TODO(fredlas) if there are enough stats for stat name length to become an issue, this current
// implementation can negate the benefit of symbolized stat names by periodically reaching the
// magnitude of memory usage that they are meant to avoid, since this map holds full-string
// names. The problem can be solved by splitting the export up over many chunks.
void HotRestartingParent::Internal::exportStatsToChild(HotRestartMessage::Reply::Stats* stats) {
  for (const auto& gauge : server_->stats().gauges()) {
    (*stats->mutable_gauges())[gauge->name()] = gauge->value();
  }
  for (const auto& counter : server_->stats().counters()) {
    // The hot restart parent is expected to have stopped its normal stat exporting (and so
    // latching) by the time it begins exporting to the hot restart child.
    uint64_t latched_value = counter->latch();
    if (latched_value > 0) {
      (*stats->mutable_counter_deltas())[counter->name()] = latched_value;
    }
  }
  stats->set_memory_allocated(Memory::Stats::totalCurrentlyAllocated());
  stats->set_num_connections(server_->listenerManager().numConnections());
}

void HotRestartingParent::Internal::drainListeners() { server_->drainListeners(); }

} // namespace Server
} // namespace Envoy
