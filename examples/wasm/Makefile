
envoy_wasm_test.wasm: envoy_wasm_test.cc envoy_wasm_intrinsics.h envoy_library.js
	em++ --std=c++11 --js-library envoy_library.js envoy_wasm_test.cc -s WASM=1 -o envoy_wasm_test.js

headers_only.wasm: headers_only.cc envoy_wasm_intrinsics.h envoy_wasm_filter_intrinsics.h envoy_library.js
	em++ --std=c++11 --js-library envoy_library.js headers_only.cc -s WASM=1 -o headers_only.js
