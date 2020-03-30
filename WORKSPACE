workspace(name = "envoy")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

load("//bazel:api_binding.bzl", "envoy_api_binding")

envoy_api_binding()

load("//bazel:api_repositories.bzl", "envoy_api_dependencies")

envoy_api_dependencies()

load("//bazel:repositories.bzl", "envoy_dependencies")

envoy_dependencies()

load("//bazel:dependency_imports.bzl", "envoy_dependency_imports")

envoy_dependency_imports()

load("@rules_antlr//antlr:deps.bzl", "antlr_dependencies")

antlr_dependencies(471)

#local_repository(
#    name = "common_wasm_test_cpp",
#    path = "../test_cpp",
#)

#git_repository(
#    name = "common_wasm_test_cpp",
#    remote = "https://github.com/jplevyak/proxy-wasm-common-test-cpp",
#    commit = "ffcc7a2a8140d6ab8ef7ca26a423143c4adabcc1",
#)

#new_local_repository(
#    name = "envoy_wasm_api",
#    path = "api/wasm/cpp",
#    workspace_file_content = 'workspace(name = "envoy_wasm_api")',
#    build_file = '//bazel/external:envoy-wasm-api.BUILD',
#)
