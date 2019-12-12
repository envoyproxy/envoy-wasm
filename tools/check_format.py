#!/usr/bin/env python3

import argparse
import common
import functools
import multiprocessing
import os
import os.path
import pathlib
import re
import subprocess
import stat
import sys
import traceback
import shutil
import paths

EXCLUDED_PREFIXES = ("./generated/", "./thirdparty/", "./build", "./.git/", "./bazel-", "./.cache",
                     "./source/extensions/extensions_build_config.bzl",
                     "./bazel/toolchains/configs/", "./tools/testdata/check_format/",
                     "./tools/pyformat/", "./api/wasm/",
                     "./test/extensions/filters/http/wasm/test_data")
SUFFIXES = ("BUILD", "WORKSPACE", ".bzl", ".cc", ".h", ".java", ".m", ".md", ".mm", ".proto",
            ".rst")
DOCS_SUFFIX = (".md", ".rst")
PROTO_SUFFIX = (".proto")

# Files in these paths can make reference to protobuf stuff directly
GOOGLE_PROTOBUF_WHITELIST = ("ci/prebuilt", "source/common/protobuf", "api/test")
REPOSITORIES_BZL = "bazel/repositories.bzl"

# Files matching these exact names can reference real-world time. These include the class
# definitions for real-world time, the construction of them in main(), and perf annotation.
# For now it includes the validation server but that really should be injected too.
REAL_TIME_WHITELIST = ("./source/common/common/utility.h",
                       "./source/extensions/filters/http/common/aws/utility.cc",
                       "./source/common/event/real_time_system.cc",
                       "./source/common/event/real_time_system.h", "./source/exe/main_common.cc",
                       "./source/exe/main_common.h", "./source/server/config_validation/server.cc",
                       "./source/common/common/perf_annotation.h",
                       "./test/test_common/simulated_time_system.cc",
                       "./test/test_common/simulated_time_system.h",
                       "./test/test_common/test_time.cc", "./test/test_common/test_time.h",
                       "./test/test_common/utility.cc", "./test/test_common/utility.h",
                       "./test/integration/integration.h")

# Files in these paths can use MessageLite::SerializeAsString
SERIALIZE_AS_STRING_WHITELIST = (
    "./source/common/config/version_converter.cc",
    "./source/extensions/filters/http/grpc_json_transcoder/json_transcoder_filter.cc",
    "./test/common/protobuf/utility_test.cc",
    "./test/common/grpc/codec_test.cc",
    "./test/common/grpc/codec_fuzz_test.cc",
)

# Files in these paths can use Protobuf::util::JsonStringToMessage
JSON_STRING_TO_MESSAGE_WHITELIST = ("./source/common/protobuf/utility.cc")

# Histogram names which are allowed to be suffixed with the unit symbol, all of the pre-existing
# ones were grandfathered as part of PR #8484 for backwards compatibility.
HISTOGRAM_WITH_SI_SUFFIX_WHITELIST = ("downstream_cx_length_ms", "downstream_cx_length_ms",
                                      "initialization_time_ms", "loop_duration_us", "poll_delay_us",
                                      "request_time_ms", "upstream_cx_connect_ms",
                                      "upstream_cx_length_ms")

# Files in these paths can use std::regex
STD_REGEX_WHITELIST = ("./source/common/common/utility.cc", "./source/common/common/regex.h",
                       "./source/common/common/regex.cc",
                       "./source/common/stats/tag_extractor_impl.h",
                       "./source/common/stats/tag_extractor_impl.cc",
                       "./source/common/access_log/access_log_formatter.cc",
                       "./source/extensions/filters/http/squash/squash_filter.h",
                       "./source/extensions/filters/http/squash/squash_filter.cc",
                       "./source/server/http/admin.h", "./source/server/http/admin.cc")

# Only one C++ file should instantiate grpc_init
GRPC_INIT_WHITELIST = ("./source/common/grpc/google_grpc_context.cc")

CLANG_FORMAT_PATH = os.getenv("CLANG_FORMAT", "clang-format-9")
BUILDIFIER_PATH = paths.getBuildifier()
BUILDOZER_PATH = paths.getBuildozer()
ENVOY_BUILD_FIXER_PATH = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])),
                                      "envoy_build_fixer.py")
HEADER_ORDER_PATH = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), "header_order.py")
SUBDIR_SET = set(common.includeDirOrder())
INCLUDE_ANGLE = "#include <"
INCLUDE_ANGLE_LEN = len(INCLUDE_ANGLE)
PROTO_PACKAGE_REGEX = re.compile(r"^package (\S+);\n*", re.MULTILINE)
X_ENVOY_USED_DIRECTLY_REGEX = re.compile(r'.*\"x-envoy-.*\".*')

