#pragma once

#include <string>

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {

/**
 * Well-known access logger names.
 * NOTE: New access loggers should use the well known name: envoy.access_loggers.name.
 */
class AccessLogNameValues {
public:
  // File access log
  const std::string File = "envoy.file_access_log";
  // HTTP gRPC access log
  const std::string HttpGrpc = "envoy.http_grpc_access_log";
  // TCP gRPC access log
  const std::string TcpGrpc = "envoy.tcp_grpc_access_log";
  // WASM access log
  const std::string Wasm = "envoy.wasm_access_log";
};

using AccessLogNames = ConstSingleton<AccessLogNameValues>;

} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
