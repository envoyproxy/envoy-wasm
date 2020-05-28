licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
    "envoy_proto_library",
)

envoy_package()

envoy_cc_library(
    name = "envoy_proxy_wasm_api",
    hdrs = [
        "envoy_context.h",
        "envoy_proxy_wasm_api.h",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:api_lib",
        "@proxy_wasm_cpp_sdk//:common_lib",
    ],
)

cc_library(
    name = "envoy_context",
    hdrs = ["envoy_context.h"],
)