# yapf: disable
PROTOBUF_TYPE_ERRORS = {
    # Well-known types should be referenced from the ProtobufWkt namespace.
    "Protobuf::Any":                    "ProtobufWkt::Any",
    "Protobuf::Empty":                  "ProtobufWkt::Empty",
    "Protobuf::ListValue":              "ProtobufWkt::ListValue",
    "Protobuf::NULL_VALUE":             "ProtobufWkt::NULL_VALUE",
    "Protobuf::StringValue":            "ProtobufWkt::StringValue",
    "Protobuf::Struct":                 "ProtobufWkt::Struct",
    "Protobuf::Value":                  "ProtobufWkt::Value",

    # Other common mis-namespacing of protobuf types.
    "ProtobufWkt::Map":                 "Protobuf::Map",
    "ProtobufWkt::MapPair":             "Protobuf::MapPair",
    "ProtobufUtil::MessageDifferencer": "Protobuf::util::MessageDifferencer"
}
LIBCXX_REPLACEMENTS = {
    "absl::make_unique<": "std::make_unique<",
}

UNOWNED_EXTENSIONS = {
  "extensions/filters/http/ratelimit",
  "extensions/filters/http/buffer",
  "extensions/filters/http/rbac",
  "extensions/filters/http/ip_tagging",
  "extensions/filters/http/tap",
  "extensions/filters/http/health_check",
  "extensions/filters/http/cors",
  "extensions/filters/http/ext_authz",
  "extensions/filters/http/dynamo",
  "extensions/filters/http/lua",
  "extensions/filters/http/common",
  "extensions/filters/common",
  "extensions/filters/common/ratelimit",
  "extensions/filters/common/rbac",
  "extensions/filters/common/lua",
  "extensions/filters/listener/original_dst",
  "extensions/filters/listener/proxy_protocol",
  "extensions/stat_sinks/statsd",
  "extensions/stat_sinks/common",
  "extensions/stat_sinks/common/statsd",
  "extensions/health_checkers/redis",
  "extensions/access_loggers/grpc",
  "extensions/access_loggers/file",
  "extensions/common/tap",
  "extensions/transport_sockets/raw_buffer",
  "extensions/transport_sockets/tap",
  "extensions/tracers/zipkin",
  "extensions/tracers/dynamic_ot",
  "extensions/tracers/opencensus",
  "extensions/tracers/lightstep",
  "extensions/tracers/common",
  "extensions/tracers/common/ot",
  "extensions/retry/host/previous_hosts",
  "extensions/filters/network/ratelimit",
  "extensions/filters/network/client_ssl_auth",
  "extensions/filters/network/rbac",
  "extensions/filters/network/tcp_proxy",
  "extensions/filters/network/echo",
  "extensions/filters/network/ext_authz",
  "extensions/filters/network/redis_proxy",
  "extensions/filters/network/kafka",
  "extensions/filters/network/kafka/protocol",
  "extensions/filters/network/kafka/serialization",
  "extensions/filters/network/mongo_proxy",
  "extensions/filters/network/common",
  "extensions/filters/network/common/redis",
}
# yapf: enable


# Map a line transformation function across each line of a file.
# .bak temporaries.
def replaceLines(path, line_xform):
  # We used to use fileinput in the older Python 2.7 script, but this doesn't do
  # inplace mode and UTF-8 in Python 3, so doing it the manual way.
  output_lines = [line_xform(line) for line in readLines(path)]
  pathlib.Path(path).write_text('\n'.join(output_lines), encoding='utf-8')


# Obtain all the lines in a given file.
def readLines(path):
  return readFile(path).split('\n')


# Read a UTF-8 encoded file as a str.
def readFile(path):
  return pathlib.Path(path).read_text(encoding='utf-8')


# lookPath searches for the given executable in all directories in PATH
# environment variable. If it cannot be found, empty string is returned.
def lookPath(executable):
  for path_dir in os.environ["PATH"].split(os.pathsep):
    executable_path = os.path.join(path_dir, executable)
    if os.path.exists(executable_path):
      return executable_path
  return ""


# pathExists checks whether the given path exists. This function assumes that
# the path is absolute and evaluates environment variables.
def pathExists(executable):
  return os.path.exists(os.path.expandvars(executable))


# executableByOthers checks whether the given path has execute permission for
# others.
def executableByOthers(executable):
  st = os.stat(os.path.expandvars(executable))
  return bool(st.st_mode & stat.S_IXOTH)


