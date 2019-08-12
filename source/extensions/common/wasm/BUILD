licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
    "envoy_select_wasm_v8",
    "envoy_select_wasm_wavm",
)

envoy_package()

envoy_cc_library(
    name = "well_known_names",
    hdrs = ["well_known_names.h"],
    deps = [
        "//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "wasm_vm_interface",
    hdrs = ["wasm_vm.h"],
    deps = [
        ":well_known_names",
    ],
)

envoy_cc_library(
    name = "wasm_hdr",
    hdrs = ["wasm.h"],
    deps = [
        ":wasm_vm_interface",
        ":well_known_names",
        "//include/envoy/http:codes_interface",
        "//include/envoy/http:filter_interface",
        "//include/envoy/server:wasm_interface",
        "//include/envoy/upstream:cluster_manager_interface",
        "//source/common/common:stack_array",
        "//source/extensions/filters/http:well_known_names",
        "@envoy_api//envoy/config/filter/http/wasm/v2:wasm_cc",
    ],
)

envoy_cc_library(
    name = "wasm_lib",
    srcs = ["wasm.cc"],
    deps = [
        ":wasm_hdr",
        "//external:abseil_node_hash_map",
        "//source/common/buffer:buffer_lib",
        "//source/common/common:enum_to_int",
        "//source/common/config:datasource_lib",
        "//source/common/http:message_lib",
        "//source/common/http:utility_lib",
        "//source/common/tracing:http_tracer_lib",
        "//source/extensions/common/wasm/null:null_lib",
    ] + envoy_select_wasm_v8([
        "//source/extensions/common/wasm/v8:v8_lib",
    ]) + envoy_select_wasm_wavm([
        "//source/extensions/common/wasm/wavm:wavm_lib",
    ]),
)
