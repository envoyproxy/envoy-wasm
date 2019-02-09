all: logging.wasm bad_signature.wasm

API=../../../../api/wasm/cpp

logging.wasm logging.wat: logging.cc ${API}/proxy_wasm_intrinsics.h ${API}/proxy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 -O3 -g3 -I${API} --js-library ${API}/proxy_wasm_intrinsics.js logging.cc -o logging.js
	wavm-disas logging.wasm logging.wat
	wavm-compile logging.wasm logging.wasm
	rm -f logging.js logging.wast
	chmod 644 logging.wat

bad_signature.wasm bad_signature.wat: bad_signature.cc ${API}/proxy_wasm_intrinsics.h ${API}/proxy_wasm_intrinsics.js
	em++ -s WASM=1 --std=c++14 -O3 -g3 -I${API} --js-library ${API}/proxy_wasm_intrinsics.js bad_signature.cc -o bad_signature.js
	wavm-disas bad_signature.wasm bad_signature.wat
	wavm-compile bad_signature.wasm bad_signature.wasm
	rm -f bad_signature.js bad_signature.wast
	chmod 644 bad_signature.wat
