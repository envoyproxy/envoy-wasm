#include "common/stats/thread_local_store.h"

#include <chrono>
#include <cstdint>
#include <list>
#include <memory>
#include <string>

#include "envoy/stats/allocator.h"
#include "envoy/stats/histogram.h"
#include "envoy/stats/sink.h"
#include "envoy/stats/stats.h"

#include "common/common/lock_guard.h"
#include "common/stats/stats_matcher_impl.h"
#include "common/stats/tag_producer_impl.h"

#include "absl/strings/str_join.h"

namespace Envoy {
namespace Stats {

ThreadLocalStoreImpl::ThreadLocalStoreImpl(Allocator& alloc)
    : alloc_(alloc), default_scope_(createScope("")),
      tag_producer_(std::make_unique<TagProducerImpl>()),
      stats_matcher_(std::make_unique<StatsMatcherImpl>()), heap_allocator_(alloc.symbolTable()),
      null_counter_(alloc.symbolTable()), null_gauge_(alloc.symbolTable()),
      null_histogram_(alloc.symbolTable()) {}

ThreadLocalStoreImpl::~ThreadLocalStoreImpl() {
  ASSERT(shutting_down_ || !threading_ever_initialized_);
  default_scope_.reset();
  ASSERT(scopes_.empty());
  for (StatNameStorageSet* rejected_stats : rejected_stats_purgatory_) {
    rejected_stats->free(symbolTable());
    delete rejected_stats;
  }
}

void ThreadLocalStoreImpl::setStatsMatcher(StatsMatcherPtr&& stats_matcher) {
  stats_matcher_ = std::move(stats_matcher);
  if (stats_matcher_->acceptsAll()) {
    return;
  }

  // The Filesystem and potentially other stat-registering objects are
  // constructed prior to the stat-matcher, and those add stats
  // in the default_scope. There should be no requests, so there will
  // be no copies in TLS caches.
  Thread::LockGuard lock(lock_);
  for (ScopeImpl* scope : scopes_) {
    removeRejectedStats(scope->central_cache_.counters_, deleted_counters_);
    removeRejectedStats(scope->central_cache_.gauges_, deleted_gauges_);
    removeRejectedStats(scope->central_cache_.histograms_, deleted_histograms_);
  }
}

template <class StatMapClass, class StatListClass>
void ThreadLocalStoreImpl::removeRejectedStats(StatMapClass& map, StatListClass& list) {
  std::vector<StatName> remove_list;
  for (auto& stat : map) {
    if (rejects(stat.first)) {
      remove_list.push_back(stat.first);
    }
  }
  for (StatName stat_name : remove_list) {
    auto iter = map.find(stat_name);
    ASSERT(iter != map.end());
    list.push_back(iter->second); // Save SharedPtr to the list to avoid invalidating refs to stat.
    map.erase(iter);
  }
}

bool ThreadLocalStoreImpl::rejects(StatName stat_name) const {
  // Don't both elaborating the StatName there are no pattern-based
  // exclusions;/inclusions.
  if (stats_matcher_->acceptsAll()) {
    return false;
  }

  // TODO(ambuc): If stats_matcher_ depends on regexes, this operation (on the
  // hot path) could become prohibitively expensive. Revisit this usage in the
  // future.
  //
  // Also note that the elaboration of the stat-name into a string is expensive,
  // so I think it might be better to move the matcher test until after caching,
  // unless its acceptsAll/rejectsAll.
  return stats_matcher_->rejectsAll() ||
         stats_matcher_->rejects(constSymbolTable().toString(stat_name));
}

std::vector<CounterSharedPtr> ThreadLocalStoreImpl::counters() const {
  // Handle de-dup due to overlapping scopes.
  std::vector<CounterSharedPtr> ret;
  StatNameHashSet names;
  Thread::LockGuard lock(lock_);
  for (ScopeImpl* scope : scopes_) {
    for (auto& counter : scope->central_cache_.counters_) {
      if (names.insert(counter.first).second) {
        ret.push_back(counter.second);
      }
    }
  }

  return ret;
}

ScopePtr ThreadLocalStoreImpl::createScope(const std::string& name) {
  auto new_scope = std::make_unique<ScopeImpl>(*this, name);
  Thread::LockGuard lock(lock_);
  scopes_.emplace(new_scope.get());
  return new_scope;
}

std::vector<GaugeSharedPtr> ThreadLocalStoreImpl::gauges() const {
  // Handle de-dup due to overlapping scopes.
  std::vector<GaugeSharedPtr> ret;
  StatNameHashSet names;
  Thread::LockGuard lock(lock_);
  for (ScopeImpl* scope : scopes_) {
    for (auto& gauge_iter : scope->central_cache_.gauges_) {
      const GaugeSharedPtr& gauge = gauge_iter.second;
      if (gauge->importMode() != Gauge::ImportMode::Uninitialized &&
          names.insert(gauge_iter.first).second) {
        ret.push_back(gauge);
      }
    }
  }

  return ret;
}

std::vector<ParentHistogramSharedPtr> ThreadLocalStoreImpl::histograms() const {
  std::vector<ParentHistogramSharedPtr> ret;
  Thread::LockGuard lock(lock_);
  // TODO(ramaraochavali): As histograms don't share storage, there is a chance of duplicate names
  // here. We need to create global storage for histograms similar to how we have a central storage
  // in shared memory for counters/gauges. In the interim, no de-dup is done here. This may result
  // in histograms with duplicate names, but until shared storage is implemented it's ultimately
  // less confusing for users who have such configs.
  for (ScopeImpl* scope : scopes_) {
    for (const auto& name_histogram_pair : scope->central_cache_.histograms_) {
      const ParentHistogramSharedPtr& parent_hist = name_histogram_pair.second;
      ret.push_back(parent_hist);
    }
  }

  return ret;
}

void ThreadLocalStoreImpl::initializeThreading(Event::Dispatcher& main_thread_dispatcher,
                                               ThreadLocal::Instance& tls) {
  threading_ever_initialized_ = true;
  main_thread_dispatcher_ = &main_thread_dispatcher;
  tls_ = tls.allocateSlot();
  tls_->set([](Event::Dispatcher&) -> ThreadLocal::ThreadLocalObjectSharedPtr {
    return std::make_shared<TlsCache>();
  });
}

void ThreadLocalStoreImpl::shutdownThreading() {
  // This will block both future cache fills as well as cache flushes.
  shutting_down_ = true;
}

void ThreadLocalStoreImpl::mergeHistograms(PostMergeCb merge_complete_cb) {
  if (!shutting_down_) {
    ASSERT(!merge_in_progress_);
    merge_in_progress_ = true;
    tls_->runOnAllThreads(
        [this]() -> void {
          for (const auto& scope : tls_->getTyped<TlsCache>().scope_cache_) {
            const TlsCacheEntry& tls_cache_entry = scope.second;
            for (const auto& name_histogram_pair : tls_cache_entry.histograms_) {
              const TlsHistogramSharedPtr& tls_hist = name_histogram_pair.second;
              tls_hist->beginMerge();
            }
          }
        },
        [this, merge_complete_cb]() -> void { mergeInternal(merge_complete_cb); });
  } else {
    // If server is shutting down, just call the callback to allow flush to continue.
    merge_complete_cb();
  }
}

void ThreadLocalStoreImpl::mergeInternal(PostMergeCb merge_complete_cb) {
  if (!shutting_down_) {
    for (const ParentHistogramSharedPtr& histogram : histograms()) {
      histogram->merge();
    }
    merge_complete_cb();
    merge_in_progress_ = false;
  }
}

void ThreadLocalStoreImpl::releaseScopeCrossThread(ScopeImpl* scope) {
  Thread::ReleasableLockGuard lock(lock_);
  ASSERT(scopes_.count(scope) == 1);
  scopes_.erase(scope);

  // This is called directly from the ScopeImpl destructor, but we can't delay
  // the destruction of scope->central_cache_.central_cache_.rejected_stats_
  // to wait for all the TLS rejected_stats_ caches to be destructed, as those
  // reference elements of SharedStatNameStorageSet. So simply swap out the set
  // contents into a local that we can hold onto until the TLS cache is cleared
  // of all references.
  //
  // We use a raw pointer here as it's easier to capture it in the lambda.
  auto rejected_stats = new StatNameStorageSet;
  rejected_stats->swap(scope->central_cache_.rejected_stats_);

  // This can happen from any thread. We post() back to the main thread which will initiate the
  // cache flush operation.
  if (!shutting_down_ && main_thread_dispatcher_) {
    const uint64_t scope_id = scope->scope_id_;

    // We must delay the cleanup of the rejected stats storage until all the
    // thread-local caches are cleared. This happens by post(), and it's
    // possible that post() will not run, such as when an exception is thrown
    // during startup. To avoid leaking memory and thus failing tests when
    // this occurs, we hold the rejected stats in 'purgatory', so they can
    // be cleared out in the ThreadLocalStoreImpl destructor. We'd prefer
    // to release the memory immediately, however, in which case we remove
    // the rejected stats set from purgatory.
    rejected_stats_purgatory_.insert(rejected_stats);
    auto clean_central_cache = [this, rejected_stats]() {
      {
        Thread::LockGuard lock(lock_);
        rejected_stats_purgatory_.erase(rejected_stats);
      }
      rejected_stats->free(symbolTable());
      delete rejected_stats;
    };
    lock.release();
    main_thread_dispatcher_->post([this, clean_central_cache, scope_id]() {
      clearScopeFromCaches(scope_id, clean_central_cache);
    });
  } else {
    rejected_stats->free(symbolTable());
    delete rejected_stats;
  }
}

void ThreadLocalStoreImpl::clearScopeFromCaches(uint64_t scope_id,
                                                const Event::PostCb& clean_central_cache) {
  // If we are shutting down we no longer perform cache flushes as workers may be shutting down
  // at the same time.
  if (!shutting_down_) {
    // Perform a cache flush on all threads.
    tls_->runOnAllThreads(
        [this, scope_id]() -> void { tls_->getTyped<TlsCache>().scope_cache_.erase(scope_id); },
        clean_central_cache);
  }
}

std::atomic<uint64_t> ThreadLocalStoreImpl::ScopeImpl::next_scope_id_;

ThreadLocalStoreImpl::ScopeImpl::ScopeImpl(ThreadLocalStoreImpl& parent, const std::string& prefix)
    : scope_id_(next_scope_id_++), parent_(parent),
      prefix_(Utility::sanitizeStatsName(prefix), parent.symbolTable()) {}

ThreadLocalStoreImpl::ScopeImpl::~ScopeImpl() {
  parent_.releaseScopeCrossThread(this);
  prefix_.free(symbolTable());
}

// Manages the truncation and tag-extraction of stat names. Tag extraction occurs
// on the original, untruncated name so the extraction can complete properly,
// even if the tag values are partially truncated.
class TagExtraction {
public:
  TagExtraction(ThreadLocalStoreImpl& tls, StatName name) {
    tls.symbolTable().callWithStringView(name, [this, &tls](absl::string_view name_str) {
      tag_extracted_name_ = tls.tagProducer().produceTags(name_str, tags_);
    });
  }

