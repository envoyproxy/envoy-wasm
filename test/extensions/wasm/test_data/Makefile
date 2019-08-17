NO_CONTEXT = true

all: logging_cpp.wasm logging_rust.wasm bad_signature_cpp.wasm segv_cpp.wasm emscripten_cpp.wasm asm2wasm_cpp.wasm stats_cpp.wasm missing_cpp.wasm

include ../../../../api/wasm/cpp/Makefile.base_lite
include ../../../../api/wasm/rust/Makefile.base

missing_cpp.wasm missing_cpp.wat: missing_cpp.cc ${CPP_API}/proxy_wasm_intrinsics.h ${CPP_API}/proxy_wasm_intrinsics.js
	em++ -s WASM=1 -s BINARYEN_TRAP_MODE='clamp' -s ERROR_ON_UNDEFINED_SYMBOLS=0 -s LEGALIZE_JS_FFI=0 -s EMIT_EMSCRIPTEN_METADATA=1 --std=c++17 -O3 -g3 -DEMSCRIPTEN_PROTOBUF_LITE=1 -I${CPP_API} -I${CPP_API}/google/protobuf -I/usr/local/include --js-library ${CPP_API}/proxy_wasm_intrinsics.js missing_cpp.cc ${CPP_API}/proxy_wasm_intrinsics_lite.pb.cc ${CPP_API}/struct_lite.pb.cc ${CPP_CONTEXT_LIB} ${CPP_API}/libprotobuf-lite.bc -o missing_cpp.js
	wavm-disas missing_cpp.wasm missing_cpp.wat
	rm -f missing_cpp.js missing_cpp.wast
	chmod 644 missing_cpp.wat
