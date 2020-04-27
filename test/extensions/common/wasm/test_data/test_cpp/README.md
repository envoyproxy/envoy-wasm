# Build instructions for test\_cpp.wasm

Sadly bazel ignores the WORKSPACE for all sub-projects. nor does it have a good solution for using different tool chains for the same type of source file (e.g. .cc).
So we use an repository_rule to make Bazel invoke itself in this repository.

The limitation of current approach is that it doesn't allow any subpackage (sub-directory with `BUILD` in it), so everything should be in top-level BUILD file.
Only targets end-with `.wasm` is exported to outside repository and should be referred as `@common_wasm_test_cpp//:<target_rule>`.

If you added/removed files to/from this directory, you might need to run `bazel sync --configure` in top-level Bazel project to refresh it.

## Makefile

Type 'make' in this directory to rebuild test\_cpp.wasm.

