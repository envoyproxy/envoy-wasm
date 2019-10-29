/*
 * Intrinsic enumerations available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#include <string>

enum class WasmResult : uint32_t {
  Ok = 0,
  // The result could not be found, e.g. a provided key did not appear in a table.
  NotFound = 1,
  // An argument was bad, e.g. did not not conform to the required range.
  BadArgument = 2,
  // A protobuf could not be serialized.
  SerializationFailure = 3,
  // A protobuf could not be parsed.
  ParseFailure = 4,
  // A provided expression (e.g. "foo.bar") was illegal or unrecognized.
  BadExpression = 5,
  // A provided memory range was not legal.
  InvalidMemoryAccess = 6,
  // Data was requested from an empty container.
  Empty = 7,
  // The provided CAS did not match that of the stored data.
  CasMismatch = 8,
  // Returned result was unexpected, e.g. of the incorrect size.
  ResultMismatch = 9,
};

inline std::string toString(WasmResult r) {
  switch (r) {
    case WasmResult::Ok: return "Ok";
    case WasmResult::NotFound : return "NotFound";
    case WasmResult::BadArgument : return "BadArgument";
    case WasmResult::SerializationFailure : return "SerializationFailure";
    case WasmResult::ParseFailure : return "ParseFailure";
    case WasmResult::BadExpression : return "BadExpression";
    case WasmResult::InvalidMemoryAccess : return "InvalidMemoryAccess";
    case WasmResult::Empty : return "Empty";
    case WasmResult::CasMismatch : return "CasMismatch";
    case WasmResult::ResultMismatch : return "ResultMismatch";
  }
}
