use std::convert::TryFrom;
use std::fmt;

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
  MAX,
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

impl TryFrom<u32> for WasmResult {
  type Error = String;
  fn try_from(n: u32) -> Result<Self, Self::Error> {
    match n {
      0 => Ok(WasmResult::Ok),
      1 => Ok(WasmResult::NotFound),
      2 => Ok(WasmResult::BadArgument),
      3 => Ok(WasmResult::SerializationFailure),
      4 => Ok(WasmResult::ParseFailure),
      5 => Ok(WasmResult::BadExpression),
      6 => Ok(WasmResult::InvalidMemoryAccess),
      7 => Ok(WasmResult::Empty),
      8 => Ok(WasmResult::CasMismatch),
      9 => Ok(WasmResult::ResultMismatch),
      10 => Ok(WasmResult::InternalFailure),
      11 => Ok(WasmResult::BrokenConnection),
      _ => Err(format!("invalid status: {}", n)),
    }
  }
}

impl std::fmt::Display for WasmResult {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    match *self {
      WasmResult::Ok => write!(f, "OK"),
      WasmResult::NotFound => write!(f, "NotFound"),
      WasmResult::BadArgument => write!(f, "BadArgument"),
      WasmResult::SerializationFailure => write!(f, "SerializationFailure"),
      WasmResult::ParseFailure => write!(f, "ParseFailure"),
      WasmResult::BadExpression => write!(f, "BadExpression"),
      WasmResult::InvalidMemoryAccess => write!(f, "InvalidMemoryAccess"),
      WasmResult::Empty => write!(f, "Empty"),
      WasmResult::CasMismatch => write!(f, "CasMismatch"),
      WasmResult::ResultMismatch => write!(f, "ResultMismatch"),
      WasmResult::InternalFailure => write!(f, "internalFailure"),
      WasmResult::BrokenConnection => write!(f, "BrokenConnection"),
      WasmResult::MAX => write!(f, "unimplemented"),
    }
  }
}

pub fn filter_trailer_status_to_int(status: FilterTrailersStatus) -> u32 {
  status as u32
}

pub fn filter_header_status_to_int(status: FilterHeadersStatus) -> u32 {
  status as u32
}

pub fn filter_data_status_to_int(status: FilterDataStatus) -> u32 {
  status as u32
}

pub fn filter_metadata_status_to_int(status: FilterMetadataStatus) -> u32 {
  status as u32
}

pub fn filter_status_to_int(status: FilterStatus) -> u32 {
  status as u32
}

pub fn header_map_type_to_int(htype: HeaderMapType) -> u32 {
  htype as u32
}
