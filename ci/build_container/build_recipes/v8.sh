#!/bin/bash

set -e

# Get wasm-c-api.

COMMIT=dc8cc29340e0c374dd350e0de8ff4b22d429af1e  # Thu May 16 21:03:37 2019 +0200
SHA256=dea56d64f58110aee585857bca279701d39bdcb2dc70bdb8eb02fe6a8571c714

curl https://github.com/WebAssembly/wasm-c-api/archive/"$COMMIT".tar.gz -sLo wasm-c-api-"$COMMIT".tar.gz \
  && echo "$SHA256" wasm-c-api-"$COMMIT".tar.gz | sha256sum --check
tar xf wasm-c-api-"$COMMIT".tar.gz
cd wasm-c-api-"$COMMIT"

# Build v8 inside v8 subdirectory to match wasm-c-api's Makefile.

mkdir v8
cd v8

# Get depot_tools.

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PATH:$PWD/depot_tools"

# Get v8.

VERSION=7.4.288.28  # match wasm-c-api (branch-heads/7.4)

fetch v8
cd v8
git checkout "$VERSION"
gclient sync

# Patch v8 for wasm-c-api.

patch -p1 < ../../patch/0001-BUILD.gn-add-wasm-v8-lowlevel.patch
cp -p ../../src/wasm-v8-lowlevel.cc src/wasm-v8-lowlevel.cc
cp -p ../../src/wasm-v8-lowlevel.hh include/wasm-v8-lowlevel.hh

# Build v8 static library.

tools/dev/v8gen.py x64.release -- v8_monolithic=true v8_use_external_startup_data=false v8_enable_i18n_support=false v8_enable_gdbjit=false use_custom_libcxx=false
ninja -v -C out.gn/x64.release v8_monolith

# Install v8.

mkdir -p "$THIRDPARTY_BUILD/include/v8/libplatform"
cp -p include/v8*.h "$THIRDPARTY_BUILD/include/v8/"
cp -p include/libplatform/*.h "$THIRDPARTY_BUILD/include/v8/libplatform/"
cp -p out.gn/x64.release/obj/libv8_monolith.a "$THIRDPARTY_BUILD/lib/"

cd ../..

# Patch wasm-c-api.

# 1. Disable DEBUG (alloc/free accounting), since it seems to be broken
#    in optimized builds and/or when using sanitizers.
# 2. Disable hardcoded sanitizers.

cat <<\EOF | patch -p1
--- a/Makefile
+++ b/Makefile
@@ -7,10 +7,10 @@ V8_VERSION = branch-heads/7.4
 V8_ARCH = x64
 V8_MODE = release

-WASM_FLAGS = -DDEBUG  # -DDEBUG_LOG
-C_FLAGS = ${WASM_FLAGS} -Wall -Werror -ggdb -O -fsanitize=address
+WASM_FLAGS =
+C_FLAGS = ${WASM_FLAGS} -Wall -Werror -ggdb -O
 CC_FLAGS = ${C_FLAGS}
-LD_FLAGS = -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor
+LD_FLAGS =

 C_COMP = clang
EOF

# 3. Force full GC when destroying VMs.

cat <<\EOF | patch -p1
--- a/src/wasm-v8.cc
+++ b/src/wasm-v8.cc
@@ -352,7 +352,7 @@ public:
   }

   ~StoreImpl() {
-#ifdef DEBUG
+#if 1 //def DEBUG
     isolate_->RequestGarbageCollectionForTesting(
       v8::Isolate::kFullGarbageCollection);
 #endif
EOF

# Build wasm-c-api.

# TODO(PiotrSikora): respect CC/CXX/CFLAGS/CXXFLAGS/LDFLAGS upstream.

make wasm

# Install wasm-c-api.

mkdir -p "$THIRDPARTY_BUILD/include/wasm-c-api"
cp -p include/wasm.hh "$THIRDPARTY_BUILD/include/wasm-c-api/"
cp -p src/wasm-bin.hh "$THIRDPARTY_BUILD/include/wasm-c-api/"
ar -r -c -s "$THIRDPARTY_BUILD/lib/libwasm.a" out/wasm-bin.o out/wasm-v8.o
