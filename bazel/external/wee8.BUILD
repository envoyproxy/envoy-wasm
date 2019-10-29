licenses(["notice"])  # Apache 2

load(":genrule_cmd.bzl", "genrule_cmd")

cc_library(
    name = "wee8",
    srcs = select({
        "@envoy//bazel:windows_x86_64": ["WINDOWS_IS_NOT_SUPPORTED_YET"],
        "//conditions:default": [
            "libwee8.a",
        ],
    }),
    hdrs = [
        "wee8/third_party/wasm-api/wasm.hh",
    ],
    includes = ["wee8/third_party"],
    visibility = ["//visibility:public"],
)

genrule(
    name = "build",
    srcs = glob(["wee8/**"]),
    outs = [
        "libwee8.a",
    ],
    cmd = genrule_cmd("@envoy//bazel/external:wee8.genrule_cmd"),
)
