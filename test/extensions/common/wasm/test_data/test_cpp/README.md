# Build instructions for test\_cpp.wasm

Sadly bazel ignores the WORKSPACE for all sub-projects and it is incapable of invoking itself recursively, nor does it have a good solution for using different tool chains for the same type of source file (e.g. .cc). If anyone would like to donate some time to fixing this it would be much appreciated.

In the meantime, the solution is to build the .wasm files and copy them into the envoy tree proper and check them in.

## Makefile

Type 'make' in this directory to rebuild test\_cpp.wasm.

