licenses(["notice"])  # Apache 2

load("//bazel:envoy_build_system.bzl", "envoy_cmake_external", "envoy_package")
load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")

envoy_package()

configure_make(
    name = "gperftools_build",
    configure_options = [
        "--enable-shared=no",
        "--enable-frame-pointers",
        "--disable-libunwind",
    ],
    lib_source = "@com_github_gperftools_gperftools//:all",
    linkopts = ["-lpthread"],
    make_commands = ["make install-libLTLIBRARIES install-perftoolsincludeHEADERS"],
    static_libraries = select({
        "//bazel:debug_tcmalloc": ["libtcmalloc_debug.a"],
        "//conditions:default": ["libtcmalloc_and_profiler.a"],
    }),
)

# Workaround for https://github.com/bazelbuild/rules_foreign_cc/issues/227
cc_library(
    name = "gperftools",
    deps = [
        "gperftools_build",
    ],
)

configure_make(
    name = "luajit",
    configure_command = "build.py",
    configure_env_vars = select({
        # This shouldn't be needed! See
        # https://github.com/envoyproxy/envoy/issues/6084
        # TODO(htuch): Remove when #6084 is fixed
        "//bazel:asan_build": {"ENVOY_CONFIG_ASAN": "1"},
        "//conditions:default": {},
    }),
    lib_source = "@com_github_luajit_luajit//:all",
    make_commands = [],
    static_libraries = [
        "libluajit-5.1.a",
    ],
)

envoy_cmake_external(
    name = "ares",
    cache_entries = {
        "CARES_SHARED": "no",
        "CARES_STATIC": "on",
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        # Disable ranlib because it is not handled by bazel, and therefore
        # doesn't respect custom toolchains such as the Android NDK,
        # see https://github.com/bazelbuild/rules_foreign_cc/issues/252
        "CMAKE_RANLIB": "",
    },
    copy_pdb = True,
    lib_source = "@com_github_c_ares_c_ares//:all",
    pdb_name = "c-ares",
    static_libraries = select({
        "//bazel:windows_x86_64": ["cares.lib"],
        "//conditions:default": ["libcares.a"],
    }),
)

envoy_cmake_external(
    name = "curl",
    cache_entries = {
        "BUILD_CURL_EXE": "off",
        "BUILD_SHARED_LIBS": "off",
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "HTTP_ONLY": "on",
        "BUILD_TESTING": "off",
        "CMAKE_USE_OPENSSL": "off",
        "CURL_CA_PATH": "none",
        "CURL_HIDDEN_SYMBOLS": "off",
        "CMAKE_USE_LIBSSH2": "off",
        "CMAKE_INSTALL_LIBDIR": "lib",
    },
    lib_source = "@com_github_curl//:all",
    static_libraries = select({
        "//bazel:windows_x86_64": ["curl.lib"],
        "//conditions:default": ["libcurl.a"],
    }),
)

envoy_cmake_external(
    name = "event",
    cache_entries = {
        "EVENT__DISABLE_OPENSSL": "on",
        "EVENT__DISABLE_REGRESS": "on",
        "EVENT__DISABLE_TESTS": "on",
        "EVENT__LIBRARY_TYPE": "STATIC",
        "CMAKE_BUILD_TYPE": "Release",
        # Disable ranlib because it is not handled by bazel, and therefore
        # doesn't respect custom toolchains such as the Android NDK,
        # see https://github.com/bazelbuild/rules_foreign_cc/issues/252
        "CMAKE_RANLIB": "",
        # Force _GNU_SOURCE on for Android builds. This would be contained in
        # a 'select' but the downstream macro uses a select on all of these
        # options, and they cannot be nested.
        # If https://github.com/bazelbuild/rules_foreign_cc/issues/289 is fixed
        # this can be removed.
        # More details https://github.com/lyft/envoy-mobile/issues/116
        "_GNU_SOURCE": "on",
    },
    copy_pdb = True,
    lib_source = "@com_github_libevent_libevent//:all",
    static_libraries = select({
        "//bazel:windows_x86_64": ["event.lib"],
        "//conditions:default": [
            "libevent.a",
            "libevent_pthreads.a",
        ],
    }),
)

