logging.wasm logging.wat: logging.cc envoy_wasm_intrinsics.h envoy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 -O3 -g3 --js-library envoy_wasm_intrinsics.js logging.cc -o logging.js
	wavm-disas logging.wasm logging.wat
	rm -f logging.js logging.wast
	chmod 644 logging.wat
