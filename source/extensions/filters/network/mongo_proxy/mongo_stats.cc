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

MongoStats::MongoStats(Stats::Scope& scope, absl::string_view prefix)
    : scope_(scope), stat_name_set_(scope.symbolTable().makeSet("Mongo")),
      prefix_(stat_name_set_->add(prefix)), callsite_(stat_name_set_->add("callsite")),
      cmd_(stat_name_set_->add("cmd")), collection_(stat_name_set_->add("collection")),
      multi_get_(stat_name_set_->add("multi_get")),
      reply_num_docs_(stat_name_set_->add("reply_num_docs")),
      reply_size_(stat_name_set_->add("reply_size")),
      reply_time_ms_(stat_name_set_->add("reply_time_ms")),
      time_ms_(stat_name_set_->add("time_ms")), query_(stat_name_set_->add("query")),
      scatter_get_(stat_name_set_->add("scatter_get")), total_(stat_name_set_->add("total")),
      unknown_command_(stat_name_set_->add("unknown_command")) {

  // TODO(jmarantz): is this the right set of mongo commands to use as builtins?
  // Should we also have builtins for callsites or collections, or do those need
  // to be dynamic?
  stat_name_set_->rememberBuiltins({"insert", "query", "update", "delete"});
}

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

void MongoStats::recordHistogram(const std::vector<Stats::StatName>& names,
                                 Stats::Histogram::Unit unit, uint64_t sample) {
  const Stats::SymbolTable::StoragePtr stat_name_storage = addPrefix(names);
  scope_.histogramFromStatName(Stats::StatName(stat_name_storage.get()), unit).recordValue(sample);
}

} // namespace MongoProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