envoy_cmake_external(
    name = "llvm",
    cache_entries = {
        # Disable both: BUILD and INCLUDE, since some of the INCLUDE
        # targets build code instead of only generating build files.
        "LLVM_BUILD_DOCS": "off",
        "LLVM_INCLUDE_DOCS": "off",
        "LLVM_BUILD_EXAMPLES": "off",
        "LLVM_INCLUDE_EXAMPLES": "off",
        "LLVM_BUILD_RUNTIME": "off",
        "LLVM_BUILD_RUNTIMES": "off",
        "LLVM_INCLUDE_RUNTIMES": "off",
        "LLVM_BUILD_TESTS": "off",
        "LLVM_INCLUDE_TESTS": "off",
        "LLVM_BUILD_TOOLS": "off",
        "LLVM_INCLUDE_TOOLS": "off",
        "LLVM_BUILD_UTILS": "off",
        "LLVM_INCLUDE_UTILS": "off",
        "LLVM_ENABLE_LIBEDIT": "off",
        "LLVM_ENABLE_LIBXML2": "off",
        "LLVM_ENABLE_TERMINFO": "off",
        "LLVM_ENABLE_ZLIB": "off",
        "LLVM_TARGETS_TO_BUILD": "X86",
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
    },
    env_vars = {
        # Workaround for the -DDEBUG flag added in fastbuild on macOS,
        # which conflicts with DEBUG macro used in LLVM.
        "CFLAGS": "-UDEBUG",
        "CXXFLAGS": "-UDEBUG",
        "ASMFLAGS": "-UDEBUG",
    },
    lib_source = "@org_llvm_llvm//:all",
    static_libraries = select({
        "//conditions:default": [
            # Order from llvm-config --libnames.
            "libLLVMLTO.a",
            "libLLVMPasses.a",
            "libLLVMObjCARCOpts.a",
            "libLLVMSymbolize.a",
            "libLLVMDebugInfoPDB.a",
            "libLLVMDebugInfoDWARF.a",
            "libLLVMFuzzMutate.a",
            "libLLVMTableGen.a",
            "libLLVMDlltoolDriver.a",
            "libLLVMLineEditor.a",
            "libLLVMOrcJIT.a",
            "libLLVMCoverage.a",
            "libLLVMMIRParser.a",
            "libLLVMObjectYAML.a",
            "libLLVMLibDriver.a",
            "libLLVMOption.a",
            "libLLVMWindowsManifest.a",
            "libLLVMX86Disassembler.a",
            "libLLVMX86AsmParser.a",
            "libLLVMX86CodeGen.a",
            "libLLVMGlobalISel.a",
            "libLLVMSelectionDAG.a",
            "libLLVMAsmPrinter.a",
            "libLLVMDebugInfoCodeView.a",
            "libLLVMDebugInfoMSF.a",
            "libLLVMX86Desc.a",
            "libLLVMMCDisassembler.a",
            "libLLVMX86Info.a",
            "libLLVMX86AsmPrinter.a",
            "libLLVMX86Utils.a",
            "libLLVMMCJIT.a",
            "libLLVMInterpreter.a",
            "libLLVMExecutionEngine.a",
            "libLLVMRuntimeDyld.a",
            "libLLVMCodeGen.a",
            "libLLVMTarget.a",
            "libLLVMCoroutines.a",
            "libLLVMipo.a",
            "libLLVMInstrumentation.a",
            "libLLVMVectorize.a",
            "libLLVMScalarOpts.a",
            "libLLVMLinker.a",
            "libLLVMIRReader.a",
            "libLLVMAsmParser.a",
            "libLLVMInstCombine.a",
            "libLLVMTransformUtils.a",
            "libLLVMBitWriter.a",
            "libLLVMAnalysis.a",
            "libLLVMProfileData.a",
            "libLLVMObject.a",
            "libLLVMMCParser.a",
            "libLLVMMC.a",
            "libLLVMBitReader.a",
            "libLLVMCore.a",
            "libLLVMBinaryFormat.a",
            "libLLVMSupport.a",
            "libLLVMDemangle.a",
        ],
    }),
)

