# Validation of content in this file is done on the bazel/repositories.bzl file to make it free of bazel
# constructs. This is to allow this file to be loaded into Python based build and maintenance tools.

# Envoy dependencies may be annotated with the following attributes:
DEPENDENCY_ANNOTATIONS = [
    # List of the categories describing how the dependency is being used. This attribute is used
    # for automatic tracking of security posture of Envoy's dependencies.
    # Possible values are documented in the USE_CATEGORIES list below.
    # This attribute is mandatory for each dependecy.
    "use_category",

    # Attribute specifying CPE (Common Platform Enumeration, see https://nvd.nist.gov/products/cpe) ID
    # of the dependency. The ID may be in v2.3 or v2.2 format, although v2.3 is prefferred. See
    # https://nvd.nist.gov/products/cpe for CPE format. Use single wildcard '*' for version and vector elements
    # i.e. 'cpe:2.3:a:nghttp2:nghttp2:*'. Use "N/A" for dependencies without CPE assigned.
    # This attribute is optional for components with use categories listed in the
    # USE_CATEGORIES_WITH_CPE_OPTIONAL
    "cpe",
]

# NOTE: If a dependency use case is either dataplane or controlplane, the other uses are not needed
# to be declared.
USE_CATEGORIES = [
    # This dependency is used in build process.
    "build",
    # This dependency is used for unit tests.
    "test",
    # This dependency is used in API protos.
    "api",
    # This dependency is used in processing downstream or upstream requests.
    "dataplane",
    # This dependency is used to process xDS requests.
    "controlplane",
    # This dependecy is used for logging, metrics or tracing. It may process unstrusted input.
    "observability",
    # This dependency does not handle untrusted data and is used for various utility purposes.
    "other",
]

# Components with these use categories are not required to specify the 'cpe' annotation.
USE_CATEGORIES_WITH_CPE_OPTIONAL = ["build", "test", "other"]

