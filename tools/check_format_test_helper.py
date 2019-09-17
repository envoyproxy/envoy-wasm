#!/usr/bin/env python

# Tests check_format.py. This must be run in a context where the clang
# version and settings are compatible with the one in the Envoy
# docker. Normally this is run via check_format_test.sh, which
# executes it in under docker.

from __future__ import print_function

import argparse
import logging
import os
import shutil
import subprocess
import sys

tools = os.path.dirname(os.path.realpath(__file__))
tmp = os.path.join(os.getenv('TEST_TMPDIR', "/tmp"), "check_format_test")
src = os.path.join(tools, 'testdata', 'check_format')
check_format = sys.executable + " " + os.path.join(tools, 'check_format.py')
errors = 0


# Echoes and runs an OS command, returning exit status and the captured
# stdout+stderr as a string array.
def runCommand(command):
  stdout = []
  status = 0
  try:
    out = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT).strip()
    if out:
      stdout = out.split("\n")
  except subprocess.CalledProcessError as e:
    status = e.returncode
    for line in e.output.splitlines():
      stdout.append(line)
  logging.info("%s" % command)
  return status, stdout


# Runs the 'check_format' operation, on the specified file, printing
# the comamnd run and the status code as well as the stdout, and returning
# all of that to the caller.
def runCheckFormat(operation, filename):
  command = check_format + " " + operation + " " + filename
  status, stdout = runCommand(command)
  return (command, status, stdout)


def getInputFile(filename):
  infile = os.path.join(src, filename)
  directory = os.path.dirname(filename)
  if not directory == '' and not os.path.isdir(directory):
    os.makedirs(directory)
  shutil.copyfile(infile, filename)
  return filename


# Attempts to fix file, returning a 4-tuple: the command, input file name,
# output filename, captured stdout as an array of lines, and the error status
# code.
def fixFileHelper(filename):
  infile = os.path.join(src, filename)
  directory = os.path.dirname(filename)
  if not directory == '' and not os.path.isdir(directory):
    os.makedirs(directory)

  shutil.copyfile(infile, filename)
  command, status, stdout = runCheckFormat("fix", getInputFile(filename))
  return (command, infile, filename, status, stdout)


# Attempts to fix a file, returning the status code and the generated output.
# If the fix was successful, the diff is returned as a string-array. If the file
# was not fixable, the error-messages are returned as a string-array.
def fixFileExpectingSuccess(file):
  command, infile, outfile, status, stdout = fixFileHelper(file)
  if status != 0:
    print("FAILED:")
    emitStdoutAsError(stdout)
    return 1
  status, stdout = runCommand('diff ' + outfile + ' ' + infile + '.gold')
  if status != 0:
    print("FAILED:")
    emitStdoutAsError(stdout)
    return 1
  return 0


def fixFileExpectingNoChange(file):
  command, infile, outfile, status, stdout = fixFileHelper(file)
  if status != 0:
    return 1
  status, stdout = runCommand('diff ' + outfile + ' ' + infile)
  if status != 0:
    logging.error(file + ': expected file to remain unchanged')
    return 1
  return 0


def emitStdoutAsError(stdout):
  logging.error("\n".join(stdout))


def expectError(filename, status, stdout, expected_substring):
  if status == 0:
    logging.error("%s: Expected failure `%s`, but succeeded" % (filename, expected_substring))
    return 1
  for line in stdout:
    if expected_substring in line:
      return 0
  logging.error("%s: Could not find '%s' in:\n" % (filename, expected_substring))
  emitStdoutAsError(stdout)
  return 1


def fixFileExpectingFailure(filename, expected_substring):
  command, infile, outfile, status, stdout = fixFileHelper(filename)
  return expectError(filename, status, stdout, expected_substring)


def checkFileExpectingError(filename, expected_substring):
  command, status, stdout = runCheckFormat("check", getInputFile(filename))
  return expectError(filename, status, stdout, expected_substring)


def checkAndFixError(filename, expected_substring):
  errors = checkFileExpectingError(filename, expected_substring)
  errors += fixFileExpectingSuccess(filename)
  return errors


def checkToolNotFoundError():
  # Temporarily change PATH to test the error about lack of external tools.
  oldPath = os.environ["PATH"]
  os.environ["PATH"] = "/sbin:/usr/sbin"
  clang_format = os.getenv("CLANG_FORMAT", "clang-format-8")
  errors = checkFileExpectingError("no_namespace_envoy.cc", "Command %s not found." % clang_format)
  os.environ["PATH"] = oldPath
  return errors


def checkUnfixableError(filename, expected_substring):
  errors = checkFileExpectingError(filename, expected_substring)
  errors += fixFileExpectingFailure(filename, expected_substring)
  return errors


