# Envoy WebAssembly Filter

In this example, we show how a WebAssembly(WASM) filter can be used with the Envoy
proxy. The Envoy proxy [configuration](./envoy.yaml) includes a Webassembly filter
as documented [here](https://www.envoyproxy.io/docs/envoy/latest/).
<!--TODO(bianpengyuan): change to the url of Wasm filter once the doc is ready.-->



## Quick Start

1. `docker-compose build`
2. `docker-compose up`
3. `curl -v localhost:18000`

Curl output should include our headers:

```
# <b> curl -v localhost:8000</b>
* Rebuilt URL to: localhost:18000/
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 18000 (#0)
> GET / HTTP/1.1
> Host: localhost:18000
> User-Agent: curl/7.58.0
> Accept: */*
> 
< HTTP/1.1 200 OK
< content-length: 13
< content-type: text/plain
< location: envoy-wasm
< date: Tue, 09 Jul 2019 00:47:14 GMT
< server: envoy
< x-envoy-upstream-service-time: 0
< newheader: newheadervalue
< 
example body
* Connection #0 to host localhost left intact
```

## Build WASM Module

Now you want to make changes to the C++ filter ([envoy_filter_http_wasm_example.cc](envoy_filter_http_wasm_example.cc))
and regenerate to WASM module ([envoy_filter_http_wasm_example.wasm](envoy_filter_http_wasm_example.wasm)).

### Build using Docker image with WASM toolchain

1. Build Docker image with WASM toolchain
   ```shell
   docker build -t wasmsdk:v2 -f ../../api/wasm/cpp/Dockerfile-sdk ../../api/wasm/cpp/
   ```
2. Build example WASM Module
   ```shell
   ./docker_build_cpp_wasm.sh
   ```

### Build using WASM toolchain installed on a host

1. Download and install [`protobuf`](https://github.com/protocolbuffers/protobuf/blob/master/src/README.md).
2. Download and install [`WAVM`](https://github.com/WAVM/WAVM).
3. Download and install [`emscripten`](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions)
4. Activate emsdk archived version 1.39.2 (Note: as of 2019/10/18 the latest known working version is 1.39.2).

    ```shell
    # in emsdk directory
    ./emsdk install 1.39.2
    ./emsdk activate 1.39.2
    source emsdk_env.sh
    # verify, should output 1.39.2
    em++ -v 2>&1 |grep Emscripten
    ```

5. Build WASM filter in envoy directory.

    ```shell
    envoy/examples/wasm$ source <path/to/emsdk>/emsdk_env.sh  # Only needed if in another shell session
    envoy/examples/wasm$ touch envoy_filter_http_wasm_example.cc && make
    ```

## Build the Envoy WASM Image

<!--TODO(incfly): remove this once we upstream WASM to envoyproxy main repo.-->

For Envoy WASM runtime developers, if you want to make changes, please

1. Follow [instructions](https://github.com/envoyproxy/envoy-wasm/blob/master/WASM.md).
2. Modify `docker-compose.yaml` to mount your own Envoy.
