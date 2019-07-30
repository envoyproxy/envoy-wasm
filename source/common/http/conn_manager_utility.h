#pragma once

#include <atomic>
#include <cstdint>

#include "envoy/http/header_map.h"
#include "envoy/network/connection.h"

#include "common/http/conn_manager_impl.h"

namespace Envoy {
namespace Http {

/**
 * Connection manager utilities split out for ease of testing.
 */
class ConnectionManagerUtility {
public:
  /**
   * Determine the next protocol to used based both on ALPN as well as protocol inspection.
   * @param connection supplies the connection to determine a protocol for.
   * @param data supplies the currently available read data on the connection.
   */
  static std::string determineNextProtocol(Network::Connection& connection,
                                           const Buffer::Instance& data);

  /**
   * Create an HTTP codec given the connection and the beginning of the incoming data.
   * @param connection supplies the connection.
   * @param data supplies the initial data supplied by the client.
   * @param callbacks supplies the codec callbacks.
   * @param scope supplies the stats scope for codec stats.
   * @param http1_settings supplies the HTTP/1 settings to use if HTTP/1 is chosen.
   * @param http2_settings supplies the HTTP/2 settings to use if HTTP/2 is chosen.
   */
  static ServerConnectionPtr
  autoCreateCodec(Network::Connection& connection, const Buffer::Instance& data,
                  ServerConnectionCallbacks& callbacks, Stats::Scope& scope,
                  const Http1Settings& http1_settings, const Http2Settings& http2_settings,
                  const uint32_t max_request_headers_kb);

  /**
   * Mutates request headers in various ways. This functionality is broken out because of its
   * complexity for ease of testing. See the method itself for detailed comments on what
   * mutations are performed.
   *
   * Note this function may be called twice on the response path if there are
   * 100-Continue headers.
   *
   * @return the final trusted remote address. This depends on various settings and the
   *         existence of the x-forwarded-for header. Again see the method for more details.
   */
  static Network::Address::InstanceConstSharedPtr
  mutateRequestHeaders(HeaderMap& request_headers, Network::Connection& connection,
                       ConnectionManagerConfig& config, const Router::Config& route_config,
                       Runtime::RandomGenerator& random, const LocalInfo::LocalInfo& local_info);

  static void mutateResponseHeaders(HeaderMap& response_headers, const HeaderMap* request_headers,
                                    const std::string& via);

  // Sanitize the path in the header map if forced by config.
  // Side affect: the string view of Path header is invalidated.
  // Return false if error happens during the sanitization.
  static bool maybeNormalizePath(HeaderMap& request_headers, const ConnectionManagerConfig& config);

  /**
   * Mutate request headers if request needs to be traced.
   */
  static void mutateTracingRequestHeader(HeaderMap& request_headers, Runtime::Loader& runtime,
                                         ConnectionManagerConfig& config,
                                         const Router::Route* route);

private:
  static void mutateXfccRequestHeader(HeaderMap& request_headers, Network::Connection& connection,
                                      ConnectionManagerConfig& config);
};

} // namespace Http
} // namespace Envoy
