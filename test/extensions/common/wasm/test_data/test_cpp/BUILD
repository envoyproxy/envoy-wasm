licenses(["notice"])  # Apache 2

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
    visibility = ["//visibility:public"],
    deps = [
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics",
    ],
)

exports_files(["test_cpp.cc"])
