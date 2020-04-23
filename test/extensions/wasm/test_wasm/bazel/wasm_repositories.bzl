load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def wasm_dependencies():
    _envoy_build_tools()

    _com_github_emscripten_toolchain()
    _com_github_bazelbuild_bazel_skylib()
    _com_github_bazelbuild_rules_proto()
    _com_github_proxy_wasm_cpp_sdk()
    _com_github_proxy_wasm_cpp_host()
    _com_github_protocolbuffers_protobuf()
    _com_github_bazelbuild_rules_rust()
    _com_github_proxy_wasm_rust_sdk()

def _envoy_build_tools():
    http_archive(
        name = "envoy_build_tools",
        sha256 = "9d348f92ae8fb2495393109aac28aea314ad1fb013cdec1ab7b1224f804be1b7",
        strip_prefix = "envoy-build-tools-823c2e9386eee5117f7ef9e3d7c90e784cd0d047",
        # 2020-04-07
        urls = ["https://github.com/envoyproxy/envoy-build-tools/archive/823c2e9386eee5117f7ef9e3d7c90e784cd0d047.tar.gz"],
    )

def _com_github_emscripten_toolchain():
    http_archive(
        name = "emscripten_toolchain",
        build_file = "//bazel/external:emscripten-toolchain.BUILD",
        patch_cmds = [
            "./emsdk install 1.39.0-upstream",
            "./emsdk activate --embedded 1.39.0-upstream",
        ],
        strip_prefix = "emsdk-a5082b232617c762cb65832429f896c838df2483",
        url = "https://github.com/emscripten-core/emsdk/archive/a5082b232617c762cb65832429f896c838df2483.tar.gz",
    )

def _com_github_bazelbuild_bazel_skylib():
    http_archive(
        name = "bazel_skylib",
        sha256 = "97e70364e9249702246c0e9444bccdc4b847bed1eb03c5a3ece4f83dfe6abc44",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.2/bazel-skylib-1.0.2.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.2/bazel-skylib-1.0.2.tar.gz",
        ],
    )

def _com_github_bazelbuild_rules_proto():
    http_archive(
        name = "rules_proto",
        sha256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208",
        strip_prefix = "rules_proto-97d8af4dc474595af3900dd85cb3a29ad28cc313",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
            "https://github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
        ],
    )

def _com_github_proxy_wasm_cpp_sdk():
    git_repository(
        name = "proxy_wasm_cpp_sdk",
        commit = "9ab06092c4579a74efee16ec37becd42aca66074",
        remote = "https://github.com/proxy-wasm/proxy-wasm-cpp-sdk",
    )

def _com_github_proxy_wasm_cpp_host():
    http_archive(
        name = "proxy_wasm_cpp_host",
        sha256 = "40558ce134552ba9afb082ad5cf7c2cf157baeb38539810edb43b6eb109596dc",
        strip_prefix = "proxy-wasm-cpp-host-375ce583a79ca98afa86a2e56f196ea7321d6b1e",
        urls = ["https://github.com/jplevyak/proxy-wasm-cpp-host/archive/375ce583a79ca98afa86a2e56f196ea7321d6b1e.tar.gz"],
    )

def _com_github_protocolbuffers_protobuf():
    git_repository(
        name = "com_google_protobuf",
        commit = "655310ca192a6e3a050e0ca0b7084a2968072260",
        remote = "https://github.com/protocolbuffers/protobuf",
    )

def _com_github_bazelbuild_rules_rust():
    git_repository(
        name = "io_bazel_rules_rust",
        commit = "8d3cb6878cf1447e81cd3d7f97057e70285fc833",
        remote = "https://github.com/bazelbuild/rules_rust",
    )

def _com_github_proxy_wasm_rust_sdk():
    git_repository(
        name = "proxy_wasm_rust_sdk",
        commit = "3ebf309fd72d2545f3909d2adc13b2450567057e",
        remote = "https://github.com/proxy-wasm/proxy-wasm-rust-sdk",
    )
