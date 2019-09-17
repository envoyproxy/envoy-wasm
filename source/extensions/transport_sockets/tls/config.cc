#include "extensions/transport_sockets/tls/config.h"

#include "envoy/api/v2/auth/cert.pb.h"
#include "envoy/api/v2/auth/cert.pb.validate.h"

#include "common/protobuf/utility.h"

#include "extensions/transport_sockets/tls/context_config_impl.h"
#include "extensions/transport_sockets/tls/ssl_socket.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {

Network::TransportSocketFactoryPtr UpstreamSslSocketFactory::createTransportSocketFactory(
    const Protobuf::Message& message,
    Server::Configuration::TransportSocketFactoryContext& context) {
  auto client_config = std::make_unique<ClientContextConfigImpl>(
      MessageUtil::downcastAndValidate<const envoy::api::v2::auth::UpstreamTlsContext&>(
          message, context.messageValidationVisitor()),
      context);
  return std::make_unique<ClientSslSocketFactory>(
      std::move(client_config), context.sslContextManager(), context.statsScope());
}

ProtobufTypes::MessagePtr UpstreamSslSocketFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::api::v2::auth::UpstreamTlsContext>();
}

REGISTER_FACTORY(UpstreamSslSocketFactory,
                 Server::Configuration::UpstreamTransportSocketConfigFactory);

Network::TransportSocketFactoryPtr DownstreamSslSocketFactory::createTransportSocketFactory(
    const Protobuf::Message& message, Server::Configuration::TransportSocketFactoryContext& context,
    const std::vector<std::string>& server_names) {
  auto server_config = std::make_unique<ServerContextConfigImpl>(
      MessageUtil::downcastAndValidate<const envoy::api::v2::auth::DownstreamTlsContext&>(
          message, context.messageValidationVisitor()),
      context);
  return std::make_unique<ServerSslSocketFactory>(
      std::move(server_config), context.sslContextManager(), context.statsScope(), server_names);
}

ProtobufTypes::MessagePtr DownstreamSslSocketFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::api::v2::auth::DownstreamTlsContext>();
}

REGISTER_FACTORY(DownstreamSslSocketFactory,
                 Server::Configuration::DownstreamTransportSocketConfigFactory);

Ssl::ContextManagerPtr SslContextManagerFactory::createContextManager(TimeSource& time_source) {
  return std::make_unique<ContextManagerImpl>(time_source);
}

REGISTER_FACTORY(SslContextManagerFactory, Ssl::ContextManagerFactory);

} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
