#include "extensions/filters/network/mongo_proxy/config.h"

#include "envoy/config/filter/network/mongo_proxy/v2/mongo_proxy.pb.validate.h"
#include "envoy/network/connection.h"
#include "envoy/registry/registry.h"

#include "common/common/fmt.h"
#include "common/config/filter_json.h"

#include "extensions/filters/network/mongo_proxy/proxy.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MongoProxy {

Network::FilterFactoryCb MongoProxyFilterConfigFactory::createFilterFactoryFromProtoTyped(
    const envoy::config::filter::network::mongo_proxy::v2::MongoProxy& proto_config,
    Server::Configuration::FactoryContext& context) {

  ASSERT(!proto_config.stat_prefix().empty());

  const std::string stat_prefix = fmt::format("mongo.{}", proto_config.stat_prefix());
  AccessLogSharedPtr access_log;
  if (!proto_config.access_log().empty()) {
    access_log.reset(new AccessLog(proto_config.access_log(), context.accessLogManager(),
                                   context.dispatcher().timeSource()));
  }

  Filters::Common::Fault::FaultDelayConfigSharedPtr fault_config;
  if (proto_config.has_delay()) {
    fault_config = std::make_shared<Filters::Common::Fault::FaultDelayConfig>(proto_config.delay());
  }

  auto stats = std::make_shared<MongoStats>(context.scope(), stat_prefix);
  const bool emit_dynamic_metadata = proto_config.emit_dynamic_metadata();
  return [stat_prefix, &context, access_log, fault_config, emit_dynamic_metadata,
          stats](Network::FilterManager& filter_manager) -> void {
    filter_manager.addFilter(std::make_shared<ProdProxyFilter>(
        stat_prefix, context.scope(), context.runtime(), access_log, fault_config,
        context.drainDecision(), context.dispatcher().timeSource(), emit_dynamic_metadata, stats));
  };
}

Network::FilterFactoryCb
MongoProxyFilterConfigFactory::createFilterFactory(const Json::Object& json_config,
                                                   Server::Configuration::FactoryContext& context) {
  envoy::config::filter::network::mongo_proxy::v2::MongoProxy proto_config;
  Config::FilterJson::translateMongoProxy(json_config, proto_config);
  return createFilterFactoryFromProtoTyped(proto_config, context);
}

/**
 * Static registration for the mongo filter. @see RegisterFactory.
 */
REGISTER_FACTORY(MongoProxyFilterConfigFactory,
                 Server::Configuration::NamedNetworkFilterConfigFactory);

} // namespace MongoProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
