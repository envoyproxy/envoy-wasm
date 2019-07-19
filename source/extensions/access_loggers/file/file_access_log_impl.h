#pragma once

#include "extensions/access_loggers/common/access_log_base.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace File {

/**
 * Access log Instance that writes logs to a file.
 */
class FileAccessLog : public Common::ImplBase {
public:
  FileAccessLog(const std::string& access_log_path, AccessLog::FilterPtr&& filter,
                AccessLog::FormatterPtr&& formatter, AccessLog::AccessLogManager& log_manager);

private:
  // Common::ImplBase
  void emitLog(const Http::HeaderMap& request_headers, const Http::HeaderMap& response_headers,
               const Http::HeaderMap& response_trailers,
               const StreamInfo::StreamInfo& stream_info) override;

  AccessLog::AccessLogFileSharedPtr log_file_;
  AccessLog::FormatterPtr formatter_;
};

} // namespace File
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
