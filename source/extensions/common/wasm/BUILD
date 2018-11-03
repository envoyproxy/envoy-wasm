licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "well_known_names",
    hdrs = ["well_known_names.h"],
    deps = [
        "//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "wasm_hdr",
    hdrs = ["wasm.h"],
)

envoy_cc_library(
    name = "wasm_lib",
    srcs = ["wasm.cc"],
    deps = [
        ":wasm_hdr",
        ":well_known_names",
        "//source/extensions/common/wasm/wavm:wavm_lib",
    ],
)
