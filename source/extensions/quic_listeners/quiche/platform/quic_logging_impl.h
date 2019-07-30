#pragma once

// NOLINT(namespace-envoy)

// This file is part of the QUICHE platform implementation, and is not to be
// consumed or referenced directly by other Envoy code. It serves purely as a
// porting layer for QUICHE.

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "common/common/assert.h"
#include "common/common/logger.h"
#include "common/common/stl_helpers.h"

#include "absl/base/optimization.h"
#include "absl/synchronization/mutex.h"

// This implementation is only used by Quiche code, use macros provided by
// assert.h and logger.h in Envoy code instead. See QUIC platform API
// dependency model described in
// https://quiche.googlesource.com/quiche/+/refs/heads/master/quic/platform/api/README.md
//
// The implementation is backed by Envoy::Logger.

// If |condition| is true, use |logstream| to stream the log message and send it to spdlog.
// If |condition| is false, |logstream| will not be instantiated.
// The switch(0) is used to suppress a compiler warning on ambiguous "else".
#define QUIC_LOG_IMPL_INTERNAL(condition, logstream)                                               \
  switch (0)                                                                                       \
  default:                                                                                         \
    if (!(condition)) {                                                                            \
    } else                                                                                         \
      logstream

#define QUIC_LOG_IF_IMPL(severity, condition)                                                      \
  QUIC_LOG_IMPL_INTERNAL((condition) && quic::IsLogLevelEnabled(quic::severity),                   \
                         quic::QuicLogEmitter(quic::severity).stream())

#define QUIC_LOG_IMPL(severity) QUIC_LOG_IF_IMPL(severity, true)

#define QUIC_VLOG_IF_IMPL(verbosity, condition)                                                    \
  QUIC_LOG_IMPL_INTERNAL((condition) && quic::IsVerboseLogEnabled(verbosity),                      \
                         quic::QuicLogEmitter(quic::INFO).stream())

#define QUIC_VLOG_IMPL(verbosity) QUIC_VLOG_IF_IMPL(verbosity, true)

// TODO(wub): Implement QUIC_LOG_FIRST_N_IMPL.
#define QUIC_LOG_FIRST_N_IMPL(severity, n) QUIC_LOG_IMPL(severity)

// TODO(wub): Implement QUIC_LOG_EVERY_N_IMPL.
#define QUIC_LOG_EVERY_N_IMPL(severity, n) QUIC_LOG_IMPL(severity)

// TODO(wub): Implement QUIC_LOG_EVERY_N_SEC_IMPL.
#define QUIC_LOG_EVERY_N_SEC_IMPL(severity, seconds) QUIC_LOG_IMPL(severity)

#define QUIC_PLOG_IMPL(severity)                                                                   \
  QUIC_LOG_IMPL_INTERNAL(quic::IsLogLevelEnabled(quic::severity),                                  \
                         quic::QuicLogEmitter(quic::severity).SetPerror().stream())

#define QUIC_LOG_INFO_IS_ON_IMPL() quic::IsLogLevelEnabled(quic::INFO)
#define QUIC_LOG_WARNING_IS_ON_IMPL() quic::IsLogLevelEnabled(quic::WARNING)
#define QUIC_LOG_ERROR_IS_ON_IMPL() quic::IsLogLevelEnabled(quic::ERROR)

#define CHECK(condition)                                                                           \
  QUIC_LOG_IF_IMPL(FATAL, ABSL_PREDICT_FALSE(!(condition))) << "CHECK failed: " #condition "."

#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_EQ(a, b) CHECK((a) == (b))

#ifdef NDEBUG
// Release build
#define DCHECK(condition) QUIC_COMPILED_OUT_LOG(condition)
#define QUIC_COMPILED_OUT_LOG(condition)                                                           \
  QUIC_LOG_IMPL_INTERNAL(false && (condition), quic::NullLogStream().stream())
