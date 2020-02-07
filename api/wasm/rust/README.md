# Compiling rust to .wasm files using the SDK

The rust SDK is a WIP.

## rust if you want to use it or rebuild the rust WebAssembly tests

curl https://sh.rustup.rs -sSf | sh

## How to build your wasm filter with Rust
```
cargo build --lib --release --target=wasm32-unknown-unknown
```