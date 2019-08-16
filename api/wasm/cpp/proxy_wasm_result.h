/*
 * Intrinsic enumerations available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#include <string>

enum class WasmResult : uint32_t {
  Ok = 0,
  NotFound = 1,
  BadArgument = 2,
  SerializationFailure = 3,
  ParseFailure = 4,
  BadExpression = 5,
  InvalidMemoryAccess = 6,
  Empty = 7,
  CasMismatch = 8,
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
  }
}
