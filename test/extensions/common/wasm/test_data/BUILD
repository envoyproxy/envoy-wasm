licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)
load("//bazel/wasm:wasm.bzl", "wasm_cc_binary")
load("@io_bazel_rules_rust//rust:rust.bzl", "rust_binary")

envoy_package()

filegroup(
    name = "modules",
    srcs = glob(["*.wasm"]),
    data = [
        ":test_cpp.wasm",
    ],
)

rust_binary(
    name = "test_rust",
    srcs = ["test_rust.rs"],
    crate_type = "cdylib",
    out_binary = True,
    rustc_flags = [
        "--edition=2018",
    ],
    deps = [
        "//test/extensions/wasm/test_data:proxy_wasm",
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

wasm_cc_binary(
    name = "test_cpp.wasm",
    srcs = ["test_cpp.cc"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

wasm_cc_binary(
    name = "bad_signature_cpp.wasm",
    srcs = ["bad_signature_cpp.cc"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)
