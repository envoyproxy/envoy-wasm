#include "extensions/quic_listeners/quiche/quic_transport_socket_factory.h"

#include "envoy/api/v2/auth/cert.pb.h"
#include "envoy/api/v2/auth/cert.pb.validate.h"

#include "extensions/transport_sockets/tls/context_config_impl.h"

namespace Envoy {
namespace Quic {

Network::TransportSocketFactoryPtr
QuicServerTransportSocketConfigFactory::createTransportSocketFactory(
    const Protobuf::Message& config, Server::Configuration::TransportSocketFactoryContext& context,
    const std::vector<std::string>& /*server_names*/) {
  auto server_config = std::make_unique<Extensions::TransportSockets::Tls::ServerContextConfigImpl>(
      MessageUtil::downcastAndValidate<const envoy::api::v2::auth::DownstreamTlsContext&>(
          config, context.messageValidationVisitor()),
      context);
  return std::make_unique<QuicServerTransportSocketFactory>(std::move(server_config));
}

ProtobufTypes::MessagePtr QuicServerTransportSocketConfigFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::api::v2::auth::DownstreamTlsContext>();
}

Network::TransportSocketFactoryPtr
QuicClientTransportSocketConfigFactory::createTransportSocketFactory(
    const Protobuf::Message& config,
    Server::Configuration::TransportSocketFactoryContext& context) {
  auto client_config = std::make_unique<Extensions::TransportSockets::Tls::ClientContextConfigImpl>(
      MessageUtil::downcastAndValidate<const envoy::api::v2::auth::UpstreamTlsContext&>(
          config, context.messageValidationVisitor()),
      context);
  return std::make_unique<QuicClientTransportSocketFactory>(std::move(client_config));
}

ProtobufTypes::MessagePtr QuicClientTransportSocketConfigFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::api::v2::auth::UpstreamTlsContext>();
}

REGISTER_FACTORY(QuicServerTransportSocketConfigFactory,
                 Server::Configuration::DownstreamTransportSocketConfigFactory);

REGISTER_FACTORY(QuicClientTransportSocketConfigFactory,
                 Server::Configuration::UpstreamTransportSocketConfigFactory);

} // namespace Quic
} // namespace Envoy