# Check whether all needed external tools (clang-format, buildifier, buildozer) are
# available.
def checkTools():
  error_messages = []

  clang_format_abs_path = lookPath(CLANG_FORMAT_PATH)
  if clang_format_abs_path:
    if not executableByOthers(clang_format_abs_path):
      error_messages.append("command {} exists, but cannot be executed by other "
                            "users".format(CLANG_FORMAT_PATH))
  else:
    error_messages.append(
        "Command {} not found. If you have clang-format in version 8.x.x "
        "installed, but the binary name is different or it's not available in "
        "PATH, please use CLANG_FORMAT environment variable to specify the path. "
        "Examples:\n"
        "    export CLANG_FORMAT=clang-format-9.0.0\n"
        "    export CLANG_FORMAT=/opt/bin/clang-format-9\n"
        "    export CLANG_FORMAT=/usr/local/opt/llvm@9/bin/clang-format".format(CLANG_FORMAT_PATH))

  def checkBazelTool(name, path, var):
    bazel_tool_abs_path = lookPath(path)
    if bazel_tool_abs_path:
      if not executableByOthers(bazel_tool_abs_path):
        error_messages.append("command {} exists, but cannot be executed by other "
                              "users".format(path))
    elif pathExists(path):
      if not executableByOthers(path):
        error_messages.append("command {} exists, but cannot be executed by other "
                              "users".format(path))
    else:

      error_messages.append(
          "Command {} not found. If you have buildifier installed, but the binary "
          "name is different or it's not available in $GOPATH/bin, please use "
          "{} environment variable to specify the path. Example:\n"
          "    export {}=/opt/bin/buildifier\n"
          "If you don't have buildifier installed, you can install it by:\n"
          "    go get -u github.com/bazelbuild/buildtools/{}".format(path, var, var, name))

  checkBazelTool('buildifier', BUILDIFIER_PATH, 'BUILDIFIER_BIN')
  checkBazelTool('buildozer', BUILDOZER_PATH, 'BUILDOZER_BIN')

  return error_messages


def checkNamespace(file_path):
  for excluded_path in namespace_check_excluded_paths:
    if file_path.startswith(excluded_path):
      return []

  nolint = "NOLINT(namespace-%s)" % namespace_check.lower()
  text = readFile(file_path)
  if not re.search("^\s*namespace\s+%s\s*{" % namespace_check, text, re.MULTILINE) and \
     not nolint in text:
    return ["Unable to find %s namespace or %s for file: %s" % (namespace_check, nolint, file_path)]
  return []


def packageNameForProto(file_path):
  package_name = None
  error_message = []
  result = PROTO_PACKAGE_REGEX.search(readFile(file_path))
  if result is not None and len(result.groups()) == 1:
    package_name = result.group(1)
  if package_name is None:
    error_message = ["Unable to find package name for proto file: %s" % file_path]

  return [package_name, error_message]


# To avoid breaking the Lyft import, we just check for path inclusion here.
def whitelistedForProtobufDeps(file_path):
  return (file_path.endswith(PROTO_SUFFIX) or file_path.endswith(REPOSITORIES_BZL) or \
          any(path_segment in file_path for path_segment in GOOGLE_PROTOBUF_WHITELIST))


# Real-world time sources should not be instantiated in the source, except for a few
# specific cases. They should be passed down from where they are instantied to where
# they need to be used, e.g. through the ServerInstance, Dispatcher, or ClusterManager.
def whitelistedForRealTime(file_path):
  if file_path.endswith(".md"):
    return True
  return file_path in REAL_TIME_WHITELIST


def whitelistedForSerializeAsString(file_path):
  return file_path in SERIALIZE_AS_STRING_WHITELIST


def whitelistedForJsonStringToMessage(file_path):
  return file_path in JSON_STRING_TO_MESSAGE_WHITELIST


def whitelistedForHistogramSiSuffix(name):
  return name in HISTOGRAM_WITH_SI_SUFFIX_WHITELIST


def whitelistedForStdRegex(file_path):
  return file_path.startswith("./test") or file_path in STD_REGEX_WHITELIST or file_path.endswith(
      DOCS_SUFFIX)


def whitelistedForGrpcInit(file_path):
  return file_path in GRPC_INIT_WHITELIST


def whitelistedForUnpackTo(file_path):
  return file_path.startswith("./test") or file_path in [
      "./source/common/protobuf/utility.cc", "./source/common/protobuf/utility.h"
  ]


def findSubstringAndReturnError(pattern, file_path, error_message):
  text = readFile(file_path)
  if pattern in text:
    error_messages = [file_path + ": " + error_message]
    for i, line in enumerate(text.splitlines()):
      if pattern in line:
        error_messages.append("  %s:%s" % (file_path, i + 1))
    return error_messages
  return []


def errorIfNoSubstringFound(pattern, file_path, error_message):
  return [] if pattern in readFile(file_path) else [file_path + ": " + error_message]


def isApiFile(file_path):
  return file_path.startswith(args.api_prefix)


def isBuildFile(file_path):
  basename = os.path.basename(file_path)
  if basename in {"BUILD", "BUILD.bazel"} or basename.endswith(".BUILD"):
    return True
  return False


def isExternalBuildFile(file_path):
  return isBuildFile(file_path) and file_path.startswith("./bazel/external/")


