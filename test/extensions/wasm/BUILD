licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_package",
)
load(
    "//test/extensions:extensions_build_system.bzl",
    "envoy_extension_cc_test",
    "envoy_extension_cc_test_binary",
)

envoy_package()

envoy_extension_cc_test(
    name = "wasm_test",
    srcs = ["wasm_test.cc"],
    data = [
        "//test/extensions/wasm/test_data:modules",
    ],
    extension_name = "envoy.wasm",
    external_deps = ["abseil_optional"],
    deps = [
        "//source/common/event:dispatcher_lib",
        "//source/common/stats:isolated_store_lib",
        "//source/common/stats:stats_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/wasm:config",
        "//test/mocks/server:server_mocks",
        "//test/mocks/upstream:upstream_mocks",
        "//test/test_common:environment_lib",
        "//test/test_common:simulated_time_system_lib",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    data = [
        "//test/extensions/wasm/test_data:modules",
    ],
    extension_name = "envoy.wasm",
    deps = [
        "//include/envoy/registry",
        "//source/common/stats:isolated_store_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/wasm:config",
        "//source/server:wasm_config_lib",
        "//test/mocks/event:event_mocks",
        "//test/mocks/server:server_mocks",
        "//test/mocks/thread_local:thread_local_mocks",
        "//test/mocks/upstream:upstream_mocks",
        "//test/test_common:environment_lib",
        "@envoy_api//envoy/config/wasm/v3:pkg_cc_proto",
    ],
)

envoy_extension_cc_test_binary(
    name = "wasm_speed_test",
    srcs = ["wasm_speed_test.cc"],
    data = [
        "//test/extensions/wasm/test_data:modules",
    ],
    extension_name = "envoy.wasm",
    external_deps = [
        "abseil_optional",
        "benchmark",
    ],
    deps = [
        "//source/common/event:dispatcher_lib",
        "//source/common/stats:isolated_store_lib",
        "//source/common/stats:stats_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/wasm:config",
        "//test/extensions/wasm/test_data:speed_cpp_plugin",
        "//test/mocks/server:server_mocks",
        "//test/mocks/upstream:upstream_mocks",
        "//test/test_common:environment_lib",
        "//test/test_common:simulated_time_system_lib",
    ],
)
