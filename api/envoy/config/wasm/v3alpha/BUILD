load("//bazel:api_build_system.bzl", "api_proto_library_internal", "api_proto_package")

licenses(["notice"])  # Apache 2

api_proto_package()

api_proto_library_internal(
    name = "wasm",
    srcs = ["wasm.proto"],
    visibility = [
        "//envoy/config/accesslog/v3alpha:__pkg__",
        "//envoy/config/bootstrap/v3alpha:__pkg__",
        "//envoy/config/filter/http/wasm/v3alpha:__pkg__",
        "//envoy/config/filter/network/wasm/v3alpha:__pkg__",
        "//source/extensions/wasm:__pkg__",
    ],
    deps = [
        "//envoy/api/v3alpha/core:base",
    ],
)
