licenses(["notice"])  # Apache 2

load("//bazel:wasm_binary.bzl", "wasm_binary")

# rust_library(
#     name = "test_rust",
#     srcs = ["test_rust/src/lib.rs"],
# )

wasm_binary(
    name = "bad_signature_cpp",
    srcs = [
        "bad_signature_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)
