#pragma once

#include <memory>

#include "envoy/config/filter/http/wasm/v2/wasm.pb.validate.h"
#include "envoy/http/filter.h"
#include "envoy/server/filter_config.h"
#include "envoy/upstream/cluster_manager.h"

#include "extensions/common/wasm/wasm.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Wasm {

using Envoy::Extensions::Common::Wasm::Context;
using Envoy::Extensions::Common::Wasm::Wasm;

class FilterConfig : Logger::Loggable<Logger::Id::wasm> {
public:
  FilterConfig(const envoy::config::filter::http::wasm::v2::Wasm& proto_config,
               Server::Configuration::FactoryContext& context);

  std::shared_ptr<Context> createFilter() { return tls_slot_->getTyped<Wasm>().createContext(); }

private:
  ThreadLocal::SlotPtr tls_slot_;
};

typedef std::shared_ptr<FilterConfig> FilterConfigSharedPtr;

} // namespace Wasm
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
