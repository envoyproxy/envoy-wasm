# DO NOT LOAD THIS FILE. Load envoy_build_system.bzl instead.
# Envoy test targets. This includes both test library and test binary targets.
load("@bazel_tools//tools/build_defs/pkg:pkg.bzl", "pkg_tar")
load(":envoy_binary.bzl", "envoy_cc_binary")
load(
    ":envoy_internal.bzl",
    "envoy_copts",
    "envoy_external_dep_path",
    "envoy_linkstatic",
    "envoy_select_force_libcpp",
    "envoy_stdlib_deps",
    "tcmalloc_external_dep",
)

# Envoy C++ related test infrastructure (that want gtest, gmock, but may be
# relied on by envoy_cc_test_library) should use this function.
def _envoy_cc_test_infrastructure_library(
        name,
        srcs = [],
        hdrs = [],
        data = [],
        external_deps = [],
        deps = [],
        repository = "",
        tags = [],
        include_prefix = None,
        copts = [],
        **kargs):
    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        data = data,
        copts = envoy_copts(repository, test = True) + copts,
        testonly = 1,
        deps = deps + [envoy_external_dep_path(dep) for dep in external_deps] + [
            envoy_external_dep_path("googletest"),
        ],
        tags = tags,
        include_prefix = include_prefix,
        alwayslink = 1,
        linkstatic = envoy_linkstatic(),
        **kargs
    )

# Compute the test linkopts based on various options.
def _envoy_test_linkopts():
    return select({
        "@envoy//bazel:apple": [
            # See note here: https://luajit.org/install.html
            "-pagezero_size 10000",
            "-image_base 100000000",
        ],
        "@envoy//bazel:windows_x86_64": [
            "-DEFAULTLIB:advapi32.lib",
            "-DEFAULTLIB:ws2_32.lib",
            "-WX",
        ],

        # TODO(mattklein123): It's not great that we universally link against the following libs.
        # In particular, -latomic and -lrt are not needed on all platforms. Make this more granular.
        "//conditions:default": ["-pthread", "-lrt", "-ldl"],
    }) + envoy_select_force_libcpp([], ["-lstdc++fs", "-latomic"])

# Envoy C++ fuzz test targets. These are not included in coverage runs.
def envoy_cc_fuzz_test(
        name,
        corpus,
        repository = "",
        size = "medium",
        deps = [],
        tags = [],
        **kwargs):
    if not (corpus.startswith("//") or corpus.startswith(":") or corpus.startswith("@")):
        corpus_name = name + "_corpus"
        corpus = native.glob([corpus + "/**"])
        native.filegroup(
            name = corpus_name,
            srcs = corpus,
        )
    else:
        corpus_name = corpus
    pkg_tar(
        name = name + "_corpus_tar",
        srcs = [corpus_name],
        testonly = 1,
    )
    test_lib_name = name + "_lib"
    envoy_cc_test_library(
        name = test_lib_name,
        deps = deps + [
            repository + "//test/fuzz:fuzz_runner_lib",
            repository + "//bazel:dynamic_stdlib",
        ],
        repository = repository,
        tags = tags,
        **kwargs
    )
    native.cc_test(
        name = name,
        copts = envoy_copts("@envoy", test = True),
        linkopts = _envoy_test_linkopts(),
        linkstatic = 1,
        args = ["$(locations %s)" % corpus_name],
        data = [corpus_name],
        # No fuzzing on macOS.
        deps = select({
            "@envoy//bazel:apple": [repository + "//test:dummy_main"],
            "//conditions:default": [
                ":" + test_lib_name,
                repository + "//test/fuzz:main",
            ],
        }),
        size = size,
        tags = tags,
    )

    # This target exists only for
    # https://github.com/google/oss-fuzz/blob/master/projects/envoy/build.sh. It won't yield
    # anything useful on its own, as it expects to be run in an environment where the linker options
    # provide a path to FuzzingEngine.
    native.cc_binary(
        name = name + "_driverless",
        copts = envoy_copts("@envoy", test = True),
        linkopts = ["-lFuzzingEngine"] + _envoy_test_linkopts(),
        linkstatic = 1,
        testonly = 1,
        deps = [":" + test_lib_name],
        tags = ["manual"] + tags,
    )

    native.cc_test(
        name = name + "_with_libfuzzer",
        copts = envoy_copts("@envoy", test = True),
        linkopts = ["-fsanitize=fuzzer"] + _envoy_test_linkopts(),
        linkstatic = 1,
        testonly = 1,
        data = [corpus_name],
        deps = [":" + test_lib_name],
        tags = ["manual", "fuzzer"] + tags,
    )

