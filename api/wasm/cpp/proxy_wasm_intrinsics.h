/*
 * API Available to WASM modules.
 */
// NOLINT(namespace-envoy)

#pragma once

#ifndef PROXY_WASM_KEEPALIVE
#define PROXY_WASM_KEEPALIVE __attribute__((used)) __attribute__((visibility("default")))
#endif
#define WASM_EXPORT(_return_type, _function_name, _function_args)                                  \
  extern "C" PROXY_WASM_KEEPALIVE _return_type _function_name _function_args
#define START_WASM_PLUGIN(_x)
#define END_WASM_PLUGIN

#include <cstdint>
#include <string_view>
using StringView = std::string_view;
#include <optional>
template <typename T> using Optional = std::optional<T>;

#include "proxy_wasm_enums.h"
#include "proxy_wasm_common.h"
#include "proxy_wasm_enums.h"
#include "proxy_wasm_externs.h"
#ifndef PROXY_WASM_PROTOBUF_LITE
#include "proxy_wasm_intrinsics.pb.h"
#else
#include "proxy_wasm_intrinsics_lite.pb.h"
#endif
#include "proxy_wasm_api.h"
