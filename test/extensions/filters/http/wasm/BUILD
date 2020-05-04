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
    size = "large",
    srcs = ["wasm_filter_test.cc"],
    data = [
        "//test/extensions/filters/http/wasm/test_data:async_call_cpp.wasm",
        "//test/extensions/filters/http/wasm/test_data:body_cpp.wasm",
        "//test/extensions/filters/http/wasm/test_data:grpc_call_cpp.wasm",
        "//test/extensions/filters/http/wasm/test_data:headers_cpp.wasm",
        "//test/extensions/filters/http/wasm/test_data:metadata_cpp.wasm",
        "//test/extensions/filters/http/wasm/test_data:queue_cpp.wasm",
        "//test/extensions/filters/http/wasm/test_data:root_id_cpp.wasm",
        "//test/extensions/filters/http/wasm/test_data:shared_cpp.wasm",
    ],
    extension_name = "envoy.filters.http.wasm",
    deps = [
        "//source/extensions/filters/http/wasm:wasm_filter_lib",
        "//test/test_common:wasm_lib",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    data = [
        "//test/extensions/filters/http/wasm/test_data:headers_cpp.wasm",
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
