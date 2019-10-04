#!/bin/bash
docker run -e uid="$(id -u)" -e gid="$(id -g)" -v $PWD:/work -v $(realpath ../../../../../../api/wasm/cpp):/external_sdk -w /work wasmsdk:v1 bash ./docker_cpp_builder.sh
