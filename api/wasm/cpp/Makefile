all: proxy_wasm_intrinsics.pb.h proxy_wasm_intrinsics_lite.pb.h struct_lite.pb.h

proxy_wasm_intrinsics.pb.h: proxy_wasm_intrinsics.proto
	protoc --cpp_out=. proxy_wasm_intrinsics.proto

proxy_wasm_intrinsics_lite.pb.h struct_lite.pb.h: proxy_wasm_intrinsics_lite.proto
	protoc --cpp_out=. -I. proxy_wasm_intrinsics_lite.proto
	protoc --cpp_out=. struct_lite.proto
