# Compiling C++ to .wasm files using the SDK

The SDK has dependencies on specific versions of emscription and the protobuf library, therefor use of a Docker image is recommended.

## Docker

A Dockerfile for the C++ SDK is provided in Dockerfile-sdk.

It can built in this directory by:

docker build -t wasmsdk:v1 -f Dockerfile-sdk .

The docker image can be used for compiling wasm files.

### Creating a project for use with the Docker build image

Create a directory parallel to envoy with your source files and a Makefile:

```
DOCKER_SDK=/sdk

all: myproject.wasm

include ${DOCKER_SDK}/Makefile.base_lite
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

### Compiling with the Docker build image

Run docker:

```bash
docker run -v $PWD:/work -w /work  wasmsdk:v1 bash /build_wasm.sh
```

### Caching the standard libraries

The first time that emscripten runs it will generate the standard libraries.  To cache these in the docker image,
after the first successful compilation (e.g myproject.cc above), commit the image with the standard libraries:

```bash
docker commit `docker ps -l | grep wasmsdk:v1 | awk '{print $1}'` wasmsdk:v1
```

This will save time on subsequent compiles.

### Using the SDK from a newer/specific version of Envoy

To use a newer/specific version of the SDK (e.g. from the version of Enovy you are going to deploy the WebAssembly module to) bind that volume and use it in the Makefile.

Makefile referencing the SDK in the /work directory:

```
DOCKER_SDK=/work/sdk

all: myproject.wasm

include ${DOCKER_SDK}/Makefile.base_lite
```

Run docker pointing to Envoy sources in a directory parallel (at the same level) as your project directory:

```bash
docker run -v $PWD:/work -v $PWD/../envoy/api/wasm/cpp:/work/sdk -w /work  wasmsdk:v1 bash /build_wasm.sh
```

### Using abseil form the image

Abseil (optionally) is built in /root/abseil and can be used by customizing the Makefile e.g.:

```
DOCKER_SDK=/sdk
CPP_API:=${DOCKER_SDK}
CPP_CONTEXT_LIB = ${CPP_API}/proxy_wasm_intrinsics.cc
ABSL = /root/abseil-cpp
ABSL_CPP = ${ABSL}/absl/strings/str_cat.cc ${ABSL}/absl/strings/str_split.cc ${ABSL}/absl/strings/numbers.cc ${ABSL}/absl/strings/ascii.cc

all: plugin.wasm

%.wasm %.wat: %.cc ${CPP_API}/proxy_wasm_intrinsics.h ${CPP_API}/proxy_wasm_enums.h ${CPP_API}/proxy_wasm_externs.h ${CPP_API}/proxy_wasm_api.h ${CPP_API}/proxy_wasm_intrinsics.js ${CPP_CONTEXT_LIB}
        ls /root
                em++ -s WASM=1 -s BINARYEN_TRAP_MODE='clamp' -s LEGALIZE_JS_FFI=0 -s EMIT_EMSCRIPTEN_METADATA=1 --std=c++17 -O3 -g3 -I${CPP_API} -I${CPP_API}/google/protobuf -I/usr/local/include -I${ABSL} --js-library ${CPP_API}/proxy_wasm_intrinsics.js ${ABSL_CPP} $*.cc ${CPP_API}/proxy_wasm_intrinsics.pb.cc ${CPP_CONTEXT_LIB} ${CPP_API}/libprotobuf.bc -o $*.js
```

Precompiled abseil libraries are also available, so the above can also be done as:

```
DOCKER_SDK=/sdk
CPP_API:=${DOCKER_SDK}
CPP_CONTEXT_LIB = ${CPP_API}/proxy_wasm_intrinsics.cc
ABSL = /root/abseil-cpp
ABSL_LIBS = ${ABSL}/absl/strings/libabsl_strings.a ${ABSL}/absl/strings/libabsl_strings_internal.a  ${ABSL}/absl/strings/libabsl_str_format_internal.a

all: plugin.wasm

%.wasm %.wat: %.cc ${CPP_API}/proxy_wasm_intrinsics.h ${CPP_API}/proxy_wasm_enums.h ${CPP_API}/proxy_wasm_externs.h ${CPP_API}/proxy_wasm_api.h ${CPP_API}/proxy_wasm_intrinsics.js ${CPP_CONTEXT_LIB}
        ls /root
                em++ -s WASM=1 -s BINARYEN_TRAP_MODE='clamp' -s LEGALIZE_JS_FFI=0 -s EMIT_EMSCRIPTEN_METADATA=1 --std=c++17 -O3 -g3 -I${CPP_API} -I${CPP_API}/google/protobuf -I/usr/local/include -I${ABSL} --js-library ${CPP_API}/proxy_wasm_intrinsics.js  $*.cc ${CPP_API}/proxy_wasm_intrinsics.pb.cc ${CPP_CONTEXT_LIB} ${CPP_API}/libprotobuf.bc ${ABSL_LIBS} -o $*.js
```

### Ownership of the resulting .wasm files

The compiled files may be owned by root.  To chown them add a line in the Makefile where ID is the desired user id (e.g. the result of "id -u"):

```
DOCKER_SDK=/work/sdk

all: myproject.wasm
  chown ID.ID $^

include ${DOCKER_SDK}/Makefile.base_lite
```

## Dependencies for building WASM modules:

If you do not wish to use the Docker file, the dependencies can be installed by script (sdk\_container.sh), or by hand.

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

