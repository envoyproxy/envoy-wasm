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
        "//test/extensions/filters/network/wasm/test_data:modules",
    ],
    extension_name = "envoy.filters.network.wasm",
    deps = [
        "//source/common/common:base64_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/filters/network/wasm:config",
        "//test/mocks/server:server_mocks",
        "//test/test_common:environment_lib",
    ],
)

envoy_extension_cc_test(
    name = "wasm_filter_test",
    srcs = envoy_select_wasm(["wasm_filter_test.cc"]),
    data = [
        "//test/extensions/filters/network/wasm/test_data:modules",
    ],
    extension_name = "envoy.filters.network.wasm",
    deps = [
        "//source/extensions/filters/network/wasm:wasm_filter_lib",
        "//test/mocks/network:network_mocks",
        "//test/mocks/server:server_mocks",
        "//test/test_common:environment_lib",
    ],
)
