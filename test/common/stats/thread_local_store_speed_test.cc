// Note: this should be run with --compilation_mode=opt, and would benefit from a
// quiescent system with disabled cstate power management.

#include "common/common/logger.h"
#include "common/common/thread.h"
#include "common/event/dispatcher_impl.h"
#include "common/stats/allocator_impl.h"
#include "common/stats/fake_symbol_table_impl.h"
#include "common/stats/tag_producer_impl.h"
#include "common/stats/thread_local_store.h"
#include "common/thread_local/thread_local_impl.h"

#include "test/common/stats/stat_test_utility.h"
#include "test/test_common/simulated_time_system.h"
#include "test/test_common/test_time.h"
#include "test/test_common/utility.h"

#include "benchmark/benchmark.h"

namespace Envoy {

class ThreadLocalStorePerf {
public:
  ThreadLocalStorePerf()
      : symbol_table_(Stats::SymbolTableCreator::makeSymbolTable()), heap_alloc_(*symbol_table_),
        store_(heap_alloc_), api_(Api::createApiForTest(store_, time_system_)) {
    store_.setTagProducer(std::make_unique<Stats::TagProducerImpl>(stats_config_));

    Stats::TestUtil::forEachSampleStat(1000, [this](absl::string_view name) {
      stat_names_.push_back(std::make_unique<Stats::StatNameStorage>(name, *symbol_table_));
    });
  }

  ~ThreadLocalStorePerf() {
    for (auto& stat_name_storage : stat_names_) {
      stat_name_storage->free(*symbol_table_);
    }
    store_.shutdownThreading();
    if (tls_) {
      tls_->shutdownGlobalThreading();
    }
  }

  void accessCounters() {
    for (auto& stat_name_storage : stat_names_) {
      store_.counterFromStatName(stat_name_storage->statName());
    }
  }

  void initThreading() {
    dispatcher_ = api_->allocateDispatcher();
    tls_ = std::make_unique<ThreadLocal::InstanceImpl>();
    store_.initializeThreading(*dispatcher_, *tls_);
  }

private:
  Stats::SymbolTablePtr symbol_table_;
  Event::SimulatedTimeSystem time_system_;
  Stats::AllocatorImpl heap_alloc_;
  Stats::ThreadLocalStoreImpl store_;
  Api::ApiPtr api_;
  Event::DispatcherPtr dispatcher_;
  std::unique_ptr<ThreadLocal::InstanceImpl> tls_;
  envoy::config::metrics::v2::StatsConfig stats_config_;
  std::vector<std::unique_ptr<Stats::StatNameStorage>> stat_names_;
};

} // namespace Envoy

// Tests the single-threaded performance of the thread-local-store stats caches
// without having initialized tls.
static void BM_StatsNoTls(benchmark::State& state) {
  Envoy::ThreadLocalStorePerf context;

  for (auto _ : state) {
    context.accessCounters();
  }
}
BENCHMARK(BM_StatsNoTls);

// Tests the single-threaded performance of the thread-local-store stats caches
// with tls. Note that this test is still single-threaded, and so there's only
// one replica of the tls cache.
static void BM_StatsWithTls(benchmark::State& state) {
  Envoy::ThreadLocalStorePerf context;
  context.initThreading();

  for (auto _ : state) {
    context.accessCounters();
  }
}
BENCHMARK(BM_StatsWithTls);

// TODO(jmarantz): add multi-threaded variant of this test, that aggressively
// looks up stats in multiple threads to try to trigger contention issues.

// Boilerplate main(), which discovers benchmarks in the same file and runs them.
int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);

  Envoy::Event::Libevent::Global::initialize();
  if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }
  benchmark::RunSpecifiedBenchmarks();
}
