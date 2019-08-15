/*
 * Intrinsic enumerations available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#include <string>

enum class WasmResult : uint32_t {
  Ok = 0,
  NotFound = 1,
  BadType = 2,
  BadArgument = 3,
  SerializationFailure = 4,
  ParseFailure = 5,
  NoSuchStruct = 6,
  NoSuchField = 7,
  BadExpression = 8,
  MemoryError = 9,
  Eos = 10,
  CasMismatch = 11,
};

inline std::string toString(WasmResult r) {
  switch (r) {
    case WasmResult::Ok: return "Ok";
    case WasmResult::NotFound : return "NotFound";
    case WasmResult::BadType : return "BadType";
    case WasmResult::BadArgument : return "BadArgument";
    case WasmResult::SerializationFailure : return "SerializationFailure";
    case WasmResult::ParseFailure : return "ParseFailure";
    case WasmResult::NoSuchStruct : return "NoSuchStruct";
    case WasmResult::NoSuchField : return "NoSuchField";
    case WasmResult::BadExpression : return "BadExpression";
    case WasmResult::MemoryError : return "MemoryError";
    case WasmResult::Eos : return "Eos";
    case WasmResult::CasMismatch : return "CasMismatch";
  }
}
