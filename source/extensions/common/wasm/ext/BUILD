licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "envoy_null_vm_wasm_api",
    hdrs = [
        "envoy_null_vm_wasm_api.h",
        "envoy_proxy_wasm_api.h",
    ],
    deps = [
        ":envoy_proxy_wasm_api",
        "@proxy_wasm_cpp_sdk//:api_lib",
        "@proxy_wasm_cpp_sdk//:common_lib",
    ],
)

cc_library(
    name = "envoy_proxy_wasm_api",
    hdrs = ["envoy_proxy_wasm_api.h"],
)

cc_library(
    name = "envoy_proxy_wasm_api_lib",
    srcs = ["envoy_proxy_wasm_api.cc"],
    tags = ["manual"],
    deps = [
        ":envoy_proxy_wasm_api",
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
    alwayslink = 1,
)