DEPENDENCY_REPOSITORIES = dict(
    bazel_compdb = dict(
        sha256 = "87e376a685eacfb27bcc0d0cdf5ded1d0b99d868390ac50f452ba6ed781caffe",
        strip_prefix = "bazel-compilation-database-0.4.2",
        urls = ["https://github.com/grailbio/bazel-compilation-database/archive/0.4.2.tar.gz"],
        use_category = ["build"],
    ),
    bazel_gazelle = dict(
        sha256 = "86c6d481b3f7aedc1d60c1c211c6f76da282ae197c3b3160f54bd3a8f847896f",
        urls = ["https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.19.1/bazel-gazelle-v0.19.1.tar.gz"],
        use_category = ["build"],
    ),
    bazel_toolchains = dict(
        sha256 = "144290c4166bd67e76a54f96cd504ed86416ca3ca82030282760f0823c10be48",
        strip_prefix = "bazel-toolchains-3.1.1",
        urls = [
            "https://github.com/bazelbuild/bazel-toolchains/releases/download/3.1.1/bazel-toolchains-3.1.1.tar.gz",
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-toolchains/archive/3.1.1.tar.gz",
        ],
        use_category = ["build"],
    ),
    build_bazel_rules_apple = dict(
        sha256 = "7a7afdd4869bb201c9352eed2daf37294d42b093579b70423490c1b4d4f6ce42",
        urls = ["https://github.com/bazelbuild/rules_apple/releases/download/0.19.0/rules_apple.0.19.0.tar.gz"],
        use_category = ["build"],
    ),
    envoy_build_tools = dict(
        sha256 = "78e794ae1c1197f59b7ecbf8bd62c053ecb1625daaccdbe287581ee6f12ec0fb",
        strip_prefix = "envoy-build-tools-b47394aa94c45e15c479d18eab18ffd43ec62d89",
        # 2020-05-14
        urls = ["https://github.com/envoyproxy/envoy-build-tools/archive/b47394aa94c45e15c479d18eab18ffd43ec62d89.tar.gz"],
        use_category = ["build"],
    ),
    boringssl = dict(
        sha256 = "8ae14b52b7889cf92f3b107610b12afb5011506c77f90c7b3d4a36ed7283905a",
        strip_prefix = "boringssl-107c03cf6d364939469194396bf7a6b2572d0f9c",
        # To update BoringSSL, which tracks Chromium releases:
        # 1. Open https://omahaproxy.appspot.com/ and note <current_version> of linux/stable release.
        # 2. Open https://chromium.googlesource.com/chromium/src/+/refs/tags/<current_version>/DEPS and note <boringssl_revision>.
        # 3. Find a commit in BoringSSL's "master-with-bazel" branch that merges <boringssl_revision>.
        #
        # chromium-83.0.4103.62
        # 2020-03-16
        urls = ["https://github.com/google/boringssl/archive/107c03cf6d364939469194396bf7a6b2572d0f9c.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    boringssl_fips = dict(
        sha256 = "b12ad676ee533824f698741bd127f6fbc82c46344398a6d78d25e62c6c418c73",
        # fips-20180730
        urls = ["https://commondatastorage.googleapis.com/chromium-boringssl-docs/fips/boringssl-66005f41fbc3529ffe8d007708756720529da20d.tar.xz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_google_absl = dict(
        sha256 = "cd477bfd0d19f803f85d118c7943b7908930310d261752730afa981118fee230",
        strip_prefix = "abseil-cpp-ca9856cabc23d771bcce634677650eb6fc4363ae",
        # 2020-04-30
        urls = ["https://github.com/abseil/abseil-cpp/archive/ca9856cabc23d771bcce634677650eb6fc4363ae.tar.gz"],
        use_category = ["dataplane", "controlplane"],
        cpe = "N/A",
    ),
    com_github_apache_thrift = dict(
        sha256 = "7d59ac4fdcb2c58037ebd4a9da5f9a49e3e034bf75b3f26d9fe48ba3d8806e6b",
        strip_prefix = "thrift-0.11.0",
        urls = ["https://files.pythonhosted.org/packages/c6/b4/510617906f8e0c5660e7d96fbc5585113f83ad547a3989b80297ac72a74c/thrift-0.11.0.tar.gz"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:apache:thrift:*",
    ),
    com_github_c_ares_c_ares = dict(
        sha256 = "d08312d0ecc3bd48eee0a4cc0d2137c9f194e0a28de2028928c0f6cae85f86ce",
        strip_prefix = "c-ares-1.16.1",
        urls = ["https://github.com/c-ares/c-ares/releases/download/cares-1_16_1/c-ares-1.16.1.tar.gz"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:c-ares_project:c-ares:*",
    ),
    com_github_circonus_labs_libcircllhist = dict(
        sha256 = "8165aa25e529d7d4b9ae849d3bf30371255a99d6db0421516abcff23214cdc2c",
        strip_prefix = "libcircllhist-63a16dd6f2fc7bc841bb17ff92be8318df60e2e1",
        # 2019-02-11
        urls = ["https://github.com/circonus-labs/libcircllhist/archive/63a16dd6f2fc7bc841bb17ff92be8318df60e2e1.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_github_cyan4973_xxhash = dict(
        sha256 = "952ebbf5b11fbf59ae5d760a562d1e9112278f244340ad7714e8556cbe54f7f7",
        strip_prefix = "xxHash-0.7.3",
        urls = ["https://github.com/Cyan4973/xxHash/archive/v0.7.3.tar.gz"],
        use_category = ["dataplane", "controlplane"],
        cpe = "N/A",
    ),
    com_github_envoyproxy_sqlparser = dict(
        sha256 = "b2d3882698cf85b64c87121e208ce0b24d5fe2a00a5d058cf4571f1b25b45403",
        strip_prefix = "sql-parser-b14d010afd4313f2372a1cc96aa2327e674cc798",
        # 2020-01-10
        urls = ["https://github.com/envoyproxy/sql-parser/archive/b14d010afd4313f2372a1cc96aa2327e674cc798.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_mirror_tclap = dict(
        sha256 = "f0ede0721dddbb5eba3a47385a6e8681b14f155e1129dd39d1a959411935098f",
        strip_prefix = "tclap-tclap-1-2-1-release-final",
        urls = ["https://github.com/mirror/tclap/archive/tclap-1-2-1-release-final.tar.gz"],
        use_category = ["other"],
    ),
    com_github_fmtlib_fmt = dict(
        sha256 = "5014aacf55285bf79654539791de0d6925063fddf4dfdd597ef76b53eb994f86",
        strip_prefix = "fmt-e2ff910675c7800e5c4e28e1509ca6a50bdceafa",
        # 2020-04-29
        urls = ["https://github.com/fmtlib/fmt/archive/e2ff910675c7800e5c4e28e1509ca6a50bdceafa.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_github_gabime_spdlog = dict(
        sha256 = "afd18f62d1bc466c60bef088e6b637b0284be88c515cedc59ad4554150af6043",
        strip_prefix = "spdlog-1.4.0",
        urls = ["https://github.com/gabime/spdlog/archive/v1.4.0.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_github_google_libprotobuf_mutator = dict(
        sha256 = "f6def6cdf63e29a367d46c0ad9e3e31eed89d031e22e0caac126f1e62d8b3fd0",
        strip_prefix = "libprotobuf-mutator-3521f47a2828da9ace403e4ecc4aece1a84feb36",
        # 2020-02-04
        urls = ["https://github.com/google/libprotobuf-mutator/archive/3521f47a2828da9ace403e4ecc4aece1a84feb36.tar.gz"],
        use_category = ["test"],
    ),
    com_github_gperftools_gperftools = dict(
        # TODO(cmluciano): Bump to release 2.8
        # The currently used version is specifically chosen to fix ppc64le builds that require inclusion
        # of asm/ptrace.h, and also s390x builds that require special handling of mmap syscall.
        sha256 = "97f0bc2b389c29305f5d1d8cc4d95e9212c33b55827ae65476fc761d78e3ec5d",
        strip_prefix = "gperftools-gperftools-2.7.90",
        urls = ["https://github.com/gperftools/gperftools/archive/gperftools-2.7.90.tar.gz"],
        use_category = ["test"],
    ),
    com_github_grpc_grpc = dict(
        # TODO(JimmyCYJ): Bump to release 1.27
        # This sha on grpc:v1.25.x branch is specifically chosen to fix gRPC STS call credential options.
        sha256 = "bbc8f020f4e85ec029b047fab939b8c81f3d67254b5c724e1003a2bc49ddd123",
        strip_prefix = "grpc-d8f4928fa779f6005a7fe55a176bdb373b0f910f",
        # 2020-02-11
        urls = ["https://github.com/grpc/grpc/archive/d8f4928fa779f6005a7fe55a176bdb373b0f910f.tar.gz"],
        use_category = ["dataplane", "controlplane"],
        cpe = "cpe:2.3:a:grpc:grpc:*",
    ),
    com_github_luajit_luajit = dict(
        sha256 = "409f7fe570d3c16558e594421c47bdd130238323c9d6fd6c83dedd2aaeb082a8",
        strip_prefix = "LuaJIT-2.1.0-beta3",
        urls = ["https://github.com/LuaJIT/LuaJIT/archive/v2.1.0-beta3.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_moonjit_moonjit = dict(
        sha256 = "83deb2c880488dfe7dd8ebf09e3b1e7613ef4b8420de53de6f712f01aabca2b6",
        strip_prefix = "moonjit-2.2.0",
        urls = ["https://github.com/moonjit/moonjit/archive/2.2.0.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_nghttp2_nghttp2 = dict(
        sha256 = "eacc6f0f8543583ecd659faf0a3f906ed03826f1d4157b536b4b385fe47c5bb8",
        strip_prefix = "nghttp2-1.41.0",
        urls = ["https://github.com/nghttp2/nghttp2/releases/download/v1.41.0/nghttp2-1.41.0.tar.gz"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:nghttp2:nghttp2:*",
    ),
    io_opentracing_cpp = dict(
        sha256 = "015c4187f7a6426a2b5196f0ccd982aa87f010cf61f507ae3ce5c90523f92301",
        strip_prefix = "opentracing-cpp-1.5.1",
        urls = ["https://github.com/opentracing/opentracing-cpp/archive/v1.5.1.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_lightstep_tracer_cpp = dict(
        sha256 = "0e99716598c010e56bc427ea3482be5ad2c534be8b039d172564deec1264a213",
        strip_prefix = "lightstep-tracer-cpp-3efe2372ee3d7c2138d6b26e542d757494a7938d",
        # 2020-03-24
        urls = ["https://github.com/lightstep/lightstep-tracer-cpp/archive/3efe2372ee3d7c2138d6b26e542d757494a7938d.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_github_datadog_dd_opentracing_cpp = dict(
        sha256 = "b84fd2fb0bb0578af4901db31d1c0ae909b532a1016fe6534cbe31a6c3ad6924",
        strip_prefix = "dd-opentracing-cpp-1.1.5",
        urls = ["https://github.com/DataDog/dd-opentracing-cpp/archive/v1.1.5.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_github_google_benchmark = dict(
        sha256 = "3c6a165b6ecc948967a1ead710d4a181d7b0fbcaa183ef7ea84604994966221a",
        strip_prefix = "benchmark-1.5.0",
        urls = ["https://github.com/google/benchmark/archive/v1.5.0.tar.gz"],
        use_category = ["test"],
    ),
    com_github_libevent_libevent = dict(
        sha256 = "c64156c24602ab7a5c66937d774cc55868911d5bbbf1650792f5877744b1c2d9",
        # This SHA includes the new "prepare" and "check" watchers, used for event loop performance
        # stats (see https://github.com/libevent/libevent/pull/793) and the fix for a race condition
        # in the watchers (see https://github.com/libevent/libevent/pull/802).
        # This also includes the fixes for https://github.com/libevent/libevent/issues/806
        # and https://github.com/lyft/envoy-mobile/issues/215.
        # This also include the fixes for Phantom events with EV_ET (see
        # https://github.com/libevent/libevent/issues/984).
        # TODO(adip): Update to v2.2 when it is released.
        strip_prefix = "libevent-06a11929511bebaaf40c52aaf91de397b1782ba2",
        # 2020-05-08
        urls = ["https://github.com/libevent/libevent/archive/06a11929511bebaaf40c52aaf91de397b1782ba2.tar.gz"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:libevent_project:libevent:*",
    ),
    net_zlib = dict(
        # Use the dev branch of zlib to resolve fuzz bugs and out of bound
        # errors resulting in crashes in zlib 1.2.11.
        # TODO(asraa): Remove when zlib > 1.2.11 is released.
        sha256 = "155a8f8c1a753fb05b16a1b0cc0a0a9f61a78e245f9e0da483d13043b3bcbf2e",
        strip_prefix = "zlib-79baebe50e4d6b73ae1f8b603f0ef41300110aa3",
        # 2019-04-14 development branch
        urls = ["https://github.com/madler/zlib/archive/79baebe50e4d6b73ae1f8b603f0ef41300110aa3.tar.gz"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:gnu:zlib:*",
    ),
    com_github_jbeder_yaml_cpp = dict(
        sha256 = "17ffa6320c33de65beec33921c9334dee65751c8a4b797ba5517e844062b98f1",
        strip_prefix = "yaml-cpp-6701275f1910bf63631528dfd9df9c3ac787365b",
        # 2020-05-25
        urls = ["https://github.com/jbeder/yaml-cpp/archive/6701275f1910bf63631528dfd9df9c3ac787365b.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_msgpack_msgpack_c = dict(
        sha256 = "433cbcd741e1813db9ae4b2e192b83ac7b1d2dd7968a3e11470eacc6f4ab58d2",
        strip_prefix = "msgpack-3.2.1",
        urls = ["https://github.com/msgpack/msgpack-c/releases/download/cpp-3.2.1/msgpack-3.2.1.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_github_google_jwt_verify = dict(
        sha256 = "118f955620509f1634cbd918c63234d2048dce56b1815caf348d78e3c3dc899c",
        strip_prefix = "jwt_verify_lib-44291b2ee4c19631e5a0a0bf4f965436a9364ca7",
        # 2020-05-21
        urls = ["https://github.com/google/jwt_verify_lib/archive/44291b2ee4c19631e5a0a0bf4f965436a9364ca7.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_nodejs_http_parser = dict(
        sha256 = "8fa0ab8770fd8425a9b431fdbf91623c4d7a9cdb842b9339289bd2b0b01b0d3d",
        strip_prefix = "http-parser-2.9.3",
        urls = ["https://github.com/nodejs/http-parser/archive/v2.9.3.tar.gz"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:nodejs:node.js:*",
    ),
    com_github_pallets_jinja = dict(
        sha256 = "db49236731373e4f3118af880eb91bb0aa6978bc0cf8b35760f6a026f1a9ffc4",
        strip_prefix = "jinja-2.10.3",
        urls = ["https://github.com/pallets/jinja/archive/2.10.3.tar.gz"],
        use_category = ["build"],
    ),
    com_github_pallets_markupsafe = dict(
        sha256 = "222a10e3237d92a9cd45ed5ea882626bc72bc5e0264d3ed0f2c9129fa69fc167",
        strip_prefix = "markupsafe-1.1.1/src",
        urls = ["https://github.com/pallets/markupsafe/archive/1.1.1.tar.gz"],
        use_category = ["build"],
    ),
    com_github_tencent_rapidjson = dict(
        sha256 = "a2faafbc402394df0fa94602df4b5e4befd734aad6bb55dfef46f62fcaf1090b",
        strip_prefix = "rapidjson-dfbe1db9da455552f7a9ad5d2aea17dd9d832ac1",
        # Changes through 2019-12-02
        urls = ["https://github.com/Tencent/rapidjson/archive/dfbe1db9da455552f7a9ad5d2aea17dd9d832ac1.tar.gz"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:tencent:rapidjson:*",
    ),
    com_github_twitter_common_lang = dict(
        sha256 = "56d1d266fd4767941d11c27061a57bc1266a3342e551bde3780f9e9eb5ad0ed1",
        strip_prefix = "twitter.common.lang-0.3.9/src",
        urls = ["https://files.pythonhosted.org/packages/08/bc/d6409a813a9dccd4920a6262eb6e5889e90381453a5f58938ba4cf1d9420/twitter.common.lang-0.3.9.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_twitter_common_rpc = dict(
        sha256 = "0792b63fb2fb32d970c2e9a409d3d00633190a22eb185145fe3d9067fdaa4514",
        strip_prefix = "twitter.common.rpc-0.3.9/src",
        urls = ["https://files.pythonhosted.org/packages/be/97/f5f701b703d0f25fbf148992cd58d55b4d08d3db785aad209255ee67e2d0/twitter.common.rpc-0.3.9.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_twitter_common_finagle_thrift = dict(
        sha256 = "1e3a57d11f94f58745e6b83348ecd4fa74194618704f45444a15bc391fde497a",
        strip_prefix = "twitter.common.finagle-thrift-0.3.9/src",
        urls = ["https://files.pythonhosted.org/packages/f9/e7/4f80d582578f8489226370762d2cf6bc9381175d1929eba1754e03f70708/twitter.common.finagle-thrift-0.3.9.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_google_googletest = dict(
        sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        strip_prefix = "googletest-release-1.10.0",
        urls = ["https://github.com/google/googletest/archive/release-1.10.0.tar.gz"],
        use_category = ["test"],
    ),
    com_google_protobuf = dict(
        sha256 = "d7cfd31620a352b2ee8c1ed883222a0d77e44346643458e062e86b1d069ace3e",
        strip_prefix = "protobuf-3.10.1",
        urls = ["https://github.com/protocolbuffers/protobuf/releases/download/v3.10.1/protobuf-all-3.10.1.tar.gz"],
        use_category = ["dataplane", "controlplane"],
        cpe = "N/A",
    ),
    grpc_httpjson_transcoding = dict(
        sha256 = "62c8cb5ea2cca1142cde9d4a0778c52c6022345c3268c60ef81666946b958ad5",
        strip_prefix = "grpc-httpjson-transcoding-faf8af1e9788cd4385b94c8f85edab5ea5d4b2d6",
        # 2020-03-02
        urls = ["https://github.com/grpc-ecosystem/grpc-httpjson-transcoding/archive/faf8af1e9788cd4385b94c8f85edab5ea5d4b2d6.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    io_bazel_rules_go = dict(
        sha256 = "e88471aea3a3a4f19ec1310a55ba94772d087e9ce46e41ae38ecebe17935de7b",
        urls = ["https://github.com/bazelbuild/rules_go/releases/download/v0.20.3/rules_go-v0.20.3.tar.gz"],
        use_category = ["build"],
    ),
    rules_cc = dict(
        sha256 = "9d48151ea71b3e225adfb6867e6d2c7d0dce46cbdc8710d9a9a628574dfd40a0",
        strip_prefix = "rules_cc-818289e5613731ae410efb54218a4077fb9dbb03",
        # 2020-05-13
        # TODO(lizan): pin to a point releases when there's a released version.
        urls = ["https://github.com/bazelbuild/rules_cc/archive/818289e5613731ae410efb54218a4077fb9dbb03.tar.gz"],
        use_category = ["build"],
    ),
    rules_foreign_cc = dict(
        sha256 = "3184c244b32e65637a74213fc448964b687390eeeca42a36286f874c046bba15",
        strip_prefix = "rules_foreign_cc-7bc4be735b0560289f6b86ab6136ee25d20b65b7",
        # 2019-09-26
        urls = ["https://github.com/bazelbuild/rules_foreign_cc/archive/7bc4be735b0560289f6b86ab6136ee25d20b65b7.tar.gz"],
        use_category = ["build"],
    ),
    rules_python = dict(
        sha256 = "76a8fd4e7eca2a3590f816958faa0d83c9b2ce9c32634c5c375bcccf161d3bb5",
        strip_prefix = "rules_python-a0fbf98d4e3a232144df4d0d80b577c7a693b570",
        # 2020-04-09
        # TODO(htuch): revert back to a point releases when pip3_import appears.
        urls = ["https://github.com/bazelbuild/rules_python/archive/a0fbf98d4e3a232144df4d0d80b577c7a693b570.tar.gz"],
        use_category = ["build"],
    ),
    six = dict(
        sha256 = "d16a0141ec1a18405cd4ce8b4613101da75da0e9a7aec5bdd4fa804d0e0eba73",
        urls = ["https://files.pythonhosted.org/packages/dd/bf/4138e7bfb757de47d1f4b6994648ec67a51efe58fa907c1e11e350cddfca/six-1.12.0.tar.gz"],
        use_category = ["other"],
    ),
    org_llvm_llvm = dict(
        sha256 = "44787a6d02f7140f145e2250d56c9f849334e11f9ae379827510ed72f12b75e7",
        strip_prefix = "llvm-8.0.1.src",
        urls = ["https://github.com/llvm/llvm-project/releases/download/llvmorg-8.0.1/llvm-8.0.1.src.tar.xz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_wavm_wavm = dict(
        sha256 = "48939906ff44a0e26e2c4c63132eebd75cfd768fac989a874395121e139933ec",
        strip_prefix = "WAVM-1ec06cd202a922015c9041c5ed84f875453c4dc7",
        urls = ["https://github.com/WAVM/WAVM/archive/1ec06cd202a922015c9041c5ed84f875453c4dc7.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    io_opencensus_cpp = dict(
        sha256 = "193ffb4e13bd7886757fd22b61b7f7a400634412ad8e7e1071e73f57bedd7fc6",
        strip_prefix = "opencensus-cpp-04ed0211931f12b03c1a76b3907248ca4db7bc90",
        # 2020-03-24
        urls = ["https://github.com/census-instrumentation/opencensus-cpp/archive/04ed0211931f12b03c1a76b3907248ca4db7bc90.tar.gz"],
        use_category = ["observability"],
        cpe = "N/A",
    ),
    com_github_curl = dict(
        sha256 = "01ae0c123dee45b01bbaef94c0bc00ed2aec89cb2ee0fd598e0d302a6b5e0a98",
        strip_prefix = "curl-7.69.1",
        urls = ["https://github.com/curl/curl/releases/download/curl-7_69_1/curl-7.69.1.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_googlesource_chromium_v8 = dict(
        # This archive was created using https://storage.googleapis.com/envoyproxy-wee8/wee8-archive.sh
        # and contains complete checkout of V8 with all dependencies necessary to build wee8.
        sha256 = "cc6f5357cd10922bfcf667bd882624ad313e21b009b919ce00f322f390012476",
        urls = ["https://storage.googleapis.com/envoyproxy-wee8/wee8-8.3.110.9.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_googlesource_quiche = dict(
        # Static snapshot of https://quiche.googlesource.com/quiche/+archive/41c9fdead26b31deefae3c325a2cf1a873688ba3.tar.gz
        sha256 = "75af53154402e1654cfd32d8aaeed5fab4dbb79d3cab8c9866019d5369c1889e",
        urls = ["https://storage.googleapis.com/quiche-envoy-integration/41c9fdead26b31deefae3c325a2cf1a873688ba3.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_googlesource_googleurl = dict(
        # Static snapshot of https://quiche.googlesource.com/quiche/+archive/googleurl_dbf5ad147f60afc125e99db7549402af49a5eae8.tar.gz
        sha256 = "b40cd22cadba577b7281a76db66f6a66dd744edbad8cc2c861c2c976ef721e4d",
        urls = ["https://storage.googleapis.com/quiche-envoy-integration/googleurl_dbf5ad147f60afc125e99db7549402af49a5eae8.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_google_cel_cpp = dict(
        sha256 = "d773ee368053f404eada87b59c9e545b2e21118ebb0577ac3e942a06518f6fd2",
        strip_prefix = "cel-cpp-0.2.0",
        # 2020-05-11
        urls = ["https://github.com/google/cel-cpp/archive/v0.2.0.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_github_google_flatbuffers = dict(
        sha256 = "b8efbc25721e76780752bad775a97c3f77a0250271e2db37fc747b20e8b0f24a",
        strip_prefix = "flatbuffers-a83caf5910644ba1c421c002ef68e42f21c15f9f",
        urls = ["https://github.com/google/flatbuffers/archive/a83caf5910644ba1c421c002ef68e42f21c15f9f.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    com_googlesource_code_re2 = dict(
        sha256 = "04ee2aaebaa5038554683329afc494e684c30f82f2a1e47eb62450e59338f84d",
        strip_prefix = "re2-2020-03-03",
        urls = ["https://github.com/google/re2/archive/2020-03-03.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    # Included to access FuzzedDataProvider.h. This is compiler agnostic but
    # provided as part of the compiler-rt source distribution. We can't use the
    # Clang variant as we are not a Clang-LLVM only shop today.
    org_llvm_releases_compiler_rt = dict(
        sha256 = "56e4cd96dd1d8c346b07b4d6b255f976570c6f2389697347a6c3dcb9e820d10e",
        # Only allow peeking at fuzzer related files for now.
        strip_prefix = "compiler-rt-9.0.0.src/lib",
        urls = ["http://releases.llvm.org/9.0.0/compiler-rt-9.0.0.src.tar.xz"],
        use_category = ["test"],
    ),
    upb = dict(
        sha256 = "e9f281c56ab1eb1f97a80ca8a83bb7ef73d230eabb8591f83876f4e7b85d9b47",
        strip_prefix = "upb-8a3ae1ef3e3e3f26b45dec735c5776737fc7247f",
        # 2019-11-19
        urls = ["https://github.com/protocolbuffers/upb/archive/8a3ae1ef3e3e3f26b45dec735c5776737fc7247f.tar.gz"],
        use_category = ["dataplane", "controlplane"],
        cpe = "N/A",
    ),
    kafka_source = dict(
        sha256 = "e7b748a62e432b5770db6dbb3b034c68c0ea212812cb51603ee7f3a8a35f06be",
        strip_prefix = "kafka-2.4.0/clients/src/main/resources/common/message",
        urls = ["https://github.com/apache/kafka/archive/2.4.0.zip"],
        use_category = ["dataplane"],
        cpe = "cpe:2.3:a:apache:kafka:*",
    ),
    kafka_server_binary = dict(
        sha256 = "b9582bab0c3e8d131953b1afa72d6885ca1caae0061c2623071e7f396f2ccfee",
        strip_prefix = "kafka_2.12-2.4.0",
        urls = ["http://us.mirrors.quenda.co/apache/kafka/2.4.0/kafka_2.12-2.4.0.tgz"],
        use_category = ["test"],
    ),
    kafka_python_client = dict(
        sha256 = "454bf3aafef9348017192417b7f0828a347ec2eaf3efba59336f3a3b68f10094",
        strip_prefix = "kafka-python-2.0.0",
        urls = ["https://github.com/dpkp/kafka-python/archive/2.0.0.tar.gz"],
        use_category = ["test"],
    ),
    proxy_wasm_cpp_sdk = dict(
        sha256 = "d1961b3594823e7a71407629980a854dd2cb423b14378326f51f4b48f3b29721",
        strip_prefix = "proxy-wasm-cpp-sdk-1e0351b4261fa94c7016a0a611992bf602a0d43e",
        urls = ["https://github.com/proxy-wasm/proxy-wasm-cpp-sdk/archive/1e0351b4261fa94c7016a0a611992bf602a0d43e.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    proxy_wasm_cpp_host = dict(
        sha256 = "06bd644911f43d75f8c344dfcabf148e212aae6cc6abef1402e6bab2ceedba6f",
        strip_prefix = "proxy-wasm-cpp-host-5fb1664bc48b08e9812453001957773f50a26c31",
        urls = ["https://github.com/proxy-wasm/proxy-wasm-cpp-host/archive/5fb1664bc48b08e9812453001957773f50a26c31.tar.gz"],
        use_category = ["dataplane"],
        cpe = "N/A",
    ),
    emscripten_toolchain = dict(
        sha256 = "4ac0f1f3de8b3f1373d435cd7e58bd94de4146e751f099732167749a229b443b",
        patch_cmds = [
            "./emsdk install 1.39.6-upstream",
            "./emsdk activate --embedded 1.39.6-upstream",
        ],
        strip_prefix = "emsdk-1.39.6",
        urls = ["https://github.com/emscripten-core/emsdk/archive/1.39.6.tar.gz"],
        use_category = ["build"],
    ),
)
