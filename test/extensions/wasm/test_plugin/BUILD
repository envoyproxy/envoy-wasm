licenses(["notice"])  # Apache 2

load("//bazel:envoy_build_system.bzl", "envoy_cc_library")

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

envoy_cc_library(
    name = "test_cpp_plugin",
    srcs = [
        "test_cpp.cc",
        "test_cpp_null_plugin.cc",
    ],
    copts = ["-DNULL_PLUGIN=1"],
    deps = [
        "//external:abseil_node_hash_map",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/extensions/common/wasm:wasm_hdr",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/common/wasm:well_known_names",
    ],
)

envoy_cc_library(
    name = "http_plugin",
    srcs = [
        "http_plugin.cc",
        "http_null_plugin.cc",
    ],
    copts = ["-DNULL_PLUGIN=1"],
    deps = [
        "//external:abseil_node_hash_map",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/common/protobuf",
        "//source/extensions/common/wasm:declare_property_cc_proto",
        "//source/extensions/common/wasm:wasm_hdr",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/common/wasm:well_known_names",
    ],
)
