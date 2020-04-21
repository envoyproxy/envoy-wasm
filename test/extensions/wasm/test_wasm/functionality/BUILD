licenses(["notice"])  # Apache 2

load("//bazel:wasm_binary.bzl", "wasm_binary")

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
    name = "speed_cpp_null_plugin",
    srcs = [
        "speed_cpp_null_plugin.cc",
    ],
    deps = [
        "@com_google_protobuf//:protobuf",
        "@proxy_wasm_cpp_host//:include",
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