def checkFileExpectingOK(filename):
  command, status, stdout = runCheckFormat("check", getInputFile(filename))
  if status != 0:
    logging.error("Expected %s to have no errors; status=%d, output:\n" % (filename, status))
    emitStdoutAsError(stdout)
  return status + fixFileExpectingNoChange(filename)


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='tester for check_format.py.')
  parser.add_argument('--log', choices=['INFO', 'WARN', 'ERROR'], default='INFO')
  args = parser.parse_args()
  logging.basicConfig(format='%(message)s', level=args.log)
  errors = 0

  # Now create a temp directory to copy the input files, so we can fix them
  # without actually fixing our testdata. This requires chdiring to the temp
  # directory, so it's annoying to comingle check-tests and fix-tests.
  shutil.rmtree(tmp, True)
  os.makedirs(tmp)
  os.chdir(tmp)

  # The following error is the error about unavailability of external tools.
  errors += checkToolNotFoundError()

  # The following errors can be detected but not fixed automatically.
  errors += checkUnfixableError("no_namespace_envoy.cc",
                                "Unable to find Envoy namespace or NOLINT(namespace-envoy)")
  errors += checkUnfixableError("mutex.cc", "Don't use <mutex> or <condition_variable*>")
  errors += checkUnfixableError("condition_variable.cc",
                                "Don't use <mutex> or <condition_variable*>")
  errors += checkUnfixableError("condition_variable_any.cc",
                                "Don't use <mutex> or <condition_variable*>")
  errors += checkUnfixableError("shared_mutex.cc", "shared_mutex")
  errors += checkUnfixableError("shared_mutex.cc", "shared_mutex")
  real_time_inject_error = (
      "Don't reference real-world time sources from production code; use injection")
  errors += checkUnfixableError("real_time_source.cc", real_time_inject_error)
  errors += checkUnfixableError("real_time_system.cc", real_time_inject_error)
  errors += checkUnfixableError("system_clock.cc", real_time_inject_error)
  errors += checkUnfixableError("steady_clock.cc", real_time_inject_error)
  errors += checkUnfixableError("condvar_wait_for.cc", real_time_inject_error)
  errors += checkUnfixableError("sleep.cc", real_time_inject_error)
  errors += checkUnfixableError("std_atomic_free_functions.cc", "std::atomic_*")
  errors += checkUnfixableError("std_get_time.cc", "std::get_time")
  errors += checkUnfixableError("no_namespace_envoy.cc",
                                "Unable to find Envoy namespace or NOLINT(namespace-envoy)")
  errors += checkUnfixableError("bazel_tools.BUILD", "unexpected @bazel_tools reference")
  errors += checkUnfixableError("proto.BUILD", "unexpected direct external dependency on protobuf")
  errors += checkUnfixableError("proto_deps.cc", "unexpected direct dependency on google.protobuf")
  errors += checkUnfixableError("attribute_packed.cc", "Don't use __attribute__((packed))")
  errors += checkUnfixableError("designated_initializers.cc", "Don't use designated initializers")
  errors += checkUnfixableError("elvis_operator.cc", "Don't use the '?:' operator")
  errors += checkUnfixableError("testing_test.cc",
                                "Don't use 'using testing::Test;, elaborate the type instead")
  errors += checkUnfixableError(
      "serialize_as_string.cc",
      "Don't use MessageLite::SerializeAsString for generating deterministic serialization")
  errors += checkUnfixableError(
      "version_history.rst",
      "Version history line malformed. Does not match VERSION_HISTORY_NEW_LINE_REGEX in "
      "check_format.py")
  errors += checkUnfixableError(
      "counter_from_string.cc",
      "Don't lookup stats by name at runtime; use StatName saved during construction")
  errors += checkUnfixableError(
      "gauge_from_string.cc",
      "Don't lookup stats by name at runtime; use StatName saved during construction")
  errors += checkUnfixableError(
      "histogram_from_string.cc",
      "Don't lookup stats by name at runtime; use StatName saved during construction")
  errors += checkUnfixableError(
      "regex.cc", "Don't use std::regex in code that handles untrusted input. Use RegexMatcher")

  errors += fixFileExpectingFailure(
      "api/missing_package.proto",
      "Unable to find package name for proto file: ./api/missing_package.proto")

  # The following files have errors that can be automatically fixed.
  errors += checkAndFixError("over_enthusiastic_spaces.cc",
                             "./over_enthusiastic_spaces.cc:3: over-enthusiastic spaces")
  errors += checkAndFixError("extra_enthusiastic_spaces.cc",
                             "./extra_enthusiastic_spaces.cc:3: over-enthusiastic spaces")
  errors += checkAndFixError("angle_bracket_include.cc",
                             "envoy includes should not have angle brackets")
  errors += checkAndFixError("proto_style.cc", "incorrect protobuf type reference")
  errors += checkAndFixError("long_line.cc", "clang-format check failed")
  errors += checkAndFixError("header_order.cc", "header_order.py check failed")
  errors += checkAndFixError("license.BUILD", "envoy_build_fixer check failed")
  errors += checkAndFixError("bad_envoy_build_sys_ref.BUILD", "Superfluous '@envoy//' prefix")
  errors += checkAndFixError("proto_format.proto", "clang-format check failed")
  errors += checkAndFixError("api/java_options.proto", "Java proto option")
  errors += checkFileExpectingError("api/go_package.proto", "go_package option should not be set")
  errors += checkAndFixError(
      "cpp_std.cc",
      "term absl::make_unique< should be replaced with standard library term std::make_unique<")

  errors += checkFileExpectingOK("real_time_source_override.cc")
  errors += checkFileExpectingOK("time_system_wait_for.cc")

  if errors != 0:
    logging.error("%d FAILURES" % errors)
    exit(1)
  logging.warn("PASS")
