#OPT=-O3
OPT=

envoy_filter_http_wasm_example.wasm: envoy_filter_http_wasm_example.cc envoy_wasm_filter_intrinsics.cc envoy_wasm_intrinsics.h envoy_wasm_filter_intrinsics.h envoy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 $(OPT) -g3 --js-library envoy_wasm_intrinsics.js envoy_filter_http_wasm_example.cc envoy_wasm_filter_intrinsics.cc -o envoy_filter_http_wasm_example.js
