load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])  # Apache 2

package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_select_wasm_wavm",
    "envoy_select_wasm_wavm_or",
)

cc_library(
    name = "include",
    hdrs = glob(["include/proxy-wasm/**/*.h"]),
    deps = [
        "@proxy_wasm_cpp_sdk//:common_lib",
    ],
)

cc_library(
    name = "lib",
    srcs = envoy_select_wasm_wavm_or(
        glob(
            [
                "src/**/*.h",
                "src/**/*.cc",
            ],
        ),
        glob(
            [
                "src/**/*.h",
                "src/**/*.cc",
            ],
            exclude = ["src/**/wavm*"],
        ),
    ),
    copts = envoy_select_wasm_wavm([
        '-DWAVM_API=""',
        "-Wno-non-virtual-dtor",
        "-Wno-old-style-cast",
    ]),
    deps = [
        ":include",
        "//external:abseil_flat_hash_map",
        "//external:abseil_optional",
        "//external:abseil_strings",
        "//external:protobuf",
        "//external:ssl",
        "//external:wee8",
        "//external:zlib",
        "@proxy_wasm_cpp_sdk//:api_lib",
        "@proxy_wasm_cpp_sdk//:common_lib",
    ] + envoy_select_wasm_wavm([
        "@envoy//bazel/foreign_cc:wavm",
    ]),
)
