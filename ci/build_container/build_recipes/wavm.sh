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

COMMIT=da97d6bcf421ac5d8b40ccccc8d36d4ba01a0356 # 2018-11-15 with a bunch of Emscripten intrinsics.
SHA256=9552e1997e04a420c7df03b54443f82cfaa0aed00ab9e2b26077f47f31d5fff4

curl https://github.com/PiotrSikora/WAVM/archive/"$COMMIT".tar.gz -sLo WAVM-"$COMMIT".tar.gz \
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
