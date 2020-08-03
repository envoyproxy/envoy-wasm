// NOLINT(namespace-envoy)
#pragma once

#define PROXY_WASM_PROTOBUF 1
#define PROXY_WASM_PROTOBUF_FULL 1

#include "envoy/config/core/v3/grpc_service.pb.h"

#include "source/extensions/common/wasm/ext/declare_property.pb.h"

#include "include/proxy-wasm/null_plugin.h"

namespace proxy_wasm {
namespace null_plugin {
#include "extensions/common/wasm/ext/envoy_proxy_wasm_api.h"
using GrpcService = envoy::config::core::v3::GrpcService;
} // namespace null_plugin
} // namespace proxy_wasm
using namespace proxy_wasm::null_plugin;
