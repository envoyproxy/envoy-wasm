#include "extensions/wasm/metadata_exchange/config.h"
#include "common/common/base64.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {
namespace MetadataExchange {

namespace {

} // namespace

// imports from the low-level API
using Common::Wasm::Null::Plugin::getRequestHeader;
using Common::Wasm::Null::Plugin::addRequestHeader;
using Common::Wasm::Null::Plugin::removeRequestHeader;
using Common::Wasm::Null::Plugin::logInfo;
using Common::Wasm::Null::Plugin::logDebug;

void PluginContext::onConfigure(std::unique_ptr<WasmData> /* configuration */) {
  logInfo("configuring metadata exchange");
  // populate and encode node ID
  id_value_ = "TODO:id value";

  // populate and encode node metadata
  auto metadata = nodeMetadataStruct();
  logInfo("metadata struct " + std::to_string(metadata.fields().size()));
  std::string metadata_bytes;
  metadata.SerializeToString(&metadata_bytes);
  metadata_value_ = Base64::encode(metadata_bytes.data(), metadata_bytes.size());
}

Http::FilterHeadersStatus PluginContext::onRequestHeaders() {
  // strip peer ID header
  auto id_header = getRequestHeader(ExchangeIDHeader);
  if (id_header != nullptr) {
    removeRequestHeader(ExchangeIDHeader);
  }

  // insert peer ID header
  addRequestHeader(ExchangeIDHeader,  id_value_);

  // strip peer metadata struct
  auto metadata_header = getRequestHeader(ExchangeMetadataHeader);
  if (metadata_header != nullptr) {
    removeRequestHeader(ExchangeMetadataHeader);
  }

  // insert peer metadata struct
  logInfo("adding request header" + metadata_value_);
  if (metadata_value_.size() > 0) {
    addRequestHeader(ExchangeMetadataHeader, metadata_value_);
  }

  return Http::FilterHeadersStatus::Continue;
}

static Registry::RegisterFactory<MetadataExchangeFactory, Common::Wasm::Null::NullVmPluginFactory> register_;

} // namespace MetadataExchange
} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
