licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

# Sample builtin Null VM/Sandbox plugin, also used in tests.
envoy_cc_library(
    name = "plugin",
    srcs = [
        "plugin.cc",  # The plugin code.
        "plugin_wrapper.cc",  # The wrapper, copy and update the name.
    ],
    copts = ["-DNULL_PLUGIN=1"],
    deps = [
        "//external:abseil_node_hash_map",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/common/protobuf",
        "//source/extensions/common/wasm:declare_property_cc_proto",
        "//source/extensions/common/wasm:wasm_hdr",
        "//source/extensions/common/wasm:well_known_names",
        "//source/extensions/common/wasm/null:null_plugin_lib",
    ],
)
