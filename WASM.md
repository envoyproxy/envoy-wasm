WebAssembly Extension Support

# C++ SDK

A C++ SDK is available including a docker image build environment. See api/wasm/cpp/README.md.

# Rust SDK

The Rust SDK is a WIP. See api/wasm/rust/README.md.

# Building in WebAssembly support

Building with WebAssembly support requires enabling one or more WebAssembly runtime via the "wasm" define:

bazel build --define wasm=enabled --explain=~/bazel.log //source/exe:envoy-static

The options are "wavm", "v8" and "enable" (for both wavm and v8).

This option must be provided for all build/test targets.
