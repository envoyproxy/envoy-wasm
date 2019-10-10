/*
 * Wasm State Class available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#include <string>

#include "envoy/stream_info/filter_state.h"

// A simple wrapper around generic values
class WasmState : public StreamInfo::FilterState::Object {
public:
  WasmState(ProtobufWkt::Value& value) : value_(value) {}
  const ProtobufWkt::Value& value() const { return value_; }

private:
  const ProtobufWkt::Value value_;
};
