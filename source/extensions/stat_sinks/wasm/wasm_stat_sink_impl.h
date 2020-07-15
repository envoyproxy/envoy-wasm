#pragma once

#include "envoy/common/platform.h"
#include "envoy/local_info/local_info.h"
#include "envoy/network/connection.h"
#include "envoy/stats/histogram.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/sink.h"
#include "envoy/stats/stats.h"
#include "envoy/stats/tag.h"
#include "envoy/thread_local/thread_local.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/macros.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/stat_sinks/well_known_names.h"

#include "absl/strings/str_join.h"

namespace Envoy {
namespace Extensions {
namespace StatSinks {
namespace Wasm {

using Envoy::Extensions::Common::Wasm::WasmHandle;

class WasmStatSink : public Stats::Sink {
public:
  WasmStatSink(absl::string_view root_id, Common::Wasm::WasmHandleSharedPtr singleton)
      : root_id_(root_id), singleton_(std::move(singleton)) {}

  void flush(Stats::MetricSnapshot& snapshot) override {
    singleton_->wasm()->onStat(root_id_, snapshot);
  }

  void setSingleton(Common::Wasm::WasmHandleSharedPtr singleton) {
    ASSERT(singleton == nullptr);
    singleton_ = std::move(singleton);
  }

  void onHistogramComplete(const Stats::Histogram& histogram, uint64_t value) override {
    (void)histogram;
    (void)value;
  }

private:
  std::string root_id_;
  Common::Wasm::WasmHandleSharedPtr singleton_;
};

} // namespace Wasm
} // namespace StatSinks
} // namespace Extensions
} // namespace Envoy
