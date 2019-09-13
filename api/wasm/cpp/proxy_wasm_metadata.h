/*
 * Intrinsic enumerations available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

enum class MetadataType : int32_t {
  Request = 0,
  Response = 1,
  RequestRoute = 2,   // Immutable
  ResponseRoute = 3,  // Immutable
  Log = 4,            // Immutable
  Node = 5,           // Immutable
  Listener = 6,       // Immutable
  Cluster = 7,        // Immutable
  Expression = 8,     // The key is a string expression. Only proxy_getMetadata().
  MAX = 8,
};
/*
  Expression and their types:

  request.protocol : string
  response.protocol : string
  request.destination_port : int32
  response.destination_port : int32
  request.response_code : int32
  response.response_code : int32
  upstream.tls_version : string
  request.tls_version : string
  response.tls_version : string
  request.peer_certificate_presented : bool
  response.peer_certificate_presented : bool
  plugin.direction : int32 (enum PluginDirection)
 */

enum class HeaderMapType : int32_t {
  RequestHeaders = 0,  // During the onLog callback these are immutable
  RequestTrailers = 1,  // During the onLog callback these are immutable
  ResponseHeaders = 2,  // During the onLog callback these are immutable
  ResponseTrailers = 3,  // During the onLog callback these are immutable
  GrpcCreateInitialMetadata = 4,
  GrpcReceiveInitialMetadata = 5,  // Immutable
  GrpcReceiveTrailingMetadata = 6,  // Immutable
  MAX = 6,
};
