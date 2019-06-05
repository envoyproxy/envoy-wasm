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
    external_deps = [
        "wee8",
    ],
    deps = [
        "//include/envoy/server:wasm_interface",
        "//include/envoy/thread_local:thread_local_interface",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/common/protobuf",
        "//source/extensions/common/wasm:wasm_hdr",
        "//source/extensions/common/wasm:well_known_names",
        "@envoy_api//envoy/config/wasm/v2:wasm_cc",
    ],
)
