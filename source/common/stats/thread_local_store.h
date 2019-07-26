#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <list>
#include <string>

#include "envoy/thread_local/thread_local.h"

#include "common/common/hash.h"
#include "common/stats/allocator_impl.h"
#include "common/stats/histogram_impl.h"
#include "common/stats/null_counter.h"
#include "common/stats/null_gauge.h"
#include "common/stats/symbol_table_impl.h"
#include "common/stats/utility.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "circllhist.h"

namespace Envoy {
namespace Stats {

/**
 * A histogram that is stored in TLS and used to record values per thread. This holds two
 * histograms, one to collect the values and other as backup that is used for merge process. The
 * swap happens during the merge process.
 */
class ThreadLocalHistogramImpl : public HistogramImplHelper {
public:
  ThreadLocalHistogramImpl(StatName name, const std::string& tag_extracted_name,
                           const std::vector<Tag>& tags, SymbolTable& symbol_table);
  ~ThreadLocalHistogramImpl() override;

  void merge(histogram_t* target);

  /**
   * Called in the beginning of merge process. Swaps the histogram used for collection so that we do
   * not have to lock the histogram in high throughput TLS writes.
   */
  void beginMerge() {
    // This switches the current_active_ between 1 and 0.
    ASSERT(std::this_thread::get_id() == created_thread_id_);
    current_active_ = otherHistogramIndex();
  }

  // Stats::Histogram
  void recordValue(uint64_t value) override;
  bool used() const override { return used_; }

  // Stats::Metric
  SymbolTable& symbolTable() override { return symbol_table_; }

private:
  uint64_t otherHistogramIndex() const { return 1 - current_active_; }
  uint64_t current_active_;
  histogram_t* histograms_[2];
  std::atomic<bool> used_;
  std::thread::id created_thread_id_;
  SymbolTable& symbol_table_;
};

using TlsHistogramSharedPtr = RefcountPtr<ThreadLocalHistogramImpl>;

class TlsScope;

/**
 * Log Linear Histogram implementation that is stored in the main thread.
 */
class ParentHistogramImpl : public MetricImpl<ParentHistogram> {
public:
  ParentHistogramImpl(StatName name, Store& parent, TlsScope& tlsScope,
                      absl::string_view tag_extracted_name, const std::vector<Tag>& tags);
  ~ParentHistogramImpl() override;

  void addTlsHistogram(const TlsHistogramSharedPtr& hist_ptr);
  void recordValue(uint64_t value) override;

  /**
   * This method is called during the main stats flush process for each of the histograms. It
   * iterates through the TLS histograms and collects the histogram data of all of them
   * in to "interval_histogram". Then the collected "interval_histogram" is merged to a
   * "cumulative_histogram".
   */
  void merge() override;

  const HistogramStatistics& intervalStatistics() const override { return interval_statistics_; }
  const HistogramStatistics& cumulativeStatistics() const override {
    return cumulative_statistics_;
  }
  const std::string quantileSummary() const override;
  const std::string bucketSummary() const override;

  // Stats::Metric
  SymbolTable& symbolTable() override { return parent_.symbolTable(); }
  bool used() const override;

  // RefcountInterface
  void incRefCount() override { refcount_helper_.incRefCount(); }
  bool decRefCount() override { return refcount_helper_.decRefCount(); }
  uint32_t use_count() const override { return refcount_helper_.use_count(); }

private:
  bool usedLockHeld() const EXCLUSIVE_LOCKS_REQUIRED(merge_lock_);

  Store& parent_;
  TlsScope& tls_scope_;
  histogram_t* interval_histogram_;
  histogram_t* cumulative_histogram_;
  HistogramStatisticsImpl interval_statistics_;
  HistogramStatisticsImpl cumulative_statistics_;
  mutable Thread::MutexBasicLockable merge_lock_;
  std::list<TlsHistogramSharedPtr> tls_histograms_ GUARDED_BY(merge_lock_);
  bool merged_;
  RefcountHelper refcount_helper_;
};

using ParentHistogramImplSharedPtr = RefcountPtr<ParentHistogramImpl>;

/**
 * Class used to create ThreadLocalHistogram in the scope.
 */
class TlsScope : public Scope {
public:
  ~TlsScope() override = default;

