#pragma once

#include "common/common/logger.h"
#include "envoy/access_log/access_log.h"

#include "extensions/access_loggers/well_known_names.h"
#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace AccessLoggers {
namespace Wasm {

class WasmAccessLog : public AccessLog::Instance {
  public:
    WasmAccessLog(ThreadLocal::SlotPtr tls_slot,  AccessLog::FilterPtr filter) : tls_slot_(std::move(tls_slot)), filter_(std::move(filter)) {}
    void log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
        const Http::HeaderMap* response_trailers, const StreamInfo::StreamInfo& stream_info) override {
      if (filter_ && request_headers) {
        if (!filter_->evaluate(stream_info, *request_headers)) {
          return;
        }
      }
      tls_slot_->getTyped<Common::Wasm::Wasm>().log(request_headers, response_headers, response_trailers, stream_info);
    }
  private:
    ThreadLocal::SlotPtr tls_slot_;
    AccessLog::FilterPtr filter_;
};

} // namespace Wasm
} // namespace AccessLoggers
} // namespace Extensions
} // namespace Envoy
