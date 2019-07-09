Proof of Concept WebAssembly Support.

# WebAssembly VM

The WebAssembly VM is https://github.com/WAVM/WAVM. This VM was chosen for the
proof of concept because:

  - It is OSS with a permissive license.
  - It compiles with LLVM for high performance.
  - It can attach pre-compiled code to the WASM files (e.g. during distribution
      from a central controller) for faster startup and reduced load on a pool
      of proxies.

# TODO

  - Check if the thread is running in the VM in the WAVM Runtime signal handlers and
    if not call the underlying signal handler from Envoy or add the WAVM signal handler
    as a pre-check to the Envoy signal handler with the same check.
  - Add support to WAVM for multple stacks for coroutines/streams with thread shared state.
    - evaluate async emscripten: https://github.com/kripken/emscripten/wiki/Asyncify
    - evaulate boost::fiber, GNU Pth, getcontext(), setcontext(), swapcontext().
  - Add support for shared global state.
  - Intercept logging messages from WAVM and send to Envoy logs.

# Build the Envoy

To build Envoy with WASM enabled, pass `--define wasm=<vm-name>` when
running `bazel build`.


```shell
bazel build  --define wasm=wavm  //source/exe:envoy-static
```

The available VM name options are

1. wavm
1. v8
1. null? (Avaialbe or not?)


To build Envoy via Docker image, pass `--define wasm=<vm-name>` as `BAZEL_EXTRA_BUILD_OPTIONS`

```shell
BAZEL_EXTRA_BUILD_OPTIONS='--define wasm=wavm' ./ci/run_envoy_docker.sh './ci/do_ci.sh bazel.dev'
```
