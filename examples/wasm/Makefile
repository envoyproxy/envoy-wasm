#OPT=-O3
OPT=
API=../../api/wasm/cpp

envoy_filter_http_wasm_example.wasm: envoy_filter_http_wasm_example.cc ${API}/proxy_wasm_intrinsics.cc ${API}/proxy_wasm_intrinsics.h ${API}/proxy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 $(OPT) -g3 -I${API} --js-library ${API}/proxy_wasm_intrinsics.js envoy_filter_http_wasm_example.cc ${API}/proxy_wasm_intrinsics.cc -o envoy_filter_http_wasm_example.js
