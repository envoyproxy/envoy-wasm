licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "v8_lib",
    srcs = ["v8.cc"],
    hdrs = ["v8.h"],
    defines = ["ENVOY_WASM_V8"],
    external_deps = [
        "wee8",
    ],
    deps = [
        "//include/envoy/server:wasm_interface",
        "//include/envoy/thread_local:thread_local_interface",
        "//source/common/common:assert_lib",
        "//source/extensions/common/wasm:wasm_vm_interface",
        "//source/extensions/common/wasm:well_known_names",
    ],
)
