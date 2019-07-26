#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/common/logger.h"

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "spdlog/spdlog.h"

namespace Envoy {

/**
 * Provides a mechanism to temporarily set the logging level on
 * construction, restoring its previous state on destruction.
 *
 * The log_level is the minimum log severity required to print messages.
 * Messages below this loglevel will be suppressed.
 *
 * Note that during the scope of this object, command-line overrides, eg
 * --log-level trace, will not take effect.
 *
 * Also note: instantiating this setter should only occur when the system is
 * in a quiescent state, e.g. at startup or between tests.
 *
 * This is intended for use in EXPECT_LOG_CONTAINS and similar macros.
 */
class LogLevelSetter {
public:
  explicit LogLevelSetter(spdlog::level::level_enum log_level);
  ~LogLevelSetter();

private:
  std::vector<spdlog::level::level_enum> previous_levels_;
};

/**
 * Records log messages in a vector<string>, forwarding them to the previous
 * delegate. This is useful for unit-testing log messages while still being able
 * to see them on stderr.
 *
 * Also note: instantiating this sink should only occur when the system is
 * in a quiescent state, e.g. at startup or between tests
 *
 * This is intended for use in EXPECT_LOG_CONTAINS and similar macros.
 */
class LogRecordingSink : public Logger::SinkDelegate {
public:
  explicit LogRecordingSink(Logger::DelegatingLogSinkPtr log_sink);
  ~LogRecordingSink() override;

  // Logger::SinkDelegate
  void log(absl::string_view msg) override;
  void flush() override;

  const std::vector<std::string>& messages() const { return messages_; }

private:
  std::vector<std::string> messages_;
};

using StringPair = std::pair<std::string, std::string>;

using ExpectedLogMessages = std::vector<StringPair>;

// Below macros specify Envoy:: before class names so that the macro can be used outside of
// namespace Envoy.

// Validates that when stmt is executed, log messages containing substr and loglevel will be
// emitted. Failure message e.g.,
//
// Logs:
//  [2018-04-12 05:51:00.245][7290192][debug][upstream] grpc_mux_impl.cc:160] Received gRPC
//  [2018-04-12 05:51:00.246][7290192][warning][upstream] grpc_mux_impl.cc:63] Called bar
//  [2018-04-12 05:51:00.246][7290192][trace][upstream] grpc_mux_impl.cc:80] Sending foo
//  Does NOT contain:
//    'warning', 'Too many sendDiscoveryRequest calls for baz’
//    'warning', 'Too man sendDiscoveryRequest calls for foo'
#define EXPECT_LOG_CONTAINS_ALL_OF(expected_messages, stmt)                                        \
  do {                                                                                             \
    ASSERT_FALSE(expected_messages.empty()) << "Expected messages cannot be empty.";               \
    Envoy::LogLevelSetter save_levels(spdlog::level::trace);                                       \
    Envoy::LogRecordingSink log_recorder(Envoy::Logger::Registry::getSink());                      \
    stmt;                                                                                          \
    if (log_recorder.messages().empty()) {                                                         \
      FAIL() << "Expected message(s), but NONE was recorded.";                                     \
    }                                                                                              \
    Envoy::ExpectedLogMessages failed_expectations;                                                \
    for (const Envoy::StringPair& expected : expected_messages) {                                  \
      const auto log_message =                                                                     \
          std::find_if(log_recorder.messages().begin(), log_recorder.messages().end(),             \
                       [&expected](const std::string& message) {                                   \
                         return (message.find(expected.second) != std::string::npos) &&            \
                                (message.find(expected.first) != std::string::npos);               \
                       });                                                                         \
      if (log_message == log_recorder.messages().end()) {                                          \
        failed_expectations.push_back(expected);                                                   \
      }                                                                                            \
    }                                                                                              \
    if (!failed_expectations.empty()) {                                                            \
      std::string failed_message;                                                                  \
      absl::StrAppend(&failed_message, "\nLogs:\n ", absl::StrJoin(log_recorder.messages(), " "),  \
                      "\n Do NOT contain:\n");                                                     \
      for (const auto& expectation : failed_expectations) {                                        \
        absl::StrAppend(&failed_message, "  '", expectation.first, "', '", expectation.second,     \
                        "'\n");                                                                    \
      }                                                                                            \
      FAIL() << failed_message;                                                                    \
    }                                                                                              \
  } while (false)

// Validates that when stmt is executed, log message containing substr and loglevel will NOT be
// emitted. Failure message e.g.,
//
// Logs:
//  [2018-04-12 05:51:00.245][7290192][warning][upstream] grpc_mux_impl.cc:160] Received gRPC
//  [2018-04-12 05:51:00.246][7290192][trace][upstream] grpc_mux_impl.cc:63] Called bar
//  Should NOT contain:
//   'warning', 'Received gRPC’
#define EXPECT_LOG_NOT_CONTAINS(loglevel, substr, stmt)                                            \
  do {                                                                                             \
    Envoy::LogLevelSetter save_levels(spdlog::level::trace);                                       \
    Envoy::LogRecordingSink log_recorder(Envoy::Logger::Registry::getSink());                      \
    stmt;                                                                                          \
    for (const std::string& message : log_recorder.messages()) {                                   \
      if ((message.find(substr) != std::string::npos) &&                                           \
          (message.find(loglevel) != std::string::npos)) {                                         \
        FAIL() << "\nLogs:\n " << absl::StrJoin(log_recorder.messages(), " ")                      \
               << "\n Should NOT contain:\n '" << loglevel << "', '" << substr "'\n";              \
      }                                                                                            \
    }                                                                                              \
  } while (false)

// Validates that when stmt is executed, the supplied substring matches at least one log message.
// Failure message e.g.,
//
// Logs:
//  [2018-04-12 05:51:00.245][7290192][debug][upstream] grpc_mux_impl.cc:160] Received gRPC
//  [2018-04-12 05:51:00.246][7290192][trace][upstream] grpc_mux_impl.cc:80] Sending foo
//  Do NOT contain:
//    'warning', 'Too many sendDiscoveryRequest calls for baz’
#define EXPECT_LOG_CONTAINS(loglevel, substr, stmt)                                                \
  do {                                                                                             \
    const Envoy::ExpectedLogMessages message{{loglevel, substr}};                                  \
    EXPECT_LOG_CONTAINS_ALL_OF(message, stmt);                                                     \
  } while (false)

// Validates that when stmt is executed, no logs will be emitted.
// Expected equality of these values:
//   0
//   logs.size()
//     Which is: 3
//  Logs:
//   [2018-04-12 05:51:00.245][7290192][debug][upstream] grpc_mux_impl.cc:160] Received gRPC
//   [2018-04-12 05:51:00.246][7290192][trace][upstream] grpc_mux_impl.cc:80] Sending foo
#define EXPECT_NO_LOGS(stmt)                                                                       \
  do {                                                                                             \
    Envoy::LogLevelSetter save_levels(spdlog::level::trace);                                       \
    Envoy::LogRecordingSink log_recorder(Envoy::Logger::Registry::getSink());                      \
    stmt;                                                                                          \
    const std::vector<std::string>& logs = log_recorder.messages();                                \
    ASSERT_EQ(0, logs.size()) << " Logs:\n   " << absl::StrJoin(logs, "   ");                      \
  } while (false)

} // namespace Envoy
