#!/bin/bash

set -e

# LLVM-6.0 is a build-time dependency of WAVM.

VERSION=6.0.1
SHA256=7ea204ecd78c39154d72dfc0d4a79f7cce1b2264da2551bb2eef10e266d54d91

curl https://releases.llvm.org/"$VERSION"/clang+llvm-"$VERSION"-x86_64-linux-gnu-ubuntu-16.04.tar.xz \
  -sLo llvm-"$VERSION".tar.xz && echo "$SHA256" llvm-"$VERSION".tar.xz | sha256sum --check
tar xf llvm-"$VERSION".tar.xz

cd clang+llvm-"$VERSION"-x86_64-linux-gnu-ubuntu-16.04

mkdir -p "$THIRDPARTY_BUILD"/lib/llvm-6.0/
cp -pR bin/ "$THIRDPARTY_BUILD"/lib/llvm-6.0/bin/
cp -pR lib/ "$THIRDPARTY_BUILD"/lib/llvm-6.0/lib/
cp -pR include/ "$THIRDPARTY_BUILD"/lib/llvm-6.0/include/

# WAVM.

COMMIT=1e171bfc3ccfe2a6a4c9dd30a0d8dbc77cae3309 # 2018-10-29
SHA256=f17e48c1a453a0d2e91823a0f80fad2dc21d4e4c5ae77d8e2435fe83d9e2af84

curl https://github.com/WAVM/WAVM/archive/"$COMMIT".tar.gz -sLo WAVM-"$COMMIT".tar.gz \
  && echo "$SHA256" WAVM-"$COMMIT".tar.gz | sha256sum --check
tar xf WAVM-"$COMMIT".tar.gz
cd WAVM-"$COMMIT"

mkdir build
cd build

build_type=RelWithDebInfo

# Always build with clang.
cmake -G "Ninja" -DCMAKE_INSTALL_PREFIX:PATH="$THIRDPARTY_BUILD" \
  -DCMAKE_BUILD_TYPE="$build_type" \
  -DCMAKE_CXX_COMPILER:STRING="clang++-7" \
  -DCMAKE_C_COMPILER:STRING="clang-7" \
  -DCMAKE_CXX_FLAGS:STRING="${CXXFLAGS} ${CPPFLAGS}" \
  -DCMAKE_C_FLAGS:STRING="${CFLAGS} ${CPPFLAGS}" \
  -DLLVM_DIR:STRING="$THIRDPARTY_BUILD/lib/llvm-6.0/lib/cmake/llvm/" \
  -DWAVM_ENABLE_STATIC_LINKING:BOOL=ON \
  -DWAVM_ENABLE_RELEASE_ASSERTS:BOOL=ON \
  ..
ninja
ninja install
