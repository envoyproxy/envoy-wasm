#!/usr/bin/env bash

SCRIPT_DIR=$(cd $(dirname $0); pwd)
PROJECT_ROOT=$SCRIPT_DIR/../

cd $SCRIPT_DIR/../test/extensions/wasm/test_data

cargo raze
mkdir -p cargo/
cp -r remote cargo && rm -r remote
mv BUILD cargo
cp -r cargo/ $PROJECT_ROOT/bazel/external/ && rm -r cargo
mv crates.bzl $PROJECT_ROOT/bazel/

echo $PWD