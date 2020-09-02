#!/usr/bin/bash.exe

set -e

function finish {
  echo "disk space at end of build:"
  df -h
}
trap finish EXIT

echo "disk space at beginning of build:"
df -h

. "$(dirname "$0")"/setup_cache.sh

# Set up TMPDIR so bash and non-bash can access
# e.g. TMPDIR=/d/tmp, make a link from /d/d to /d so both bash and Windows programs resolve the
# same path
# This is due to this issue: https://github.com/bazelbuild/rules_foreign_cc/issues/334
# rules_foreign_cc does not currently use bazel output/temp directories by default, it uses mktemp
# which respects the value of the TMPDIR environment variable
drive="$(readlink -f $TMPDIR | cut -d '/' -f2)"
if [ ! -e "/$drive/$drive" ]; then
  /c/windows/system32/cmd.exe /c "mklink /d $drive:\\$drive $drive:\\"
fi

BAZEL_STARTUP_OPTIONS="--output_base=c:/_eb"
BAZEL_BUILD_OPTIONS="-c opt --define wasm=disabled --config=msvc-cl --show_task_finish --verbose_failures \
  --test_output=errors ${BAZEL_BUILD_EXTRA_OPTIONS} ${BAZEL_EXTRA_TEST_OPTIONS}"

# Test to validate updates of all dependency libraries in bazel/external and bazel/foreign_cc
# bazel ${BAZEL_STARTUP_OPTIONS} build ${BAZEL_BUILD_OPTIONS} //bazel/... --build_tag_filters=-skip_on_windows

# Complete envoy-static build (nothing needs to be skipped, build failure indicates broken dependencies)
bazel ${BAZEL_STARTUP_OPTIONS} build ${BAZEL_BUILD_OPTIONS} //source/exe:envoy-static

# Test invocations of known-working tests on Windows
bazel ${BAZEL_STARTUP_OPTIONS} test ${BAZEL_BUILD_OPTIONS} //test/... --test_tag_filters=-skip_on_windows,-fails_on_windows,-flaky_on_windows --build_tests_only

# Build tests that are known-flaky or known-failing to ensure no compilation regressions
bazel ${BAZEL_STARTUP_OPTIONS} build ${BAZEL_BUILD_OPTIONS} //test/... --test_tag_filters=-skip_on_windows,fails_on_windows,flaky_on_windows --build_tests_only

# Summarize tests bypasssed to monitor the progress of porting to Windows
echo Tests bypassed as skip_on_windows: `bazel query 'kind(".*test rule", attr("tags", "skip_on_windows", //test/...))' 2>/dev/null | sort | wc -l` known unbuildable or inapplicable tests
echo Tests bypassed as fails_on_windows: `bazel query 'kind(".*test rule", attr("tags", "fails_on_windows", //test/...))' 2>/dev/null | sort | wc -l` known incompatible tests
echo Tests bypassed as flaky_on_windows: `bazel query 'kind(".*test rule", attr("tags", "flaky_on_windows", //test/...))' 2>/dev/null | sort | wc -l` known unstable tests
