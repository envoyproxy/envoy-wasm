/*
 * Intrinsic enumerations available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

enum class LogLevel : int32_t { trace, debug, info, warn, error, critical };
enum class FilterStatus : int32_t { Continue = 0, StopIteration = 1 };
enum class FilterHeadersStatus : int32_t { Continue = 0, StopIteration = 1 };
enum class FilterMetadataStatus : int32_t { Continue = 0 };
enum class FilterTrailersStatus : int32_t { Continue = 0, StopIteration = 1 };
enum class FilterDataStatus : int32_t {
  Continue = 0,
  StopIterationAndBuffer = 1,
  StopIterationAndWatermark = 2,
  StopIterationNoBuffer = 3
};
enum class GrpcStatus : int32_t {
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
enum class MetricType : int32_t {
  Counter = 0,
  Gauge = 1,
  Histogram = 2,
};
enum class PeerType : int32_t {
  Unknown = 0,
  Local = 1,
  Remote = 2,
};
