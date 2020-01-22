#pragma once

#include "common/config/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {

/**
 * Well-known network filter names.
 * NOTE: New filters should use the well known name: envoy.filters.network.name.
 */
class NetworkFilterNameValues {
public:
  // Client ssl auth filter
  const std::string ClientSslAuth = "envoy.client_ssl_auth";
  // Echo filter
  const std::string Echo = "envoy.echo";
  // Dubbo proxy filter
  const std::string DubboProxy = "envoy.filters.network.dubbo_proxy";
  // HTTP connection manager filter
  const std::string HttpConnectionManager = "envoy.http_connection_manager";
  // Local rate limit filter
  const std::string LocalRateLimit = "envoy.filters.network.local_ratelimit";
  // Mongo proxy filter
  const std::string MongoProxy = "envoy.mongo_proxy";
  // MySQL proxy filter
  const std::string MySQLProxy = "envoy.filters.network.mysql_proxy";
  // Rate limit filter
  const std::string RateLimit = "envoy.ratelimit";
  // Redis proxy filter
  const std::string RedisProxy = "envoy.redis_proxy";
  // TCP proxy filter
  const std::string TcpProxy = "envoy.tcp_proxy";
  // Authorization filter
  const std::string ExtAuthorization = "envoy.ext_authz";
  // Kafka Broker filter
  const std::string KafkaBroker = "envoy.filters.network.kafka_broker";
  // Thrift proxy filter
  const std::string ThriftProxy = "envoy.filters.network.thrift_proxy";
  // Role based access control filter
  const std::string Rbac = "envoy.filters.network.rbac";
  // SNI Cluster filter
  const std::string SniCluster = "envoy.filters.network.sni_cluster";
  // ZooKeeper proxy filter
  const std::string ZooKeeperProxy = "envoy.filters.network.zookeeper_proxy";
  // WebAssembly filter
  const std::string Wasm = "envoy.filters.network.wasm";
};

using NetworkFilterNames = ConstSingleton<NetworkFilterNameValues>;

} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
