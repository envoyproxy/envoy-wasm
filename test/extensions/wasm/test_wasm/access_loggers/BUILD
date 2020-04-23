licenses(["notice"])  # Apache 2

load("//bazel:wasm_binary.bzl", "wasm_binary")

wasm_binary(
    name = "logging_cpp",
    srcs = [
        "logging_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)
