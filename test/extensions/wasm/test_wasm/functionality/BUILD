licenses(["notice"])  # Apache 2

load("@io_bazel_rules_rust//rust:rust.bzl", "rust_library")
load("//bazel:wasm_binary.bzl", "wasm_binary")

# TODO(shikugawa): Prefer to use bazel on proxy_wasm_rust_sdk
rust_library(
    name = "proxy_wasm_rust_sdk",
    srcs = glob(["proxy-wasm/**/*"]),
)

# rust_binary(
#     name = "logging_rust.wasm",
#     srcs = ["logging_rust/src/lib.rs"],
#     deps = [
#       ":proxy_wasm_rust_sdk",
#     ]
# )

wasm_binary(
    name = "asm2wasm_cpp",
    srcs = [
        "asm2wasm_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "bad_signature_cpp",
    srcs = [
        "bad_signature_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "emscripten_cpp",
    srcs = [
        "emscripten_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "logging_cpp",
    srcs = [
        "logging_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "missing_cpp",
    srcs = [
        "missing_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "segv_cpp",
    srcs = [
        "segv_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "speed_cpp",
    srcs = [
        "speed_cpp.cc",
    ],
    deps = [
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "start_cpp",
    srcs = [
        "start_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "stats_cpp",
    srcs = [
        "stats_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)
