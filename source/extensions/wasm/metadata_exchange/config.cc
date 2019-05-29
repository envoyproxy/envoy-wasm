#include "extensions/wasm/metadata_exchange/config.h"
#include "common/common/base64.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {
namespace MetadataExchange {

// imports from the low-level API
using Common::Wasm::Null::Plugin::logInfo;
using Common::Wasm::Null::Plugin::logDebug;
using Common::Wasm::Null::Plugin::replaceRequestHeader;
using Common::Wasm::Null::Plugin::removeRequestHeader;
using Common::Wasm::Null::Plugin::getRequestHeader;
using Common::Wasm::Null::Plugin::getMetadataValue;
using Common::Wasm::Null::Plugin::proxy_setMetadataStruct;

void PluginContext::onCreate() {
  // TODO: populate and encode node ID

  // populate and encode node metadata
  auto metadata = getMetadataValue(Common::Wasm::MetadataType::Node, NodeMetadataKey);
  switch (metadata.kind_case()) {
  case google::protobuf::Value::kNullValue:
  case google::protobuf::Value::KIND_NOT_SET:
    break;
  default:
    std::string metadata_bytes;
    metadata.SerializeToString(&metadata_bytes);
    metadata_value_ = Base64::encode(metadata_bytes.data(), metadata_bytes.size());
    logInfo("metadata_value_ id:" + std::to_string(id()) + " value:" + metadata_value_);
  }
}

Http::FilterHeadersStatus PluginContext::onRequestHeaders() {
  // strip peer metadata struct and store it as a downstream peer metadata
  auto downstream_metadata_value = getRequestHeader(ExchangeMetadataHeader);
  if (downstream_metadata_value != nullptr && downstream_metadata_value->view().size() > 0) {
    removeRequestHeader(ExchangeMetadataHeader);
    auto downstream_metadata_bytes = Base64::decode(downstream_metadata_value->toString());
    proxy_setMetadataStruct(Common::Wasm::MetadataType::Response,
                            DownstreamMetadataKey, strlen(DownstreamMetadataKey),
                            downstream_metadata_bytes.data(), downstream_metadata_bytes.size());
  }

  // insert peer metadata struct
  if (metadata_value_.size() > 0) {
    replaceRequestHeader(ExchangeMetadataHeader, metadata_value_);
  }

  return Http::FilterHeadersStatus::Continue;
}

static Registry::RegisterFactory<MetadataExchangeFactory, Common::Wasm::Null::NullVmPluginFactory> register_;

} // namespace MetadataExchange
} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