  // TODO(ramaraochavali): Allow direct TLS access for the advanced consumers.
  /**
   * @return a ThreadLocalHistogram within the scope's namespace.
   * @param name name of the histogram with scope prefix attached.
   */
  virtual Histogram& tlsHistogram(StatName name, ParentHistogramImpl& parent) PURE;
};

/**
 * Store implementation with thread local caching. For design details see
 * https://github.com/envoyproxy/envoy/blob/master/source/docs/stats.md
 */
class ThreadLocalStoreImpl : Logger::Loggable<Logger::Id::stats>, public StoreRoot {
public:
  ThreadLocalStoreImpl(Allocator& alloc);
  ~ThreadLocalStoreImpl() override;

  // Stats::Scope
  Counter& counterFromStatName(StatName name) override {
    return default_scope_->counterFromStatName(name);
  }
  Counter& counter(const std::string& name) override { return default_scope_->counter(name); }
  ScopePtr createScope(const std::string& name) override;
  void deliverHistogramToSinks(const Histogram& histogram, uint64_t value) override {
    return default_scope_->deliverHistogramToSinks(histogram, value);
  }
  Gauge& gaugeFromStatName(StatName name, Gauge::ImportMode import_mode) override {
    return default_scope_->gaugeFromStatName(name, import_mode);
  }
  Gauge& gauge(const std::string& name, Gauge::ImportMode import_mode) override {
    return default_scope_->gauge(name, import_mode);
  }
  Histogram& histogramFromStatName(StatName name) override {
    return default_scope_->histogramFromStatName(name);
  }
  Histogram& histogram(const std::string& name) override { return default_scope_->histogram(name); }
  NullGaugeImpl& nullGauge(const std::string&) override { return null_gauge_; }
  const SymbolTable& constSymbolTable() const override { return alloc_.constSymbolTable(); }
  SymbolTable& symbolTable() override { return alloc_.symbolTable(); }
  const TagProducer& tagProducer() const { return *tag_producer_; }
  OptionalCounter findCounter(StatName name) const override {
    OptionalCounter found_counter;
    Thread::LockGuard lock(lock_);
    for (ScopeImpl* scope : scopes_) {
      found_counter = scope->findCounter(name);
      if (found_counter.has_value()) {
        return found_counter;
      }
    }
    return absl::nullopt;
  }
  OptionalGauge findGauge(StatName name) const override {
    OptionalGauge found_gauge;
    Thread::LockGuard lock(lock_);
    for (ScopeImpl* scope : scopes_) {
      found_gauge = scope->findGauge(name);
      if (found_gauge.has_value()) {
        return found_gauge;
      }
    }
    return absl::nullopt;
  }
  OptionalHistogram findHistogram(StatName name) const override {
    OptionalHistogram found_histogram;
    Thread::LockGuard lock(lock_);
    for (ScopeImpl* scope : scopes_) {
      found_histogram = scope->findHistogram(name);
      if (found_histogram.has_value()) {
        return found_histogram;
      }
    }
    return absl::nullopt;
  }
  // Stats::Store
  std::vector<CounterSharedPtr> counters() const override;
  std::vector<GaugeSharedPtr> gauges() const override;
  std::vector<ParentHistogramSharedPtr> histograms() const override;

  // Stats::StoreRoot
  void addSink(Sink& sink) override { timer_sinks_.push_back(sink); }
  void setTagProducer(TagProducerPtr&& tag_producer) override {
    tag_producer_ = std::move(tag_producer);
  }
  void setStatsMatcher(StatsMatcherPtr&& stats_matcher) override;
  void initializeThreading(Event::Dispatcher& main_thread_dispatcher,
                           ThreadLocal::Instance& tls) override;
  void shutdownThreading() override;
  void mergeHistograms(PostMergeCb mergeCb) override;

private:
  template <class Stat> using StatMap = StatNameHashMap<Stat>;

