licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_package",
    "envoy_select_wasm",
)
load(
    "//test/extensions:extensions_build_system.bzl",
    "envoy_extension_cc_test",
)

envoy_package()

envoy_extension_cc_test(
    name = "config_test",
    srcs = envoy_select_wasm(["config_test.cc"]),
    data = [
        "//test/extensions/access_loggers/wasm/test_data:modules",
    ],
    extension_name = "envoy.access_loggers.wasm",
    deps = [
        "//source/extensions/access_loggers/wasm:config",
        "//test/mocks/server:server_mocks",
        "//test/test_common:environment_lib",
        "//test/test_common:utility_lib",
    ],
)
