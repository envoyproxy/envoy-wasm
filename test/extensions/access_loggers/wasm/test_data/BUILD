licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_package",
)
load("//bazel/wasm:wasm.bzl", "wasm_cc_binary")

envoy_package()

wasm_cc_binary(
    name = "logging_cpp.wasm",
    srcs = ["logging_cpp.cc"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_lite",
    ],
)
