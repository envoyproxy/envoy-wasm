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

# 1. Disable hardcoded sanitizers.

cat <<\EOF | patch -p1
--- a/Makefile
+++ b/Makefile
@@ -8,9 +8,9 @@ V8_ARCH = x64
 V8_MODE = release

 WASM_FLAGS = -DDEBUG  # -DDEBUG_LOG
-C_FLAGS = ${WASM_FLAGS} -Wall -Werror -ggdb -O -fsanitize=address
+C_FLAGS = ${WASM_FLAGS} -Wall -Werror -ggdb -O
 CC_FLAGS = ${C_FLAGS}
-LD_FLAGS = -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor
+LD_FLAGS =

 C_COMP = clang

EOF

# 2. Don't leak implementation into headers.

cat <<\EOF | patch -p1
--- a/include/wasm.hh
+++ b/include/wasm.hh
@@ -111,13 +111,8 @@ class vec {
   size_t size_;
   std::unique_ptr<T[]> data_;

-#ifdef DEBUG
   void make_data();
   void free_data();
-#else
-  void make_data() {}
-  void free_data() {}
-#endif

   vec(size_t size) : vec(size, size ? new(std::nothrow) T[size] : nullptr) {
     make_data();
--- a/src/wasm-v8.cc
+++ b/src/wasm-v8.cc
@@ -210,6 +210,14 @@ Stats stats;
     if (data_) stats.free(Stats::STAT, data_.get(), Stats::VEC); \
   }

+#else
+
+#define DEFINE_VEC(type, STAT) \
+  template<> void vec<type>::make_data() {} \
+  template<> void vec<type>::free_data() {}
+
+#endif  // #ifdef DEBUG
+
 DEFINE_VEC(byte_t, BYTE)
 DEFINE_VEC(Frame*, FRAME)
 DEFINE_VEC(ValType*, VALTYPE)
@@ -231,8 +239,6 @@ DEFINE_VEC(Memory*, MEMORY)
 DEFINE_VEC(Extern*, EXTERN)
 DEFINE_VEC(Val, VAL)

-#endif  // #ifdef DEBUG
-

 ///////////////////////////////////////////////////////////////////////////////
 // Runtime Environment

EOF

# Build wasm-c-api.

# TODO(PiotrSikora): respect CC/CXX/CFLAGS/CXXFLAGS/LDFLAGS upstream.

make wasm

# Install wasm-c-api.

mkdir -p "$THIRDPARTY_BUILD/include/wasm-c-api"
cp -p include/wasm.hh "$THIRDPARTY_BUILD/include/wasm-c-api/"
cp -p src/wasm-bin.hh "$THIRDPARTY_BUILD/include/wasm-c-api/"
ar -r -c -s "$THIRDPARTY_BUILD/lib/libwasm.a" out/wasm-bin.o out/wasm-v8.o
