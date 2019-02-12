API=../../../../../../api/wasm/cpp

EMSCRIPTEN_OPT=-s WASM=1 -s EMIT_EMSCRIPTEN_METADATA=1

# Note that optimizations are disabled, because optimized WASM module
# throws wavm.integerDivideByZeroOrOverflow, possibly due to an issue
# with getFunctionWavm() templates and/or their usage.
#CXXFLAGS=--std=c++14 -O3 -g3
CXXFLAGS=--std=c++14 -g3

all: headers.wasm async_call.wasm

headers.wasm headers.wat: headers.cc ${API}/proxy_wasm_intrinsics.h ${API}/proxy_wasm_intrinsics.cc ${API}/proxy_wasm_intrinsics.js
	em++ ${EMSCRIPTEN_OPT} ${CXXFLAGS} -I${API} --js-library ${API}/proxy_wasm_intrinsics.js headers.cc ${API}/proxy_wasm_intrinsics.cc -o headers.js
	wasm-gc headers.wasm
	wavm-disas headers.wasm headers.wat
	rm -f headers.js headers.wast
	chmod 644 headers.wat

async_call.wasm async_call.wat: async_call.cc ${API}/proxy_wasm_intrinsics.h ${API}/proxy_wasm_intrinsics.cc ${API}/proxy_wasm_intrinsics.js
	em++ ${EMSCRIPTEN_OPT} ${CXXFLAGS} -I${API} --js-library ${API}/proxy_wasm_intrinsics.js async_call.cc ${API}/proxy_wasm_intrinsics.cc -o async_call.js
	wasm-gc async_call.wasm
	wavm-disas async_call.wasm async_call.wat
	rm -f async_call.js async_call.wast
	chmod 644 async_call.wat
