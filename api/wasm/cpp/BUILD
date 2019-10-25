licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "api_lib",
    hdrs = [
        "proxy_wasm_api.h",
    ],
)

envoy_cc_library(
    name = "common_lib",
    hdrs = [
        "proxy_wasm_common.h",
    ],
)
