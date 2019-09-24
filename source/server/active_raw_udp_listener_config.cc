#include "server/active_raw_udp_listener_config.h"

#include "server/connection_handler_impl.h"
#include "server/well_known_names.h"

namespace Envoy {
namespace Server {

Network::ConnectionHandler::ActiveListenerPtr
ActiveRawUdpListenerFactory::createActiveUdpListener(Network::ConnectionHandler& parent,
                                                     Event::Dispatcher& dispatcher,
                                                     Network::ListenerConfig& config) const {
  return std::make_unique<ActiveUdpListener>(parent, dispatcher, config);
}

ProtobufTypes::MessagePtr ActiveRawUdpListenerConfigFactory::createEmptyConfigProto() {
  return std::make_unique<ProtobufWkt::Empty>();
}

Network::ActiveUdpListenerFactoryPtr
ActiveRawUdpListenerConfigFactory::createActiveUdpListenerFactory(
    const Protobuf::Message& /*message*/) {
  return std::make_unique<Server::ActiveRawUdpListenerFactory>();
}

std::string ActiveRawUdpListenerConfigFactory::name() { return UdpListenerNames::get().RawUdp; }

REGISTER_FACTORY(ActiveRawUdpListenerConfigFactory, Server::ActiveUdpListenerConfigFactory);

} // namespace Server
} // namespace Envoy
