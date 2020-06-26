#!/bin/bash

# For this test we use a slightly modiified test binary, based on
# source/exe/enovy-static. If this starts failing to run or build, ensure that
# source/exe/main.cc and ./hotrestart_main.cc have not diverged except for
# adding the new gauge.
export ENVOY_BIN="${TEST_SRCDIR}"/envoy/test/integration/hotrestart_main
source "$TEST_SRCDIR/envoy/test/integration/test_utility.sh"

# TODO(htuch): In this test script, we are duplicating work done in test_environment.cc via sed.
# Instead, we can add a simple C++ binary that links against test_environment.cc and uses the
# substitution methods provided there.
JSON_TEST_ARRAY=()

# Ensure that the runtime watch root exist.
mkdir -p "${TEST_TMPDIR}"/test/common/runtime/test_data/current/envoy
mkdir -p "${TEST_TMPDIR}"/test/common/runtime/test_data/current/envoy_override

# Parameterize IPv4 and IPv6 testing.
if [[ -z "${ENVOY_IP_TEST_VERSIONS}" ]] || [[ "${ENVOY_IP_TEST_VERSIONS}" == "all" ]] \
  || [[ "${ENVOY_IP_TEST_VERSIONS}" == "v4only" ]]; then
  HOT_RESTART_JSON_V4="${TEST_TMPDIR}"/hot_restart_v4.yaml
  echo building ${HOT_RESTART_JSON_V4} ...
  cat "${TEST_SRCDIR}/envoy"/test/config/integration/server.yaml |
    sed -e "s#{{ upstream_. }}#0#g" | \
    sed -e "s#{{ test_rundir }}#$TEST_SRCDIR/envoy#" | \
    sed -e "s#{{ test_tmpdir }}#$TEST_TMPDIR#" | \
    sed -e "s#{{ ip_loopback_address }}#127.0.0.1#" | \
    sed -e "s#{{ reuse_port }}#false#" | \
    sed -e "s#{{ dns_lookup_family }}#V4_ONLY#" | \
    sed -e "s#{{ null_device_path }}#/dev/null#" | \
    cat > "${HOT_RESTART_JSON_V4}"
  JSON_TEST_ARRAY+=("${HOT_RESTART_JSON_V4}")
fi

if [[ -z "${ENVOY_IP_TEST_VERSIONS}" ]] || [[ "${ENVOY_IP_TEST_VERSIONS}" == "all" ]] \
  || [[ "${ENVOY_IP_TEST_VERSIONS}" == "v6only" ]]; then
  HOT_RESTART_JSON_V6="${TEST_TMPDIR}"/hot_restart_v6.yaml
  cat "${TEST_SRCDIR}/envoy"/test/config/integration/server.yaml |
    sed -e "s#{{ upstream_. }}#0#g" | \
    sed -e "s#{{ test_rundir }}#$TEST_SRCDIR/envoy#" | \
    sed -e "s#{{ test_tmpdir }}#$TEST_TMPDIR#" | \
    sed -e "s#{{ ip_loopback_address }}#::1#" | \
    sed -e "s#{{ reuse_port }}#false#" | \
    sed -e "s#{{ dns_lookup_family }}#v6_only#" | \
    sed -e "s#{{ null_device_path }}#/dev/null#" | \
    cat > "${HOT_RESTART_JSON_V6}"
  JSON_TEST_ARRAY+=("${HOT_RESTART_JSON_V6}")
fi

# Also test for listening on UNIX domain sockets. We use IPv4 for the
# upstreams to avoid too much wild sedding.
HOT_RESTART_JSON_UDS="${TEST_TMPDIR}"/hot_restart_uds.yaml
SOCKET_DIR="$(mktemp -d /tmp/envoy_test_hotrestart.XXXXXX)"
cat "${TEST_SRCDIR}/envoy"/test/config/integration/server_unix_listener.yaml |
  sed -e "s#{{ socket_dir }}#${SOCKET_DIR}#" | \
  sed -e "s#{{ ip_loopback_address }}#127.0.0.1#" | \
  sed -e "s#{{ null_device_path }}#/dev/null#" | \
  cat > "${HOT_RESTART_JSON_UDS}"
JSON_TEST_ARRAY+=("${HOT_RESTART_JSON_UDS}")

