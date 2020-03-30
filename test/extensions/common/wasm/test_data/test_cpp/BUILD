load("@rules_proto//proto:defs.bzl", "proto_library")

cc_binary(
    name = "test_cpp.wasm",
    srcs = [
        "test_cpp.cc",
    ],
    additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
    linkopts = [
        "--js-library",
        "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
    ],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
    visibility = ["//visibility:public"],
)

exports_files(["test_cpp.cc"])
