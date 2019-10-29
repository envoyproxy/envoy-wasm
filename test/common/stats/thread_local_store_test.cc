#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "envoy/config/metrics/v2/stats.pb.h"

#include "common/common/c_smart_ptr.h"
#include "common/event/dispatcher_impl.h"
#include "common/memory/stats.h"
#include "common/stats/stats_matcher_impl.h"
#include "common/stats/tag_producer_impl.h"
#include "common/stats/thread_local_store.h"

#include "test/common/stats/stat_test_utility.h"
#include "test/mocks/event/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/mocks/stats/mocks.h"
#include "test/mocks/thread_local/mocks.h"
#include "test/test_common/logging.h"
#include "test/test_common/utility.h"

#include "absl/strings/str_split.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::InSequence;
using testing::NiceMock;
using testing::Ref;
using testing::Return;

namespace Envoy {
namespace Stats {

const uint64_t MaxStatNameLength = 127;

class StatsThreadLocalStoreTest : public testing::Test {
public:
  StatsThreadLocalStoreTest()
      : symbol_table_(SymbolTableCreator::makeSymbolTable()), alloc_(*symbol_table_),
        store_(std::make_unique<ThreadLocalStoreImpl>(alloc_)) {
    store_->addSink(sink_);
  }

  void resetStoreWithAlloc(Allocator& alloc) {
    store_ = std::make_unique<ThreadLocalStoreImpl>(alloc);
    store_->addSink(sink_);
  }

  SymbolTablePtr symbol_table_;
  NiceMock<Event::MockDispatcher> main_thread_dispatcher_;
  NiceMock<ThreadLocal::MockInstance> tls_;
  AllocatorImpl alloc_;
  MockSink sink_;
  std::unique_ptr<ThreadLocalStoreImpl> store_;
};

class HistogramWrapper {
public:
  HistogramWrapper() : histogram_(hist_alloc()) {}

  ~HistogramWrapper() { hist_free(histogram_); }

  const histogram_t* getHistogram() { return histogram_; }

  void setHistogramValues(const std::vector<uint64_t>& values) {
    for (uint64_t value : values) {
      hist_insert_intscale(histogram_, value, 0, 1);
    }
  }

private:
  histogram_t* histogram_;
};

class HistogramTest : public testing::Test {
public:
  using NameHistogramMap = std::map<std::string, ParentHistogramSharedPtr>;

  HistogramTest() : symbol_table_(SymbolTableCreator::makeSymbolTable()), alloc_(*symbol_table_) {}

  void SetUp() override {
    store_ = std::make_unique<ThreadLocalStoreImpl>(alloc_);
    store_->addSink(sink_);
    store_->initializeThreading(main_thread_dispatcher_, tls_);
  }

  void TearDown() override {
    store_->shutdownThreading();
    tls_.shutdownThread();
  }

  NameHistogramMap makeHistogramMap(const std::vector<ParentHistogramSharedPtr>& hist_list) {
    NameHistogramMap name_histogram_map;
    for (const ParentHistogramSharedPtr& histogram : hist_list) {
      // Exclude the scope part of the name.
      const std::vector<std::string>& split_vector = absl::StrSplit(histogram->name(), '.');
      name_histogram_map.insert(std::make_pair(split_vector.back(), histogram));
    }
    return name_histogram_map;
  }

  /**
   * Validates that Histogram merge happens as desired and returns the processed histogram count
   * that can be asserted later.
   */
  uint64_t validateMerge() {
    bool merge_called = false;
    store_->mergeHistograms([&merge_called]() -> void { merge_called = true; });

    EXPECT_TRUE(merge_called);

    std::vector<ParentHistogramSharedPtr> histogram_list = store_->histograms();

    HistogramWrapper hist1_cumulative;
    HistogramWrapper hist2_cumulative;
    HistogramWrapper hist1_interval;
    HistogramWrapper hist2_interval;

    hist1_cumulative.setHistogramValues(h1_cumulative_values_);
    hist2_cumulative.setHistogramValues(h2_cumulative_values_);
    hist1_interval.setHistogramValues(h1_interval_values_);
    hist2_interval.setHistogramValues(h2_interval_values_);

    HistogramStatisticsImpl h1_cumulative_statistics(hist1_cumulative.getHistogram());
    HistogramStatisticsImpl h2_cumulative_statistics(hist2_cumulative.getHistogram());
    HistogramStatisticsImpl h1_interval_statistics(hist1_interval.getHistogram());
    HistogramStatisticsImpl h2_interval_statistics(hist2_interval.getHistogram());

    NameHistogramMap name_histogram_map = makeHistogramMap(histogram_list);
    const ParentHistogramSharedPtr& h1 = name_histogram_map["h1"];
    EXPECT_EQ(h1->cumulativeStatistics().quantileSummary(),
              h1_cumulative_statistics.quantileSummary());
    EXPECT_EQ(h1->intervalStatistics().quantileSummary(), h1_interval_statistics.quantileSummary());
    EXPECT_EQ(h1->cumulativeStatistics().bucketSummary(), h1_cumulative_statistics.bucketSummary());
    EXPECT_EQ(h1->intervalStatistics().bucketSummary(), h1_interval_statistics.bucketSummary());

    if (histogram_list.size() > 1) {
      const ParentHistogramSharedPtr& h2 = name_histogram_map["h2"];
      EXPECT_EQ(h2->cumulativeStatistics().quantileSummary(),
                h2_cumulative_statistics.quantileSummary());
      EXPECT_EQ(h2->intervalStatistics().quantileSummary(),
                h2_interval_statistics.quantileSummary());
      EXPECT_EQ(h2->cumulativeStatistics().bucketSummary(),
                h2_cumulative_statistics.bucketSummary());
      EXPECT_EQ(h2->intervalStatistics().bucketSummary(), h2_interval_statistics.bucketSummary());
    }

    h1_interval_values_.clear();
    h2_interval_values_.clear();

    return histogram_list.size();
  }

  void expectCallAndAccumulate(Histogram& histogram, uint64_t record_value) {
    EXPECT_CALL(sink_, onHistogramComplete(Ref(histogram), record_value));
    histogram.recordValue(record_value);

    if (histogram.name() == "h1") {
      h1_cumulative_values_.push_back(record_value);
      h1_interval_values_.push_back(record_value);
    } else {
      h2_cumulative_values_.push_back(record_value);
      h2_interval_values_.push_back(record_value);
    }
  }

