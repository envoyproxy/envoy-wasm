#pragma once

#include <string>

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Extensions {
namespace ListenerFilters {

/**
 * Well-known listener filter names.
 * NOTE: New filters should use the well known name: envoy.filters.listener.name.
 */
class ListenerFilterNameValues {
public:
  // HTTP Inspector listener filter
  const std::string HttpInspector = "envoy.listener.http_inspector";
  // Original destination listener filter
  const std::string OriginalDst = "envoy.listener.original_dst";
  // Original source listener filter
  const std::string OriginalSrc = "envoy.listener.original_src";
  // Proxy Protocol listener filter
  const std::string ProxyProtocol = "envoy.listener.proxy_protocol";
  // TLS Inspector listener filter
  const std::string TlsInspector = "envoy.listener.tls_inspector";
};

using ListenerFilterNames = ConstSingleton<ListenerFilterNameValues>;

} // namespace ListenerFilters
} // namespace Extensions
} // namespace Envoy
