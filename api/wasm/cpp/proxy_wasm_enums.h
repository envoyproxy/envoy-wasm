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
enum class StreamType : EnumType { Request = 0, Response = 1 };
enum class MetadataType : EnumType {
  Request = 0,
  Response = 1,
  RequestRoute = 2,   // Immutable
  ResponseRoute = 3,  // Immutable
  Log = 4,            // Immutable
  Node = 5            // Immutable
};
enum class HeaderMapType : EnumType {
  RequestHeaders = 0,  // During the onLog callback these are immutable
  RequestTrailers = 1,  // During the onLog callback these are immutable
  ResponseHeaders = 2,  // During the onLog callback these are immutable
  ResponseTrailers = 3,  // During the onLog callback these are immutable
  GrpcCreateInitialMetadata = 4,
  GrpcReceiveInitialMetadata = 5,  // Immutable
  GrpcReceiveTrailingMetadata = 6,  // Immutable
};
enum GrpcStatus : EnumType {
  OK = 0,
  CANCELLED = 1,
  UNKNOWN = 2,
  INVALID_ARGUMENT = 3,
  DEADLINE_EXCEEDED = 4,
  NOT_FOUND = 5,
  ALREADY_EXISTS = 6,
  PERMISSION_DENIED = 7,
  UNAUTHENTICATED = 16,
  RESOURCE_EXHAUSTED = 8,
  FAILED_PRECONDITION = 9,
  ABORTED = 10,
  OUT_OF_RANGE = 11,
  UNIMPLEMENTED = 12,
  INTERNAL = 13,
  UNAVAILABLE = 14,
  DATA_LOSS = 15,
  DO_NOT_USE = 0xFFFFFFFF 
};
enum class MetricType : EnumType {
  Counter = 0,
  Gauge = 1,
  Histogram = 2,
};