def isSkylarkFile(file_path):
  return file_path.endswith(".bzl")


def isWorkspaceFile(file_path):
  return os.path.basename(file_path) == "WORKSPACE"


def isBuildFixerExcludedFile(file_path):
  for excluded_path in build_fixer_check_excluded_paths:
    if file_path.startswith(excluded_path):
      return True
  return False


def hasInvalidAngleBracketDirectory(line):
  if not line.startswith(INCLUDE_ANGLE):
    return False
  path = line[INCLUDE_ANGLE_LEN:]
  slash = path.find("/")
  if slash == -1:
    return False
  subdir = path[0:slash]
  return subdir in SUBDIR_SET


VERSION_HISTORY_NEW_LINE_REGEX = re.compile("\* [a-z \-_]*: [a-z:`]")
VERSION_HISTORY_NEW_RELEASE_REGEX = re.compile("^====[=]+$")


def checkCurrentReleaseNotes(file_path, error_messages):
  in_current_release = False

  for line_number, line in enumerate(readLines(file_path)):

    def reportError(message):
      error_messages.append("%s:%d: %s" % (file_path, line_number + 1, message))

    if VERSION_HISTORY_NEW_RELEASE_REGEX.match(line):
      # If we were in the section for the current release this means we have passed it.
      if in_current_release:
        break
      # If we see a version marker we are now in the section for the current release.
      in_current_release = True

    if line.startswith("*") and not VERSION_HISTORY_NEW_LINE_REGEX.match(line):
      reportError("Version history line malformed. "
                  "Does not match VERSION_HISTORY_NEW_LINE_REGEX in check_format.py\n %s" % line)


def checkFileContents(file_path, checker):
  error_messages = []

  if file_path.endswith("version_history.rst"):
    # Version file checking has enough special cased logic to merit its own checks.
    # This only validates entries for the current release as very old release
    # notes have a different format.
    checkCurrentReleaseNotes(file_path, error_messages)

  for line_number, line in enumerate(readLines(file_path)):

    def reportError(message):
      error_messages.append("%s:%d: %s" % (file_path, line_number + 1, message))

    checker(line, file_path, reportError)
  return error_messages


DOT_MULTI_SPACE_REGEX = re.compile("\\. +")


def fixSourceLine(line):
  # Strip double space after '.'  This may prove overenthusiastic and need to
  # be restricted to comments and metadata files but works for now.
  line = re.sub(DOT_MULTI_SPACE_REGEX, ". ", line)

  if hasInvalidAngleBracketDirectory(line):
    line = line.replace("<", '"').replace(">", '"')

  # Fix incorrect protobuf namespace references.
  for invalid_construct, valid_construct in PROTOBUF_TYPE_ERRORS.items():
    line = line.replace(invalid_construct, valid_construct)

  # Use recommended cpp stdlib
  for invalid_construct, valid_construct in LIBCXX_REPLACEMENTS.items():
    line = line.replace(invalid_construct, valid_construct)

  return line


# We want to look for a call to condvar.waitFor, but there's no strong pattern
# to the variable name of the condvar. If we just look for ".waitFor" we'll also
# pick up time_system_.waitFor(...), and we don't want to return true for that
# pattern. But in that case there is a strong pattern of using time_system in
# various spellings as the variable name.
def hasCondVarWaitFor(line):
  wait_for = line.find(".waitFor(")
  if wait_for == -1:
    return False
  preceding = line[0:wait_for]
  if preceding.endswith("time_system") or preceding.endswith("timeSystem()") or \
     preceding.endswith("time_system_"):
    return False
  return True


# Determines whether the filename is either in the specified subdirectory, or
# at the top level. We consider files in the top level for the benefit of
# the check_format testcases in tools/testdata/check_format.
def isInSubdir(filename, *subdirs):
  # Skip this check for check_format's unit-tests.
  if filename.count("/") <= 1:
    return True
  for subdir in subdirs:
    if filename.startswith('./' + subdir + '/'):
      return True
  return False


