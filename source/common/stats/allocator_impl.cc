#include "common/stats/allocator_impl.h"

#include <cstdint>

#include "envoy/stats/stats.h"
#include "envoy/stats/symbol_table.h"

#include "common/common/hash.h"
#include "common/common/lock_guard.h"
#include "common/common/logger.h"
#include "common/common/thread.h"
#include "common/common/thread_annotations.h"
#include "common/common/utility.h"
#include "common/stats/metric_impl.h"
#include "common/stats/stat_merger.h"
#include "common/stats/symbol_table_impl.h"

#include "absl/container/flat_hash_set.h"

namespace Envoy {
namespace Stats {

AllocatorImpl::~AllocatorImpl() {
  ASSERT(counters_.empty());
  ASSERT(gauges_.empty());
}

void AllocatorImpl::removeCounterFromSet(Counter* counter) {
  Thread::LockGuard lock(mutex_);
  const size_t count = counters_.erase(counter->statName());
  ASSERT(count == 1);
}

void AllocatorImpl::removeGaugeFromSet(Gauge* gauge) {
  Thread::LockGuard lock(mutex_);
  const size_t count = gauges_.erase(gauge->statName());
  ASSERT(count == 1);
}

#ifndef ENVOY_CONFIG_COVERAGE
void AllocatorImpl::debugPrint() {
  Thread::LockGuard lock(mutex_);
  for (Counter* counter : counters_) {
    ENVOY_LOG_MISC(info, "counter: {}", symbolTable().toString(counter->statName()));
  }
  for (Gauge* gauge : gauges_) {
    ENVOY_LOG_MISC(info, "gauge: {}", symbolTable().toString(gauge->statName()));
  }
}
#endif

// Counter and Gauge both inherit from from RefcountInterface and
// Metric. MetricImpl takes care of most of the Metric API, but we need to cover
// symbolTable() here, which we don't store directly, but get it via the alloc,
// which we need in order to clean up the counter and gauge maps in that class
// when they are destroyed.
//
// We implement the RefcountInterface API, using 16 bits that would otherwise be
// wasted in the alignment padding next to flags_.
template <class BaseClass> class StatsSharedImpl : public MetricImpl<BaseClass> {
public:
  StatsSharedImpl(StatName name, AllocatorImpl& alloc, absl::string_view tag_extracted_name,
                  const std::vector<Tag>& tags)
      : MetricImpl<BaseClass>(name, tag_extracted_name, tags, alloc.symbolTable()), alloc_(alloc) {}

  ~StatsSharedImpl() override {
    // MetricImpl must be explicitly cleared() before destruction, otherwise it
    // will not be able to access the SymbolTable& to free the symbols. An RAII
    // alternative would be to store the SymbolTable reference in the
    // MetricImpl, costing 8 bytes per stat.
    this->clear(symbolTable());
  }

  // Metric
  SymbolTable& symbolTable() override { return alloc_.symbolTable(); }
  bool used() const override { return flags_ & Metric::Flags::Used; }

  // RefcountInterface
  void incRefCount() override { ++ref_count_; }
  bool decRefCount() override {
    ASSERT(ref_count_ >= 1);
    return --ref_count_ == 0;
  }
  uint32_t use_count() const override { return ref_count_; }

protected:
  AllocatorImpl& alloc_;

  // Holds backing store shared by both CounterImpl and GaugeImpl. CounterImpl
  // adds another field, pending_increment_, that is not used in Gauge.
  std::atomic<uint64_t> value_{0};
  std::atomic<uint16_t> flags_{0};
  std::atomic<uint16_t> ref_count_{0};
};

class CounterImpl : public StatsSharedImpl<Counter> {
public:
  CounterImpl(StatName name, AllocatorImpl& alloc, absl::string_view tag_extracted_name,
              const std::vector<Tag>& tags)
      : StatsSharedImpl(name, alloc, tag_extracted_name, tags) {}
  ~CounterImpl() override { alloc_.removeCounterFromSet(this); }

