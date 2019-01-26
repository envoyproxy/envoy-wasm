#!/bin/bash

set -e

# LLVM-6.0 is a build-time dependency of WAVM.

if [[ `uname` == "Darwin" ]]; then
  CC="clang"
  CXX="clang++"
  VERSION=6.0.0
  PLATFORM="x86_64-apple-darwin"
  SHA256=0ef8e99e9c9b262a53ab8f2821e2391d041615dd3f3ff36fdf5370916b0f4268
else
  CC="clang-7"
  CXX="clang++-7"
  VERSION=6.0.1
  PLATFORM="x86_64-linux-gnu-ubuntu-16.04"
  SHA256=7ea204ecd78c39154d72dfc0d4a79f7cce1b2264da2551bb2eef10e266d54d91
fi

curl https://releases.llvm.org/"$VERSION"/clang+llvm-"$VERSION"-"$PLATFORM".tar.xz \
  -sLo llvm-"$VERSION".tar.xz && echo "$SHA256" llvm-"$VERSION".tar.xz | sha256sum --check
tar xf llvm-"$VERSION".tar.xz

cd clang+llvm-"$VERSION"-"$PLATFORM"

mkdir -p "$THIRDPARTY_BUILD"/lib/llvm-6.0/
cp -pR bin/ "$THIRDPARTY_BUILD"/lib/llvm-6.0/bin/
cp -pR lib/ "$THIRDPARTY_BUILD"/lib/llvm-6.0/lib/
cp -pR include/ "$THIRDPARTY_BUILD"/lib/llvm-6.0/include/

# WAVM.

COMMIT=69d647ecf6d355921b79a58aa6b1d674a76b4c2b # 2019-01-24
SHA256=b0789161eb96cc0695cda6e0ea0b2db0bd1919795cebf5d139d66a173234d301

curl https://github.com/WAVM/WAVM/archive/"$COMMIT".tar.gz -sLo WAVM-"$COMMIT".tar.gz \
  && echo "$SHA256" WAVM-"$COMMIT".tar.gz | sha256sum --check
tar xf WAVM-"$COMMIT".tar.gz
cd WAVM-"$COMMIT"

mkdir out
cd out

build_type=RelWithDebInfo

# Always build with clang.
cmake -G "Ninja" -DCMAKE_INSTALL_PREFIX:PATH="$THIRDPARTY_BUILD" \
  -DCMAKE_BUILD_TYPE="$build_type" \
  -DCMAKE_CXX_COMPILER:STRING="${CXX}" \
  -DCMAKE_C_COMPILER:STRING="${CC}" \
  -DCMAKE_CXX_FLAGS:STRING="${CXXFLAGS} ${CPPFLAGS}" \
  -DCMAKE_C_FLAGS:STRING="${CFLAGS} ${CPPFLAGS}" \
  -DLLVM_DIR:STRING="$THIRDPARTY_BUILD/lib/llvm-6.0/lib/cmake/llvm/" \
  -DWAVM_ENABLE_STATIC_LINKING:BOOL=ON \
  -DWAVM_ENABLE_RELEASE_ASSERTS:BOOL=ON \
  ..
ninja
ninja install
