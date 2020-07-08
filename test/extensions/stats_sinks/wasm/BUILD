load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_test",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    data = [
        "//test/extensions/stats_sinks/wasm/test_data:test_context_cpp.wasm",
    ],
    external_deps = ["abseil_optional"],
    deps = [
        "//source/common/stats:stats_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//test/extensions/stats_sinks/wasm/test_data:test_context_cpp_plugin",
        "//test/mocks/stats:stats_mocks",
        "//test/test_common:wasm_lib",
    ],
)
