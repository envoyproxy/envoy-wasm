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
    name = "wasm_filter_test",
    size = "large",
    srcs = ["wasm_filter_test.cc"],
    data = [
        "//test/extensions/filters/http/wasm/test_data:modules",
    ],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/common/stream_info:stream_info_lib",
        "//source/extensions/common/wasm:wasm_interoperation_lib",
        "//source/extensions/common/wasm/null/sample_plugin:plugin",
        "//source/extensions/filters/http/wasm:wasm_filter_lib",
        "//test/mocks/grpc:grpc_mocks",
        "//test/mocks/http:http_mocks",
        "//test/mocks/network:network_mocks",
        "//test/mocks/server:server_mocks",
        "//test/mocks/ssl:ssl_mocks",
        "//test/mocks/stream_info:stream_info_mocks",
        "//test/mocks/thread_local:thread_local_mocks",
        "//test/mocks/upstream:upstream_mocks",
        "//test/test_common:environment_lib",
        "//test/test_common:utility_lib",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = envoy_select_wasm(["config_test.cc"]),
    data = [
        "//test/extensions/filters/http/wasm/test_data:modules",
    ],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/common/common:base64_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/filters/http/wasm:config",
        "//test/mocks/server:server_mocks",
        "//test/test_common:environment_lib",
    ],
)

envoy_extension_cc_test(
    name = "wasm_filter_stress_test",
    size = "large",
    srcs = envoy_select_wasm(["wasm_filter_stress_test.cc"]),
    data = [
        "//test/extensions/filters/http/wasm/test_data:modules",
    ],
    extension_name = "envoy.filters.http.wasm",
    external_deps = ["abseil_optional"],
    deps = [
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/filters/http/wasm:config",
        "//source/extensions/wasm:config",
        "//test/proto:helloworld_proto_cc_proto",
        "//test/stress:stress_test_lib",
        "//test/test_common:environment_lib",
    ],
)
