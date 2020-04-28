licenses(["notice"])  # Apache 2

load("@io_bazel_rules_rust//rust:rust.bzl", "rust_binary")
load("//bazel:wasm_binary.bzl", "wasm_binary")

rust_binary(
    name = "test_rust",
    srcs = ["test_rust.rs"],
    crate_type = "cdylib",
    out_binary = True,
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