  // Stats::Counter
  void add(uint64_t amount) override {
    // Note that a reader may see a new value but an old pending_increment_ or
    // used(). From a system perspective this should be eventually consistent.
    value_ += amount;
    pending_increment_ += amount;
    flags_ |= Flags::Used;
  }
  void inc() override { add(1); }
  uint64_t latch() override { return pending_increment_.exchange(0); }
  void reset() override { value_ = 0; }
  uint64_t value() const override { return value_; }

private:
  std::atomic<uint64_t> pending_increment_{0};
};

class GaugeImpl : public StatsSharedImpl<Gauge> {
public:
  GaugeImpl(StatName name, AllocatorImpl& alloc, absl::string_view tag_extracted_name,
            const std::vector<Tag>& tags, ImportMode import_mode)
      : StatsSharedImpl(name, alloc, tag_extracted_name, tags) {
    switch (import_mode) {
    case ImportMode::Accumulate:
      flags_ |= Flags::LogicAccumulate;
      break;
    case ImportMode::NeverImport:
      flags_ |= Flags::NeverImport;
      break;
    case ImportMode::Uninitialized:
      // Note that we don't clear any flag bits for import_mode==Uninitialized,
      // as we may have an established import_mode when this stat was created in
      // an alternate scope. See
      // https://github.com/envoyproxy/envoy/issues/7227.
      break;
    }
  }
  ~GaugeImpl() override { alloc_.removeGaugeFromSet(this); }

  // Stats::Gauge
  void add(uint64_t amount) override {
    value_ += amount;
    flags_ |= Flags::Used;
  }
  void dec() override { sub(1); }
  void inc() override { add(1); }
  void set(uint64_t value) override {
    value_ = value;
    flags_ |= Flags::Used;
  }
  void sub(uint64_t amount) override {
    ASSERT(value_ >= amount);
    ASSERT(used() || amount == 0);
    value_ -= amount;
  }
  uint64_t value() const override { return value_; }

  ImportMode importMode() const override {
    if (flags_ & Flags::NeverImport) {
      return ImportMode::NeverImport;
    } else if (flags_ & Flags::LogicAccumulate) {
      return ImportMode::Accumulate;
    }
    return ImportMode::Uninitialized;
  }

  void mergeImportMode(ImportMode import_mode) override {
    ImportMode current = importMode();
    if (current == import_mode) {
      return;
    }

    switch (import_mode) {
    case ImportMode::Uninitialized:
      // mergeImportNode(ImportMode::Uninitialized) is called when merging an
      // existing stat with importMode() == Accumulate or NeverImport.
      break;
    case ImportMode::Accumulate:
      ASSERT(current == ImportMode::Uninitialized);
      flags_ |= Flags::LogicAccumulate;
      break;
    case ImportMode::NeverImport:
      ASSERT(current == ImportMode::Uninitialized);
      // A previous revision of Envoy may have transferred a gauge that it
      // thought was Accumulate. But the new version thinks it's NeverImport, so
      // we clear the accumulated value.
      value_ = 0;
      flags_ &= ~Flags::Used;
      flags_ |= Flags::NeverImport;
      break;
    }
  }
};

CounterSharedPtr AllocatorImpl::makeCounter(StatName name, absl::string_view tag_extracted_name,
                                            const std::vector<Tag>& tags) {
  Thread::LockGuard lock(mutex_);
  ASSERT(gauges_.find(name) == gauges_.end());
  auto iter = counters_.find(name);
  if (iter != counters_.end()) {
    return CounterSharedPtr(*iter);
  }
  auto counter = CounterSharedPtr(new CounterImpl(name, *this, tag_extracted_name, tags));
  counters_.insert(counter.get());
  return counter;
}

GaugeSharedPtr AllocatorImpl::makeGauge(StatName name, absl::string_view tag_extracted_name,
                                        const std::vector<Tag>& tags,
                                        Gauge::ImportMode import_mode) {
  Thread::LockGuard lock(mutex_);
  ASSERT(counters_.find(name) == counters_.end());
  auto iter = gauges_.find(name);
  if (iter != gauges_.end()) {
    return GaugeSharedPtr(*iter);
  }
  auto gauge = GaugeSharedPtr(new GaugeImpl(name, *this, tag_extracted_name, tags, import_mode));
  gauges_.insert(gauge.get());
  return gauge;
}

} // namespace Stats
} // namespace Envoy
