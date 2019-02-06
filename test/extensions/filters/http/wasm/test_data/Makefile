# Note that optimizations are disabled, because optimized WASM module
# throws wavm.integerDivideByZeroOrOverflow, possibly due to an issue
# with getFunctionWavm() templates and/or their usage.
#OPT=-O3
OPT=
API=../../../../../../api/wasm/cpp

all: headers.wasm async_call.wasm

headers.wasm headers.wat: headers.cc ${API}/envoy_wasm_intrinsics.h ${API}/envoy_wasm_intrinsics.cc ${API}/envoy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 $(OPT) -g3 -I${API} --js-library ${API}/envoy_wasm_intrinsics.js headers.cc ${API}/envoy_wasm_intrinsics.cc -o headers.js
	wasm-gc headers.wasm
	wavm-disas headers.wasm headers.wat
	rm -f headers.js headers.wast
	chmod 644 headers.wat

async_call.wasm async_call.wat: async_call.cc ${API}/envoy_wasm_intrinsics.h ${API}/envoy_wasm_intrinsics.cc ${API}/envoy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 $(OPT) -g3 -I${API} --js-library ${API}/envoy_wasm_intrinsics.js async_call.cc ${API}/envoy_wasm_intrinsics.cc -o async_call.js
	wasm-gc async_call.wasm
	wavm-disas async_call.wasm async_call.wat
	rm -f async_call.js async_call.wast
	chmod 644 async_call.wat
