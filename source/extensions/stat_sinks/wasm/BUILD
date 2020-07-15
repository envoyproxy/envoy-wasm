load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

# Stats sink for wasm

envoy_package()

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    security_posture = "data_plane_agnostic",
    deps = [
        ":wasm_stat_sink_lib",
        "//include/envoy/registry",
        "//include/envoy/server:bootstrap_extension_config_interface",
        "//include/envoy/server:factory_context_interface",
        "//include/envoy/server:instance_interface",
        "//source/common/network:address_lib",
        "//source/common/network:resolver_lib",
        "//source/extensions/common/wasm:wasm_lib",
        "//source/extensions/stat_sinks:well_known_names",
        "//source/server:configuration_lib",
        "@envoy_api//envoy/config/metrics/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/stat_sinks/wasm/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "wasm_stat_sink_lib",
    hdrs = ["wasm_stat_sink_impl.h"],
    deps = [
    ],
)