envoy_cmake_external(
    name = "nghttp2",
    cache_entries = {
        "ENABLE_STATIC_LIB": "on",
        "ENABLE_LIB_ONLY": "on",
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "CMAKE_INSTALL_LIBDIR": "lib",
        "CMAKE_CXX_COMPILER_FORCED": "on",
        # Disable ranlib because it is not handled by bazel, and therefore
        # doesn't respect custom toolchains such as the Android NDK,
        # see https://github.com/bazelbuild/rules_foreign_cc/issues/252
        "CMAKE_RANLIB": "",
    },
    cmake_files_dir = "$BUILD_TMPDIR/lib/CMakeFiles",
    copy_pdb = True,
    debug_cache_entries = {"ENABLE_DEBUG": "on"},
    lib_source = "@com_github_nghttp2_nghttp2//:all",
    pdb_name = "nghttp2_static",
    static_libraries = select({
        "//bazel:windows_x86_64": ["nghttp2_static.lib"],
        "//conditions:default": ["libnghttp2_static.a"],
    }),
)

envoy_cmake_external(
    name = "wavm",
    cache_entries = {
        "LLVM_DIR": "$EXT_BUILD_DEPS/copy_llvm/llvm/lib/cmake/llvm",
        "WAVM_ENABLE_STATIC_LINKING": "on",
        "WAVM_ENABLE_RELEASE_ASSERTS": "on",
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
    },
    env_vars = {
        # Workaround for the -DDEBUG flag added in fastbuild on macOS,
        # which conflicts with DEBUG macro used in LLVM.
        "CFLAGS": "-UDEBUG",
        "CXXFLAGS": "-UDEBUG",
        "ASMFLAGS": "-UDEBUG",
    },
    lib_source = "@com_github_wavm_wavm//:all",
    static_libraries = select({
        "//conditions:default": [
            "libWAVM.a",
            "WAVM/libWAVMUnwind.a",
        ],
    }),
    deps = [":llvm"],
)

envoy_cmake_external(
    name = "yaml",
    cache_entries = {
        "YAML_CPP_BUILD_TESTS": "off",
        "YAML_CPP_BUILD_TOOLS": "off",
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "CMAKE_CXX_COMPILER_FORCED": "on",
        # Disable ranlib because it is not handled by bazel, and therefore
        # doesn't respect custom toolchains such as the Android NDK,
        # see https://github.com/bazelbuild/rules_foreign_cc/issues/252
        "CMAKE_RANLIB": "",
    },
    lib_source = "@com_github_jbeder_yaml_cpp//:all",
    static_libraries = select({
        "//bazel:windows_dbg_build": ["libyaml-cppmdd.lib"],
        "//bazel:windows_fastbuild_build": ["libyaml-cppmd.lib"],
        "//bazel:windows_opt_build": ["libyaml-cppmd.lib"],
        "//conditions:default": ["libyaml-cpp.a"],
    }),
)

envoy_cmake_external(
    name = "zlib",
    cache_entries = {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        # Disable ranlib because it is not handled by bazel, and therefore
        # doesn't respect custom toolchains such as the Android NDK,
        # see https://github.com/bazelbuild/rules_foreign_cc/issues/252
        "CMAKE_RANLIB": "",
    },
    copy_pdb = True,
    lib_source = "@net_zlib//:all",
    pdb_name = "zlibstatic",
    static_libraries = select({
        "//bazel:windows_dbg_build": ["zlibstaticd.lib"],
        "//bazel:windows_fastbuild_build": ["zlibstatic.lib"],
        "//bazel:windows_opt_build": ["zlibstatic.lib"],
        "//conditions:default": ["libz.a"],
    }),
)
