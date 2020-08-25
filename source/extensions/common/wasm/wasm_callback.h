#pragma once

#include "include/proxy-wasm/word.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

proxy_wasm::Word set_active_span_tag(void* raw_context, proxy_wasm::Word key_ptr, proxy_wasm::Word key_size, proxy_wasm::Word value_ptr,
                         proxy_wasm::Word value_size);

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