  struct TlsCacheEntry {
    StatMap<CounterSharedPtr> counters_;
    StatMap<GaugeSharedPtr> gauges_;
    StatMap<TlsHistogramSharedPtr> histograms_;
    StatMap<ParentHistogramSharedPtr> parent_histograms_;

    // We keep a TLS cache of rejected stat names. This costs memory, but
    // reduces runtime overhead running the matcher. Moreover, once symbol
    // tables are integrated, rejection will need the fully elaborated string,
    // and it we need to take a global symbol-table lock to run. We keep this
    // StatName set here in the TLS cache to avoid taking a lock to compute
    // rejection.
    StatNameHashSet rejected_stats_;
  };

  struct CentralCacheEntry {
    StatMap<CounterSharedPtr> counters_;
    StatMap<GaugeSharedPtr> gauges_;
    StatMap<ParentHistogramImplSharedPtr> histograms_;
    StatNameStorageSet rejected_stats_;
  };

  struct ScopeImpl : public TlsScope {
    ScopeImpl(ThreadLocalStoreImpl& parent, const std::string& prefix);
    ~ScopeImpl() override;

    // Stats::Scope
    Counter& counterFromStatName(StatName name) override;
    void deliverHistogramToSinks(const Histogram& histogram, uint64_t value) override;
    Gauge& gaugeFromStatName(StatName name, Gauge::ImportMode import_mode) override;
    Histogram& histogramFromStatName(StatName name) override;
    Histogram& tlsHistogram(StatName name, ParentHistogramImpl& parent) override;
    ScopePtr createScope(const std::string& name) override {
      return parent_.createScope(symbolTable().toString(prefix_.statName()) + "." + name);
    }
    const SymbolTable& constSymbolTable() const override { return parent_.constSymbolTable(); }
    SymbolTable& symbolTable() override { return parent_.symbolTable(); }

    Counter& counter(const std::string& name) override {
      StatNameManagedStorage storage(name, symbolTable());
      return counterFromStatName(storage.statName());
    }
    Gauge& gauge(const std::string& name, Gauge::ImportMode import_mode) override {
      StatNameManagedStorage storage(name, symbolTable());
      return gaugeFromStatName(storage.statName(), import_mode);
    }
    Histogram& histogram(const std::string& name) override {
      StatNameManagedStorage storage(name, symbolTable());
      return histogramFromStatName(storage.statName());
    }

    NullGaugeImpl& nullGauge(const std::string&) override { return parent_.null_gauge_; }

    // NOTE: The find methods assume that `name` is fully-qualified.
    // Implementations will not add the scope prefix.
    OptionalCounter findCounter(StatName name) const override;
    OptionalGauge findGauge(StatName name) const override;
    OptionalHistogram findHistogram(StatName name) const override;

    template <class StatType>
    using MakeStatFn = std::function<RefcountPtr<StatType>(Allocator&, StatName name,
                                                           absl::string_view tag_extracted_name,
                                                           const std::vector<Tag>& tags)>;

    /**
     * Makes a stat either by looking it up in the central cache,
     * generating it from the parent allocator, or as a last
     * result, creating it with the heap allocator.
     *
     * @param name the full name of the stat (not tag extracted).
     * @param central_cache_map a map from name to the desired object in the central cache.
     * @param make_stat a function to generate the stat object, called if it's not in cache.
     * @param tls_ref possibly null reference to a cache entry for this stat, which will be
     *     used if non-empty, or filled in if empty (and non-null).
     */
    template <class StatType>
    StatType& safeMakeStat(StatName name, StatMap<RefcountPtr<StatType>>& central_cache_map,
                           StatNameStorageSet& central_rejected_stats,
                           MakeStatFn<StatType> make_stat,
                           StatMap<RefcountPtr<StatType>>* tls_cache,
                           StatNameHashSet* tls_rejected_stats, StatType& null_stat);

