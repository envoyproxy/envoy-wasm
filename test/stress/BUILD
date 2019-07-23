licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_test",
    "envoy_cc_test_library",
    "envoy_package",
)

envoy_package()

envoy_cc_test_library(
    name = "stress_test_lib",
    srcs = [
        "stress_test.cc",
        "stress_test_common.cc",
        "stress_test_downstream.cc",
        "stress_test_upstream.cc",
    ],
    hdrs = [
        "stress_test.h",
        "stress_test_common.h",
        "stress_test_downstream.h",
        "stress_test_upstream.h",
    ],
    deps = [
        "//source/server:server_lib",
        "//test/integration:http_protocol_integration_lib",
    ],
)

envoy_cc_test(
    name = "stress_test_self_test",
    srcs = [
        "stress_test_self_test.cc",
    ],
    deps = [
        ":stress_test_lib",
    ],
)
