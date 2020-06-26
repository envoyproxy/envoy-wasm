load("@rules_cc//cc:defs.bzl", "cc_library")
load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "envoy_null_vm_wasm_api",
    hdrs = [
        "envoy_null_vm_wasm_api.h",
        "envoy_proxy_wasm_api.h",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:api_lib",
        "@proxy_wasm_cpp_sdk//:common_lib",
    ],
)

envoy_cc_library(
    name = "envoy_null_plugin",
    hdrs = [
        "envoy_null_plugin.h",
        "envoy_proxy_wasm_api.h",
    ],
)

cc_library(
    name = "envoy_proxy_wasm_api_lib",
    srcs = ["envoy_proxy_wasm_api.cc"],
    hdrs = ["envoy_proxy_wasm_api.h"],
    tags = ["manual"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
    alwayslink = 1,
)
