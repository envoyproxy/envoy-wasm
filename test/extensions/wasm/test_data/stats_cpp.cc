// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

// Test the low level interface.
extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onStart(uint32_t, uint32_t, uint32_t) {
  auto c = defineMetric(MetricType::Counter, "test_counter");
  auto g = defineMetric(MetricType::Gauge, "test_gauges");
  auto h = defineMetric(MetricType::Histogram, "test_histogram");

  incrementMetric(c, 1);
  recordMetric(g, 2);
  recordMetric(h, 3);

  logTrace(std::string("get counter = ") + std::to_string(getMetric(c)));
  incrementMetric(c, 1);
  logDebug(std::string("get counter = ") + std::to_string(getMetric(c)));
  recordMetric(c, 3);
  logInfo(std::string("get counter = ") + std::to_string(getMetric(c)));
  logWarn(std::string("get gauge = ") + std::to_string(getMetric(g)));
  logError(std::string("get histogram = ") + std::to_string(getMetric(h)));
}

// Test the higher level interface.
extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onTick(uint32_t) {
  Metric c(MetricType::Counter, "test_counter",
           {MetricTag{"counter_tag", MetricTag::TagType::String}});
  Metric g(MetricType::Gauge, "test_gauge", {MetricTag{"gauge_int_tag", MetricTag::TagType::Int}});
  Metric h(MetricType::Histogram, "test_histogram",
           {MetricTag{"histogram_int_tag", MetricTag::TagType::Int},
            MetricTag{"histogram_string_tag", MetricTag::TagType::String},
            MetricTag{"histogram_bool_tag", MetricTag::TagType::Bool}});

  c.increment(1, "test_tag");
  g.record(2, 9);
  h.record(3, 7, "test_tag", true);

  logTrace(std::string("get counter = ") + std::to_string(c.get("test_tag")));
  c.increment(1, "test_tag");
  logDebug(std::string("get counter = ") + std::to_string(c.get("test_tag")));
  c.record(3, "test_tag");
  logInfo(std::string("get counter = ") + std::to_string(c.get("test_tag")));
  logWarn(std::string("get gauge = ") + std::to_string(g.get(9)));

  auto hh = h.partiallyResolve(7);
  auto h_id = hh.resolve("test_tag", true);
  logError(std::string("resolved histogram name = ") + hh.nameFromIdSlow(h_id));
}

// Test the high level interface.
extern "C" EMSCRIPTEN_KEEPALIVE void proxy_onLog(uint32_t /* context_zero */) {
  auto c =
      Counter<std::string, int, bool>::New("test_counter", "string_tag", "int_tag", "bool_tag");
  auto g = Gauge<std::string, std::string>::New("test_gauge", "string_tag1", "string_tag2");
  auto h =
      Histogram<int, std::string, bool>::New("test_histogram", "int_tag", "string_tag", "bool_tag");

  c->increment(1, "test_tag", 7, true);
  logTrace(std::string("get counter = ") + std::to_string(c->get("test_tag", 7, true)));
  auto simple_c = c->resolve("test_tag", 7, true);
  simple_c++;
  logDebug(std::string("get counter = ") + std::to_string(c->get("test_tag", 7, true)));
  c->record(3, "test_tag", 7, true);
  logInfo(std::string("get counter = ") + std::to_string(c->get("test_tag", 7, true)));

  g->record(2, "test_tag1", "test_tag2");
  logWarn(std::string("get gauge = ") + std::to_string(g->get("test_tag1", "test_tag2")));

  h->record(3, 7, "test_tag", true);
  auto base_h = Counter<int>::New("test_histogram", "int_tag");
  auto complete_h = base_h->resolveAndExtend<std::string, bool>(7, "string_tag", "bool_tag");
  auto simple_h = complete_h->resolve("test_tag", true);
  logError(std::string("h_id = ") + complete_h->nameFromIdSlow(simple_h.metric_id));
}
