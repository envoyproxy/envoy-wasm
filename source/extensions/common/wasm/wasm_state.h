/*
 * Wasm State Class available to WASM/Non-WASM modules.
 */

#pragma once

#include <string>

#include "envoy/stream_info/filter_state.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

// A simple wrapper around generic values
class WasmState : public StreamInfo::FilterState::Object {
public:
  WasmState(ProtobufWkt::Value& value) : value_(value) {}
  const ProtobufWkt::Value& value() const { return value_; }

private:
  const ProtobufWkt::Value value_;
};

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy