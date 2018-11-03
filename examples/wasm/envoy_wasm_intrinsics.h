/*
 * Intrinsic functions available to WASM modules.
 */

#include <string>

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE __attribute__((used)) __attribute__((visibility ("default")))
#endif

enum class LogLevel : int { trace, debug, info, warn, error, critical };
extern "C" void wasmLog(LogLevel level, const char* logMessage, size_t messageSize);

inline void logTrace(const std::string& logMessage) {
  wasmLog(LogLevel::trace, logMessage.c_str(), logMessage.size());
}
inline void logDebug(const std::string& logMessage) {
  wasmLog(LogLevel::debug, logMessage.c_str(), logMessage.size());
}
inline void logInfo(const std::string& logMessage) {
  wasmLog(LogLevel::info, logMessage.c_str(), logMessage.size());
}
inline void logWarn(const std::string& logMessage) {
  wasmLog(LogLevel::warn, logMessage.c_str(), logMessage.size());
}
inline void logErr(const std::string& logMessage) {
  wasmLog(LogLevel::error, logMessage.c_str(), logMessage.size());
}
inline void logCritical(const std::string& logMessage) {
  wasmLog(LogLevel::critical, logMessage.c_str(), logMessage.size());
}
