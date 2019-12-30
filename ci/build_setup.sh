#!/bin/bash

# Configure environment variables for Bazel build and test.

set -e

export PPROF_PATH=/thirdparty_build/bin/pprof

[ -z "${NUM_CPUS}" ] && NUM_CPUS=`grep -c ^processor /proc/cpuinfo`
[ -z "${ENVOY_SRCDIR}" ] && export ENVOY_SRCDIR=/source
echo "ENVOY_SRCDIR=${ENVOY_SRCDIR}"

function setup_gcc_toolchain() {
  if [[ ! -z "${ENVOY_STDLIB}" && "${ENVOY_STDLIB}" != "libstdc++" ]]; then
    echo "gcc toolchain doesn't support ${ENVOY_STDLIB}."
    exit 1
  fi
  if [[ -z "${ENVOY_RBE}" ]]; then
    export CC=gcc
    export CXX=g++
    export BAZEL_COMPILER=gcc
    echo "$CC/$CXX toolchain configured"
  else
    export BAZEL_BUILD_OPTIONS="--config=remote-gcc ${BAZEL_BUILD_OPTIONS}"
  fi
}

function setup_clang_toolchain() {
  ENVOY_STDLIB="${ENVOY_STDLIB:-libc++}"
  if [[ -z "${ENVOY_RBE}" ]]; then
    if [[ "${ENVOY_STDLIB}" == "libc++" ]]; then
      export BAZEL_BUILD_OPTIONS="--config=libc++ ${BAZEL_BUILD_OPTIONS}"
    else
      export BAZEL_BUILD_OPTIONS="--config=clang ${BAZEL_BUILD_OPTIONS}"
    fi
  else
    if [[ "${ENVOY_STDLIB}" == "libc++" ]]; then
      export BAZEL_BUILD_OPTIONS="--config=remote-clang-libc++ ${BAZEL_BUILD_OPTIONS}"
    else
      export BAZEL_BUILD_OPTIONS="--config=remote-clang ${BAZEL_BUILD_OPTIONS}"
    fi
  fi
  echo "clang toolchain with ${ENVOY_STDLIB} configured"
}

# Create a fake home. Python site libs tries to do getpwuid(3) if we don't and the CI
# Docker image gets confused as it has no passwd entry when running non-root
# unless we do this.
FAKE_HOME=/tmp/fake_home
mkdir -p "${FAKE_HOME}"
export HOME="${FAKE_HOME}"
export PYTHONUSERBASE="${FAKE_HOME}"

export BUILD_DIR=${BUILD_DIR:-/build}
if [[ ! -d "${BUILD_DIR}" ]]
then
  echo "${BUILD_DIR} mount missing - did you forget -v <something>:${BUILD_DIR}? Creating."
  mkdir -p "${BUILD_DIR}"
fi
export ENVOY_FILTER_EXAMPLE_SRCDIR="${BUILD_DIR}/envoy-filter-example"

# Environment setup.
export USER=bazel
export TEST_TMPDIR=${BUILD_DIR}/tmp
export BAZEL="bazel"
export PATH=/opt/llvm/bin:$PATH
export CLANG_FORMAT=clang-format

if [[ -f "/etc/redhat-release" ]]; then
  export BAZEL_BUILD_EXTRA_OPTIONS+="--copt=-DENVOY_IGNORE_GLIBCXX_USE_CXX11_ABI_ERROR=1"
fi

function cleanup() {
  # Remove build artifacts. This doesn't mess with incremental builds as these
  # are just symlinks.
  rm -rf "${ENVOY_SRCDIR}"/bazel-* clang.bazelrc
}

cleanup
trap cleanup EXIT

export LLVM_ROOT=/opt/llvm
bazel/setup_clang.sh "${LLVM_ROOT}"

[[ "${BUILD_REASON}" != "PullRequest" ]] && BAZEL_EXTRA_TEST_OPTIONS+=" --nocache_test_results --test_output=all"

export BAZEL_QUERY_OPTIONS="${BAZEL_OPTIONS}"
export BAZEL_BUILD_OPTIONS="--verbose_failures ${BAZEL_OPTIONS} --action_env=HOME --action_env=PYTHONUSERBASE \
  --local_cpu_resources=${NUM_CPUS} --show_task_finish --experimental_generate_json_trace_profile \
  --test_env=HOME --test_env=PYTHONUSERBASE --test_output=errors \
  --repository_cache=${BUILD_DIR}/repository_cache --experimental_repository_cache_hardlinks \
  ${BAZEL_BUILD_EXTRA_OPTIONS} ${BAZEL_EXTRA_TEST_OPTIONS}"

[[ "${BAZEL_EXPUNGE}" == "1" ]] && "${BAZEL}" clean --expunge

if [ "$1" != "-nofetch" ]; then
  # Setup Envoy consuming project.
  if [[ ! -d "${ENVOY_FILTER_EXAMPLE_SRCDIR}/.git" ]]; then
    rm -rf "${ENVOY_FILTER_EXAMPLE_SRCDIR}"
    git clone https://github.com/envoyproxy/envoy-filter-example.git "${ENVOY_FILTER_EXAMPLE_SRCDIR}"
  fi

  # This is the hash on https://github.com/envoyproxy/envoy-filter-example.git we pin to.
  (cd "${ENVOY_FILTER_EXAMPLE_SRCDIR}" && git fetch origin && git checkout -f 03b45933284b332fd1df42cfb3270751fe543842)
  sed -e "s|{ENVOY_SRCDIR}|${ENVOY_SRCDIR}|" "${ENVOY_SRCDIR}"/ci/WORKSPACE.filter.example > "${ENVOY_FILTER_EXAMPLE_SRCDIR}"/WORKSPACE
  cp -f "${ENVOY_SRCDIR}"/.bazelversion "${ENVOY_FILTER_EXAMPLE_SRCDIR}"/.bazelversion
fi

# Also setup some space for building Envoy standalone.
export ENVOY_BUILD_DIR="${BUILD_DIR}"/envoy
mkdir -p "${ENVOY_BUILD_DIR}"

# This is where we copy build deliverables to.
export ENVOY_DELIVERY_DIR="${ENVOY_BUILD_DIR}"/source/exe
mkdir -p "${ENVOY_DELIVERY_DIR}"

# This is where we copy the coverage report to.
export ENVOY_COVERAGE_DIR="${ENVOY_BUILD_DIR}"/generated/coverage
mkdir -p "${ENVOY_COVERAGE_DIR}"

# This is where we dump failed test logs for CI collection.
export ENVOY_FAILED_TEST_LOGS="${ENVOY_BUILD_DIR}"/generated/failed-testlogs
mkdir -p "${ENVOY_FAILED_TEST_LOGS}"

# This is where we copy the build profile to.
export ENVOY_BUILD_PROFILE="${ENVOY_BUILD_DIR}"/generated/build-profile
mkdir -p "${ENVOY_BUILD_PROFILE}"

mkdir -p "${ENVOY_FILTER_EXAMPLE_SRCDIR}"/bazel
ln -sf "${ENVOY_SRCDIR}"/bazel/get_workspace_status "${ENVOY_FILTER_EXAMPLE_SRCDIR}"/bazel/
cp -f "${ENVOY_SRCDIR}"/.bazelrc "${ENVOY_FILTER_EXAMPLE_SRCDIR}"/
cp -f "${ENVOY_SRCDIR}"/*.bazelrc "${ENVOY_FILTER_EXAMPLE_SRCDIR}"/

export BUILDIFIER_BIN="/usr/local/bin/buildifier"
export BUILDOZER_BIN="/usr/local/bin/buildozer"
