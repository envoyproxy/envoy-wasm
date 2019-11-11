licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "wavm_lib",
    srcs = ["wavm.cc"],
    hdrs = ["wavm.h"],
    copts = [
        '-DWAVM_API=""',
        "-Wno-non-virtual-dtor",
        "-Wno-old-style-cast",
    ],
    defines = ["ENVOY_WASM_WAVM"],
    external_deps = [
        "wavm",
    ],
    deps = [
        "//external:abseil_node_hash_map",
        "//source/common/common:assert_lib",
        "//source/extensions/common/wasm:wasm_vm_base",
        "//source/extensions/common/wasm:wasm_vm_interface",
        "//source/extensions/common/wasm:well_known_names",
    ],
)
