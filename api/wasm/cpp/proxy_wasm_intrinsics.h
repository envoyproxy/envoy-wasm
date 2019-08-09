/*
 * API Available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE __attribute__((used)) __attribute__((visibility("default")))
#endif

#include <cstdint>
using EnumType = int32_t;
#include <string_view>
using StringView = std::string_view;

#include "proxy_wasm_enums.h"
#include "proxy_wasm_externs.h"
#ifndef EMSCRIPTEN_PROTOBUF_LITE
#include "proxy_wasm_intrinsics.pb.h"
#else
#include "proxy_wasm_intrinsics_lite.pb.h"
#endif
#include "proxy_wasm_impl.h"