def checkSourceLine(line, file_path, reportError):
  # Check fixable errors. These may have been fixed already.
  if line.find(".  ") != -1:
    reportError("over-enthusiastic spaces")
  if isInSubdir(file_path, 'source', 'include') and X_ENVOY_USED_DIRECTLY_REGEX.match(line):
    reportError(
        "Please do not use the raw literal x-envoy in source code.  See Envoy::Http::PrefixValue.")
  if hasInvalidAngleBracketDirectory(line):
    reportError("envoy includes should not have angle brackets")
  for invalid_construct, valid_construct in PROTOBUF_TYPE_ERRORS.items():
    if invalid_construct in line:
      reportError("incorrect protobuf type reference %s; "
                  "should be %s" % (invalid_construct, valid_construct))
  for invalid_construct, valid_construct in LIBCXX_REPLACEMENTS.items():
    if invalid_construct in line:
      reportError("term %s should be replaced with standard library term %s" %
                  (invalid_construct, valid_construct))
  # Do not include the virtual_includes headers.
  if re.search("#include.*/_virtual_includes/", line):
    reportError("Don't include the virtual includes headers.")

  # Some errors cannot be fixed automatically, and actionable, consistent,
  # navigable messages should be emitted to make it easy to find and fix
  # the errors by hand.
  if not whitelistedForProtobufDeps(file_path):
    if '"google/protobuf' in line or "google::protobuf" in line:
      reportError("unexpected direct dependency on google.protobuf, use "
                  "the definitions in common/protobuf/protobuf.h instead.")
  if line.startswith("#include <mutex>") or line.startswith("#include <condition_variable"):
    # We don't check here for std::mutex because that may legitimately show up in
    # comments, for example this one.
    reportError("Don't use <mutex> or <condition_variable*>, switch to "
                "Thread::MutexBasicLockable in source/common/common/thread.h")
  if line.startswith("#include <shared_mutex>"):
    # We don't check here for std::shared_timed_mutex because that may
    # legitimately show up in comments, for example this one.
    reportError("Don't use <shared_mutex>, use absl::Mutex for reader/writer locks.")
  if not whitelistedForRealTime(file_path) and not "NO_CHECK_FORMAT(real_time)" in line:
    if "RealTimeSource" in line or \
       ("RealTimeSystem" in line and not "TestRealTimeSystem" in line) or \
       "std::chrono::system_clock::now" in line or "std::chrono::steady_clock::now" in line or \
       "std::this_thread::sleep_for" in line or hasCondVarWaitFor(line):
      reportError("Don't reference real-world time sources from production code; use injection")
  if not whitelistedForUnpackTo(file_path):
    if "UnpackTo" in line:
      reportError("Don't use UnpackTo() directly, use MessageUtil::unpackTo() instead")
  # Check that we use the absl::Time library
  if "std::get_time" in line:
    if "test/" in file_path:
      reportError("Don't use std::get_time; use TestUtility::parseTime in tests")
    else:
      reportError("Don't use std::get_time; use the injectable time system")
  if "std::put_time" in line:
    reportError("Don't use std::put_time; use absl::Time equivalent instead")
  if "gmtime" in line:
    reportError("Don't use gmtime; use absl::Time equivalent instead")
  if "mktime" in line:
    reportError("Don't use mktime; use absl::Time equivalent instead")
  if "localtime" in line:
    reportError("Don't use localtime; use absl::Time equivalent instead")
  if "strftime" in line:
    reportError("Don't use strftime; use absl::FormatTime instead")
  if "strptime" in line:
    reportError("Don't use strptime; use absl::FormatTime instead")
  if "std::atomic_" in line:
    # The std::atomic_* free functions are functionally equivalent to calling
    # operations on std::atomic<T> objects, so prefer to use that instead.
    reportError("Don't use free std::atomic_* functions, use std::atomic<T> members instead.")
  if "__attribute__((packed))" in line and file_path != "./include/envoy/common/platform.h":
    # __attribute__((packed)) is not supported by MSVC, we have a PACKED_STRUCT macro that
    # can be used instead
    reportError("Don't use __attribute__((packed)), use the PACKED_STRUCT macro defined "
                "in include/envoy/common/platform.h instead")
  if re.search("\{\s*\.\w+\s*\=", line):
    # Designated initializers are not part of the C++14 standard and are not supported
    # by MSVC
    reportError("Don't use designated initializers in struct initialization, "
                "they are not part of C++14")
  if " ?: " in line:
    # The ?: operator is non-standard, it is a GCC extension
    reportError("Don't use the '?:' operator, it is a non-standard GCC extension")
  if line.startswith("using testing::Test;"):
    reportError("Don't use 'using testing::Test;, elaborate the type instead")
  if line.startswith("using testing::TestWithParams;"):
    reportError("Don't use 'using testing::Test;, elaborate the type instead")
  if not whitelistedForSerializeAsString(file_path) and "SerializeAsString" in line:
    # The MessageLite::SerializeAsString doesn't generate deterministic serialization,
    # use MessageUtil::hash instead.
    reportError(
        "Don't use MessageLite::SerializeAsString for generating deterministic serialization, use MessageUtil::hash instead."
    )
  if not whitelistedForJsonStringToMessage(file_path) and "JsonStringToMessage" in line:
    # Centralize all usage of JSON parsing so it is easier to make changes in JSON parsing
    # behavior.
    reportError("Don't use Protobuf::util::JsonStringToMessage, use TestUtility::loadFromJson.")

  if isInSubdir(file_path, 'source') and file_path.endswith('.cc') and \
     ('.counter(' in line or '.gauge(' in line or '.histogram(' in line):
    reportError("Don't lookup stats by name at runtime; use StatName saved during construction")

  hist_m = re.search("(?<=HISTOGRAM\()[a-zA-Z0-9_]+_(b|kb|mb|ns|us|ms|s)(?=,)", line)
  if hist_m and not whitelistedForHistogramSiSuffix(hist_m.group(0)):
    reportError(
        "Don't suffix histogram names with the unit symbol, "
        "it's already part of the histogram object and unit-supporting sinks can use this information natively, "
        "other sinks can add the suffix automatically on flush should they prefer to do so.")

  if not whitelistedForStdRegex(file_path) and "std::regex" in line:
    reportError("Don't use std::regex in code that handles untrusted input. Use RegexMatcher")

  if not whitelistedForGrpcInit(file_path):
    grpc_init_or_shutdown = line.find("grpc_init()")
    grpc_shutdown = line.find("grpc_shutdown()")
    if grpc_init_or_shutdown == -1 or (grpc_shutdown != -1 and
                                       grpc_shutdown < grpc_init_or_shutdown):
      grpc_init_or_shutdown = grpc_shutdown
    if grpc_init_or_shutdown != -1:
      comment = line.find("// ")
      if comment == -1 or comment > grpc_init_or_shutdown:
        reportError("Don't call grpc_init() or grpc_shutdown() directly, instantiate " +
                    "Grpc::GoogleGrpcContext. See #8282")


