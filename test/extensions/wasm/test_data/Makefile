logging.wasm: logging.cc envoy_wasm_intrinsics.h envoy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 -O3 -g3 --js-library envoy_wasm_intrinsics.js logging.cc -o logging.js
	rm -f logging.js logging.wast
