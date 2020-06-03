// NOLINT(namespace-envoy)
#pragma once

#include "include/proxy-wasm/null_plugin.h"

namespace proxy_wasm {
namespace null_plugin {
#include "extensions/common/wasm/ext/envoy_proxy_wasm_api.h"
} // namespace null_plugin
} // namespace proxy_wasm
using namespace proxy_wasm::null_plugin;
