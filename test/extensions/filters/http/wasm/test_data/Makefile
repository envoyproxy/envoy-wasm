headers.wasm headers.wat: headers.cc envoy_wasm_intrinsics.h envoy_wasm_intrinsics.js
	# Note that optimizations are disabled, because optimized WASM module
	# throws wavm.integerDivideByZeroOrOverflow, possibly due to an issue
	# with getFunctionWavm() templates and/or their usage.
	em++ -s WASM=1 --std=c++14 -g3 --js-library envoy_wasm_intrinsics.js headers.cc -o headers.js
	wasm-gc headers.wasm
	wavm-disas headers.wasm headers.wat
	rm -f headers.js headers.wast
	chmod 644 headers.wat
