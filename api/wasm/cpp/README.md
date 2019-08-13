Dependencies for building WASM modules:

- You must install the version of protobuf on your build system that matches the libprotobuf.bc files (without any patches) so that the generated code matches the .bc library.
  Currently this is based on tag v3.6.1 of https://github.com/protocolbuffers/protobuf.

- If want to rebuild the .bc files or use a different version see the instructions at https://github.com/kwonoj/protobuf-wasm  (note: this is pinned to git tag v3.6.1)
  A pre-patched repo is available at https://github.com/jplevyak/protobuf branch envoy-wasm
