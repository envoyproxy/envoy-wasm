#pragma once

#include <memory>
#include <string>
#include <vector>

#include "envoy/access_log/access_log.h"
#include "envoy/common/pure.h"
#include "envoy/http/header_map.h"

namespace Envoy {
namespace Tracing {

constexpr uint32_t DefaultMaxPathTagLength = 256;

enum class OperationName { Ingress, Egress };

/**
 * The reasons why trace sampling may or may not be performed.
 */
enum class Reason {
  // Not sampled based on supplied request id.
  NotTraceableRequestId,
  // Not sampled due to being a health check.
  HealthCheck,
  // Sampling enabled.
  Sampling,
  // Sampling forced by the service.
  ServiceForced,
  // Sampling forced by the client.
  ClientForced,
};

/**
 * The decision regarding whether traces should be sampled, and the reason for it.
 */
struct Decision {
  Reason reason;
  bool traced;
};

/**
 * Tracing configuration, it carries additional data needed to populate the span.
 */
class Config {
public:
  virtual ~Config() = default;

  /**
   * @return operation name for tracing, e.g., ingress.
   */
  virtual OperationName operationName() const PURE;

  /**
   * @return list of headers to populate tags on the active span.
   */
  virtual const std::vector<Http::LowerCaseString>& requestHeadersForTags() const PURE;

  /**
   * @return true if spans should be annotated with more detailed information.
   */
  virtual bool verbose() const PURE;

  /**
   * @return the maximum length allowed for paths in the extracted HttpUrl tag.
   */
  virtual uint32_t maxPathTagLength() const PURE;
};

class Span;
using SpanPtr = std::unique_ptr<Span>;

/**
 * Basic abstraction for span.
 */
class Span {
public:
  virtual ~Span() = default;

  /**
   * Set the operation name.
   * @param operation the operation name
   */
  virtual void setOperation(absl::string_view operation) PURE;

  /**
   * Attach metadata to a Span, to be handled in an implementation-dependent fashion.
   * @param name the name of the tag
   * @param value the value to associate with the tag
   */
  virtual void setTag(absl::string_view name, absl::string_view value) PURE;

  /**
   * Record an event associated with a span, to be handled in an implementation-dependent fashion.
   * @param timestamp the time of the event.
   * @param event the name of the event.
   */
  virtual void log(SystemTime timestamp, const std::string& event) PURE;

  /**
   * Capture the final duration for this Span and carry out any work necessary to complete it.
   * Once this method is called, the Span may be safely discarded.
   */
  virtual void finishSpan() PURE;

  /**
   * Mutate the provided headers with the context necessary to propagate this
   * (implementation-specific) trace.
   * @param request_headers the headers to which propagation context will be added
   */
  virtual void injectContext(Http::HeaderMap& request_headers) PURE;

  /**
   * Create and start a child Span, with this Span as its parent in the trace.
   * @param config the tracing configuration
   * @param name operation name captured by the spawned child
   * @param start_time initial start time for the operation captured by the child
   */
  virtual SpanPtr spawnChild(const Config& config, const std::string& name,
                             SystemTime start_time) PURE;

  /**
   * This method overrides any previous sampling decision associated with the trace instance.
   * If the sampled parameter is false, this span and any subsequent child spans
   * are not reported to the tracing system.
   * @param sampled whether the span and any subsequent child spans should be sampled
   */
  virtual void setSampled(bool sampled) PURE;
};

/**
 * Tracing driver is responsible for span creation.
 */
class Driver {
public:
  virtual ~Driver() = default;

  /**
   * Start driver specific span.
   */
  virtual SpanPtr startSpan(const Config& config, Http::HeaderMap& request_headers,
                            const std::string& operation_name, SystemTime start_time,
                            const Tracing::Decision tracing_decision) PURE;
};

using DriverPtr = std::unique_ptr<Driver>;

/**
 * HttpTracer is responsible for handling traces and delegate actions to the
 * corresponding drivers.
 */
class HttpTracer {
public:
  virtual ~HttpTracer() = default;

  virtual SpanPtr startSpan(const Config& config, Http::HeaderMap& request_headers,
                            const StreamInfo::StreamInfo& stream_info,
                            const Tracing::Decision tracing_decision) PURE;
};

using HttpTracerPtr = std::unique_ptr<HttpTracer>;

} // namespace Tracing
} // namespace Envoy
