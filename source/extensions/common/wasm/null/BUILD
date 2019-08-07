licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "null_plugin_hdr",
    hdrs = [
        "wasm_api_impl.h",
    ],
    deps = [
        "//api/wasm/cpp:intrinsics_hdr",
    ],
)

envoy_cc_library(
    name = "null_lib",
    srcs = ["null.cc"],
    hdrs = ["null.h"],
    deps = [
        ":null_plugin_hdr",
        "//external:abseil_node_hash_map",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/common/protobuf",
        "//source/extensions/common/wasm:wasm_hdr",
        "//source/extensions/common/wasm:well_known_names",
        "@envoy_api//envoy/config/wasm/v2:wasm_cc",
    ],
)

# Sample builtin Null VM/Sandbox plugin, also used in tests.
envoy_cc_library(
    name = "plugin",
    srcs = [
        "null_plugin_wrapper.cc",  # The wrapper, copy and update the name.
        "plugin.cc",  # The plugin code.
    ],
    copts = ["-DNULL_PLUGIN=1"],
    deps = [
        ":null_lib",
        "//external:abseil_node_hash_map",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/common/protobuf",
        "//source/extensions/common/wasm:wasm_hdr",
        "//source/extensions/common/wasm:well_known_names",
        "@envoy_api//envoy/config/wasm/v2:wasm_cc",
    ],
)
