licenses(["notice"])  # Apache 2

load("//bazel:wasm_binary.bzl", "wasm_binary")

wasm_binary(
    name = "async_call_cpp",
    srcs = [
        "async_call_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "body_cpp",
    srcs = [
        "body_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_lite",
    ],
)

wasm_binary(
    name = "grpc_call_cpp",
    srcs = [
        "headers_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_lite",
    ],
)

wasm_binary(
    name = "headers_cpp",
    srcs = [
        "headers_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "metadata_cpp",
    srcs = [
        "metadata_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_lite",
        "@proxy_wasm_cpp_sdk//contrib:contrib_lib",
    ],
)

wasm_binary(
    name = "queue_cpp",
    srcs = [
        "queue_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_binary(
    name = "root_id_cpp",
    srcs = [
        "shared_cpp.cc",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)