  const std::vector<Tag>& tags() { return tags_; }
  const std::string& tagExtractedName() { return tag_extracted_name_; }

private:
  std::vector<Tag> tags_;
  std::string tag_extracted_name_;
};

bool ThreadLocalStoreImpl::checkAndRememberRejection(StatName name,
                                                     StatNameStorageSet& central_rejected_stats,
                                                     StatNameHashSet* tls_rejected_stats) {
  if (stats_matcher_->acceptsAll()) {
    return false;
  }

  auto iter = central_rejected_stats.find(name);
  const StatNameStorage* rejected_name = nullptr;
  if (iter != central_rejected_stats.end()) {
    rejected_name = &(*iter);
  } else {
    if (rejects(name)) {
      auto insertion = central_rejected_stats.insert(StatNameStorage(name, symbolTable()));
      const StatNameStorage& rejected_name_ref = *(insertion.first);
      rejected_name = &rejected_name_ref;
    }
  }
  if (rejected_name != nullptr) {
    if (tls_rejected_stats != nullptr) {
      tls_rejected_stats->insert(rejected_name->statName());
    }
    return true;
  }
  return false;
}

template <class StatType>
StatType& ThreadLocalStoreImpl::ScopeImpl::safeMakeStat(
    StatName name, StatMap<RefcountPtr<StatType>>& central_cache_map,
    StatNameStorageSet& central_rejected_stats, MakeStatFn<StatType> make_stat,
    StatMap<RefcountPtr<StatType>>* tls_cache, StatNameHashSet* tls_rejected_stats,
    StatType& null_stat) {

  if (tls_rejected_stats != nullptr &&
      tls_rejected_stats->find(name) != tls_rejected_stats->end()) {
    return null_stat;
  }

  // If we have a valid cache entry, return it.
  if (tls_cache) {
    auto pos = tls_cache->find(name);
    if (pos != tls_cache->end()) {
      return *pos->second;
    }
  }

  // We must now look in the central store so we must be locked. We grab a reference to the
  // central store location. It might contain nothing. In this case, we allocate a new stat.
  Thread::LockGuard lock(parent_.lock_);
  auto iter = central_cache_map.find(name);
  RefcountPtr<StatType>* central_ref = nullptr;
  if (iter != central_cache_map.end()) {
    central_ref = &(iter->second);
  } else if (parent_.checkAndRememberRejection(name, central_rejected_stats, tls_rejected_stats)) {
    // Note that again we do the name-rejection lookup on the untruncated name.
    return null_stat;
  } else {
    TagExtraction extraction(parent_, name);
    RefcountPtr<StatType> stat =
        make_stat(parent_.alloc_, name, extraction.tagExtractedName(), extraction.tags());
    ASSERT(stat != nullptr);
    central_ref = &central_cache_map[stat->statName()];
    *central_ref = stat;
  }

  // If we have a TLS cache, insert the stat.
  if (tls_cache) {
    tls_cache->insert(std::make_pair((*central_ref)->statName(), *central_ref));
  }

  // Finally we return the reference.
  return **central_ref;
}

template <class StatType>
absl::optional<std::reference_wrapper<const StatType>>
ThreadLocalStoreImpl::ScopeImpl::findStatLockHeld(
    StatName name, StatMap<RefcountPtr<StatType>>& central_cache_map) const {
  auto iter = central_cache_map.find(name);
  if (iter == central_cache_map.end()) {
    return absl::nullopt;
  }

  return std::cref(*iter->second);
}

Counter& ThreadLocalStoreImpl::ScopeImpl::counterFromStatName(StatName name) {
  if (parent_.rejectsAll()) {
    return parent_.null_counter_;
  }

  // Determine the final name based on the prefix and the passed name.
  //
  // Note that we can do map.find(final_name.c_str()), but we cannot do
  // map[final_name.c_str()] as the char*-keyed maps would then save the pointer
  // to a temporary, and address sanitization errors would follow. Instead we
  // must do a find() first, using the value if it succeeds. If it fails, then
  // after we construct the stat we can insert it into the required maps. This
  // strategy costs an extra hash lookup for each miss, but saves time
  // re-copying the string and significant memory overhead.
  Stats::SymbolTable::StoragePtr final_name = symbolTable().join({prefix_.statName(), name});
  StatName final_stat_name(final_name.get());

  // We now find the TLS cache. This might remain null if we don't have TLS
  // initialized currently.
  StatMap<CounterSharedPtr>* tls_cache = nullptr;
  StatNameHashSet* tls_rejected_stats = nullptr;
  if (!parent_.shutting_down_ && parent_.tls_) {
    TlsCacheEntry& entry = parent_.tls_->getTyped<TlsCache>().scope_cache_[this->scope_id_];
    tls_cache = &entry.counters_;
    tls_rejected_stats = &entry.rejected_stats_;
  }

  return safeMakeStat<Counter>(
      final_stat_name, central_cache_.counters_, central_cache_.rejected_stats_,
      [](Allocator& allocator, StatName name, absl::string_view tag_extracted_name,
         const std::vector<Tag>& tags) -> CounterSharedPtr {
        return allocator.makeCounter(name, tag_extracted_name, tags);
      },
      tls_cache, tls_rejected_stats, parent_.null_counter_);
}

void ThreadLocalStoreImpl::ScopeImpl::deliverHistogramToSinks(const Histogram& histogram,
                                                              uint64_t value) {
  // Thread local deliveries must be blocked outright for histograms and timers during shutdown.
  // This is because the sinks may end up trying to create new connections via the thread local
  // cluster manager which may already be destroyed (there is no way to sequence this because the
  // cluster manager destroying can create deliveries). We special case this explicitly to avoid
  // having to implement a shutdown() method (or similar) on every TLS object.
  if (parent_.shutting_down_) {
    return;
  }

  for (Sink& sink : parent_.timer_sinks_) {
    sink.onHistogramComplete(histogram, value);
  }
}

Gauge& ThreadLocalStoreImpl::ScopeImpl::gaugeFromStatName(StatName name,
                                                          Gauge::ImportMode import_mode) {
  if (parent_.rejectsAll()) {
    return parent_.null_gauge_;
  }

  // See comments in counter(). There is no super clean way (via templates or otherwise) to
  // share this code so I'm leaving it largely duplicated for now.
  //
  // Note that we can do map.find(final_name.c_str()), but we cannot do
  // map[final_name.c_str()] as the char*-keyed maps would then save the pointer to
  // a temporary, and address sanitization errors would follow. Instead we must
  // do a find() first, using that if it succeeds. If it fails, then after we
  // construct the stat we can insert it into the required maps.
  Stats::SymbolTable::StoragePtr final_name = symbolTable().join({prefix_.statName(), name});
  StatName final_stat_name(final_name.get());

  StatMap<GaugeSharedPtr>* tls_cache = nullptr;
  StatNameHashSet* tls_rejected_stats = nullptr;
  if (!parent_.shutting_down_ && parent_.tls_) {
    TlsCacheEntry& entry = parent_.tls_->getTyped<TlsCache>().scope_cache_[this->scope_id_];
    tls_cache = &entry.gauges_;
    tls_rejected_stats = &entry.rejected_stats_;
  }

  Gauge& gauge = safeMakeStat<Gauge>(
      final_stat_name, central_cache_.gauges_, central_cache_.rejected_stats_,
      [import_mode](Allocator& allocator, StatName name, absl::string_view tag_extracted_name,
                    const std::vector<Tag>& tags) -> GaugeSharedPtr {
        return allocator.makeGauge(name, tag_extracted_name, tags, import_mode);
      },
      tls_cache, tls_rejected_stats, parent_.null_gauge_);
  gauge.mergeImportMode(import_mode);
  return gauge;
}

Histogram& ThreadLocalStoreImpl::ScopeImpl::histogramFromStatName(StatName name,
                                                                  Histogram::Unit unit) {
  if (parent_.rejectsAll()) {
    return parent_.null_histogram_;
  }

  // See comments in counter(). There is no super clean way (via templates or otherwise) to
  // share this code so I'm leaving it largely duplicated for now.
  //
  // Note that we can do map.find(final_name.c_str()), but we cannot do
  // map[final_name.c_str()] as the char*-keyed maps would then save the pointer to
  // a temporary, and address sanitization errors would follow. Instead we must
  // do a find() first, using that if it succeeds. If it fails, then after we
  // construct the stat we can insert it into the required maps.
  Stats::SymbolTable::StoragePtr final_name = symbolTable().join({prefix_.statName(), name});
  StatName final_stat_name(final_name.get());

  StatMap<ParentHistogramSharedPtr>* tls_cache = nullptr;
  StatNameHashSet* tls_rejected_stats = nullptr;
  if (!parent_.shutting_down_ && parent_.tls_) {
    TlsCacheEntry& entry = parent_.tls_->getTyped<TlsCache>().scope_cache_[this->scope_id_];
    tls_cache = &entry.parent_histograms_;
    auto iter = tls_cache->find(final_stat_name);
    if (iter != tls_cache->end()) {
      return *iter->second;
    }
    tls_rejected_stats = &entry.rejected_stats_;
    if (tls_rejected_stats->find(final_stat_name) != tls_rejected_stats->end()) {
      return parent_.null_histogram_;
    }
  }

  Thread::LockGuard lock(parent_.lock_);
  auto iter = central_cache_.histograms_.find(final_stat_name);
  ParentHistogramImplSharedPtr* central_ref = nullptr;
  if (iter != central_cache_.histograms_.end()) {
    central_ref = &iter->second;
  } else if (parent_.checkAndRememberRejection(final_stat_name, central_cache_.rejected_stats_,
                                               tls_rejected_stats)) {
    return parent_.null_histogram_;
  } else {
    TagExtraction extraction(parent_, final_stat_name);

    RefcountPtr<ParentHistogramImpl> stat(new ParentHistogramImpl(
        final_stat_name, unit, parent_, *this, extraction.tagExtractedName(), extraction.tags()));
    central_ref = &central_cache_.histograms_[stat->statName()];
    *central_ref = stat;
  }

  if (tls_cache != nullptr) {
    tls_cache->insert(std::make_pair((*central_ref)->statName(), *central_ref));
  }
  return **central_ref;
}

OptionalCounter ThreadLocalStoreImpl::ScopeImpl::findCounter(StatName name) const {
  return findStatLockHeld<Counter>(name, central_cache_.counters_);
}

OptionalGauge ThreadLocalStoreImpl::ScopeImpl::findGauge(StatName name) const {
  return findStatLockHeld<Gauge>(name, central_cache_.gauges_);
}

OptionalHistogram ThreadLocalStoreImpl::ScopeImpl::findHistogram(StatName name) const {
  auto iter = central_cache_.histograms_.find(name);
  if (iter == central_cache_.histograms_.end()) {
    return absl::nullopt;
  }

  RefcountPtr<Histogram> histogram_ref(iter->second);
  return std::cref(*histogram_ref);
}

Histogram& ThreadLocalStoreImpl::ScopeImpl::tlsHistogram(StatName name,
                                                         ParentHistogramImpl& parent) {
  // tlsHistogram() is generally not called for a histogram that is rejected by
  // the matcher, so no further rejection-checking is needed at this level.
  // TlsHistogram inherits its reject/accept status from ParentHistogram.

  // See comments in counterFromStatName() which explains the logic here.

  StatMap<TlsHistogramSharedPtr>* tls_cache = nullptr;
  if (!parent_.shutting_down_ && parent_.tls_) {
    tls_cache = &parent_.tls_->getTyped<TlsCache>().scope_cache_[this->scope_id_].histograms_;
    auto iter = tls_cache->find(name);
    if (iter != tls_cache->end()) {
      return *iter->second;
    }
  }

  std::vector<Tag> tags;
  std::string tag_extracted_name =
      parent_.tagProducer().produceTags(symbolTable().toString(name), tags);
  TlsHistogramSharedPtr hist_tls_ptr(
      new ThreadLocalHistogramImpl(name, parent.unit(), tag_extracted_name, tags, symbolTable()));

  parent.addTlsHistogram(hist_tls_ptr);

  if (tls_cache) {
    tls_cache->insert(std::make_pair(hist_tls_ptr->statName(), hist_tls_ptr));
  }
  return *hist_tls_ptr;
}

ThreadLocalHistogramImpl::ThreadLocalHistogramImpl(StatName name, Histogram::Unit unit,
                                                   const std::string& tag_extracted_name,
                                                   const std::vector<Tag>& tags,
                                                   SymbolTable& symbol_table)
    : HistogramImplHelper(name, tag_extracted_name, tags, symbol_table), unit_(unit),
      current_active_(0), used_(false), created_thread_id_(std::this_thread::get_id()),
      symbol_table_(symbol_table) {
  histograms_[0] = hist_alloc();
  histograms_[1] = hist_alloc();
}

ThreadLocalHistogramImpl::~ThreadLocalHistogramImpl() {
  MetricImpl::clear(symbolTable());
  hist_free(histograms_[0]);
  hist_free(histograms_[1]);
}

void ThreadLocalHistogramImpl::recordValue(uint64_t value) {
  ASSERT(std::this_thread::get_id() == created_thread_id_);
  hist_insert_intscale(histograms_[current_active_], value, 0, 1);
  used_ = true;
}

void ThreadLocalHistogramImpl::merge(histogram_t* target) {
  histogram_t** other_histogram = &histograms_[otherHistogramIndex()];
  hist_accumulate(target, other_histogram, 1);
  hist_clear(*other_histogram);
}

ParentHistogramImpl::ParentHistogramImpl(StatName name, Histogram::Unit unit, Store& parent,
                                         TlsScope& tls_scope, absl::string_view tag_extracted_name,
                                         const std::vector<Tag>& tags)
    : MetricImpl(name, tag_extracted_name, tags, parent.symbolTable()), unit_(unit),
      parent_(parent), tls_scope_(tls_scope), interval_histogram_(hist_alloc()),
      cumulative_histogram_(hist_alloc()), interval_statistics_(interval_histogram_),
      cumulative_statistics_(cumulative_histogram_), merged_(false) {}

ParentHistogramImpl::~ParentHistogramImpl() {
  MetricImpl::clear(symbolTable());
  hist_free(interval_histogram_);
  hist_free(cumulative_histogram_);
}

Histogram::Unit ParentHistogramImpl::unit() const { return unit_; }

void ParentHistogramImpl::recordValue(uint64_t value) {
  Histogram& tls_histogram = tls_scope_.tlsHistogram(statName(), *this);
  tls_histogram.recordValue(value);
  parent_.deliverHistogramToSinks(*this, value);
}

bool ParentHistogramImpl::used() const {
  // Consider ParentHistogram used only if has ever been merged.
  return merged_;
}

void ParentHistogramImpl::merge() {
  Thread::ReleasableLockGuard lock(merge_lock_);
  if (merged_ || usedLockHeld()) {
    hist_clear(interval_histogram_);
    // Here we could copy all the pointers to TLS histograms in the tls_histogram_ list,
    // then release the lock before we do the actual merge. However it is not a big deal
    // because the tls_histogram merge is not that expensive as it is a single histogram
    // merge and adding TLS histograms is rare.
    for (const TlsHistogramSharedPtr& tls_histogram : tls_histograms_) {
      tls_histogram->merge(interval_histogram_);
    }
    // Since TLS merge is done, we can release the lock here.
    lock.release();
    hist_accumulate(cumulative_histogram_, &interval_histogram_, 1);
    cumulative_statistics_.refresh(cumulative_histogram_);
    interval_statistics_.refresh(interval_histogram_);
    merged_ = true;
  }
}

const std::string ParentHistogramImpl::quantileSummary() const {
  if (used()) {
    std::vector<std::string> summary;
    const std::vector<double>& supported_quantiles_ref = interval_statistics_.supportedQuantiles();
    summary.reserve(supported_quantiles_ref.size());
    for (size_t i = 0; i < supported_quantiles_ref.size(); ++i) {
      summary.push_back(fmt::format("P{}({},{})", 100 * supported_quantiles_ref[i],
                                    interval_statistics_.computedQuantiles()[i],
                                    cumulative_statistics_.computedQuantiles()[i]));
    }
    return absl::StrJoin(summary, " ");
  } else {
    return std::string("No recorded values");
  }
}

const std::string ParentHistogramImpl::bucketSummary() const {
  if (used()) {
    std::vector<std::string> bucket_summary;
    const std::vector<double>& supported_buckets = interval_statistics_.supportedBuckets();
    bucket_summary.reserve(supported_buckets.size());
    for (size_t i = 0; i < supported_buckets.size(); ++i) {
      bucket_summary.push_back(fmt::format("B{}({},{})", supported_buckets[i],
                                           interval_statistics_.computedBuckets()[i],
                                           cumulative_statistics_.computedBuckets()[i]));
    }
    return absl::StrJoin(bucket_summary, " ");
  } else {
    return std::string("No recorded values");
  }
}

void ParentHistogramImpl::addTlsHistogram(const TlsHistogramSharedPtr& hist_ptr) {
  Thread::LockGuard lock(merge_lock_);
  tls_histograms_.emplace_back(hist_ptr);
}

bool ParentHistogramImpl::usedLockHeld() const {
  for (const TlsHistogramSharedPtr& tls_histogram : tls_histograms_) {
    if (tls_histogram->used()) {
      return true;
    }
  }
  return false;
}

} // namespace Stats
} // namespace Envoy
