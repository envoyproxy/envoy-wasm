licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)
load("//bazel/wasm:wasm.bzl", "wasm_cc_binary")

envoy_package()

wasm_cc_binary(
    name = "async_call_cpp.wasm",
    srcs = ["async_call_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_cc_binary(
    name = "body_cpp.wasm",
    srcs = ["body_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_cc_binary(
    name = "grpc_call_cpp.wasm",
    srcs = ["grpc_call_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_lite",
    ],
)

wasm_cc_binary(
    name = "headers_cpp.wasm",
    srcs = ["headers_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_cc_binary(
    name = "metadata_cpp.wasm",
    srcs = ["metadata_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_lite",
        "@proxy_wasm_cpp_sdk//contrib:contrib_lib",
    ],
)

wasm_cc_binary(
    name = "queue_cpp.wasm",
    srcs = ["queue_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_cc_binary(
    name = "root_id_cpp.wasm",
    srcs = ["root_id_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_cc_binary(
    name = "shared_cpp.wasm",
    srcs = ["shared_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)
