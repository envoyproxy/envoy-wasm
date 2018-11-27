/*
 * Intrinsic functions available to WASM modules.
 */

#include <string>

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE __attribute__((used)) __attribute__((visibility ("default")))
#endif

enum class LogLevel : int { trace, debug, info, warn, error, critical };
extern "C" void envoy_log(LogLevel level, const char* logMessage, size_t messageSize);

inline void logTrace(const std::string& logMessage) {
  envoy_log(LogLevel::trace, logMessage.c_str(), logMessage.size());
}
inline void logDebug(const std::string& logMessage) {
  envoy_log(LogLevel::debug, logMessage.c_str(), logMessage.size());
}
inline void logInfo(const std::string& logMessage) {
  envoy_log(LogLevel::info, logMessage.c_str(), logMessage.size());
}
inline void logWarn(const std::string& logMessage) {
  envoy_log(LogLevel::warn, logMessage.c_str(), logMessage.size());
}
inline void logErr(const std::string& logMessage) {
  envoy_log(LogLevel::error, logMessage.c_str(), logMessage.size());
}
inline void logCritical(const std::string& logMessage) {
  envoy_log(LogLevel::critical, logMessage.c_str(), logMessage.size());
}
