NO_CONTEXT=1

all: test_cpp.wasm bad_signature_cpp.wasm test_rust.wasm

test_rust.wasm: test_rust.rs
	rustc -C lto -C opt-level=3 -C panic=abort -C link-arg=-S -C link-arg=-zstack-size=32768 --crate-type cdylib --target wasm32-unknown-unknown test_rust.rs
	../../../../../bazel-bin/test/tools/wee8_compile/wee8_compile_tool test_rust.wasm test_rust.wasm

include ../../../../../api/wasm/cpp/Makefile.base_lite