# Test reuse port listener.
HOT_RESTART_JSON_REUSE_PORT="${TEST_TMPDIR}"/hot_restart_v4.yaml
echo building ${HOT_RESTART_JSON_V4} ...
cat "${TEST_SRCDIR}/envoy"/test/config/integration/server.yaml |
  sed -e "s#{{ upstream_. }}#0#g" | \
  sed -e "s#{{ test_rundir }}#$TEST_SRCDIR/envoy#" | \
  sed -e "s#{{ test_tmpdir }}#$TEST_TMPDIR#" | \
  sed -e "s#{{ ip_loopback_address }}#127.0.0.1#" | \
  sed -e "s#{{ reuse_port }}#true#" | \
  sed -e "s#{{ dns_lookup_family }}#V4_ONLY#" | \
  sed -e "s#{{ null_device_path }}#/dev/null#" | \
  cat > "${HOT_RESTART_JSON_REUSE_PORT}"
JSON_TEST_ARRAY+=("${HOT_RESTART_JSON_REUSE_PORT}")

echo "Hot restart test using dynamic base id"

TEST_INDEX=0
function run_testsuite() {
  local HOT_RESTART_JSON="$1"
  local FAKE_SYMBOL_TABLE="$2"

  start_test validation
  check "${ENVOY_BIN}" -c "${HOT_RESTART_JSON}" --mode validate --service-cluster cluster \
      --use-fake-symbol-table "$FAKE_SYMBOL_TABLE" --service-node node --disable-hot-restart

  local BASE_ID_PATH=$(mktemp 'envoy_test_base_id.XXXXXX')
  echo "Selected dynamic base id path ${BASE_ID_PATH}"

  # Now start the real server, hot restart it twice, and shut it all down as a
  # basic hot restart sanity test. We expect SERVER_0 to exit quickly when
  # SERVER_2 starts, and are not relying on timeouts.
  start_test Starting epoch 0
  ADMIN_ADDRESS_PATH_0="${TEST_TMPDIR}"/admin.0."${TEST_INDEX}".address
  run_in_background_saving_pid "${ENVOY_BIN}" -c "${HOT_RESTART_JSON}" \
      --restart-epoch 0  --use-dynamic-base-id --base-id-path "${BASE_ID_PATH}" \
      --service-cluster cluster --service-node node --use-fake-symbol-table "$FAKE_SYMBOL_TABLE" \
      --admin-address-path "${ADMIN_ADDRESS_PATH_0}"

  local BASE_ID=$(cat "${BASE_ID_PATH}")
  while [ -z "${BASE_ID}" ]; do
      echo "Waiting for base id"
      sleep 0.5
      BASE_ID=$(cat "${BASE_ID_PATH}")
  done

  echo "Selected dynamic base id ${BASE_ID}"

  SERVER_0_PID=$BACKGROUND_PID

  start_test Updating original config listener addresses
  sleep 3

  UPDATED_HOT_RESTART_JSON="${TEST_TMPDIR}"/hot_restart_updated."${TEST_INDEX}".yaml
  "${TEST_SRCDIR}/envoy"/tools/socket_passing "-o" "${HOT_RESTART_JSON}" "-a" "${ADMIN_ADDRESS_PATH_0}" \
    "-u" "${UPDATED_HOT_RESTART_JSON}"

  # Send SIGUSR1 signal to the first server, this should not kill it. Also send SIGHUP which should
  # get eaten.
  echo "Sending SIGUSR1/SIGHUP to first server"
  kill -SIGUSR1 ${SERVER_0_PID}
  kill -SIGHUP ${SERVER_0_PID}
  sleep 3

  disableHeapCheck

  # To ensure that we don't accidentally change the /hot_restart_version
  # string, compare it against a hard-coded string.
  start_test Checking for consistency of /hot_restart_version
  CLI_HOT_RESTART_VERSION=$("${ENVOY_BIN}" --hot-restart-version --base-id "${BASE_ID}" 2>&1)
  EXPECTED_CLI_HOT_RESTART_VERSION="11.104"
  echo "The Envoy's hot restart version is ${CLI_HOT_RESTART_VERSION}"
  echo "Now checking that the above version is what we expected."
  check [ "${CLI_HOT_RESTART_VERSION}" = "${EXPECTED_CLI_HOT_RESTART_VERSION}" ]

  start_test Checking for consistency of /hot_restart_version with --use-fake-symbol-table "$FAKE_SYMBOL_TABLE"
  CLI_HOT_RESTART_VERSION=$("${ENVOY_BIN}" --hot-restart-version --base-id "${BASE_ID}" \
    --use-fake-symbol-table "$FAKE_SYMBOL_TABLE" 2>&1)
  EXPECTED_CLI_HOT_RESTART_VERSION="11.104"
  check [ "${CLI_HOT_RESTART_VERSION}" = "${EXPECTED_CLI_HOT_RESTART_VERSION}" ]

  start_test Checking for match of --hot-restart-version and admin /hot_restart_version
  ADMIN_ADDRESS_0=$(cat "${ADMIN_ADDRESS_PATH_0}")
  echo fetching hot restart version from http://${ADMIN_ADDRESS_0}/hot_restart_version ...
  ADMIN_HOT_RESTART_VERSION=$(curl -sg http://${ADMIN_ADDRESS_0}/hot_restart_version)
  echo "Fetched ADMIN_HOT_RESTART_VERSION is ${ADMIN_HOT_RESTART_VERSION}"
  CLI_HOT_RESTART_VERSION=$("${ENVOY_BIN}" --hot-restart-version --base-id "${BASE_ID}" \
    --use-fake-symbol-table "$FAKE_SYMBOL_TABLE" 2>&1)
  check [ "${ADMIN_HOT_RESTART_VERSION}" = "${CLI_HOT_RESTART_VERSION}" ]

  start_test Checking server.hot_restart_generation 1
  GENERATION_0=$(scrape_stat "${ADMIN_ADDRESS_0}" "server.hot_restart_generation")
  check [ "$GENERATION_0" = "1" ];

  # Verify we can see server.live in the admin port.
  SERVER_LIVE_0=$(scrape_stat "${ADMIN_ADDRESS_0}" "server.live")
  check [ "$SERVER_LIVE_0" = "1" ];

  # Capture the value of test_gauge from the initial parent: it should be 1.
  TEST_GAUGE_0=$(scrape_stat "${ADMIN_ADDRESS_0}" "hotrestart_test_gauge")
  check [ "$TEST_GAUGE_0" = "1" ];

  enableHeapCheck

  ADMIN_ADDRESS_PATH_1="${TEST_TMPDIR}"/admin.1."${TEST_INDEX}".address
  run_in_background_saving_pid "${ENVOY_BIN}" -c "${UPDATED_HOT_RESTART_JSON}" \
      --restart-epoch 1 --base-id "${BASE_ID}" --service-cluster cluster --service-node node \
      --use-fake-symbol-table "$FAKE_SYMBOL_TABLE" --admin-address-path "${ADMIN_ADDRESS_PATH_1}"

  SERVER_1_PID=$BACKGROUND_PID

  # Wait for stat flushing
  sleep 7

  ADMIN_ADDRESS_1=$(cat "${ADMIN_ADDRESS_PATH_1}")
  SERVER_LIVE_1=$(scrape_stat "${ADMIN_ADDRESS_1}" "server.live")
  check [ "$SERVER_LIVE_1" = "1" ];

  # Check to see that the SERVER_1 accumulates the test_gauge value from
  # SERVER_0, This will be erased once SERVER_0 terminates.
  if [ "$TEST_GAUGE_0" != 0 ]; then
    start_test Checking that the hotrestart_test_gauge incorporates SERVER_0 and SERVER_1.
    TEST_GAUGE_1=$(scrape_stat "${ADMIN_ADDRESS_1}" "hotrestart_test_gauge")
    check [ $TEST_GAUGE_1 = "2" ]
  fi

  start_test Checking that listener addresses have not changed
  HOT_RESTART_JSON_1="${TEST_TMPDIR}"/hot_restart.1."${TEST_INDEX}".yaml
  "${TEST_SRCDIR}/envoy"/tools/socket_passing "-o" "${UPDATED_HOT_RESTART_JSON}" "-a" "${ADMIN_ADDRESS_PATH_1}" \
    "-u" "${HOT_RESTART_JSON_1}"
  CONFIG_DIFF=$(diff "${UPDATED_HOT_RESTART_JSON}" "${HOT_RESTART_JSON_1}")
  [[ -z "${CONFIG_DIFF}" ]]

  # Send SIGUSR1 signal to the second server, this should not kill it, and
  # we prove that by checking its stats after having sent it a signal.
  start_test Sending SIGUSR1 to SERVER_1.
  kill -SIGUSR1 ${SERVER_1_PID}
  sleep 3

  start_test Checking server.hot_restart_generation 2
  GENERATION_1=$(scrape_stat "${ADMIN_ADDRESS_1}" "server.hot_restart_generation")
  check [ "$GENERATION_1" = "2" ];

  ADMIN_ADDRESS_PATH_2="${TEST_TMPDIR}"/admin.2."${TEST_INDEX}".address
  start_test Starting epoch 2
  run_in_background_saving_pid "${ENVOY_BIN}" -c "${UPDATED_HOT_RESTART_JSON}" \
      --restart-epoch 2  --base-id "${BASE_ID}" --service-cluster cluster --service-node node \
      --use-fake-symbol-table "$FAKE_SYMBOL_TABLE" --admin-address-path "${ADMIN_ADDRESS_PATH_2}" \
      --parent-shutdown-time-s 3

  SERVER_2_PID=$BACKGROUND_PID

  # Now wait for the SERVER_0 to exit. It should occur immediately when SERVER_2 starts, as
  # SERVER_1 will terminate SERVER_0 when it becomes the parent.
  start_test Waiting for epoch 0 to finish.
  echo time wait ${SERVER_0_PID}
  time wait ${SERVER_0_PID}
  [[ $? == 0 ]]

  # Then wait for the SERVER_1 to exit, which should happen within a few seconds
  # due to '--parent-shutdown-time-s 3' on SERVER_2.
  start_test Waiting for epoch 1 to finish.
  echo time wait ${SERVER_1_PID}
  time wait ${SERVER_1_PID}
  [[ $? == 0 ]]

  # This tests that we are retaining the generation count. For most Gauges,
  # we erase the parent contribution when the parent exits, but
  # server.hot_restart_generation is excluded. Commenting out the call to
  # stat_merger_->retainParentGaugeValue(hot_restart_generation_stat_name_)
  # in source/server/hot_restarting_child.cc results in this test failing,
  # with the generation being decremented back to 1.
  start_test Checking server.hot_restart_generation 2
  ADMIN_ADDRESS_2=$(cat "${ADMIN_ADDRESS_PATH_2}")
  GENERATION_2=$(scrape_stat "${ADMIN_ADDRESS_2}" "server.hot_restart_generation")
  check [ "$GENERATION_2" = "3" ];

  # Check to see that the SERVER_2's test_gauge value reverts bac to 1, since
  # its parents have now exited and we have erased their gauge contributions.
  start_test Check that the hotrestart_test_gauge reported in SERVER_2 excludes parent contribution
  wait_status=$(wait_for_stat "$ADMIN_ADDRESS_2" "hotrestart_test_gauge" -eq 1 5)
  echo $wait_status
  if [[ "$wait_status" != success* ]]; then
    handle_failure timeout
  fi

  start_test Checking that listener addresses have not changed
  HOT_RESTART_JSON_2="${TEST_TMPDIR}"/hot_restart.2."${TEST_INDEX}".yaml
  "${TEST_SRCDIR}/envoy"/tools/socket_passing "-o" "${UPDATED_HOT_RESTART_JSON}" "-a" "${ADMIN_ADDRESS_PATH_2}" \
    "-u" "${HOT_RESTART_JSON_2}"
  CONFIG_DIFF=$(diff "${UPDATED_HOT_RESTART_JSON}" "${HOT_RESTART_JSON_2}")
  [[ -z "${CONFIG_DIFF}" ]]

  # Now term the last server, and the other one should exit also.
  start_test Killing and waiting for epoch 2
  kill ${SERVER_2_PID}
  wait ${SERVER_2_PID}
  [[ $? == 0 ]]
}

for HOT_RESTART_JSON in "${JSON_TEST_ARRAY[@]}"
do
  # Run one of the tests with real symbol tables. No need to do all of them.
  if [ "$TEST_INDEX" = "0" ]; then
    run_testsuite "$HOT_RESTART_JSON" "0" || exit 1
  fi

  run_testsuite "$HOT_RESTART_JSON" "1" || exit 1
done

start_test disabling hot_restart by command line.
CLI_HOT_RESTART_VERSION=$("${ENVOY_BIN}" --hot-restart-version --disable-hot-restart 2>&1)
check [ "disabled" = "${CLI_HOT_RESTART_VERSION}" ]

echo "PASS"
