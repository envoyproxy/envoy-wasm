licenses(["notice"])  # Apache 2

package(default_visibility = ["//visibility:public"])

load(":cc_toolchain_config.bzl", "cc_toolchain_config")

cc_toolchain_suite(
    name = "emscripten",
    toolchains = {
        "wasm": ":wasm_toolchain",
    },
)

filegroup(name = "empty")

filegroup(
    name = "emscripten_cache_content",
    srcs = glob(["tmp/emscripten_cache/**/*"]),
)

filegroup(
    name = "all",
    srcs = [
        "common.sh",
        "emar.sh",
        "emcc.sh",
        ":emscripten_cache_content",
        "@emscripten_toolchain//:all",
    ],
)

cc_toolchain(
    name = "wasm_toolchain",
    all_files = ":all",
    ar_files = ":all",
    compiler_files = ":all",
    dwp_files = ":empty",
    linker_files = ":all",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
    toolchain_config = ":wasm_toolchain_config",
    toolchain_identifier = "wasm-toolchain",
)

cc_toolchain_config(name = "wasm_toolchain_config")
