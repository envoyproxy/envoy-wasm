#pragma once

#include "envoy/config/accesslog/v2/als.pb.h"
#include "envoy/data/accesslog/v2/accesslog.pb.h"
#include "envoy/stream_info/stream_info.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace GrpcCommon {

class Utility {
public:
  static void extractCommonAccessLogProperties(
      envoy::data::accesslog::v2::AccessLogCommon& common_access_log,
      const StreamInfo::StreamInfo& stream_info,
      const envoy::config::accesslog::v2::CommonGrpcAccessLogConfig& filter_states_to_log);

  static void responseFlagsToAccessLogResponseFlags(
      envoy::data::accesslog::v2::AccessLogCommon& common_access_log,
      const StreamInfo::StreamInfo& stream_info);
};

} // namespace GrpcCommon
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy