load("@rules_cc//cc:defs.bzl", "cc_binary")

def wasm_binary(name, srcs, deps):
    native.cc_binary(
        name = name + ".wasm",
        srcs = srcs,
        deps = deps,
        additional_linker_inputs = ["@proxy_wasm_cpp_sdk//:jslib"],
        linkopts = [
            "--js-library",
            "external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js",
        ],
        visibility = ["//visibility:public"],
    )