def checkBuildLine(line, file_path, reportError):
  if "@bazel_tools" in line and not (isSkylarkFile(file_path) or file_path.startswith("./bazel/")):
    reportError("unexpected @bazel_tools reference, please indirect via a definition in //bazel")
  if not whitelistedForProtobufDeps(file_path) and '"protobuf"' in line:
    reportError("unexpected direct external dependency on protobuf, use "
                "//source/common/protobuf instead.")
  if (envoy_build_rule_check and not isSkylarkFile(file_path) and not isWorkspaceFile(file_path) and
      not isExternalBuildFile(file_path) and "@envoy//" in line):
    reportError("Superfluous '@envoy//' prefix")


def fixBuildLine(file_path, line):
  if (envoy_build_rule_check and not isSkylarkFile(file_path) and not isWorkspaceFile(file_path) and
      not isExternalBuildFile(file_path)):
    line = line.replace("@envoy//", "//")
  return line


def fixBuildPath(file_path):
  replaceLines(file_path, functools.partial(fixBuildLine, file_path))

  error_messages = []

  # TODO(htuch): Add API specific BUILD fixer script.
  if not isBuildFixerExcludedFile(file_path) and not isApiFile(file_path) and not isSkylarkFile(
      file_path) and not isWorkspaceFile(file_path):
    if os.system("%s %s %s" % (ENVOY_BUILD_FIXER_PATH, file_path, file_path)) != 0:
      error_messages += ["envoy_build_fixer rewrite failed for file: %s" % file_path]

  if os.system("%s -mode=fix %s" % (BUILDIFIER_PATH, file_path)) != 0:
    error_messages += ["buildifier rewrite failed for file: %s" % file_path]
  return error_messages


def checkBuildPath(file_path):
  error_messages = []

  if not isBuildFixerExcludedFile(file_path) and not isApiFile(file_path) and not isSkylarkFile(
      file_path) and not isWorkspaceFile(file_path):
    command = "%s %s | diff %s -" % (ENVOY_BUILD_FIXER_PATH, file_path, file_path)
    error_messages += executeCommand(command, "envoy_build_fixer check failed", file_path)

  if isBuildFile(file_path) and file_path.startswith(args.api_prefix + "envoy"):
    found = False
    for line in readLines(file_path):
      if "api_proto_package(" in line:
        found = True
        break
    if not found:
      error_messages += ["API build file does not provide api_proto_package()"]

  command = "%s -mode=diff %s" % (BUILDIFIER_PATH, file_path)
  error_messages += executeCommand(command, "buildifier check failed", file_path)
  error_messages += checkFileContents(file_path, checkBuildLine)
  return error_messages


def fixSourcePath(file_path):
  replaceLines(file_path, fixSourceLine)

  error_messages = []

  if not file_path.endswith(DOCS_SUFFIX):
    if not file_path.endswith(PROTO_SUFFIX):
      error_messages += fixHeaderOrder(file_path)
    error_messages += clangFormat(file_path)
  if file_path.endswith(PROTO_SUFFIX) and isApiFile(file_path):
    package_name, error_message = packageNameForProto(file_path)
    if package_name is None:
      error_messages += error_message
  return error_messages


