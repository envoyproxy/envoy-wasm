load("//bazel/wasm:wasm.bzl", "wasm_rust_binary")

licenses(["notice"])  # Apache 2

package(default_visibility = ["//visibility:public"])

wasm_rust_binary(
    name = "logging_rust.wasm",
    srcs = ["test_data/src/logging_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)

wasm_rust_binary(
    name = "async_call_rust.wasm",
    srcs = ["test_data/src/async_call_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)

wasm_rust_binary(
    name = "body_rust.wasm",
    srcs = ["test_data/src/body_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)

wasm_rust_binary(
    name = "headers_rust.wasm",
    srcs = ["test_data/src/headers_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)

wasm_rust_binary(
    name = "metadata_rust.wasm",
    srcs = ["test_data/src/metadata_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)

wasm_rust_binary(
    name = "shared_data_rust.wasm",
    srcs = ["test_data/src/shared_data_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)

wasm_rust_binary(
    name = "shared_queue_rust.wasm",
    srcs = ["test_data/src/shared_queue_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)

wasm_rust_binary(
    name = "network_logging_rust.wasm",
    srcs = ["test_data/src/network_logging_rust.rs"],
    rustc_flags = ["-Clink-arg=-zstack-size=32768"],
    deps = [
        "//bazel/external/cargo:log",
        "//bazel/external/cargo:proxy_wasm",
    ],
)
