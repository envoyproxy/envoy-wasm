#!/bin/bash

source "${TEST_SRCDIR}/envoy/test/integration/test_utility.sh"

# Use TEST_RANDOM_SEED or TEST_SHARD_INDEX to choose a base id. This
# replicates the logic of TestEnvironment::chooseBaseId(2). See that method
# for details.
let BASE_ID=2000000+${TEST_RANDOM_SEED:-${TEST_SHARD_INDEX:-0}}

function expect_fail_with_error() {
  log="${TEST_TMPDIR}/envoy.log"
  rm -f "$log"
  expected_error="$1"
  shift
  echo ${ENVOY_BIN} --base-id "${BASE_ID}" "$@" ">&" "$log"
  ${ENVOY_BIN} --base-id "${BASE_ID}" "$@" >& "$log"
  EXIT_CODE=$?
  cat "$log"
  check [ $EXIT_CODE -eq 1 ]
  check grep "$expected_error" "$log"
}


start_test Launching envoy with a bogus command line flag.
expect_fail_with_error "PARSE ERROR: Argument: --bogus-flag" --bogus-flag

start_test Launching envoy without --config-path or --config-yaml fails.
expect_fail_with_error \
  "At least one of --config-path or --config-yaml or Options::configProto() should be non-empty"

start_test Launching envoy with unknown IP address.
expect_fail_with_error "error: unknown IP address version" --local-address-ip-version foo

start_test Launching envoy with unknown mode.
expect_fail_with_error "error: unknown mode" --mode foo

start_test Launching envoy with bogus component log level.
expect_fail_with_error "error: component log level not correctly specified" --component-log-level upstream:foo:bar

start_test Launching envoy with invalid log level.
expect_fail_with_error "error: invalid log level specified" --component-log-level upstream:foo

start_test Launching envoy with invalid component.
expect_fail_with_error "error: invalid component specified" --component-log-level foo:debug