def checkSourcePath(file_path):
  error_messages = checkFileContents(file_path, checkSourceLine)

  if not file_path.endswith(DOCS_SUFFIX):
    if not file_path.endswith(PROTO_SUFFIX):
      error_messages += checkNamespace(file_path)
      command = ("%s --include_dir_order %s --path %s | diff %s -" %
                 (HEADER_ORDER_PATH, include_dir_order, file_path, file_path))
      error_messages += executeCommand(command, "header_order.py check failed", file_path)
    command = ("%s %s | diff %s -" % (CLANG_FORMAT_PATH, file_path, file_path))
    error_messages += executeCommand(command, "clang-format check failed", file_path)

  if file_path.endswith(PROTO_SUFFIX) and isApiFile(file_path):
    package_name, error_message = packageNameForProto(file_path)
    if package_name is None:
      error_messages += error_message
  return error_messages


# Example target outputs are:
#   - "26,27c26"
#   - "12,13d13"
#   - "7a8,9"
def executeCommand(command,
                   error_message,
                   file_path,
                   regex=re.compile(r"^(\d+)[a|c|d]?\d*(?:,\d+[a|c|d]?\d*)?$")):
  try:
    output = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT).strip()
    if output:
      return output.decode('utf-8').split("\n")
    return []
  except subprocess.CalledProcessError as e:
    if (e.returncode != 0 and e.returncode != 1):
      return ["ERROR: something went wrong while executing: %s" % e.cmd]
    # In case we can't find any line numbers, record an error message first.
    error_messages = ["%s for file: %s" % (error_message, file_path)]
    for line in e.output.decode('utf-8').splitlines():
      for num in regex.findall(line):
        error_messages.append("  %s:%s" % (file_path, num))
    return error_messages


def fixHeaderOrder(file_path):
  command = "%s --rewrite --include_dir_order %s --path %s" % (HEADER_ORDER_PATH, include_dir_order,
                                                               file_path)
  if os.system(command) != 0:
    return ["header_order.py rewrite error: %s" % (file_path)]
  return []


def clangFormat(file_path):
  command = "%s -i %s" % (CLANG_FORMAT_PATH, file_path)
  if os.system(command) != 0:
    return ["clang-format rewrite error: %s" % (file_path)]
  return []


def checkFormat(file_path):
  if file_path.startswith(EXCLUDED_PREFIXES):
    return []

  if not file_path.endswith(SUFFIXES):
    return []

  error_messages = []
  # Apply fixes first, if asked, and then run checks. If we wind up attempting to fix
  # an issue, but there's still an error, that's a problem.
  try_to_fix = operation_type == "fix"
  if isBuildFile(file_path) or isSkylarkFile(file_path) or isWorkspaceFile(file_path):
    if try_to_fix:
      error_messages += fixBuildPath(file_path)
    error_messages += checkBuildPath(file_path)
  else:
    if try_to_fix:
      error_messages += fixSourcePath(file_path)
    error_messages += checkSourcePath(file_path)

  if error_messages:
    return ["From %s" % file_path] + error_messages
  return error_messages


def checkFormatReturnTraceOnError(file_path):
  """Run checkFormat and return the traceback of any exception."""
  try:
    return checkFormat(file_path)
  except:
    return traceback.format_exc().split("\n")


def checkOwners(dir_name, owned_directories, error_messages):
  """Checks to make sure a given directory is present either in CODEOWNERS or OWNED_EXTENSIONS

  Args:
    dir_name: the directory being checked.
    owned_directories: directories currently listed in CODEOWNERS.
    error_messages: where to put an error message for new unowned directories.
  """
  found = False
  for owned in owned_directories:
    if owned.startswith(dir_name) or dir_name.startswith(owned):
      found = True
  if not found and dir_name not in UNOWNED_EXTENSIONS:
    error_messages.append("New directory %s appears to not have owners in CODEOWNERS" % dir_name)


def checkFormatVisitor(arg, dir_name, names):
  """Run checkFormat in parallel for the given files.

  Args:
    arg: a tuple (pool, result_list, owned_directories, error_messages)
      pool and result_list are for starting tasks asynchronously.
      owned_directories tracks directories listed in the CODEOWNERS file.
      error_messages is a list of string format errors.
    dir_name: the parent directory of the given files.
    names: a list of file names.
  """

  # Unpack the multiprocessing.Pool process pool and list of results. Since
  # python lists are passed as references, this is used to collect the list of
  # async results (futures) from running checkFormat and passing them back to
  # the caller.
  pool, result_list, owned_directories, error_messags = arg

  # Sanity check CODEOWNERS.  This doesn't need to be done in a multi-threaded
  # manner as it is a small and limited list.
  source_prefix = './source/'
  full_prefix = './source/extensions/'
  # Check to see if this directory is a subdir under /source/extensions
  # Also ignore top level directories under /source/extensions since we don't
  # need owners for source/extensions/access_loggers etc, just the subdirectories.
  if dir_name.startswith(full_prefix) and '/' in dir_name[len(full_prefix):]:
    checkOwners(dir_name[len(source_prefix):], owned_directories, error_messages)

  for file_name in names:
    result = pool.apply_async(checkFormatReturnTraceOnError, args=(dir_name + "/" + file_name,))
    result_list.append(result)


