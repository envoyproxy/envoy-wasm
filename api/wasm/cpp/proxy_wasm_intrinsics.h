/*
 * API Available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE __attribute__((used)) __attribute__((visibility("default")))
#endif

#include <cstdint>
#include <string_view>
using StringView = std::string_view;
#include <optional>
template <typename T> using Optional = std::optional<T>;

#include "proxy_wasm_enums.h"
#include "proxy_wasm_result.h"
#include "proxy_wasm_metadata.h"
#include "proxy_wasm_enums.h"
#include "proxy_wasm_externs.h"
#ifndef EMSCRIPTEN_PROTOBUF_LITE
#include "proxy_wasm_intrinsics.pb.h"
#else
#include "proxy_wasm_intrinsics_lite.pb.h"
#endif
#include "proxy_wasm_api.h"