#define QUIC_DVLOG_IMPL(verbosity) QUIC_COMPILED_OUT_LOG(false)
#define QUIC_DVLOG_IF_IMPL(verbosity, condition) QUIC_COMPILED_OUT_LOG(condition)
#define QUIC_DLOG_IMPL(severity) QUIC_COMPILED_OUT_LOG(false)
#define QUIC_DLOG_IF_IMPL(severity, condition) QUIC_COMPILED_OUT_LOG(condition)
#define QUIC_DLOG_INFO_IS_ON_IMPL() 0
#define QUIC_DLOG_EVERY_N_IMPL(severity, n) QUIC_COMPILED_OUT_LOG(false)
#define QUIC_NOTREACHED_IMPL()
#else
// Debug build
#define DCHECK(condition) CHECK(condition)
#define QUIC_DVLOG_IMPL(verbosity) QUIC_VLOG_IMPL(verbosity)
#define QUIC_DVLOG_IF_IMPL(verbosity, condition) QUIC_VLOG_IF_IMPL(verbosity, condition)
#define QUIC_DLOG_IMPL(severity) QUIC_LOG_IMPL(severity)
#define QUIC_DLOG_IF_IMPL(severity, condition) QUIC_LOG_IF_IMPL(severity, condition)
#define QUIC_DLOG_INFO_IS_ON_IMPL() QUIC_LOG_INFO_IS_ON_IMPL()
#define QUIC_DLOG_EVERY_N_IMPL(severity, n) QUIC_LOG_EVERY_N_IMPL(severity, n)
#define QUIC_NOTREACHED_IMPL() NOT_REACHED_GCOVR_EXCL_LINE
#endif

#define DCHECK_GE(a, b) DCHECK((a) >= (b))
#define DCHECK_GT(a, b) DCHECK((a) > (b))
#define DCHECK_LT(a, b) DCHECK((a) < (b))
#define DCHECK_LE(a, b) DCHECK((a) <= (b))
#define DCHECK_NE(a, b) DCHECK((a) != (b))
#define DCHECK_EQ(a, b) DCHECK((a) == (b))

#define QUIC_PREDICT_FALSE_IMPL(x) ABSL_PREDICT_FALSE(x)

namespace quic {

using QuicLogLevel = spdlog::level::level_enum;

static const QuicLogLevel INFO = spdlog::level::info;
static const QuicLogLevel WARNING = spdlog::level::warn;
static const QuicLogLevel ERROR = spdlog::level::err;
static const QuicLogLevel FATAL = spdlog::level::critical;

// DFATAL is FATAL in debug mode, ERROR in release mode.
#ifdef NDEBUG
static const QuicLogLevel DFATAL = ERROR;
#else
static const QuicLogLevel DFATAL = FATAL;
#endif

class QuicLogEmitter {
public:
  explicit QuicLogEmitter(QuicLogLevel level);

  ~QuicLogEmitter();

  QuicLogEmitter& SetPerror() {
    is_perror_ = true;
    return *this;
  }

  std::ostringstream& stream() { return stream_; }

private:
  const QuicLogLevel level_;
  const int saved_errno_;
  bool is_perror_ = false;
  std::ostringstream stream_;
};

class NullLogStream : public std::ostream {
public:
  NullLogStream& stream() { return *this; }
};

template <typename T> inline NullLogStream& operator<<(NullLogStream& s, const T&) { return s; }

inline spdlog::logger& GetLogger() {
  return Envoy::Logger::Registry::getLog(Envoy::Logger::Id::quic);
}

inline bool IsLogLevelEnabled(QuicLogLevel level) { return level >= GetLogger().level(); }

int GetVerbosityLogThreshold();
void SetVerbosityLogThreshold(int new_verbosity);

inline bool IsVerboseLogEnabled(int verbosity) {
  return IsLogLevelEnabled(INFO) && verbosity <= GetVerbosityLogThreshold();
}

bool IsDFatalExitDisabled();
void SetDFatalExitDisabled(bool is_disabled);

// QuicLogSink is used to capture logs emitted from the QUIC_LOG... macros.
class QuicLogSink {
public:
  virtual ~QuicLogSink() = default;

  // Called when |message| is emitted at |level|.
  virtual void Log(QuicLogLevel level, const std::string& message) = 0;
};

// Only one QuicLogSink can capture log at a time. SetLogSink causes future logs
// to be captured by the |new_sink|.
// Return the previous sink.
QuicLogSink* SetLogSink(QuicLogSink* new_sink);

} // namespace quic
