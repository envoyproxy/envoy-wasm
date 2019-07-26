#!/bin/bash

set -e

function finish {
  echo "disk space at end of build:"
  df -h
}
trap finish EXIT

echo "disk space at beginning of build:"
df -h

. "$(dirname "$0")"/setup_cache.sh

# TODO(zuercher): remove --flaky_test_attempts when https://github.com/envoyproxy/envoy/issues/2428
# is resolved.
BAZEL_BUILD_OPTIONS="--curses=no --show_task_finish --verbose_failures \
  --action_env=PATH=/usr/local/bin:/opt/local/bin:/usr/bin:/bin --test_output=all \
  --flaky_test_attempts=integration@2 ${BAZEL_BUILD_EXTRA_OPTIONS} ${BAZEL_EXTRA_TEST_OPTIONS}"

# Build envoy and run tests as separate steps so that failure output
# is somewhat more deterministic (rather than interleaving the build
# and test steps).

if [[ $# -gt 0 ]]; then
  TEST_TARGETS=$*
else
  TEST_TARGETS=//test/...
fi

if [[ "$TEST_TARGETS" == "//test/..." ]]; then
  bazel build ${BAZEL_BUILD_OPTIONS} //source/exe:envoy-static
fi
bazel test ${BAZEL_BUILD_OPTIONS} ${TEST_TARGETS}