  SymbolTablePtr symbol_table_;
  NiceMock<Event::MockDispatcher> main_thread_dispatcher_;
  NiceMock<ThreadLocal::MockInstance> tls_;
  AllocatorImpl alloc_;
  MockSink sink_;
  std::unique_ptr<ThreadLocalStoreImpl> store_;
  InSequence s;
  std::vector<uint64_t> h1_cumulative_values_, h2_cumulative_values_, h1_interval_values_,
      h2_interval_values_;
};

TEST_F(StatsThreadLocalStoreTest, NoTls) {
  InSequence s;

  Counter& c1 = store_->counter("c1");
  EXPECT_EQ(&c1, &store_->counter("c1"));
  StatNameManagedStorage c1_name("c1", *symbol_table_);
  c1.add(100);
  auto found_counter = store_->findCounter(c1_name.statName());
  ASSERT_TRUE(found_counter.has_value());
  EXPECT_EQ(&c1, &found_counter->get());
  EXPECT_EQ(100, found_counter->get().value());
  c1.add(100);
  EXPECT_EQ(200, found_counter->get().value());

  Gauge& g1 = store_->gauge("g1", Gauge::ImportMode::Accumulate);
  EXPECT_EQ(&g1, &store_->gauge("g1", Gauge::ImportMode::Accumulate));
  StatNameManagedStorage g1_name("g1", *symbol_table_);
  g1.set(100);
  auto found_gauge = store_->findGauge(g1_name.statName());
  ASSERT_TRUE(found_gauge.has_value());
  EXPECT_EQ(&g1, &found_gauge->get());
  EXPECT_EQ(100, found_gauge->get().value());
  g1.set(0);
  EXPECT_EQ(0, found_gauge->get().value());

  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(&h1, &store_->histogram("h1", Stats::Histogram::Unit::Unspecified));
  StatNameManagedStorage h1_name("h1", *symbol_table_);
  auto found_histogram = store_->findHistogram(h1_name.statName());
  ASSERT_TRUE(found_histogram.has_value());
  EXPECT_EQ(&h1, &found_histogram->get());

  EXPECT_CALL(sink_, onHistogramComplete(Ref(h1), 200));
  h1.recordValue(200);
  EXPECT_CALL(sink_, onHistogramComplete(Ref(h1), 100));
  store_->deliverHistogramToSinks(h1, 100);

  EXPECT_EQ(1UL, store_->counters().size());
  EXPECT_EQ(&c1, TestUtility::findCounter(*store_, "c1").get());
  EXPECT_EQ(2L, TestUtility::findCounter(*store_, "c1").use_count());
  EXPECT_EQ(1UL, store_->gauges().size());
  EXPECT_EQ(&g1, store_->gauges().front().get()); // front() ok when size()==1
  EXPECT_EQ(2L, store_->gauges().front().use_count());

  store_->shutdownThreading();
}

TEST_F(StatsThreadLocalStoreTest, Tls) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  Counter& c1 = store_->counter("c1");
  EXPECT_EQ(&c1, &store_->counter("c1"));
  StatNameManagedStorage c1_name("c1", *symbol_table_);
  c1.add(100);
  auto found_counter = store_->findCounter(c1_name.statName());
  ASSERT_TRUE(found_counter.has_value());
  EXPECT_EQ(&c1, &found_counter->get());
  EXPECT_EQ(100, found_counter->get().value());
  c1.add(100);
  EXPECT_EQ(200, found_counter->get().value());

  Gauge& g1 = store_->gauge("g1", Gauge::ImportMode::Accumulate);
  EXPECT_EQ(&g1, &store_->gauge("g1", Gauge::ImportMode::Accumulate));
  StatNameManagedStorage g1_name("g1", *symbol_table_);
  g1.set(100);
  auto found_gauge = store_->findGauge(g1_name.statName());
  ASSERT_TRUE(found_gauge.has_value());
  EXPECT_EQ(&g1, &found_gauge->get());
  EXPECT_EQ(100, found_gauge->get().value());
  g1.set(0);
  EXPECT_EQ(0, found_gauge->get().value());

  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(&h1, &store_->histogram("h1", Stats::Histogram::Unit::Unspecified));
  StatNameManagedStorage h1_name("h1", *symbol_table_);
  auto found_histogram = store_->findHistogram(h1_name.statName());
  ASSERT_TRUE(found_histogram.has_value());
  EXPECT_EQ(&h1, &found_histogram->get());

  EXPECT_EQ(1UL, store_->counters().size());
  EXPECT_EQ(&c1, TestUtility::findCounter(*store_, "c1").get());
  EXPECT_EQ(3L, TestUtility::findCounter(*store_, "c1").use_count());
  EXPECT_EQ(1UL, store_->gauges().size());
  EXPECT_EQ(&g1, store_->gauges().front().get()); // front() ok when size()==1
  EXPECT_EQ(3L, store_->gauges().front().use_count());

  store_->shutdownThreading();
  tls_.shutdownThread();

  EXPECT_EQ(1UL, store_->counters().size());
  EXPECT_EQ(&c1, TestUtility::findCounter(*store_, "c1").get());
  EXPECT_EQ(2L, TestUtility::findCounter(*store_, "c1").use_count());
  EXPECT_EQ(1UL, store_->gauges().size());
  EXPECT_EQ(&g1, store_->gauges().front().get()); // front() ok when size()==1
  EXPECT_EQ(2L, store_->gauges().front().use_count());
}

TEST_F(StatsThreadLocalStoreTest, BasicScope) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  ScopePtr scope1 = store_->createScope("scope1.");
  Counter& c1 = store_->counter("c1");
  Counter& c2 = scope1->counter("c2");
  EXPECT_EQ("c1", c1.name());
  EXPECT_EQ("scope1.c2", c2.name());
  StatNameManagedStorage c1_name("c1", *symbol_table_);
  auto found_counter = store_->findCounter(c1_name.statName());
  ASSERT_TRUE(found_counter.has_value());
  EXPECT_EQ(&c1, &found_counter->get());
  StatNameManagedStorage c2_name("scope1.c2", *symbol_table_);
  auto found_counter2 = store_->findCounter(c2_name.statName());
  ASSERT_TRUE(found_counter2.has_value());
  EXPECT_EQ(&c2, &found_counter2->get());

