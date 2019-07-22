WebAssembly Extension Support

# Build Dependencies

## glib2.0

Note: this may be required on Debian/Ubuntu.

apt-get install libglib2.0-dev

## emscripten

git clone https://github.com/emscripten-core/emsdk.git  
cd emsdk  
./emsdk install sdk-1.38.25-64bit  
./emsdk activate sdk-1.38.25-64bit  

. ./emsdk\_env.sh  

It is possible later versions will work, e.g.

./emsdk install latest  
./emsdk activate latest  

However 1.38.25 is known to work.

## clang-7 or clang-8

export CC=clang  
export CXX=clang++  

Note: ensure that you have clang in your path (e.g. /usr/lib/llvm-7/bin).

## protobuf v3.6.1

git clone https://github.com/protocolbuffers/protobuf  
cd protobuf  
git checkout v3.6.1  
git submodule update --init --recursive  
./autogen.sh  
./configure  
make  
make check  
sudo make install  

# Dependencies for regenerating test modules

## WAVM binaries if you want to rebuild the c++ WebAssembly tests

git clone git@github.com:WAVM/WAVM.git  
cd WAVM  
cmake "."  
make  
sudo make install  

Note: ensure /usr/local/bin is in your path

## rust if you want to use it or rebuild the rust WebAssembly tests

curl https://sh.rustup.rs -sSf | sh

# Building

Building with WebAssembly support requires enabling one or more WebAssembly runtime via the "wasm" define:

bazel build --define wasm=enabled --explain=~/bazel.log //source/exe:envoy-static

The options are "wavm", "v8" and "enable" (for both wavm and v8).

This option must be provided for all build/test targets.
