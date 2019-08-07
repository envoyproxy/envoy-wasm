licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "intrinsics_hdr",
    hdrs = [
        "proxy_wasm_enums.h",
        "proxy_wasm_externs.h",
        "proxy_wasm_impl.h",
        "proxy_wasm_intrinsics.h",
    ],
)
