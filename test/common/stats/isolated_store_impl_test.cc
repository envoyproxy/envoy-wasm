#include <string>

#include "envoy/stats/stats_macros.h"

#include "common/stats/isolated_store_impl.h"
#include "common/stats/null_counter.h"
#include "common/stats/null_gauge.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Stats {

class StatsIsolatedStoreImplTest : public testing::Test {
protected:
  StatsIsolatedStoreImplTest() : pool_(store_.symbolTable()) {}
  ~StatsIsolatedStoreImplTest() override {
    pool_.clear();
    EXPECT_EQ(0, store_.symbolTable().numSymbols());
  }

  StatName makeStatName(absl::string_view name) { return pool_.add(name); }

  IsolatedStoreImpl store_;
  StatNamePool pool_;
};

TEST_F(StatsIsolatedStoreImplTest, All) {
  ScopePtr scope1 = store_.createScope("scope1.");
  Counter& c1 = store_.counter("c1");
  Counter& c2 = scope1->counter("c2");
  EXPECT_EQ("c1", c1.name());
  EXPECT_EQ("scope1.c2", c2.name());
  EXPECT_EQ("c1", c1.tagExtractedName());
  EXPECT_EQ("scope1.c2", c2.tagExtractedName());
  EXPECT_EQ(0, c1.tags().size());
  EXPECT_EQ(0, c1.tags().size());
  OptionalCounter opt_counter = scope1->findCounter(c2.statName());
  ASSERT_TRUE(opt_counter);
  EXPECT_EQ(&c2, &opt_counter->get());
  StatName not_found = pool_.add("not_found");
  EXPECT_FALSE(scope1->findCounter(not_found));

  StatNameManagedStorage c1_name("c1", store_.symbolTable());
  c1.add(100);
  auto found_counter = store_.findCounter(c1_name.statName());
  ASSERT_TRUE(found_counter.has_value());
  EXPECT_EQ(&c1, &found_counter->get());
  EXPECT_EQ(100, found_counter->get().value());
  c1.add(100);
  EXPECT_EQ(200, found_counter->get().value());

  Gauge& g1 = store_.gauge("g1", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope1->gauge("g2", Gauge::ImportMode::Accumulate);
  EXPECT_EQ("g1", g1.name());
  EXPECT_EQ("scope1.g2", g2.name());
  EXPECT_EQ("g1", g1.tagExtractedName());
  EXPECT_EQ("scope1.g2", g2.tagExtractedName());
  EXPECT_EQ(0, g1.tags().size());
  EXPECT_EQ(0, g2.tags().size());
  OptionalGauge opt_gauge = scope1->findGauge(g2.statName());
  ASSERT_TRUE(opt_gauge);
  EXPECT_EQ(&g2, &opt_gauge->get());
  EXPECT_FALSE(scope1->findGauge(not_found));
  // TODO(jmarantz): There may be a bug with
  // scope1->findGauge(h1.statName()), which finds the histogram added to
  // the store, which is arguably not in the scope. Investigate what the
  // behavior should be.

  StatNameManagedStorage g1_name("g1", store_.symbolTable());
  g1.set(100);
  auto found_gauge = store_.findGauge(g1_name.statName());
  ASSERT_TRUE(found_gauge.has_value());
  EXPECT_EQ(&g1, &found_gauge->get());
  EXPECT_EQ(100, found_gauge->get().value());
  g1.set(0);
  EXPECT_EQ(0, found_gauge->get().value());

  Histogram& h1 = store_.histogram("h1", Stats::Histogram::Unit::Unspecified);
  EXPECT_TRUE(h1.used()); // hardcoded in impl to be true always.
  EXPECT_TRUE(h1.use_count() == 1);
  Histogram& h2 = scope1->histogram("h2", Stats::Histogram::Unit::Unspecified);
  scope1->deliverHistogramToSinks(h2, 0);
  EXPECT_EQ("h1", h1.name());
  EXPECT_EQ("scope1.h2", h2.name());
  EXPECT_EQ("h1", h1.tagExtractedName());
  EXPECT_EQ("scope1.h2", h2.tagExtractedName());
  EXPECT_EQ(0, h1.tags().size());
  EXPECT_EQ(0, h2.tags().size());
  h1.recordValue(200);
  h2.recordValue(200);
  OptionalHistogram opt_histogram = scope1->findHistogram(h2.statName());
  ASSERT_TRUE(opt_histogram);
  EXPECT_EQ(&h2, &opt_histogram->get());
  EXPECT_FALSE(scope1->findHistogram(not_found));
  // TODO(jmarantz): There may be a bug with
  // scope1->findHistogram(h1.statName()), which finds the histogram added to
  // the store, which is arguably not in the scope. Investigate what the
  // behavior should be.

  StatNameManagedStorage h1_name("h1", store_.symbolTable());
  auto found_histogram = store_.findHistogram(h1_name.statName());
  ASSERT_TRUE(found_histogram.has_value());
  EXPECT_EQ(&h1, &found_histogram->get());

  ScopePtr scope2 = scope1->createScope("foo.");
  EXPECT_EQ("scope1.foo.bar", scope2->counter("bar").name());

  // Validate that we sanitize away bad characters in the stats prefix.
  ScopePtr scope3 = scope1->createScope(std::string("foo:\0:.", 7));
  EXPECT_EQ("scope1.foo___.bar", scope3->counter("bar").name());

  EXPECT_EQ(4UL, store_.counters().size());
  EXPECT_EQ(2UL, store_.gauges().size());

  StatNameManagedStorage nonexistent_name("nonexistent", store_.symbolTable());
  EXPECT_EQ(store_.findCounter(nonexistent_name.statName()), absl::nullopt);
  EXPECT_EQ(store_.findGauge(nonexistent_name.statName()), absl::nullopt);
  EXPECT_EQ(store_.findHistogram(nonexistent_name.statName()), absl::nullopt);
}

TEST_F(StatsIsolatedStoreImplTest, PrefixIsStatName) {
  ScopePtr scope1 = store_.createScope("scope1");
  ScopePtr scope2 = scope1->createScope("scope2");
  Counter& c1 = scope2->counter("c1");
  EXPECT_EQ("scope1.scope2.c1", c1.name());
}

TEST_F(StatsIsolatedStoreImplTest, AllWithSymbolTable) {
  ScopePtr scope1 = store_.createScope("scope1.");
  Counter& c1 = store_.counterFromStatName(makeStatName("c1"));
  Counter& c2 = scope1->counterFromStatName(makeStatName("c2"));
  EXPECT_EQ("c1", c1.name());
  EXPECT_EQ("scope1.c2", c2.name());
  EXPECT_EQ("c1", c1.tagExtractedName());
  EXPECT_EQ("scope1.c2", c2.tagExtractedName());
  EXPECT_EQ(0, c1.tags().size());
  EXPECT_EQ(0, c1.tags().size());

  Gauge& g1 = store_.gaugeFromStatName(makeStatName("g1"), Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope1->gaugeFromStatName(makeStatName("g2"), Gauge::ImportMode::Accumulate);
  EXPECT_EQ("g1", g1.name());
  EXPECT_EQ("scope1.g2", g2.name());
  EXPECT_EQ("g1", g1.tagExtractedName());
  EXPECT_EQ("scope1.g2", g2.tagExtractedName());
  EXPECT_EQ(0, g1.tags().size());
  EXPECT_EQ(0, g1.tags().size());

  Histogram& h1 =
      store_.histogramFromStatName(makeStatName("h1"), Stats::Histogram::Unit::Unspecified);
  Histogram& h2 =
      scope1->histogramFromStatName(makeStatName("h2"), Stats::Histogram::Unit::Unspecified);
  scope1->deliverHistogramToSinks(h2, 0);
  EXPECT_EQ("h1", h1.name());
  EXPECT_EQ("scope1.h2", h2.name());
  EXPECT_EQ("h1", h1.tagExtractedName());
  EXPECT_EQ("scope1.h2", h2.tagExtractedName());
  EXPECT_EQ(0, h1.tags().size());
  EXPECT_EQ(0, h2.tags().size());
  h1.recordValue(200);
  h2.recordValue(200);

  ScopePtr scope2 = scope1->createScope("foo.");
  EXPECT_EQ("scope1.foo.bar", scope2->counterFromStatName(makeStatName("bar")).name());

  // Validate that we sanitize away bad characters in the stats prefix.
  ScopePtr scope3 = scope1->createScope(std::string("foo:\0:.", 7));
  EXPECT_EQ("scope1.foo___.bar", scope3->counter("bar").name());

  EXPECT_EQ(4UL, store_.counters().size());
  EXPECT_EQ(2UL, store_.gauges().size());
}

TEST_F(StatsIsolatedStoreImplTest, ConstSymtabAccessor) {
  ScopePtr scope = store_.createScope("scope.");
  const Scope& cscope = *scope;
  const SymbolTable& const_symbol_table = cscope.constSymbolTable();
  SymbolTable& symbol_table = scope->symbolTable();
  EXPECT_EQ(&const_symbol_table, &symbol_table);
}

TEST_F(StatsIsolatedStoreImplTest, LongStatName) {
  const std::string long_string(128, 'A');

  ScopePtr scope = store_.createScope("scope.");
  Counter& counter = scope->counter(long_string);
  EXPECT_EQ(absl::StrCat("scope.", long_string), counter.name());
}

/**
 * Test stats macros. @see stats_macros.h
 */
#define ALL_TEST_STATS(COUNTER, GAUGE, HISTOGRAM)                                                  \
  COUNTER(test_counter)                                                                            \
  GAUGE(test_gauge, Accumulate)                                                                    \
  HISTOGRAM(test_histogram, Microseconds)

struct TestStats {
  ALL_TEST_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT, GENERATE_HISTOGRAM_STRUCT)
};

TEST_F(StatsIsolatedStoreImplTest, StatsMacros) {
  TestStats test_stats{ALL_TEST_STATS(POOL_COUNTER_PREFIX(store_, "test."),
                                      POOL_GAUGE_PREFIX(store_, "test."),
                                      POOL_HISTOGRAM_PREFIX(store_, "test."))};

  Counter& counter = test_stats.test_counter_;
  EXPECT_EQ("test.test_counter", counter.name());

  Gauge& gauge = test_stats.test_gauge_;
  EXPECT_EQ("test.test_gauge", gauge.name());

  Histogram& histogram = test_stats.test_histogram_;
  EXPECT_EQ("test.test_histogram", histogram.name());
  EXPECT_EQ(Histogram::Unit::Microseconds, histogram.unit());
}

TEST_F(StatsIsolatedStoreImplTest, NullImplCoverage) {
  NullCounterImpl& c = store_.nullCounter();
  c.inc();
  EXPECT_EQ(0, c.value());
  NullGaugeImpl& g = store_.nullGauge("");
  g.inc();
  EXPECT_EQ(0, g.value());
}

} // namespace Stats
} // namespace Envoy
