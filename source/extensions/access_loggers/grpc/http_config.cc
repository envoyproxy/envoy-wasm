#include "extensions/access_loggers/grpc/http_config.h"

#include "envoy/config/accesslog/v2/als.pb.validate.h"
#include "envoy/config/filter/accesslog/v2/accesslog.pb.validate.h"
#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "common/common/assert.h"
#include "common/common/macros.h"
#include "common/grpc/async_client_impl.h"
#include "common/protobuf/protobuf.h"

#include "extensions/access_loggers/grpc/config_utils.h"
#include "extensions/access_loggers/grpc/grpc_access_log_proto_descriptors.h"
#include "extensions/access_loggers/grpc/http_grpc_access_log_impl.h"
#include "extensions/access_loggers/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace HttpGrpc {

AccessLog::InstanceSharedPtr
HttpGrpcAccessLogFactory::createAccessLogInstance(const Protobuf::Message& config,
                                                  AccessLog::FilterPtr&& filter,
                                                  Server::Configuration::FactoryContext& context) {
  GrpcCommon::validateProtoDescriptors();

  const auto& proto_config = MessageUtil::downcastAndValidate<
      const envoy::config::accesslog::v2::HttpGrpcAccessLogConfig&>(
      config, context.messageValidationVisitor());

  return std::make_shared<HttpGrpcAccessLog>(
      std::move(filter), proto_config, context.threadLocal(),
      GrpcCommon::getGrpcAccessLoggerCacheSingleton(context));
}

ProtobufTypes::MessagePtr HttpGrpcAccessLogFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::config::accesslog::v2::HttpGrpcAccessLogConfig>();
}

std::string HttpGrpcAccessLogFactory::name() const { return AccessLogNames::get().HttpGrpc; }

/**
 * Static registration for the HTTP gRPC access log. @see RegisterFactory.
 */
REGISTER_FACTORY(HttpGrpcAccessLogFactory, Server::Configuration::AccessLogInstanceFactory);

} // namespace HttpGrpc
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