  Gauge& g1 = store_->gauge("g1", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope1->gauge("g2", Gauge::ImportMode::Accumulate);
  EXPECT_EQ("g1", g1.name());
  EXPECT_EQ("scope1.g2", g2.name());
  StatNameManagedStorage g1_name("g1", *symbol_table_);
  auto found_gauge = store_->findGauge(g1_name.statName());
  ASSERT_TRUE(found_gauge.has_value());
  EXPECT_EQ(&g1, &found_gauge->get());
  StatNameManagedStorage g2_name("scope1.g2", *symbol_table_);
  auto found_gauge2 = store_->findGauge(g2_name.statName());
  ASSERT_TRUE(found_gauge2.has_value());
  EXPECT_EQ(&g2, &found_gauge2->get());

  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  Histogram& h2 = scope1->histogram("h2", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ("h1", h1.name());
  EXPECT_EQ("scope1.h2", h2.name());
  EXPECT_CALL(sink_, onHistogramComplete(Ref(h1), 100));
  h1.recordValue(100);
  EXPECT_CALL(sink_, onHistogramComplete(Ref(h2), 200));
  h2.recordValue(200);
  StatNameManagedStorage h1_name("h1", *symbol_table_);
  auto found_histogram = store_->findHistogram(h1_name.statName());
  ASSERT_TRUE(found_histogram.has_value());
  EXPECT_EQ(&h1, &found_histogram->get());
  StatNameManagedStorage h2_name("scope1.h2", *symbol_table_);
  auto found_histogram2 = store_->findHistogram(h2_name.statName());
  ASSERT_TRUE(found_histogram2.has_value());
  EXPECT_EQ(&h2, &found_histogram2->get());

  store_->shutdownThreading();
  scope1->deliverHistogramToSinks(h1, 100);
  scope1->deliverHistogramToSinks(h2, 200);
  tls_.shutdownThread();
}

// Validate that we sanitize away bad characters in the stats prefix.
TEST_F(StatsThreadLocalStoreTest, SanitizePrefix) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  ScopePtr scope1 = store_->createScope(std::string("scope1:\0:foo.", 13));
  Counter& c1 = scope1->counter("c1");
  EXPECT_EQ("scope1___foo.c1", c1.name());

  store_->shutdownThreading();
  tls_.shutdownThread();
}

TEST_F(StatsThreadLocalStoreTest, ConstSymtabAccessor) {
  ScopePtr scope = store_->createScope("scope.");
  const Scope& cscope = *scope;
  const SymbolTable& const_symbol_table = cscope.constSymbolTable();
  SymbolTable& symbol_table = scope->symbolTable();
  EXPECT_EQ(&const_symbol_table, &symbol_table);
}

TEST_F(StatsThreadLocalStoreTest, ScopeDelete) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  ScopePtr scope1 = store_->createScope("scope1.");
  scope1->counter("c1");
  EXPECT_EQ(1UL, store_->counters().size());
  CounterSharedPtr c1 = TestUtility::findCounter(*store_, "scope1.c1");
  EXPECT_EQ("scope1.c1", c1->name());
  EXPECT_EQ(TestUtility::findByName(store_->counters(), "scope1.c1"), c1);

  EXPECT_CALL(main_thread_dispatcher_, post(_));
  EXPECT_CALL(tls_, runOnAllThreads(_, _));
  scope1.reset();
  EXPECT_EQ(0UL, store_->counters().size());

  EXPECT_EQ(1L, c1.use_count());
  c1.reset();

  store_->shutdownThreading();
  tls_.shutdownThread();
}

TEST_F(StatsThreadLocalStoreTest, NestedScopes) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  ScopePtr scope1 = store_->createScope("scope1.");
  Counter& c1 = scope1->counter("foo.bar");
  EXPECT_EQ("scope1.foo.bar", c1.name());
  StatNameManagedStorage c1_name("scope1.foo.bar", *symbol_table_);
  auto found_counter = store_->findCounter(c1_name.statName());
  ASSERT_TRUE(found_counter.has_value());
  EXPECT_EQ(&c1, &found_counter->get());

  ScopePtr scope2 = scope1->createScope("foo.");
  Counter& c2 = scope2->counter("bar");
  EXPECT_EQ(&c1, &c2);
  EXPECT_EQ("scope1.foo.bar", c2.name());
  StatNameManagedStorage c2_name("scope1.foo.bar", *symbol_table_);
  auto found_counter2 = store_->findCounter(c2_name.statName());
  ASSERT_TRUE(found_counter2.has_value());

  // Different allocations point to the same referenced counted backing memory.
  c1.inc();
  EXPECT_EQ(1UL, c1.value());
  EXPECT_EQ(c1.value(), c2.value());

  Gauge& g1 = scope2->gauge("some_gauge", Gauge::ImportMode::Accumulate);
  EXPECT_EQ("scope1.foo.some_gauge", g1.name());

  store_->shutdownThreading();
  tls_.shutdownThread();
}

TEST_F(StatsThreadLocalStoreTest, OverlappingScopes) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  // Both scopes point to the same namespace. This can happen during reload of a cluster for
  // example.
  ScopePtr scope1 = store_->createScope("scope1.");
  ScopePtr scope2 = store_->createScope("scope1.");

  // We will call alloc twice, but they should point to the same backing storage.
  Counter& c1 = scope1->counter("c");
  Counter& c2 = scope2->counter("c");
  EXPECT_EQ(&c1, &c2);
  c1.inc();
  EXPECT_EQ(1UL, c1.value());
  EXPECT_EQ(1UL, c2.value());
  c2.inc();
  EXPECT_EQ(2UL, c1.value());
  EXPECT_EQ(2UL, c2.value());

  // We should dedup when we fetch all counters to handle the overlapping case.
  EXPECT_EQ(1UL, store_->counters().size());

  // Gauges should work the same way.
  Gauge& g1 = scope1->gauge("g", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope2->gauge("g", Gauge::ImportMode::Accumulate);
  EXPECT_EQ(&g1, &g2);
  g1.set(5);
  EXPECT_EQ(5UL, g1.value());
  EXPECT_EQ(5UL, g2.value());
  g2.set(1);
  EXPECT_EQ(1UL, g1.value());
  EXPECT_EQ(1UL, g2.value());
  EXPECT_EQ(1UL, store_->gauges().size());

  // Deleting scope 1 will call free but will be reference counted. It still leaves scope 2 valid.
  scope1.reset();
  c2.inc();
  EXPECT_EQ(3UL, c2.value());
  EXPECT_EQ(1UL, store_->counters().size());
  g2.set(10);
  EXPECT_EQ(10UL, g2.value());
  EXPECT_EQ(1UL, store_->gauges().size());

  store_->shutdownThreading();
  tls_.shutdownThread();
}

