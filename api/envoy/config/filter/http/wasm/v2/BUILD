load("//bazel:api_build_system.bzl", "api_proto_library_internal")

licenses(["notice"])  # Apache 2

api_proto_library_internal(
    name = "wasm",
    srcs = ["wasm.proto"],
    deps = [
        "//envoy/config/wasm/v2:wasm",
    ],
)
