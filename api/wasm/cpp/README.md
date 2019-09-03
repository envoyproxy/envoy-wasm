# Compiling C++ to .wasm files using the SDK

The SDK has dependencies on specific versions of emscription and the protobuf library, therefor use of a Docker image is recomended.

## Dependencies for building WASM modules:

  * You must install the version of protobuf on your build system that matches the libprotobuf.bc files (without any patches) so that the generated code matches the .bc library.
  Currently this is based on tag v3.6.1 of https://github.com/protocolbuffers/protobuf.

  * If want to rebuild the .bc files or use a different version see the instructions at https://github.com/kwonoj/protobuf-wasm  (note: this is pinned to git tag v3.6.1).
  A pre-patched repo is available at https://github.com/jplevyak/protobuf branch envoy-wasm.

  * emscripten 1.38.42.

## Docker

A Dockerfile for the C++ SDK is provided in Dockerfile-sdk.

It can build built with (in this directory):

docker build -t wasmsdk:v1 -f Dockerfile-sdk .

The docker image can be used for compiling wasm files.

Create a directory parallel to envoy with your source files and a Makefile:

```
DOCKER_SDK=1

all: myproject.wasm

include sdk/Makefile.base_lite
```

Source file (myproject.cc):

```
#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders() override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  void onDone() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterHeadersStatus ExampleContext::onRequestHeaders() {
  logInfo(std::string("onRequestHeaders ") + std::to_string(id()));
  auto path = getRequestHeader(":path");
  logInfo(std::string("header path ") + std::string(path->view()));
  return FilterHeadersStatus::Continue;
}

void ExampleContext::onDone() { logInfo("onDone " + std::to_string(id())); }
```

Run docker

```bash
docker run -v $PWD:/work -v $PWD/../envoy/api/wasm/cpp:/work/sdk -w /work  wasmsdk:v1 bash /build_wasm.sh
```

The first time that emscripten runs it will generate the standard libraries.  To cache these in the docker image,
after the first successful compilation (e.g myproject.cc above), commit the image with the standard libraries:

```bash
docker commit `docker ps -l | grep wasmsdk:v1 | awk '{print $1}'` wasmsdk:v1
```

This will save time on subsequent compiles.
