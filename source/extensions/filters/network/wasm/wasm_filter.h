#pragma once

#include <memory>

#include "envoy/config/filter/network/wasm/v2/wasm.pb.validate.h"
#include "envoy/network/filter.h"
#include "envoy/server/filter_config.h"
#include "envoy/upstream/cluster_manager.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/network/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Wasm {

using Envoy::Extensions::Common::Wasm::Context;
using Envoy::Extensions::Common::Wasm::Wasm;

class FilterConfig : public Logger::Loggable<Logger::Id::wasm> {
public:
  FilterConfig(const envoy::config::filter::network::wasm::v2::Wasm& proto_config,
               Server::Configuration::FactoryContext& context);

  std::shared_ptr<Context> getWasmContext() {
    auto& wasm = tls_slot_->getTyped<Wasm>();
    if (!root_context_id_) {
      root_context_id_ = wasm.getRootContext(root_id_)->id();
    }
    return std::make_shared<Context>(&tls_slot_->getTyped<Wasm>(), root_context_id_);
  }

private:
  std::string root_id_;
  uint32_t root_context_id_{0};
  ThreadLocal::SlotPtr tls_slot_;
};

typedef std::shared_ptr<FilterConfig> FilterConfigSharedPtr;

class Filter : public Logger::Loggable<Logger::Id::wasm>,
               public Network::ConnectionCallbacks,
               public Network::Filter {
public:
  Filter(std::shared_ptr<Context> wasm_context) : wasm_context_(wasm_context) {}

  // Network::ConnectionCallbacks
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // Network::ReadFilter
  Network::FilterStatus onNewConnection() override;
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override;

  // Network::WriteFilter
  Network::FilterStatus onWrite(Buffer::Instance& data, bool end_stream) override;
  void initializeWriteFilterCallbacks(Network::WriteFilterCallbacks& callbacks) override;

private:
  Network::ReadFilterCallbacks* read_filter_callbacks_{};
  Network::WriteFilterCallbacks* write_filter_callbacks_{};
  std::shared_ptr<Context> wasm_context_;
};

} // namespace Wasm
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
