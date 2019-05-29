#include "extensions/wasm/metadata_exchange/config.h"
#include "common/common/base64.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {
namespace MetadataExchange {

// imports from the low-level API
using Common::Wasm::Null::Plugin::logInfo;
using Common::Wasm::Null::Plugin::logDebug;
using Common::Wasm::Null::Plugin::addRequestHeader;
using Common::Wasm::Null::Plugin::removeRequestHeader;
using Common::Wasm::Null::Plugin::getRequestHeader;
using Common::Wasm::Null::Plugin::getMetadataValue;

void PluginContext::onConfigure(std::unique_ptr<WasmData> /* configuration */) {
  // TODO: populate and encode node ID

  // populate and encode node metadata
  auto metadata = getMetadataValue(Common::Wasm::MetadataType::Node, MetadataKey);
  switch (metadata.kind_case()) {
  case google::protobuf::Value::kNullValue:
  case google::protobuf::Value::KIND_NOT_SET:
    break;
  default:
    std::string metadata_bytes;
    metadata.SerializeToString(&metadata_bytes);
    metadata_value_ = Base64::encode(metadata_bytes.data(), metadata_bytes.size());
    logInfo("metadata_value: " + std::to_string(id()) + " " + metadata_value_);
  }
}

Http::FilterHeadersStatus PluginContext::onRequestHeaders() {
  logInfo("onRequestHeaders metadata_value: "  + std::to_string(id()) + " " + metadata_value_);
  // strip peer metadata struct and store it as a downstream peer metadata
  auto downstream_metadata_value = getRequestHeader(ExchangeMetadataHeader);
  if (downstream_metadata_value != nullptr) {
    removeRequestHeader(ExchangeMetadataHeader);
    auto downstream_metadata_bytes = Base64::decode(downstream_metadata_value->toString());
    google::protobuf::Value downstream_metadata;
    downstream_metadata.ParseFromString(downstream_metadata_bytes);

    logInfo("downstream_metadata: " + downstream_metadata.DebugString());
  }

  // insert peer metadata struct
  logInfo("adding request header: " + metadata_value_);
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
