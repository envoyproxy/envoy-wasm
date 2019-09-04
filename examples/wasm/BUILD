licenses(["notice"])  # Apache 2

cc_binary(
    name = "envoy_filter_wasm_http_example.wasm",
    srcs = ["envoy_filter_http_wasm_example.cc"],
    copts = [
      "-Iapi/wasm/cpp",
      "-Iapi/wasm/cpp/google/protobuf",
      "--js-library api/wasm/cpp/proto_wasm_intrinsics.js",
      "-s WASM=1",
      "-s BINARYEN_TRAP_MODE='clamp'",
      "-s LEGALIZE_JS_FFI=0",
      "-s EMIT_EMSCRIPTEN_METADATA=1",
      "--std=c++17",
      "-O3",
      "-g3",
      "-DEMSCRIPTEN_PROTOBUF_LITE=1",
    ],
    linkopts = [
      "--js-library api/wasm/cpp/proto_wasm_intrinsics.js",
    ],
    deps = [
        "//api/wasm/cpp:emscripten_lib",
        "@com_google_protobuf_emscripten//:protobuf_headers",
    ],
)
