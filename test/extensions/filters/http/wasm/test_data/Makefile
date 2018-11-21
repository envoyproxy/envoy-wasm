headers.wasm: headers.cc envoy_wasm_intrinsics.h envoy_wasm_filter_intrinsics.h envoy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 -O3 -g3 --js-library envoy_wasm_intrinsics.js headers.cc -o headers.js
	rm -f headers.js headers.wast
