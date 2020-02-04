REPOSITORY_LOCATIONS = dict(
    bazel_compdb = dict(
        sha256 = "87e376a685eacfb27bcc0d0cdf5ded1d0b99d868390ac50f452ba6ed781caffe",
        strip_prefix = "bazel-compilation-database-0.4.2",
        urls = ["https://github.com/grailbio/bazel-compilation-database/archive/0.4.2.tar.gz"],
    ),
    bazel_gazelle = dict(
        sha256 = "86c6d481b3f7aedc1d60c1c211c6f76da282ae197c3b3160f54bd3a8f847896f",
        urls = ["https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.19.1/bazel-gazelle-v0.19.1.tar.gz"],
    ),
    bazel_toolchains = dict(
        sha256 = "e2126599d29f2028e6b267eba273dcc8e7f4a35ff323e9600cf42fb03875b7c6",
        strip_prefix = "bazel-toolchains-2.0.0",
        urls = [
            "https://github.com/bazelbuild/bazel-toolchains/releases/download/2.0.0/bazel-toolchains-2.0.0.tar.gz",
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-toolchains/archive/2.0.0.tar.gz",
        ],
    ),
    build_bazel_rules_apple = dict(
        sha256 = "7a7afdd4869bb201c9352eed2daf37294d42b093579b70423490c1b4d4f6ce42",
        urls = ["https://github.com/bazelbuild/rules_apple/releases/download/0.19.0/rules_apple.0.19.0.tar.gz"],
    ),
    envoy_build_tools = dict(
        sha256 = "97f7276eeae150ce84de5406174e87ea82c6c9aad3e3e4ede1067b5d8205c980",
        strip_prefix = "envoy-build-tools-0a98f4bd8b3eeeaa11a10f6a4fe5c59e7c2e16df",
        # 2019-12-23
        urls = ["https://github.com/envoyproxy/envoy-build-tools/archive/0a98f4bd8b3eeeaa11a10f6a4fe5c59e7c2e16df.tar.gz"],
    ),
    boringssl = dict(
        sha256 = "3eea198c8e3f587ffc8ea6acf87d7575f571bbe6dd88ec90405e236303f3dc01",
        strip_prefix = "boringssl-65e0aad1b721a5aa67f2a8041cf48f691139bb9f",
        # To update BoringSSL, which tracks Chromium releases:
        # 1. Open https://omahaproxy.appspot.com/ and note <current_version> of linux/beta release.
        # 2. Open https://chromium.googlesource.com/chromium/src/+/refs/tags/<current_version>/DEPS and note <boringssl_revision>.
        # 3. Find a commit in BoringSSL's "master-with-bazel" branch that merges <boringssl_revision>.
        #
        # chromium-79.0.3945.16 (BETA)
        urls = ["https://github.com/google/boringssl/archive/65e0aad1b721a5aa67f2a8041cf48f691139bb9f.tar.gz"],
    ),
    boringssl_fips = dict(
        sha256 = "b12ad676ee533824f698741bd127f6fbc82c46344398a6d78d25e62c6c418c73",
        # fips-20180730
        urls = ["https://commondatastorage.googleapis.com/chromium-boringssl-docs/fips/boringssl-66005f41fbc3529ffe8d007708756720529da20d.tar.xz"],
    ),
    com_google_absl = dict(
        sha256 = "19391fb4882601a65cb648d638c11aa301ce5f525ef02da1a9eafd22f72d7c59",
        strip_prefix = "abseil-cpp-37dd2562ec830d547a1524bb306be313ac3f2556",
        # 2020-01-29
        urls = ["https://github.com/abseil/abseil-cpp/archive/37dd2562ec830d547a1524bb306be313ac3f2556.tar.gz"],
    ),
    com_github_apache_thrift = dict(
        sha256 = "7d59ac4fdcb2c58037ebd4a9da5f9a49e3e034bf75b3f26d9fe48ba3d8806e6b",
        strip_prefix = "thrift-0.11.0",
        urls = ["https://files.pythonhosted.org/packages/c6/b4/510617906f8e0c5660e7d96fbc5585113f83ad547a3989b80297ac72a74c/thrift-0.11.0.tar.gz"],
    ),
    com_github_c_ares_c_ares = dict(
        sha256 = "bbaab13d6ad399a278d476f533e4d88a7ec7d729507348bb9c2e3b207ba4c606",
        strip_prefix = "c-ares-d7e070e7283f822b1d2787903cce3615536c5610",
        # 2019-06-19
        # 27 new commits from release-1.15.0. Upgrade for commit 7d3591ee8a1a63e7748e68e6d880bd1763a32885 "getaddrinfo enhancements" and follow up fixes.
        # Use getaddrinfo to query DNS record and TTL.
        # TODO(crazyxy): Update to release-1.16.0 when it is released.
        urls = ["https://github.com/c-ares/c-ares/archive/d7e070e7283f822b1d2787903cce3615536c5610.tar.gz"],
    ),
    com_github_circonus_labs_libcircllhist = dict(
        sha256 = "8165aa25e529d7d4b9ae849d3bf30371255a99d6db0421516abcff23214cdc2c",
        strip_prefix = "libcircllhist-63a16dd6f2fc7bc841bb17ff92be8318df60e2e1",
        # 2019-02-11
        urls = ["https://github.com/circonus-labs/libcircllhist/archive/63a16dd6f2fc7bc841bb17ff92be8318df60e2e1.tar.gz"],
    ),
    com_github_cyan4973_xxhash = dict(
        sha256 = "7e93d28e81c3e95ff07674a400001d0cdf23b7842d49b211e5582d00d8e3ac3e",
        strip_prefix = "xxHash-0.7.2",
        urls = ["https://github.com/Cyan4973/xxHash/archive/v0.7.2.tar.gz"],
    ),
    com_github_envoyproxy_sqlparser = dict(
        sha256 = "8ccd2fe9087d96a91eda0929f3a0bdb5d9f877dd9cdc91f9fcf85385fe70cc14",
        strip_prefix = "sql-parser-85904962d8723bbd3ae280dfb13cf4cc563faabc",
        # 2020-01-08
        urls = ["https://github.com/envoyproxy/sql-parser/archive/85904962d8723bbd3ae280dfb13cf4cc563faabc.tar.gz"],
    ),
    com_github_mirror_tclap = dict(
        sha256 = "f0ede0721dddbb5eba3a47385a6e8681b14f155e1129dd39d1a959411935098f",
        strip_prefix = "tclap-tclap-1-2-1-release-final",
        urls = ["https://github.com/mirror/tclap/archive/tclap-1-2-1-release-final.tar.gz"],
    ),
    com_github_fmtlib_fmt = dict(
        sha256 = "4c0741e10183f75d7d6f730b8708a99b329b2f942dad5a9da3385ab92bb4a15c",
        strip_prefix = "fmt-5.3.0",
        urls = ["https://github.com/fmtlib/fmt/releases/download/5.3.0/fmt-5.3.0.zip"],
    ),
    com_github_gabime_spdlog = dict(
        sha256 = "160845266e94db1d4922ef755637f6901266731c4cb3b30b45bf41efa0e6ab70",
        strip_prefix = "spdlog-1.3.1",
        urls = ["https://github.com/gabime/spdlog/archive/v1.3.1.tar.gz"],
    ),
    com_github_google_libprotobuf_mutator = dict(
        sha256 = "54597f640c0ab5e5d783d2f3d3cfe8ad6da999ef1a194d89c2c5ab89a1fd8e13",
        strip_prefix = "libprotobuf-mutator-dd89da92b59b1714bab6e2a135093948a1cf1c6a",
        # 2019-10-08
        urls = ["https://github.com/google/libprotobuf-mutator/archive/dd89da92b59b1714bab6e2a135093948a1cf1c6a.tar.gz"],
    ),
    com_github_gperftools_gperftools = dict(
        # TODO(cmluciano): Bump to release 2.8
        # This sha is specifically chosen to fix ppc64le builds that require inclusion
        # of asm/ptrace.h
        sha256 = "18574813a062eee487bc1b761e8024a346075a7cb93da19607af362dc09565ef",
        strip_prefix = "gperftools-fc00474ddc21fff618fc3f009b46590e241e425e",
        urls = ["https://github.com/gperftools/gperftools/archive/fc00474ddc21fff618fc3f009b46590e241e425e.tar.gz"],
    ),
    com_github_grpc_grpc = dict(
        sha256 = "ffbe61269160ea745e487f79b0fd06b6edd3d50c6d9123f053b5634737cf2f69",
        strip_prefix = "grpc-1.25.0",
        urls = ["https://github.com/grpc/grpc/archive/v1.25.0.tar.gz"],
    ),
    com_github_luajit_luajit = dict(
        sha256 = "409f7fe570d3c16558e594421c47bdd130238323c9d6fd6c83dedd2aaeb082a8",
        strip_prefix = "LuaJIT-2.1.0-beta3",
        urls = ["https://github.com/LuaJIT/LuaJIT/archive/v2.1.0-beta3.tar.gz"],
    ),
    com_github_nghttp2_nghttp2 = dict(
        sha256 = "eb9d9046495a49dd40c7ef5d6c9907b51e5a6b320ea6e2add11eb8b52c982c47",
        strip_prefix = "nghttp2-1.40.0",
        urls = ["https://github.com/nghttp2/nghttp2/releases/download/v1.40.0/nghttp2-1.40.0.tar.gz"],
    ),
    io_opentracing_cpp = dict(
        sha256 = "015c4187f7a6426a2b5196f0ccd982aa87f010cf61f507ae3ce5c90523f92301",
        strip_prefix = "opentracing-cpp-1.5.1",
        urls = ["https://github.com/opentracing/opentracing-cpp/archive/v1.5.1.tar.gz"],
    ),
    com_lightstep_tracer_cpp = dict(
        sha256 = "defbf471facfebde6523ca1177529b63784893662d4ef2c60db074be8aef0634",
        strip_prefix = "lightstep-tracer-cpp-0.8.0",
        urls = ["https://github.com/lightstep/lightstep-tracer-cpp/archive/v0.8.0.tar.gz"],
    ),
    com_github_datadog_dd_opentracing_cpp = dict(
        sha256 = "052fd37cd698e24ab73ee18fc3fa55acd1d43153c12a0e65b0fba0447de1117e",
        strip_prefix = "dd-opentracing-cpp-1.1.1",
        urls = ["https://github.com/DataDog/dd-opentracing-cpp/archive/v1.1.1.tar.gz"],
    ),
    com_github_google_benchmark = dict(
        sha256 = "3c6a165b6ecc948967a1ead710d4a181d7b0fbcaa183ef7ea84604994966221a",
        strip_prefix = "benchmark-1.5.0",
        urls = ["https://github.com/google/benchmark/archive/v1.5.0.tar.gz"],
    ),
    com_github_libevent_libevent = dict(
        sha256 = "549d34065eb2485dfad6c8de638caaa6616ed130eec36dd978f73b6bdd5af113",
        # This SHA includes the new "prepare" and "check" watchers, used for event loop performance
        # stats (see https://github.com/libevent/libevent/pull/793) and the fix for a race condition
        # in the watchers (see https://github.com/libevent/libevent/pull/802).
        # This also includes the fixes for https://github.com/libevent/libevent/issues/806
        # and https://github.com/lyft/envoy-mobile/issues/215.
        # TODO(mergeconflict): Update to v2.2 when it is released.
        strip_prefix = "libevent-0d7d85c2083f7a4c9efe01c061486f332b576d28",
        # 2019-07-02
        urls = ["https://github.com/libevent/libevent/archive/0d7d85c2083f7a4c9efe01c061486f332b576d28.tar.gz"],
    ),
    net_zlib = dict(
        # Use the dev branch of zlib to resolve fuzz bugs and out of bound
        # errors resulting in crashes in zlib 1.2.11.
        # TODO(asraa): Remove when zlib > 1.2.11 is released.
        sha256 = "155a8f8c1a753fb05b16a1b0cc0a0a9f61a78e245f9e0da483d13043b3bcbf2e",
        strip_prefix = "zlib-79baebe50e4d6b73ae1f8b603f0ef41300110aa3",
        # 2019-04-14 development branch
        urls = ["https://github.com/madler/zlib/archive/79baebe50e4d6b73ae1f8b603f0ef41300110aa3.tar.gz"],
    ),
    com_github_jbeder_yaml_cpp = dict(
        sha256 = "77ea1b90b3718aa0c324207cb29418f5bced2354c2e483a9523d98c3460af1ed",
        strip_prefix = "yaml-cpp-yaml-cpp-0.6.3",
        urls = ["https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.3.tar.gz"],
    ),
    com_github_msgpack_msgpack_c = dict(
        sha256 = "fbaa28c363a316fd7523f31d1745cf03eab0d1e1ea5a1c60aa0dffd4ce551afe",
        strip_prefix = "msgpack-3.2.0",
        urls = ["https://github.com/msgpack/msgpack-c/releases/download/cpp-3.2.0/msgpack-3.2.0.tar.gz"],
    ),
    com_github_google_jwt_verify = dict(
        sha256 = "b6e04123e801dc8204d9bcab5716ec92a510a770546fca823e6f399dc920479d",
        strip_prefix = "jwt_verify_lib-2d8dfd2dd5b715536af2531e107bde995796f749",
        # 2020-01-08
        urls = ["https://github.com/google/jwt_verify_lib/archive/2d8dfd2dd5b715536af2531e107bde995796f749.tar.gz"],
    ),
    com_github_nodejs_http_parser = dict(
        sha256 = "ef26268c54c8084d17654ba2ed5140bffeffd2a040a895ffb22a6cca3f6c613f",
        strip_prefix = "http-parser-2.9.0",
        urls = ["https://github.com/nodejs/http-parser/archive/v2.9.0.tar.gz"],
    ),
    com_github_pallets_jinja = dict(
        sha256 = "db49236731373e4f3118af880eb91bb0aa6978bc0cf8b35760f6a026f1a9ffc4",
        strip_prefix = "jinja-2.10.3",
        urls = ["https://github.com/pallets/jinja/archive/2.10.3.tar.gz"],
    ),
    com_github_pallets_markupsafe = dict(
        sha256 = "222a10e3237d92a9cd45ed5ea882626bc72bc5e0264d3ed0f2c9129fa69fc167",
        strip_prefix = "markupsafe-1.1.1/src",
        urls = ["https://github.com/pallets/markupsafe/archive/1.1.1.tar.gz"],
    ),
    com_github_tencent_rapidjson = dict(
        sha256 = "bf7ced29704a1e696fbccf2a2b4ea068e7774fa37f6d7dd4039d0787f8bed98e",
        strip_prefix = "rapidjson-1.1.0",
        urls = ["https://github.com/Tencent/rapidjson/archive/v1.1.0.tar.gz"],
    ),
    com_github_twitter_common_lang = dict(
        sha256 = "56d1d266fd4767941d11c27061a57bc1266a3342e551bde3780f9e9eb5ad0ed1",
        strip_prefix = "twitter.common.lang-0.3.9/src",
        urls = ["https://files.pythonhosted.org/packages/08/bc/d6409a813a9dccd4920a6262eb6e5889e90381453a5f58938ba4cf1d9420/twitter.common.lang-0.3.9.tar.gz"],
    ),
    com_github_twitter_common_rpc = dict(
        sha256 = "0792b63fb2fb32d970c2e9a409d3d00633190a22eb185145fe3d9067fdaa4514",
        strip_prefix = "twitter.common.rpc-0.3.9/src",
        urls = ["https://files.pythonhosted.org/packages/be/97/f5f701b703d0f25fbf148992cd58d55b4d08d3db785aad209255ee67e2d0/twitter.common.rpc-0.3.9.tar.gz"],
    ),
    com_github_twitter_common_finagle_thrift = dict(
        sha256 = "1e3a57d11f94f58745e6b83348ecd4fa74194618704f45444a15bc391fde497a",
        strip_prefix = "twitter.common.finagle-thrift-0.3.9/src",
        urls = ["https://files.pythonhosted.org/packages/f9/e7/4f80d582578f8489226370762d2cf6bc9381175d1929eba1754e03f70708/twitter.common.finagle-thrift-0.3.9.tar.gz"],
    ),
    com_google_googletest = dict(
        sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        strip_prefix = "googletest-release-1.10.0",
        urls = ["https://github.com/google/googletest/archive/release-1.10.0.tar.gz"],
    ),
    com_google_protobuf = dict(
        sha256 = "d7cfd31620a352b2ee8c1ed883222a0d77e44346643458e062e86b1d069ace3e",
        strip_prefix = "protobuf-3.10.1",
        urls = ["https://github.com/protocolbuffers/protobuf/releases/download/v3.10.1/protobuf-all-3.10.1.tar.gz"],
    ),
    grpc_httpjson_transcoding = dict(
        sha256 = "a447458b47ea4dc1d31499f555769af437c5d129d988ec1e13d5fdd0a6a36b4e",
        strip_prefix = "grpc-httpjson-transcoding-2feabd5d64436e670084091a937855972ee35161",
        # 2019-08-28
        urls = ["https://github.com/grpc-ecosystem/grpc-httpjson-transcoding/archive/2feabd5d64436e670084091a937855972ee35161.tar.gz"],
    ),
    io_bazel_rules_go = dict(
        sha256 = "e88471aea3a3a4f19ec1310a55ba94772d087e9ce46e41ae38ecebe17935de7b",
        urls = ["https://github.com/bazelbuild/rules_go/releases/download/v0.20.3/rules_go-v0.20.3.tar.gz"],
    ),
    rules_foreign_cc = dict(
        sha256 = "3184c244b32e65637a74213fc448964b687390eeeca42a36286f874c046bba15",
        strip_prefix = "rules_foreign_cc-7bc4be735b0560289f6b86ab6136ee25d20b65b7",
        # 2019-09-26
        urls = ["https://github.com/bazelbuild/rules_foreign_cc/archive/7bc4be735b0560289f6b86ab6136ee25d20b65b7.tar.gz"],
    ),
    rules_python = dict(
        sha256 = "aa96a691d3a8177f3215b14b0edc9641787abaaa30363a080165d06ab65e1161",
        urls = ["https://github.com/bazelbuild/rules_python/releases/download/0.0.1/rules_python-0.0.1.tar.gz"],
    ),
    six = dict(
        sha256 = "d16a0141ec1a18405cd4ce8b4613101da75da0e9a7aec5bdd4fa804d0e0eba73",
        urls = ["https://files.pythonhosted.org/packages/dd/bf/4138e7bfb757de47d1f4b6994648ec67a51efe58fa907c1e11e350cddfca/six-1.12.0.tar.gz"],
    ),
    org_llvm_llvm = dict(
        sha256 = "44787a6d02f7140f145e2250d56c9f849334e11f9ae379827510ed72f12b75e7",
        strip_prefix = "llvm-8.0.1.src",
        urls = ["https://github.com/llvm/llvm-project/releases/download/llvmorg-8.0.1/llvm-8.0.1.src.tar.xz"],
    ),
    com_github_wavm_wavm = dict(
        sha256 = "48939906ff44a0e26e2c4c63132eebd75cfd768fac989a874395121e139933ec",
        strip_prefix = "WAVM-1ec06cd202a922015c9041c5ed84f875453c4dc7",
        urls = ["https://github.com/WAVM/WAVM/archive/1ec06cd202a922015c9041c5ed84f875453c4dc7.tar.gz"],
    ),
    io_opencensus_cpp = dict(
        sha256 = "a9ba6027436cfa1264860c6be602da7633d9a1f9abcb8838f2ae6bda8c2c14f6",
        strip_prefix = "opencensus-cpp-13b1a2f29f541b6b2c4cb8bc3f6fbf3589d44227",
        # 2019-12-01
        urls = ["https://github.com/census-instrumentation/opencensus-cpp/archive/13b1a2f29f541b6b2c4cb8bc3f6fbf3589d44227.tar.gz"],
    ),
    com_github_curl = dict(
        sha256 = "1dd7604e418b0b9a9077f62f763f6684c1b092a7bc17e3f354b8ad5c964d7358",
        strip_prefix = "curl-7.68.0",
        urls = ["https://github.com/curl/curl/releases/download/curl-7_68_0/curl-7.68.0.tar.gz"],
    ),
    com_googlesource_chromium_v8 = dict(
        # This archive was created using https://storage.googleapis.com/envoyproxy-wee8/wee8-archive.sh
        # and contains complete checkout of V8 with all dependencies necessary to build wee8.
        sha256 = "e46ebfd9752b54c13653ee4dd5af8533cb6722d938ba9496f034bc58391edb38",
        urls = ["https://storage.googleapis.com/envoyproxy-wee8/wee8-8.0.426.12.tar.gz"],
    ),
    com_googlesource_quiche = dict(
        # Static snapshot of https://quiche.googlesource.com/quiche/+archive/85240a12ed2b9ccb08ae449bca1bbf9eb93c8a12.tar.gz
        sha256 = "63ca7aa58511beb8cd6e6b93dbb25154bd14c8c8a316a1b20430e830060fa7db",
        urls = ["https://storage.googleapis.com/quiche-envoy-integration/85240a12ed2b9ccb08ae449bca1bbf9eb93c8a12.tar.gz"],
    ),
    com_google_cel_cpp = dict(
        sha256 = "d942a8d2e5831bcf7f5b1e99b07f90534eb082f40fd9bda05bcc24ff9c0c3571",
        strip_prefix = "cel-cpp-a9eec4686b72c28980a09fe2e253ec897a781c32",
        # 2019-12-09
        urls = ["https://github.com/google/cel-cpp/archive/a9eec4686b72c28980a09fe2e253ec897a781c32.tar.gz"],
    ),
    com_googlesource_code_re2 = dict(
        sha256 = "7268e1b4254d9ffa5ccf010fee954150dbb788fd9705234442e7d9f0ee5a42d3",
        strip_prefix = "re2-2019-12-01",
        urls = ["https://github.com/google/re2/archive/2019-12-01.tar.gz"],
    ),
    # Included to access FuzzedDataProvider.h. This is compiler agnostic but
    # provided as part of the compiler-rt source distribution. We can't use the
    # Clang variant as we are not a Clang-LLVM only shop today.
    org_llvm_releases_compiler_rt = dict(
        sha256 = "56e4cd96dd1d8c346b07b4d6b255f976570c6f2389697347a6c3dcb9e820d10e",
        # Only allow peeking at fuzzer related files for now.
        strip_prefix = "compiler-rt-9.0.0.src/lib/fuzzer",
        urls = ["http://releases.llvm.org/9.0.0/compiler-rt-9.0.0.src.tar.xz"],
    ),
    fuzzit_linux = dict(
        sha256 = "9ca76ac1c22d9360936006efddf992977ebf8e4788ded8e5f9d511285c9ac774",
        urls = ["https://github.com/fuzzitdev/fuzzit/releases/download/v2.4.76/fuzzit_Linux_x86_64.zip"],
    ),
    upb = dict(
        sha256 = "e9f281c56ab1eb1f97a80ca8a83bb7ef73d230eabb8591f83876f4e7b85d9b47",
        strip_prefix = "upb-8a3ae1ef3e3e3f26b45dec735c5776737fc7247f",
        # 2019-11-19
        urls = ["https://github.com/protocolbuffers/upb/archive/8a3ae1ef3e3e3f26b45dec735c5776737fc7247f.tar.gz"],
    ),
    kafka_source = dict(
        sha256 = "feaa32e5c42acf42bd587f8f0b1ccce679db227620da97eed013f4c44a44f64d",
        strip_prefix = "kafka-2.3.1/clients/src/main/resources/common/message",
        urls = ["https://github.com/apache/kafka/archive/2.3.1.zip"],
    ),
    kafka_server_binary = dict(
        sha256 = "5a3ddd4148371284693370d56f6f66c7a86d86dd96c533447d2a94d176768d2e",
        strip_prefix = "kafka_2.12-2.3.1",
        urls = ["http://us.mirrors.quenda.co/apache/kafka/2.3.1/kafka_2.12-2.3.1.tgz"],
    ),
    kafka_python_client = dict(
        sha256 = "81f24a5d297531495e0ccb931fbd6c4d1ec96583cf5a730579a3726e63f59c47",
        strip_prefix = "kafka-python-1.4.7",
        urls = ["https://github.com/dpkp/kafka-python/archive/1.4.7.tar.gz"],
    ),
)
