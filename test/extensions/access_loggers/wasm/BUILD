licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_package",
)
load(
    "//test/extensions:extensions_build_system.bzl",
    "envoy_extension_cc_test",
)

envoy_package()

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    data = [
        "//test/extensions/access_loggers/wasm/test_data:modules",
    ],
    extension_name = "envoy.access_loggers.wasm",
    deps = [
        "//source/extensions/access_loggers/wasm:config",
        "//test/mocks/server:server_mocks",
        "//test/test_common:environment_lib",
        "//test/test_common:utility_lib",
        "@envoy_api//envoy/extensions/access_loggers/wasm/v3:pkg_cc_proto",
    ],
)
