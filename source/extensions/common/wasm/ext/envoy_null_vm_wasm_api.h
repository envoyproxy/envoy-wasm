// NOLINT(namespace-envoy)
#pragma once

namespace proxy_wasm {
namespace null_plugin {

template <typename T> using Optional = absl::optional<T>;
using StringView = absl::string_view;

#include "proxy_wasm_enums.h"
#include "proxy_wasm_common.h"
#include "proxy_wasm_enums.h"
#include "proxy_wasm_externs.h"
#include "proxy_wasm_api.h"
#include "extensions/common/wasm/ext/envoy_proxy_wasm_api.h"
} // namespace null_plugin
} // namespace proxy_wasm
