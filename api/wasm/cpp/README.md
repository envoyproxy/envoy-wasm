# Compiling C++ to .wasm files using the SDK

The SDK has dependencies on specific versions of emscription and the protobuf library, therefor use of a Docker image is recommended.

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

## Dependencies for building WASM modules:

If you do not wish to use the Docker file, the dependencies can be installed manually (see the sdk\_container.sh), or by hand.

### protobuf v3.6.1

You must install the version of protobuf on your build system that matches the libprotobuf.bc files (without any patches) so that the generated code matches the .bc library.  Currently this is based on tag v3.6.1 of https://github.com/protocolbuffers/protobuf.

```bash
git clone https://github.com/protocolbuffers/protobuf
cd protobuf
git checkout v3.6.1
git submodule update --init --recursive
./autogen.sh
./configure
make
make check
sudo make install
```

### rebulding the protobuf.bc files

If want to rebuild the .bc files or use a different version see the instructions at https://github.com/kwonoj/protobuf-wasm  (note: this is pinned to git tag v3.6.1).  A pre-patched repo is available at https://github.com/jplevyak/protobuf branch envoy-wasm.

### emscripten

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install sdk-1.38.42-64bit
./emsdk activate sdk-1.38.42-64bit

. ./emsdk\_env.sh
```

It is possible later versions will work, e.g.

```bash
./emsdk install latest
./emsdk activate latest
```

However 1.38.42 is known to work.

### WAVM binaries

```bash
git clone git@github.com:WAVM/WAVM.git
cd WAVM
cmake "."
make
sudo make install
```

Note: ensure /usr/local/bin is in your path

