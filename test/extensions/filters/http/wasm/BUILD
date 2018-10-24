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
    name = "wasm_filter_test",
    srcs = ["wasm_filter_test.cc"],
    data = [
        ":bad_code.wasm",
    ],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/common/stream_info:stream_info_lib",
        "//source/extensions/filters/http/wasm:wasm_filter_lib",
        "//test/mocks/http:http_mocks",
        "//test/mocks/network:network_mocks",
        "//test/mocks/ssl:ssl_mocks",
        "//test/mocks/thread_local:thread_local_mocks",
        "//test/mocks/upstream:upstream_mocks",
        "//test/test_common:utility_lib",
    ],
)

envoy_extension_cc_test(
    name = "wasm_integration_test",
    srcs = ["wasm_integration_test.cc"],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/extensions/filters/http/wasm:config",
        "//test/integration:http_integration_lib",
        "//test/test_common:utility_lib",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/extensions/filters/http/wasm:config",
        "//test/mocks/server:server_mocks",
    ],
)
