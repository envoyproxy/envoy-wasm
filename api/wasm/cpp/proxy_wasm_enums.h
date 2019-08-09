/*
 * Intrinsic enumerations available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

enum class LogLevel : EnumType { trace, debug, info, warn, error, critical };
enum class FilterHeadersStatus : EnumType { Continue = 0, StopIteration = 1 };
enum class FilterMetadataStatus : EnumType { Continue = 0 };
enum class FilterTrailersStatus : EnumType { Continue = 0, StopIteration = 1 };
enum class FilterDataStatus : EnumType {
  Continue = 0,
  StopIterationAndBuffer = 1,
  StopIterationAndWatermark = 2,
  StopIterationNoBuffer = 3
};
enum class MetadataType : EnumType {
  Request = 0,
  Response = 1,
  RequestRoute = 2,   // Immutable
  ResponseRoute = 3,  // Immutable
  Log = 4,            // Immutable
  Node = 5,           // Immutable
  Listener = 6,       // Immutable
  Cluster = 7,        // Immutable
  Expression = 8,     // The key is a string expression. Only proxy_getMetadata().
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
  downstream.tsl_version : string
 */
enum class HeaderMapType : EnumType {
  RequestHeaders = 0,  // During the onLog callback these are immutable
  RequestTrailers = 1,  // During the onLog callback these are immutable
  ResponseHeaders = 2,  // During the onLog callback these are immutable
  ResponseTrailers = 3,  // During the onLog callback these are immutable
  GrpcCreateInitialMetadata = 4,
  GrpcReceiveInitialMetadata = 5,  // Immutable
  GrpcReceiveTrailingMetadata = 6,  // Immutable
};
enum class GrpcStatus : EnumType {
  Ok = 0,
  Canceled = 1,
  Unknown = 2,
  InvalidArgument = 3,
  DeadlineExceeded = 4,
  NotFound = 5,
  AlreadyExists = 6,
  PermissionDenied = 7,
  ResourceExhausted = 8,
  FailedPrecondition = 9,
  Aborted = 10,
  OutOfRange = 11,
  Unimplemented = 12,
  Internal = 13,
  Unavailable = 14,
  DataLoss = 15,
  Unauthenticated = 16,
  MaximumValid = Unauthenticated,
  InvalidCode = -1
};
enum class MetricType : EnumType {
  Counter = 0,
  Gauge = 1,
  Histogram = 2,
};
enum class MetadataResult : EnumType {
  Ok = 0,
  StructNotFound = 1,
  FieldNotFound = 2,
  SerializationFailure = 3,
  BadType = 4,
  ParseFailure = 5,
};
