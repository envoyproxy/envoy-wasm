#include "extensions/filters/network/mongo_proxy/mongo_stats.h"

#include <memory>
#include <string>
#include <vector>

#include "envoy/stats/scope.h"

#include "common/stats/symbol_table_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MongoProxy {

MongoStats::MongoStats(Stats::Scope& scope, const std::string& prefix)
    : scope_(scope), stat_name_set_(scope.symbolTable()), prefix_(stat_name_set_.add(prefix)),
      callsite_(stat_name_set_.add("callsite")), cmd_(stat_name_set_.add("cmd")),
      collection_(stat_name_set_.add("collection")), multi_get_(stat_name_set_.add("multi_get")),
      reply_num_docs_(stat_name_set_.add("reply_num_docs")),
      reply_size_(stat_name_set_.add("reply_size")),
      reply_time_ms_(stat_name_set_.add("reply_time_ms")), time_ms_(stat_name_set_.add("time_ms")),
      query_(stat_name_set_.add("query")), scatter_get_(stat_name_set_.add("scatter_get")),
      total_(stat_name_set_.add("total")) {}

Stats::SymbolTable::StoragePtr MongoStats::addPrefix(const std::vector<Stats::StatName>& names) {
  std::vector<Stats::StatName> names_with_prefix;
  names_with_prefix.reserve(1 + names.size());
  names_with_prefix.push_back(prefix_);
  names_with_prefix.insert(names_with_prefix.end(), names.begin(), names.end());
  return scope_.symbolTable().join(names_with_prefix);
}

void MongoStats::incCounter(const std::vector<Stats::StatName>& names) {
  const Stats::SymbolTable::StoragePtr stat_name_storage = addPrefix(names);
  scope_.counterFromStatName(Stats::StatName(stat_name_storage.get())).inc();
}

void MongoStats::recordHistogram(const std::vector<Stats::StatName>& names, uint64_t sample) {
  const Stats::SymbolTable::StoragePtr stat_name_storage = addPrefix(names);
  scope_.histogramFromStatName(Stats::StatName(stat_name_storage.get())).recordValue(sample);
}

} // namespace MongoProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