# checkErrorMessages iterates over the list with error messages and prints
# errors and returns a bool based on whether there were any errors.
def checkErrorMessages(error_messages):
  if error_messages:
    for e in error_messages:
      print("ERROR: %s" % e)
    return True
  return False


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Check or fix file format.")
  parser.add_argument("operation_type",
                      type=str,
                      choices=["check", "fix"],
                      help="specify if the run should 'check' or 'fix' format.")
  parser.add_argument(
      "target_path",
      type=str,
      nargs="?",
      default=".",
      help="specify the root directory for the script to recurse over. Default '.'.")
  parser.add_argument("--add-excluded-prefixes",
                      type=str,
                      nargs="+",
                      help="exclude additional prefixes.")
  parser.add_argument("-j",
                      "--num-workers",
                      type=int,
                      default=multiprocessing.cpu_count(),
                      help="number of worker processes to use; defaults to one per core.")
  parser.add_argument("--api-prefix", type=str, default="./api/", help="path of the API tree.")
  parser.add_argument("--skip_envoy_build_rule_check",
                      action="store_true",
                      help="skip checking for '@envoy//' prefix in build rules.")
  parser.add_argument("--namespace_check",
                      type=str,
                      nargs="?",
                      default="Envoy",
                      help="specify namespace check string. Default 'Envoy'.")
  parser.add_argument("--namespace_check_excluded_paths",
                      type=str,
                      nargs="+",
                      default=[],
                      help="exclude paths from the namespace_check.")
  parser.add_argument("--build_fixer_check_excluded_paths",
                      type=str,
                      nargs="+",
                      default=[],
                      help="exclude paths from envoy_build_fixer check.")
  parser.add_argument("--include_dir_order",
                      type=str,
                      default=",".join(common.includeDirOrder()),
                      help="specify the header block include directory order.")
  args = parser.parse_args()

  operation_type = args.operation_type
  target_path = args.target_path
  envoy_build_rule_check = not args.skip_envoy_build_rule_check
  namespace_check = args.namespace_check
  namespace_check_excluded_paths = args.namespace_check_excluded_paths
  build_fixer_check_excluded_paths = args.build_fixer_check_excluded_paths + [
      "./bazel/external/",
      "./bazel/toolchains/",
      "./bazel/BUILD",
      "./tools/clang_tools",
  ]
  include_dir_order = args.include_dir_order
  if args.add_excluded_prefixes:
    EXCLUDED_PREFIXES += tuple(args.add_excluded_prefixes)

  # Check whether all needed external tools are available.
  ct_error_messages = checkTools()
  if checkErrorMessages(ct_error_messages):
    sys.exit(1)

  # Returns the list of directories with owners listed in CODEOWNERS. May append errors to
  # error_messages.
  def ownedDirectories(error_messages):
    owned = []
    maintainers = [
        '@mattklein123', '@htuch', '@alyssawilk', '@zuercher', '@lizan', '@snowp', '@junr03',
        '@dnoe', '@dio', '@jmarantz'
    ]

    try:
      with open('./CODEOWNERS') as f:
        for line in f:
          # If this line is of the form "extensions/... @owner1 @owner2" capture the directory
          # name and store it in the list of directories with documented owners.
          m = re.search(r'.*(extensions[^@]*\s+)(@.*)', line)
          if m is not None and not line.startswith('#'):
            owned.append(m.group(1).strip())
            owners = re.findall('@\S+', m.group(2).strip())
            if len(owners) < 2:
              error_messages.append("Extensions require at least 2 owners in CODEOWNERS:\n"
                                    "    {}".format(line))
            maintainer = len(set(owners).intersection(set(maintainers))) > 0
            if not maintainer:
              error_messages.append("Extensions require at least one maintainer OWNER:\n"
                                    "    {}".format(line))

      return owned
    except IOError:
      return []  # for the check format tests.

  # Calculate the list of owned directories once per run.
  error_messages = []
  owned_directories = ownedDirectories(error_messages)

  if os.path.isfile(target_path):
    error_messages += checkFormat("./" + target_path)
  else:
    pool = multiprocessing.Pool(processes=args.num_workers)
    results = []
    # For each file in target_path, start a new task in the pool and collect the
    # results (results is passed by reference, and is used as an output).
    for root, _, files in os.walk(target_path):
      checkFormatVisitor((pool, results, owned_directories, error_messages), root, files)

    # Close the pool to new tasks, wait for all of the running tasks to finish,
    # then collect the error messages.
    pool.close()
    pool.join()
    error_messages += sum((r.get() for r in results), [])

  if checkErrorMessages(error_messages):
    print("ERROR: check format failed. run 'tools/check_format.py fix'")
    sys.exit(1)

  if operation_type == "check":
    print("PASS")
