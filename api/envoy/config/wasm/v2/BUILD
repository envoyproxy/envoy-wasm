load("//bazel:api_build_system.bzl", "api_go_grpc_library", "api_go_proto_library", "api_proto_library_internal")

licenses(["notice"])  # Apache 2

api_proto_library_internal(
    name = "wasm",
    srcs = ["wasm.proto"],
    visibility = [
        "//envoy/config/bootstrap/v2:__pkg__",
        "//source/extensions/wasm:__pkg__",
    ],
    deps = [
        "//envoy/api/v2/core:base",
    ],
)

api_go_proto_library(
    name = "wasm",
    proto = ":wasm",
)
