licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)
load("//bazel/wasm:wasm.bzl", "wasm_cc_binary")

envoy_package()

filegroup(
    name = "modules",
    srcs = glob(["*.wasm"]),
)

envoy_cc_library(
    name = "speed_cpp_plugin",
    srcs = [
        "speed_cpp.cc",
        "speed_cpp_null_plugin.cc",
    ],
    copts = ["-DNULL_PLUGIN=1"],
    deps = [
        "//external:abseil_node_hash_map",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/common/wasm:well_known_names",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
    ],
)

wasm_cc_binary(
    name = "asm2wasm_cpp.wasm",
    srcs = ["asm2wasm_cpp.cc"],
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
    name = "bad_signature_cpp.wasm",
    srcs = ["bad_signature_cpp.cc"],
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
    name = "emscripten_cpp.wasm",
    srcs = ["emscripten_cpp.cc"],
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
    name = "logging_cpp.wasm",
    srcs = ["logging_cpp.cc"],
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
    name = "missing_cpp.wasm",
    srcs = ["missing_cpp.cc"],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
        "-s ERROR_ON_UNDEFINED_SYMBOLS=0",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_cc_binary(
    name = "segv_cpp.wasm",
    srcs = ["segv_cpp.cc"],
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
    name = "speed_cpp.wasm",
    srcs = ["speed_cpp.cc"],
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
    name = "start_cpp.wasm",
    srcs = ["start_cpp.cc"],
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
    name = "stats_cpp.wasm",
    srcs = ["stats_cpp.cc"],
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
