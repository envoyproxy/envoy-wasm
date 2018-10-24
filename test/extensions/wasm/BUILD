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
    name = "wasm_test",
    srcs = ["wasm_test.cc"],
    data = [
        ":envoy_wasm_test.wasm",
    ],
    extension_name = "envoy.wasm",
    external_deps = ["abseil_optional"],
    deps = [
        "//source/common/event:dispatcher_lib",
        "//source/common/stats:isolated_store_lib",
        "//source/common/stats:stats_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//test/test_common:environment_lib",
        "//test/test_common:test_time_lib",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    data = [
        ":envoy_wasm_test.wasm",
    ],
    extension_name = "envoy.wasm",
    deps = [
        "//include/envoy/registry",
        "//source/extensions/wasm:config",
        "//source/server:wasm_config_lib",
        "//test/mocks/event:event_mocks",
        "//test/test_common:environment_lib",
        "@envoy_api//envoy/config/wasm/v2:wasm_cc",
    ],
)
