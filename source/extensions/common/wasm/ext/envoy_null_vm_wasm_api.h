// NOLINT(namespace-envoy)
#pragma once

namespace proxy_wasm {
namespace null_plugin {

template <typename T> using Optional = absl::optional<T>;
using StringView = absl::string_view;

#include "proxy_wasm_common.h"
#include "proxy_wasm_enums.h"
#include "proxy_wasm_externs.h"

#define _THE_FOLLOWING_FILE_MUST_COME_AFTER_THOSE_ABOVE_ 1
#include "proxy_wasm_api.h"
#undef _THE_FOLLOWING_FILE_MUST_COME_AFTER_THOSE_ABOVE_
#define _THE_FOLLOWING_FILE_MUST_COME_AFTER_THOSE_ABOVE_ 1
#include "extensions/common/wasm/ext/envoy_proxy_wasm_api.h"
#undef _THE_FOLLOWING_FILE_MUST_COME_AFTER_THOSE_ABOVE_

} // namespace null_plugin
} // namespace proxy_wasm
