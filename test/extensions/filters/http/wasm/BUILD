load(
    "//bazel:envoy_build_system.bzl",
    "envoy_package",
)
load(
    "//test/extensions:extensions_build_system.bzl",
    "envoy_extension_cc_test",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_extension_cc_test(
    name = "wasm_filter_test",
    size = "large",
    srcs = ["wasm_filter_test.cc"],
    data = [
        "//test/extensions/filters/http/wasm/test_data:test_cpp.wasm",
    ],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/extensions/filters/http/wasm:wasm_filter_lib",
        "//test/extensions/filters/http/wasm/test_data:test_cpp_plugin",
        "//test/test_common:wasm_lib",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    data = [
        "//test/extensions/filters/http/wasm/test_data:test_cpp.wasm",
    ],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/common/common:base64_lib",
        "//source/common/common:hex_lib",
        "//source/common/crypto:utility_lib",
        "//source/extensions/common/crypto:utility_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/filters/http/wasm:config",
        "//test/mocks/server:server_mocks",
        "//test/test_common:environment_lib",
        "@envoy_api//envoy/extensions/filters/http/wasm/v3:pkg_cc_proto",
    ],
)
