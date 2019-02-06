# Note that optimizations are disabled, because optimized WASM module
# throws wavm.integerDivideByZeroOrOverflow, possibly due to an issue
# with getFunctionWavm() templates and/or their usage.
#OPT=-O3
OPT=
API=../../../../../api/wasm/cpp

all: logging.wasm

logging.wasm logging.wat: logging.cc ${API}/envoy_wasm_intrinsics.h ${API}/envoy_wasm_intrinsics.cc ${API}/envoy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 $(OPT) -g3 -I${API} --js-library ${API}/envoy_wasm_intrinsics.js logging.cc ${API}/envoy_wasm_intrinsics.cc -o logging.js
	wasm-gc logging.wasm
	wavm-disas logging.wasm logging.wat
	rm -f logging.js logging.wast
	chmod 644 logging.wat

