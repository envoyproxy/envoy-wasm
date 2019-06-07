#!/bin/bash

set -e

# Get depot_tools.

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PATH:$PWD/depot_tools"

# Get v8.

VERSION=7.5.288.22

fetch v8
cd v8
git checkout "$VERSION"
gclient sync

# Patch wee8.

# 1. Fix handling of f64 globals.
# 2. Force full GC when destroying VMs.

cat <<\EOF | patch -p1
--- a/src/wasm/c-api.cc
+++ b/src/wasm/c-api.cc
@@ -825,7 +825,7 @@ void global_set_f32(v8::Local<v8::Object> global, float val) {
 void global_set_f64(v8::Local<v8::Object> global, double val) {
   auto v8_object = v8::Utils::OpenHandle<v8::Object, i::JSReceiver>(global);
   auto v8_global = i::Handle<i::WasmGlobalObject>::cast(v8_object);
-  v8_global->SetF32(val);
+  v8_global->SetF64(val);
 }

 // Tables
@@ -1107,7 +1107,7 @@ class StoreImpl {
   StoreImpl() {}

   ~StoreImpl() {
-#ifdef DEBUG
+#if 1
     reinterpret_cast<i::Isolate*>(isolate_)->heap()->PreciseCollectAllGarbage(
         i::Heap::kNoGCFlags, i::GarbageCollectionReason::kTesting,
         v8::kGCCallbackFlagForced);
EOF

# Build wee8 library.

gn gen out/wee8 --args="v8_monolithic=true v8_use_external_startup_data=false v8_enable_i18n_support=false v8_enable_gdbjit=false use_custom_libcxx=false is_component_build=false is_debug=false"
ninja -v -C out/wee8 wee8

# Install wee8 library.

mkdir -p "$THIRDPARTY_BUILD/include/wasm-c-api"
cp -p third_party/wasm-api/wasm.hh "$THIRDPARTY_BUILD/include/wasm-c-api/"
cp -p out/wee8/obj/libwee8.a "$THIRDPARTY_BUILD/lib/"
