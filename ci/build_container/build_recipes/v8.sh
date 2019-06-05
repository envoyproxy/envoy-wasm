#!/bin/bash

set -e

# Get depot_tools.

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PATH:$PWD/depot_tools"

# Get v8.

VERSION=7.5.288.22

cat >.gclient <<\EOF
solutions = [
  {
    "url"         : "https://chromium.googlesource.com/v8/v8.git",
    "name"        : "v8",
    "managed"     : False,
    "deps_file"   : "DEPS",
    "custom_deps" : {
      "v8/buildtools/clang_format/script"        : None,
      "v8/buildtools/third_party/libc++abi/trunk": None,
      "v8/buildtools/third_party/libc++/trunk"   : None,
      "v8/buildtools/third_party/libunwind/trunk": None,
      "v8/test/benchmarks/data"                  : None,
      "v8/test/mozilla/data"                     : None,
      "v8/test/test262/data"                     : None,
      "v8/test/test262/harness"                  : None,
      "v8/test/wasm-js/data"                     : None,
      "v8/third_party/android_ndk"               : None,
      "v8/third_party/android_sdk/public"        : None,
      "v8/third_party/android_tools"             : None,
      "v8/third_party/catapult"                  : None,
      "v8/third_party/colorama/src"              : None,
      "v8/third_party/fuchsia-sdk"               : None,
      "v8/third_party/icu"                       : None,
      "v8/third_party/instrumented_libraries"    : None,
      "v8/third_party/perfetto"                  : None,
      "v8/third_party/protobuf"                  : None,
      "v8/third_party/qemu-linux-x64"            : None,
      "v8/third_party/qemu-mac-x64"              : None,
      "v8/tools/luci-go"                         : None,
      "v8/tools/swarming_client"                 : None,
    },
  },
]
EOF

gclient sync --nohooks --no-history --revision v8@"$VERSION"

# Don't download x86 sysroot on x64 host.

cat <<\EOF | patch -p1
--- a/v8/DEPS
+++ b/v8/DEPS
@@ -371,7 +371,7 @@ hooks = [
   {
     'name': 'sysroot_x86',
     'pattern': '.',
-    'condition': '(checkout_linux and (checkout_x86 or checkout_x64))',
+    'condition': '(checkout_linux and checkout_x86)',
     'action': ['python', 'v8/build/linux/sysroot_scripts/install-sysroot.py',
                '--arch=x86'],
   },
EOF

gclient runhooks

# Patch wee8.

# 1. Fix handling of f64 globals.
# 2. Force full GC when destroying VMs.

cat <<\EOF | patch -p1
--- a/v8/src/wasm/c-api.cc
+++ b/v8/src/wasm/c-api.cc
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

cd v8
gn gen out/wee8 --args="v8_monolithic=true v8_use_external_startup_data=false v8_enable_i18n_support=false v8_enable_gdbjit=false use_custom_libcxx=false is_component_build=false is_debug=false"
ninja -v -C out/wee8 wee8

# Install wee8 library.

mkdir -p "$THIRDPARTY_BUILD/include/wasm-c-api"
cp -p third_party/wasm-api/wasm.hh "$THIRDPARTY_BUILD/include/wasm-c-api/"
cp -p out/wee8/obj/libwee8.a "$THIRDPARTY_BUILD/lib/"
