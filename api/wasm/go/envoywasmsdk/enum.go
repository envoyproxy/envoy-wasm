package envoywasmsdk

type LogLevel int32
const (
    TRACE LogLevel = iota
	DEBUG
	INFO
	WARN
	ERROR
	CRITICAL
)

type WasmResult int32
const (
	Ok WasmResult = iota
	// The result could not be found, e.g. a provided key did not appear in a table.
	NotFound
	// An argument was bad, e.g. did not not conform to the required range.
	BadArgument
	// A protobuf could not be serialized.
	SerializationFailure
	// A protobuf could not be parsed.
	ParseFailure
	// A provided expression (e.g. "foo.bar") was illegal or unrecognized.
	BadExpression
	// A provided memory range was not legal.
	InvalidMemoryAccess
	// Data was requested from an empty container.
	Empty
	// The provided CAS did not match that of the stored data.
	CasMismatch
	// Returned result was unexpected, e.g. of the incorrect size.
	ResultMismatch
)

type FilterHeadersStatus int32
const (
	Continue FilterHeadersStatus = iota
	StopIteration
)

type HeaderMapType int32
const (
	RequestHeaders HeaderMapType = iota  // During the onLog callback these are immutable
	RequestTrailers  // During the onLog callback these are immutable
	ResponseHeaders  // During the onLog callback these are immutable
	ResponseTrailers  // During the onLog callback these are immutable
	GrpcCreateInitialMetadata
	GrpcReceiveInitialMetadata  // Immutable
	GrpcReceiveTrailingMetadata  // Immutable
	MAX
)
