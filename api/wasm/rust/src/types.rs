pub enum WasmResult {
  Ok,
  // The result could not be found, e.g. a provided key did not appear in a table.
  NotFound,
  // An argument was bad, e.g. did not not conform to the required range.
  BadArgument,
  // A protobuf could not be serialized.
  SerializationFailure,
  // A protobuf could not be parsed.
  ParseFailure,
  // A provided expression (e.g. "foo.bar") was illegal or unrecognized.
  BadExpression,
  // A provided memory range was not legal.
  InvalidMemoryAccess,
  // Data was requested from an empty container.
  Empty,
  // The provided CAS did not match that of the stored data.
  CasMismatch,
  // Returned result was unexpected, e.g. of the incorrect size.
  ResultMismatch,
  // Internal failure: trying check logs of the surrounding system.
  InternalFailure,
  // The connection/stream/pipe was broken/closed unexpectedly.
  BrokenConnection,
}

pub enum HeaderMapType {
  RequestHeaders,   // During the onLog callback these are immutable
  RequestTrailers,  // During the onLog callback these are immutable
  ResponseHeaders,  // During the onLog callback these are immutable
  ResponseTrailers, // During the onLog callback these are immutable
  GrpcCreateInitialMetadata,
  GrpcReceiveInitialMetadata,  // Immutable
  GrpcReceiveTrailingMetadata, // Immutable
  HttpCallResponseHeaders,     // Immutable
  HttpCallResponseTrailers,    // Immutable
  MAX,
}

pub enum BufferType {
  HttpRequestBody,       // During the onLog callback these are immutable
  HttpResponseBody,      // During the onLog callback these are immutable
  NetworkDownstreamData, // During the onLog callback these are immutable
  NetworkUpstreamData,   // During the onLog callback these are immutable
  HttpCallResponseBody,  // Immutable
  GrpcReceiveBuffer,     // Immutable
  MAX,
}

pub enum BufferFlags {
  EndOfStream,
}

pub enum FilterStatus {
  Continue,
  StopIteration,
}

pub enum FilterHeadersStatus {
  Continue,
  StopIteration,
}

pub enum FilterMetadataStatus {
  Continue,
}

pub enum FilterTrailersStatus {
  Continue,
  StopIteration,
}

pub enum FilterDataStatus {
  Continue,
  StopIterationAndBuffer,
  StopIterationAndWatermark,
  StopIterationNoBuffer,
}

pub enum GrpcStatus {
  Ok,
  Canceled,
  Unknown,
  InvalidArgument,
  DeadlineExceeded,
  NotFound,
  AlreadyExists,
  PermissionDenied,
  ResourceExhausted,
  FailedPrecondition,
  Aborted,
  OutOfRange,
  Unimplemented,
  Internal,
  Unavailable,
  DataLoss,
  Unauthenticated,
  MaximumValid,
  InvalidCode,
}

pub enum MetricType {
  Counter,
  Gauge,
  Histogram,
}

pub enum PeerType {
  Unknown,
  Local,
  Remote,
}

pub fn filter_trailer_status_to_int(status: FilterTrailersStatus) -> u32 {
  match status {
    FilterTrailersStatus::Continue => 0,
    FilterTrailersStatus::StopIteration => 1,
  }
}

pub fn filter_header_status_to_int(status: FilterHeadersStatus) -> u32 {
  match status {
    FilterHeadersStatus::Continue => 0,
    FilterHeadersStatus::StopIteration => 1,
  }
}

pub fn filter_data_status_to_int(status: FilterDataStatus) -> u32 {
  match status {
    FilterDataStatus::Continue => 0,
    FilterDataStatus::StopIterationAndBuffer => 1,
    FilterDataStatus::StopIterationAndWatermark => 2,
    FilterDataStatus::StopIterationNoBuffer => 3,
  }
}

pub fn filter_metadata_status_to_int(status: FilterMetadataStatus) -> u32 {
  match status {
    FilterMetadataStatus::Continue => 0,
  }
}

pub fn filter_status_to_int(status: FilterStatus) -> u32 {
  match status {
    FilterStatus::Continue => 0,
    FilterStatus::StopIteration => 1,
  }
}