    /**
     * Looks up an existing stat, populating the local cache if necessary. Does
     * not check the TLS or rejects, and does not create a stat if it does not
     * exist.
     *
     * @param name the full name of the stat (not tag extracted).
     * @param central_cache_map a map from name to the desired object in the central cache.
     * @return a reference to the stat, if it exists.
     */
    template <class StatType>
    absl::optional<std::reference_wrapper<const StatType>>
    findStatLockHeld(StatName name, StatMap<RefcountPtr<StatType>>& central_cache_map) const;

    void extractTagsAndTruncate(StatName& name,
                                std::unique_ptr<StatNameManagedStorage>& truncated_name_storage,
                                std::vector<Tag>& tags, std::string& tag_extracted_name);

    static std::atomic<uint64_t> next_scope_id_;

    const uint64_t scope_id_;
    ThreadLocalStoreImpl& parent_;
    StatNameStorage prefix_;
    mutable CentralCacheEntry central_cache_;
  };

  struct TlsCache : public ThreadLocal::ThreadLocalObject {
    // The TLS scope cache is keyed by scope ID. This is used to avoid complex circular references
    // during scope destruction. An ID is required vs. using the address of the scope pointer
    // because it's possible that the memory allocator will recycle the scope pointer immediately
    // upon destruction, leading to a situation in which a new scope with the same address is used
    // to reference the cache, and then subsequently cache flushed, leaving nothing in the central
    // store. See the overview for more information. This complexity is required for lockless
    // operation in the fast path.
    absl::flat_hash_map<uint64_t, TlsCacheEntry> scope_cache_;
  };

  std::string getTagsForName(const std::string& name, std::vector<Tag>& tags) const;
  void clearScopeFromCaches(uint64_t scope_id, const Event::PostCb& clean_central_cache);
  void releaseScopeCrossThread(ScopeImpl* scope);
  void mergeInternal(PostMergeCb mergeCb);
  bool rejects(StatName name) const;
  bool rejectsAll() const { return stats_matcher_->rejectsAll(); }
  template <class StatMapClass, class StatListClass>
  void removeRejectedStats(StatMapClass& map, StatListClass& list);
  bool checkAndRememberRejection(StatName name, StatNameStorageSet& central_rejected_stats,
                                 StatNameHashSet* tls_rejected_stats);

  Allocator& alloc_;
  Event::Dispatcher* main_thread_dispatcher_{};
  ThreadLocal::SlotPtr tls_;
  mutable Thread::MutexBasicLockable lock_;
  absl::flat_hash_set<ScopeImpl*> scopes_ GUARDED_BY(lock_);
  ScopePtr default_scope_;
  std::list<std::reference_wrapper<Sink>> timer_sinks_;
  TagProducerPtr tag_producer_;
  StatsMatcherPtr stats_matcher_;
  std::atomic<bool> threading_ever_initialized_{};
  std::atomic<bool> shutting_down_{};
  std::atomic<bool> merge_in_progress_{};
  AllocatorImpl heap_allocator_;

  NullCounterImpl null_counter_;
  NullGaugeImpl null_gauge_;
  NullHistogramImpl null_histogram_;

  // Retain storage for deleted stats; these are no longer in maps because the
  // matcher-pattern was established after they were created. Since the stats
  // are held by reference in code that expects them to be there, we can't
  // actually delete the stats.
  //
  // It seems like it would be better to have each client that expects a stat
  // to exist to hold it as (e.g.) a CounterSharedPtr rather than a Counter&
  // but that would be fairly complex to change.
  std::vector<CounterSharedPtr> deleted_counters_;
  std::vector<GaugeSharedPtr> deleted_gauges_;
  std::vector<HistogramSharedPtr> deleted_histograms_;

  absl::flat_hash_set<StatNameStorageSet*> rejected_stats_purgatory_ GUARDED_BY(lock_);
};

} // namespace Stats
} // namespace Envoy