class LookupWithStatNameTest : public testing::Test {
public:
  LookupWithStatNameTest()
      : symbol_table_(SymbolTableCreator::makeSymbolTable()), alloc_(*symbol_table_),
        store_(alloc_), pool_(*symbol_table_) {}
  ~LookupWithStatNameTest() override { store_.shutdownThreading(); }

  StatName makeStatName(absl::string_view name) { return pool_.add(name); }

  SymbolTablePtr symbol_table_;
  AllocatorImpl alloc_;
  ThreadLocalStoreImpl store_;
  StatNamePool pool_;
};

TEST_F(LookupWithStatNameTest, All) {
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

TEST_F(LookupWithStatNameTest, NotFound) {
  StatName not_found(makeStatName("not_found"));
  EXPECT_FALSE(store_.findCounter(not_found));
  EXPECT_FALSE(store_.findGauge(not_found));
  EXPECT_FALSE(store_.findHistogram(not_found));
}

class StatsMatcherTLSTest : public StatsThreadLocalStoreTest {
public:
  envoy::config::metrics::v2::StatsConfig stats_config_;
};

TEST_F(StatsMatcherTLSTest, TestNoOpStatImpls) {
  InSequence s;

  stats_config_.mutable_stats_matcher()->mutable_exclusion_list()->add_patterns()->set_prefix(
      "noop");
  store_->setStatsMatcher(std::make_unique<StatsMatcherImpl>(stats_config_));

  // Testing No-op counters, gauges, histograms which match the prefix "noop".

  // Counter
  Counter& noop_counter = store_->counter("noop_counter");
  EXPECT_EQ(noop_counter.name(), "");
  EXPECT_EQ(noop_counter.value(), 0);
  noop_counter.add(1);
  EXPECT_EQ(noop_counter.value(), 0);
  noop_counter.inc();
  EXPECT_EQ(noop_counter.value(), 0);
  noop_counter.reset();
  EXPECT_EQ(noop_counter.value(), 0);
  Counter& noop_counter_2 = store_->counter("noop_counter_2");
  EXPECT_EQ(&noop_counter, &noop_counter_2);
  EXPECT_FALSE(noop_counter.used());      // hardcoded to return false in NullMetricImpl.
  EXPECT_EQ(0, noop_counter.latch());     // hardcoded to 0.
  EXPECT_EQ(0, noop_counter.use_count()); // null counter is contained in ThreadLocalStoreImpl.

  // Gauge
  Gauge& noop_gauge = store_->gauge("noop_gauge", Gauge::ImportMode::Accumulate);
  EXPECT_EQ(noop_gauge.name(), "");
  EXPECT_EQ(noop_gauge.value(), 0);
  noop_gauge.add(1);
  EXPECT_EQ(noop_gauge.value(), 0);
  noop_gauge.inc();
  EXPECT_EQ(noop_gauge.value(), 0);
  noop_gauge.dec();
  EXPECT_EQ(noop_gauge.value(), 0);
  noop_gauge.set(2);
  EXPECT_EQ(noop_gauge.value(), 0);
  noop_gauge.sub(2);
  EXPECT_EQ(noop_gauge.value(), 0);
  EXPECT_EQ(Gauge::ImportMode::NeverImport, noop_gauge.importMode());
  EXPECT_FALSE(noop_gauge.used());      // null gauge is contained in ThreadLocalStoreImpl.
  EXPECT_EQ(0, noop_gauge.use_count()); // null gauge is contained in ThreadLocalStoreImpl.

  Gauge& noop_gauge_2 = store_->gauge("noop_gauge_2", Gauge::ImportMode::Accumulate);
  EXPECT_EQ(&noop_gauge, &noop_gauge_2);

  // Histogram
  Histogram& noop_histogram =
      store_->histogram("noop_histogram", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(noop_histogram.name(), "");
  EXPECT_FALSE(noop_histogram.used());
  Histogram& noop_histogram_2 =
      store_->histogram("noop_histogram_2", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(&noop_histogram, &noop_histogram_2);

  store_->shutdownThreading();
}

// We only test the exclusion list -- the inclusion list is the inverse, and both are tested in
// test/common/stats:stats_matcher_test.
TEST_F(StatsMatcherTLSTest, TestExclusionRegex) {
  InSequence s;

  // Expected to alloc lowercase_counter, lowercase_gauge, valid_counter, valid_gauge

  // Will block all stats containing any capital alphanumeric letter.
  stats_config_.mutable_stats_matcher()->mutable_exclusion_list()->add_patterns()->set_regex(
      ".*[A-Z].*");
  store_->setStatsMatcher(std::make_unique<StatsMatcherImpl>(stats_config_));

  // The creation of counters/gauges/histograms which have no uppercase letters should succeed.
  Counter& lowercase_counter = store_->counter("lowercase_counter");
  EXPECT_EQ(lowercase_counter.name(), "lowercase_counter");
  Gauge& lowercase_gauge = store_->gauge("lowercase_gauge", Gauge::ImportMode::Accumulate);
  EXPECT_EQ(lowercase_gauge.name(), "lowercase_gauge");
  Histogram& lowercase_histogram =
      store_->histogram("lowercase_histogram", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(lowercase_histogram.name(), "lowercase_histogram");

  // And the creation of counters/gauges/histograms which have uppercase letters should fail.
  Counter& uppercase_counter = store_->counter("UPPERCASE_counter");
  EXPECT_EQ(uppercase_counter.name(), "");
  uppercase_counter.inc();
  EXPECT_EQ(uppercase_counter.value(), 0);
  uppercase_counter.inc();
  EXPECT_EQ(uppercase_counter.value(), 0);

  Gauge& uppercase_gauge = store_->gauge("uppercase_GAUGE", Gauge::ImportMode::Accumulate);
  EXPECT_EQ(uppercase_gauge.name(), "");
  uppercase_gauge.inc();
  EXPECT_EQ(uppercase_gauge.value(), 0);
  uppercase_gauge.inc();
  EXPECT_EQ(uppercase_gauge.value(), 0);

  // Histograms are harder to query and test, so we resort to testing that name() returns the empty
  // string.
  Histogram& uppercase_histogram =
      store_->histogram("upperCASE_histogram", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(uppercase_histogram.name(), "");

  // Adding another exclusion rule -- now we reject not just uppercase stats but those starting with
  // the string "invalid".
  stats_config_.mutable_stats_matcher()->mutable_exclusion_list()->add_patterns()->set_prefix(
      "invalid");
  store_->setStatsMatcher(std::make_unique<StatsMatcherImpl>(stats_config_));

  Counter& valid_counter = store_->counter("valid_counter");
  valid_counter.inc();
  EXPECT_EQ(valid_counter.value(), 1);

  Counter& invalid_counter = store_->counter("invalid_counter");
  invalid_counter.inc();
  EXPECT_EQ(invalid_counter.value(), 0);

  // But the old exclusion rule still holds.
  Counter& invalid_counter_2 = store_->counter("also_INVALID_counter");
  invalid_counter_2.inc();
  EXPECT_EQ(invalid_counter_2.value(), 0);

  // And we expect the same behavior from gauges and histograms.
  Gauge& valid_gauge = store_->gauge("valid_gauge", Gauge::ImportMode::Accumulate);
  valid_gauge.set(2);
  EXPECT_EQ(valid_gauge.value(), 2);

  Gauge& invalid_gauge_1 = store_->gauge("invalid_gauge", Gauge::ImportMode::Accumulate);
  invalid_gauge_1.inc();
  EXPECT_EQ(invalid_gauge_1.value(), 0);

  Gauge& invalid_gauge_2 = store_->gauge("also_INVALID_gauge", Gauge::ImportMode::Accumulate);
  invalid_gauge_2.inc();
  EXPECT_EQ(invalid_gauge_2.value(), 0);

  Histogram& valid_histogram =
      store_->histogram("valid_histogram", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(valid_histogram.name(), "valid_histogram");

  Histogram& invalid_histogram_1 =
      store_->histogram("invalid_histogram", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(invalid_histogram_1.name(), "");

  Histogram& invalid_histogram_2 =
      store_->histogram("also_INVALID_histogram", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ(invalid_histogram_2.name(), "");

  // Expected to free lowercase_counter, lowercase_gauge, valid_counter, valid_gauge
  store_->shutdownThreading();
}

// Tests the logic for caching the stats-matcher results, and in particular the
// private impl method checkAndRememberRejection(). That method behaves
// differently depending on whether TLS is enabled or not, so we parameterize
// the test accordingly; GetParam()==true means we want a TLS cache. In either
// case, we should never be calling the stats-matcher rejection logic more than
// once on given stat name.
class RememberStatsMatcherTest : public testing::TestWithParam<bool> {
public:
  RememberStatsMatcherTest()
      : symbol_table_(SymbolTableCreator::makeSymbolTable()), heap_alloc_(*symbol_table_),
        store_(heap_alloc_), scope_(store_.createScope("scope.")) {
    if (GetParam()) {
      store_.initializeThreading(main_thread_dispatcher_, tls_);
    }
  }

  ~RememberStatsMatcherTest() override {
    store_.shutdownThreading();
    tls_.shutdownThread();
  }

  using LookupStatFn = std::function<std::string(const std::string&)>;

  // Helper function to test the rejection cache. The goal here is to use
  // mocks to ensure that we don't call rejects() more than once on any of the
  // stats, even with 5 name-based lookups.
  void testRememberMatcher(const LookupStatFn lookup_stat) {
    InSequence s;

    MockStatsMatcher* matcher = new MockStatsMatcher;
    StatsMatcherPtr matcher_ptr(matcher);
    store_.setStatsMatcher(std::move(matcher_ptr));

    EXPECT_CALL(*matcher, rejects("scope.reject")).WillOnce(Return(true));
    EXPECT_CALL(*matcher, rejects("scope.ok")).WillOnce(Return(false));

    for (int j = 0; j < 5; ++j) {
      EXPECT_EQ("", lookup_stat("reject"));
      EXPECT_EQ("scope.ok", lookup_stat("ok"));
    }
  }

  void testRejectsAll(const LookupStatFn lookup_stat) {
    InSequence s;

    MockStatsMatcher* matcher = new MockStatsMatcher;
    matcher->rejects_all_ = true;
    StatsMatcherPtr matcher_ptr(matcher);
    store_.setStatsMatcher(std::move(matcher_ptr));

    ScopePtr scope = store_.createScope("scope.");

    for (int j = 0; j < 5; ++j) {
      // Note: zero calls to reject() are made, as reject-all should short-circuit.
      EXPECT_EQ("", lookup_stat("reject"));
    }
  }

  void testAcceptsAll(const LookupStatFn lookup_stat) {
    InSequence s;

    auto* matcher = new MockStatsMatcher;
    matcher->accepts_all_ = true;
    StatsMatcherPtr matcher_ptr(matcher);
    store_.setStatsMatcher(std::move(matcher_ptr));

    for (int j = 0; j < 5; ++j) {
      // Note: zero calls to reject() are made, as accept-all should short-circuit.
      EXPECT_EQ("scope.ok", lookup_stat("ok"));
    }
  }

  LookupStatFn lookupCounterFn() {
    return [this](const std::string& stat_name) -> std::string {
      return scope_->counter(stat_name).name();
    };
  }

  LookupStatFn lookupGaugeFn() {
    return [this](const std::string& stat_name) -> std::string {
      return scope_->gauge(stat_name, Gauge::ImportMode::Accumulate).name();
    };
  }

// TODO(jmarantz): restore BoolIndicator tests when https://github.com/envoyproxy/envoy/pull/6280
// is reverted.
#define HAS_BOOL_INDICATOR 0
#if HAS_BOOL_INDICATOR
  LookupStatFn lookupBoolIndicator() {
    return [this](const std::string& stat_name) -> std::string {
      return scope_->boolIndicator(stat_name).name();
    };
  }
#endif

  LookupStatFn lookupHistogramFn() {
    return [this](const std::string& stat_name) -> std::string {
      return scope_->histogram(stat_name, Stats::Histogram::Unit::Unspecified).name();
    };
  }

  Stats::SymbolTablePtr symbol_table_;
  NiceMock<Event::MockDispatcher> main_thread_dispatcher_;
  NiceMock<ThreadLocal::MockInstance> tls_;
  AllocatorImpl heap_alloc_;
  ThreadLocalStoreImpl store_;
  ScopePtr scope_;
};

INSTANTIATE_TEST_SUITE_P(RememberStatsMatcherTest, RememberStatsMatcherTest,
                         testing::ValuesIn({false, true}));

// Tests that the logic for remembering rejected stats works properly, both
// with and without threading.
TEST_P(RememberStatsMatcherTest, CounterRejectOne) { testRememberMatcher(lookupCounterFn()); }

TEST_P(RememberStatsMatcherTest, CounterRejectsAll) { testRejectsAll(lookupCounterFn()); }

TEST_P(RememberStatsMatcherTest, CounterAcceptsAll) { testAcceptsAll(lookupCounterFn()); }

TEST_P(RememberStatsMatcherTest, GaugeRejectOne) { testRememberMatcher(lookupGaugeFn()); }

TEST_P(RememberStatsMatcherTest, GaugeRejectsAll) { testRejectsAll(lookupGaugeFn()); }

TEST_P(RememberStatsMatcherTest, GaugeAcceptsAll) { testAcceptsAll(lookupGaugeFn()); }

#if HAS_BOOL_INDICATOR
TEST_P(RememberStatsMatcherTest, BoolIndicatorRejectOne) {
  testRememberMatcher(lookupBoolIndicator());
}

TEST_P(RememberStatsMatcherTest, BoolIndicatorRejectsAll) { testRejectsAll(lookupBoolIndicator()); }

TEST_P(RememberStatsMatcherTest, BoolIndicatorAcceptsAll) { testAcceptsAll(lookupBoolIndicator()); }
#endif

TEST_P(RememberStatsMatcherTest, HistogramRejectOne) { testRememberMatcher(lookupHistogramFn()); }

TEST_P(RememberStatsMatcherTest, HistogramRejectsAll) { testRejectsAll(lookupHistogramFn()); }

TEST_P(RememberStatsMatcherTest, HistogramAcceptsAll) { testAcceptsAll(lookupHistogramFn()); }

TEST_F(StatsThreadLocalStoreTest, RemoveRejectedStats) {
  store_->initializeThreading(main_thread_dispatcher_, tls_);
  Counter& counter = store_->counter("c1");
  Gauge& gauge = store_->gauge("g1", Gauge::ImportMode::Accumulate);
  Histogram& histogram = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  ASSERT_EQ(1, store_->counters().size()); // "c1".
  EXPECT_TRUE(&counter == store_->counters()[0].get() ||
              &counter == store_->counters()[1].get()); // counters() order is non-deterministic.
  ASSERT_EQ(1, store_->gauges().size());
  EXPECT_EQ("g1", store_->gauges()[0]->name());
  ASSERT_EQ(1, store_->histograms().size());
  EXPECT_EQ("h1", store_->histograms()[0]->name());

  // Will effectively block all stats, and remove all the non-matching stats.
  envoy::config::metrics::v2::StatsConfig stats_config;
  stats_config.mutable_stats_matcher()->mutable_inclusion_list()->add_patterns()->set_exact(
      "no-such-stat");
  store_->setStatsMatcher(std::make_unique<StatsMatcherImpl>(stats_config));

  // They can no longer be found.
  EXPECT_EQ(0, store_->counters().size());
  EXPECT_EQ(0, store_->gauges().size());
  EXPECT_EQ(0, store_->histograms().size());

  // However, referencing the previously allocated stats will not crash.
  counter.inc();
  gauge.inc();
  EXPECT_CALL(sink_, onHistogramComplete(Ref(histogram), 42));
  histogram.recordValue(42);
  store_->shutdownThreading();
  tls_.shutdownThread();
}

TEST_F(StatsThreadLocalStoreTest, NonHotRestartNoTruncation) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  // Allocate a stat greater than the max name length.
  const std::string name_1(MaxStatNameLength + 1, 'A');

  store_->counter(name_1);

  // This works fine, and we can find it by its long name because heap-stats do not
  // get truncated.
  EXPECT_NE(nullptr, TestUtility::findCounter(*store_, name_1).get());
  store_->shutdownThreading();
  tls_.shutdownThread();
}

class StatsThreadLocalStoreTestNoFixture : public testing::Test {
protected:
  StatsThreadLocalStoreTestNoFixture()
      : save_use_fakes_(SymbolTableCreator::useFakeSymbolTables()) {}
  ~StatsThreadLocalStoreTestNoFixture() override {
    TestUtil::SymbolTableCreatorTestPeer::setUseFakeSymbolTables(save_use_fakes_);
    if (threading_enabled_) {
      store_->shutdownThreading();
      tls_.shutdownThread();
    }
  }

  void init(bool use_fakes) {
    TestUtil::SymbolTableCreatorTestPeer::setUseFakeSymbolTables(use_fakes);
    symbol_table_ = SymbolTableCreator::makeSymbolTable();
    alloc_ = std::make_unique<AllocatorImpl>(*symbol_table_);
    store_ = std::make_unique<ThreadLocalStoreImpl>(*alloc_);
    store_->addSink(sink_);

    // Use a tag producer that will produce tags.
    envoy::config::metrics::v2::StatsConfig stats_config;
    store_->setTagProducer(std::make_unique<TagProducerImpl>(stats_config));
  }

  void initThreading() {
    threading_enabled_ = true;
    store_->initializeThreading(main_thread_dispatcher_, tls_);
  }

  static constexpr size_t million_ = 1000 * 1000;

  MockSink sink_;
  SymbolTablePtr symbol_table_;
  std::unique_ptr<AllocatorImpl> alloc_;
  std::unique_ptr<ThreadLocalStoreImpl> store_;
  NiceMock<Event::MockDispatcher> main_thread_dispatcher_;
  NiceMock<ThreadLocal::MockInstance> tls_;
  const bool save_use_fakes_;
  bool threading_enabled_{false};
};

// Tests how much memory is consumed allocating 100k stats.
TEST_F(StatsThreadLocalStoreTestNoFixture, MemoryWithoutTlsFakeSymbolTable) {
  init(true);
  TestUtil::MemoryTest memory_test;
  TestUtil::forEachSampleStat(
      1000, [this](absl::string_view name) { store_->counter(std::string(name)); });
  EXPECT_MEMORY_EQ(memory_test.consumedBytes(), 15268336); // June 30, 2019
  EXPECT_MEMORY_LE(memory_test.consumedBytes(), 16 * million_);
}

TEST_F(StatsThreadLocalStoreTestNoFixture, MemoryWithTlsFakeSymbolTable) {
  init(true);
  initThreading();
  TestUtil::MemoryTest memory_test;
  TestUtil::forEachSampleStat(
      1000, [this](absl::string_view name) { store_->counter(std::string(name)); });
  EXPECT_MEMORY_EQ(memory_test.consumedBytes(), 17496848); // June 30, 2019
  EXPECT_MEMORY_LE(memory_test.consumedBytes(), 18 * million_);
}

// Tests how much memory is consumed allocating 100k stats.
TEST_F(StatsThreadLocalStoreTestNoFixture, MemoryWithoutTlsRealSymbolTable) {
  init(false);
  TestUtil::MemoryTest memory_test;
  TestUtil::forEachSampleStat(
      1000, [this](absl::string_view name) { store_->counter(std::string(name)); });
  EXPECT_MEMORY_EQ(memory_test.consumedBytes(), 9139120); // Aug 9, 2019
  EXPECT_MEMORY_LE(memory_test.consumedBytes(), 10 * million_);
}

TEST_F(StatsThreadLocalStoreTestNoFixture, MemoryWithTlsRealSymbolTable) {
  init(false);
  initThreading();
  TestUtil::MemoryTest memory_test;
  TestUtil::forEachSampleStat(
      1000, [this](absl::string_view name) { store_->counter(std::string(name)); });
  EXPECT_MEMORY_EQ(memory_test.consumedBytes(), 11367632); // Aug 9, 2019
  EXPECT_MEMORY_LE(memory_test.consumedBytes(), 12 * million_);
}

TEST_F(StatsThreadLocalStoreTest, ShuttingDown) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  store_->counter("c1");
  store_->gauge("g1", Gauge::ImportMode::Accumulate);
  store_->shutdownThreading();
  store_->counter("c2");
  store_->gauge("g2", Gauge::ImportMode::Accumulate);

  // c1, g1 should have a thread local ref, but c2, g2 should not.
  EXPECT_EQ(3L, TestUtility::findCounter(*store_, "c1").use_count());
  EXPECT_EQ(3L, TestUtility::findGauge(*store_, "g1").use_count());
  EXPECT_EQ(2L, TestUtility::findCounter(*store_, "c2").use_count());
  EXPECT_EQ(2L, TestUtility::findGauge(*store_, "g2").use_count());

  store_->shutdownThreading();
  tls_.shutdownThread();
}

TEST_F(StatsThreadLocalStoreTest, MergeDuringShutDown) {
  InSequence s;
  store_->initializeThreading(main_thread_dispatcher_, tls_);

  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ("h1", h1.name());

  EXPECT_CALL(sink_, onHistogramComplete(Ref(h1), 1));
  h1.recordValue(1);

  store_->shutdownThreading();

  // Validate that merge callback is called during shutdown and there is no ASSERT.
  bool merge_called = false;
  store_->mergeHistograms([&merge_called]() -> void { merge_called = true; });

  EXPECT_TRUE(merge_called);
  store_->shutdownThreading();
  tls_.shutdownThread();
}

TEST(ThreadLocalStoreThreadTest, ConstructDestruct) {
  SymbolTablePtr symbol_table(SymbolTableCreator::makeSymbolTable());
  Api::ApiPtr api = Api::createApiForTest();
  Event::DispatcherPtr dispatcher = api->allocateDispatcher();
  NiceMock<ThreadLocal::MockInstance> tls;
  AllocatorImpl alloc(*symbol_table);
  ThreadLocalStoreImpl store(alloc);

  store.initializeThreading(*dispatcher, tls);
  { ScopePtr scope1 = store.createScope("scope1."); }
  store.shutdownThreading();
}

// Histogram tests
TEST_F(HistogramTest, BasicSingleHistogramMerge) {
  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ("h1", h1.name());

  expectCallAndAccumulate(h1, 0);
  expectCallAndAccumulate(h1, 43);
  expectCallAndAccumulate(h1, 41);
  expectCallAndAccumulate(h1, 415);
  expectCallAndAccumulate(h1, 2201);
  expectCallAndAccumulate(h1, 3201);
  expectCallAndAccumulate(h1, 125);
  expectCallAndAccumulate(h1, 13);

  EXPECT_EQ(1, validateMerge());
}

TEST_F(HistogramTest, BasicMultiHistogramMerge) {
  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  Histogram& h2 = store_->histogram("h2", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ("h1", h1.name());
  EXPECT_EQ("h2", h2.name());

  expectCallAndAccumulate(h1, 1);
  expectCallAndAccumulate(h2, 1);
  expectCallAndAccumulate(h2, 2);

  EXPECT_EQ(2, validateMerge());
}

TEST_F(HistogramTest, MultiHistogramMultipleMerges) {
  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  Histogram& h2 = store_->histogram("h2", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ("h1", h1.name());
  EXPECT_EQ("h2", h2.name());

  // Insert one value in to one histogram and validate
  expectCallAndAccumulate(h1, 1);
  EXPECT_EQ(2, validateMerge());

  // Insert value into second histogram and validate that it is merged properly.
  expectCallAndAccumulate(h2, 1);
  EXPECT_EQ(2, validateMerge());

  // Insert more values into both the histograms and validate that it is merged properly.
  expectCallAndAccumulate(h1, 2);
  EXPECT_EQ(2, validateMerge());

  expectCallAndAccumulate(h2, 3);
  EXPECT_EQ(2, validateMerge());

  expectCallAndAccumulate(h2, 2);
  EXPECT_EQ(2, validateMerge());

  // Do not insert any value and validate that intervalSummary is empty for both the histograms and
  // cumulativeSummary has right values.
  EXPECT_EQ(2, validateMerge());
}

TEST_F(HistogramTest, BasicScopeHistogramMerge) {
  ScopePtr scope1 = store_->createScope("scope1.");

  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  Histogram& h2 = scope1->histogram("h2", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ("h1", h1.name());
  EXPECT_EQ("scope1.h2", h2.name());

  expectCallAndAccumulate(h1, 2);
  expectCallAndAccumulate(h2, 2);
  EXPECT_EQ(2, validateMerge());
}

TEST_F(HistogramTest, BasicHistogramSummaryValidate) {
  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  Histogram& h2 = store_->histogram("h2", Stats::Histogram::Unit::Unspecified);

  expectCallAndAccumulate(h1, 1);

  EXPECT_EQ(2, validateMerge());

  const std::string h1_expected_summary =
      "P0: 1, P25: 1.025, P50: 1.05, P75: 1.075, P90: 1.09, P95: 1.095, "
      "P99: 1.099, P99.5: 1.0995, P99.9: 1.0999, P100: 1.1";
  const std::string h2_expected_summary =
      "P0: 0, P25: 25, P50: 50, P75: 75, P90: 90, P95: 95, P99: 99, "
      "P99.5: 99.5, P99.9: 99.9, P100: 100";

  const std::string h1_expected_buckets =
      "B0.5: 0, B1: 0, B5: 1, B10: 1, B25: 1, B50: 1, B100: 1, B250: 1, "
      "B500: 1, B1000: 1, B2500: 1, B5000: 1, B10000: 1, B30000: 1, B60000: 1, "
      "B300000: 1, B600000: 1, B1.8e+06: 1, B3.6e+06: 1";
  const std::string h2_expected_buckets =
      "B0.5: 1, B1: 1, B5: 5, B10: 10, B25: 25, B50: 50, B100: 100, B250: 100, "
      "B500: 100, B1000: 100, B2500: 100, B5000: 100, B10000: 100, B30000: 100, "
      "B60000: 100, B300000: 100, B600000: 100, B1.8e+06: 100, B3.6e+06: 100";

  for (size_t i = 0; i < 100; ++i) {
    expectCallAndAccumulate(h2, i);
  }

  EXPECT_EQ(2, validateMerge());

  NameHistogramMap name_histogram_map = makeHistogramMap(store_->histograms());
  EXPECT_EQ(h1_expected_summary,
            name_histogram_map["h1"]->cumulativeStatistics().quantileSummary());
  EXPECT_EQ(h2_expected_summary,
            name_histogram_map["h2"]->cumulativeStatistics().quantileSummary());
  EXPECT_EQ(h1_expected_buckets, name_histogram_map["h1"]->cumulativeStatistics().bucketSummary());
  EXPECT_EQ(h2_expected_buckets, name_histogram_map["h2"]->cumulativeStatistics().bucketSummary());
}

// Validates the summary after known value merge in to same histogram.
TEST_F(HistogramTest, BasicHistogramMergeSummary) {
  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);

  for (size_t i = 0; i < 50; ++i) {
    expectCallAndAccumulate(h1, i);
  }
  EXPECT_EQ(1, validateMerge());

  for (size_t i = 50; i < 100; ++i) {
    expectCallAndAccumulate(h1, i);
  }
  EXPECT_EQ(1, validateMerge());

  const std::string expected_summary = "P0: 0, P25: 25, P50: 50, P75: 75, P90: 90, P95: 95, P99: "
                                       "99, P99.5: 99.5, P99.9: 99.9, P100: 100";
  const std::string expected_bucket_summary =
      "B0.5: 1, B1: 1, B5: 5, B10: 10, B25: 25, B50: 50, B100: 100, B250: 100, "
      "B500: 100, B1000: 100, B2500: 100, B5000: 100, B10000: 100, B30000: 100, "
      "B60000: 100, B300000: 100, B600000: 100, B1.8e+06: 100, B3.6e+06: 100";

  NameHistogramMap name_histogram_map = makeHistogramMap(store_->histograms());
  EXPECT_EQ(expected_summary, name_histogram_map["h1"]->cumulativeStatistics().quantileSummary());
  EXPECT_EQ(expected_bucket_summary,
            name_histogram_map["h1"]->cumulativeStatistics().bucketSummary());
}

TEST_F(HistogramTest, BasicHistogramUsed) {
  ScopePtr scope1 = store_->createScope("scope1.");

  Histogram& h1 = store_->histogram("h1", Stats::Histogram::Unit::Unspecified);
  Histogram& h2 = scope1->histogram("h2", Stats::Histogram::Unit::Unspecified);
  EXPECT_EQ("h1", h1.name());
  EXPECT_EQ("scope1.h2", h2.name());

  EXPECT_CALL(sink_, onHistogramComplete(Ref(h1), 1));
  h1.recordValue(1);

  NameHistogramMap name_histogram_map = makeHistogramMap(store_->histograms());
  EXPECT_FALSE(name_histogram_map["h1"]->used());
  EXPECT_FALSE(name_histogram_map["h2"]->used());

  // Merge the histograms and validate that h1 is considered used.
  store_->mergeHistograms([]() -> void {});
  EXPECT_TRUE(name_histogram_map["h1"]->used());

  EXPECT_CALL(sink_, onHistogramComplete(Ref(h2), 2));
  h2.recordValue(2);
  EXPECT_FALSE(name_histogram_map["h2"]->used());

  // Merge histograms again and validate that both h1 and h2 are used.
  store_->mergeHistograms([]() -> void {});

  for (const ParentHistogramSharedPtr& histogram : store_->histograms()) {
    EXPECT_TRUE(histogram->used());
  }
}

TEST_F(HistogramTest, ParentHistogramBucketSummary) {
  ScopePtr scope1 = store_->createScope("scope1.");
  Histogram& histogram = store_->histogram("histogram", Stats::Histogram::Unit::Unspecified);
  store_->mergeHistograms([]() -> void {});
  ASSERT_EQ(1, store_->histograms().size());
  ParentHistogramSharedPtr parent_histogram = store_->histograms()[0];
  EXPECT_EQ("No recorded values", parent_histogram->bucketSummary());

  EXPECT_CALL(sink_, onHistogramComplete(Ref(histogram), 10));
  histogram.recordValue(10);
  store_->mergeHistograms([]() -> void {});
  EXPECT_EQ("B0.5(0,0) B1(0,0) B5(0,0) B10(0,0) B25(1,1) B50(1,1) B100(1,1) "
            "B250(1,1) B500(1,1) B1000(1,1) B2500(1,1) B5000(1,1) B10000(1,1) "
            "B30000(1,1) B60000(1,1) B300000(1,1) B600000(1,1) B1.8e+06(1,1) "
            "B3.6e+06(1,1)",
            parent_histogram->bucketSummary());
}

} // namespace Stats
} // namespace Envoy