# Envoy C++ test targets should be specified with this function.
def envoy_cc_test(
        name,
        srcs = [],
        data = [],
        # List of pairs (Bazel shell script target, shell script args)
        repository = "",
        external_deps = [],
        deps = [],
        tags = [],
        args = [],
        copts = [],
        shard_count = None,
        coverage = True,
        local = False,
        size = "medium"):
    if coverage:
        coverage_tags = tags + ["coverage_test_lib"]
    else:
        coverage_tags = tags
    _envoy_cc_test_infrastructure_library(
        name = name + "_lib_internal_only",
        srcs = srcs,
        data = data,
        external_deps = external_deps,
        deps = deps + [repository + "//test/test_common:printers_includes"],
        repository = repository,
        tags = coverage_tags,
        copts = copts,
        # Allow public visibility so these can be consumed in coverage tests in external projects.
        visibility = ["//visibility:public"],
    )
    if coverage:
        coverage_tags = tags + ["coverage_test"]
    native.cc_test(
        name = name,
        copts = envoy_copts(repository, test = True) + copts,
        linkopts = _envoy_test_linkopts(),
        linkstatic = envoy_linkstatic(),
        malloc = tcmalloc_external_dep(repository),
        deps = envoy_stdlib_deps() + [
            ":" + name + "_lib_internal_only",
            repository + "//test:main",
        ],
        # from https://github.com/google/googletest/blob/6e1970e2376c14bf658eb88f655a054030353f9f/googlemock/src/gmock.cc#L51
        # 2 - by default, mocks act as StrictMocks.
        args = args + ["--gmock_default_mock_behavior=2"],
        tags = coverage_tags,
        local = local,
        shard_count = shard_count,
        size = size,
    )

# Envoy C++ test related libraries (that want gtest, gmock) should be specified
# with this function.
def envoy_cc_test_library(
        name,
        srcs = [],
        hdrs = [],
        data = [],
        external_deps = [],
        deps = [],
        repository = "",
        tags = [],
        include_prefix = None,
        copts = [],
        **kargs):
    deps = deps + [
        repository + "//test/test_common:printers_includes",
    ]
    _envoy_cc_test_infrastructure_library(
        name,
        srcs,
        hdrs,
        data,
        external_deps,
        deps,
        repository,
        tags,
        include_prefix,
        copts,
        visibility = ["//visibility:public"],
        **kargs
    )

# Envoy test binaries should be specified with this function.
def envoy_cc_test_binary(
        name,
        **kargs):
    envoy_cc_binary(
        name,
        testonly = 1,
        linkopts = _envoy_test_linkopts(),
        **kargs
    )

# Envoy Python test binaries should be specified with this function.
def envoy_py_test_binary(
        name,
        external_deps = [],
        deps = [],
        **kargs):
    native.py_binary(
        name = name,
        deps = deps + [envoy_external_dep_path(dep) for dep in external_deps],
        **kargs
    )

# Envoy C++ mock targets should be specified with this function.
def envoy_cc_mock(name, **kargs):
    envoy_cc_test_library(name = name, **kargs)

# Envoy shell tests that need to be included in coverage run should be specified with this function.
def envoy_sh_test(
        name,
        srcs = [],
        data = [],
        coverage = True,
        tags = [],
        **kargs):
    if coverage:
        test_runner_cc = name + "_test_runner.cc"
        native.genrule(
            name = name + "_gen_test_runner",
            srcs = srcs,
            outs = [test_runner_cc],
            cmd = "$(location //bazel:gen_sh_test_runner.sh) $(SRCS) >> $@",
            tools = ["//bazel:gen_sh_test_runner.sh"],
        )
        envoy_cc_test_library(
            name = name + "_lib",
            srcs = [test_runner_cc],
            data = srcs + data,
            tags = tags + ["coverage_test_lib"],
            deps = ["//test/test_common:environment_lib"],
        )
    native.sh_test(
        name = name,
        srcs = ["//bazel:sh_test_wrapper.sh"],
        data = srcs + data,
        args = srcs,
        tags = tags,
        **kargs
    )
