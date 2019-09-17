#include "extensions/access_loggers/grpc/tcp_grpc_access_log_impl.h"

#include "common/common/assert.h"
#include "common/network/utility.h"
#include "common/stream_info/utility.h"

#include "extensions/access_loggers/grpc/grpc_access_log_utils.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace TcpGrpc {

TcpGrpcAccessLog::ThreadLocalLogger::ThreadLocalLogger(GrpcCommon::GrpcAccessLoggerSharedPtr logger)
    : logger_(std::move(logger)) {}

TcpGrpcAccessLog::TcpGrpcAccessLog(AccessLog::FilterPtr&& filter,
                                   envoy::config::accesslog::v2::TcpGrpcAccessLogConfig config,
                                   ThreadLocal::SlotAllocator& tls,
                                   GrpcCommon::GrpcAccessLoggerCacheSharedPtr access_logger_cache)
    : Common::ImplBase(std::move(filter)), config_(std::move(config)),
      tls_slot_(tls.allocateSlot()), access_logger_cache_(std::move(access_logger_cache)) {
  tls_slot_->set([this](Event::Dispatcher&) {
    return std::make_shared<ThreadLocalLogger>(access_logger_cache_->getOrCreateLogger(
        config_.common_config(), GrpcCommon::GrpcAccessLoggerType::TCP));
  });
}

void TcpGrpcAccessLog::emitLog(const Http::HeaderMap&, const Http::HeaderMap&,
                               const Http::HeaderMap&, const StreamInfo::StreamInfo& stream_info) {
  // Common log properties.
  envoy::data::accesslog::v2::TCPAccessLogEntry log_entry;
  GrpcCommon::Utility::extractCommonAccessLogProperties(*log_entry.mutable_common_properties(),
                                                        stream_info);

  envoy::data::accesslog::v2::ConnectionProperties& connection_properties =
      *log_entry.mutable_connection_properties();
  connection_properties.set_received_bytes(stream_info.bytesReceived());
  connection_properties.set_sent_bytes(stream_info.bytesSent());

  // request_properties->set_request_body_bytes(stream_info.bytesReceived());
  tls_slot_->getTyped<ThreadLocalLogger>().logger_->log(std::move(log_entry));
}

} // namespace TcpGrpc
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
