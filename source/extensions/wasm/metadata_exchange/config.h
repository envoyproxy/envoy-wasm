#pragma once

#include "extensions/common/wasm/null/null.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {
namespace MetadataExchange {

// Node ID is expected to be a primary key for the node metadata
constexpr char ExchangeIDHeader[] = "x-envoy-peer-id";
constexpr char ExchangeMetadataHeader[] = "x-envoy-peer-metadata";

// NodeMetadata key is the key in the node metadata struct that is passed between peers.
constexpr char NodeMetadataKey[] = "istio.io/metadata";

// DownstreamMetadataKey is the key in the request metadata for downstream peer metadata
constexpr char DownstreamMetadataKey[] = "envoy.wasm.metadata_exchange.downstream";

using Common::Wasm::Null::Plugin::Context;

// TODO(kuat) Using h2 METADATA frames:
// - requires h2 in Envoy
// - requires ACCEPTS_METADATA h2 setting
// - not subject to flow control
// - stream ID 0 allows attaching connection level metadata (not supported yet)

class PluginContext : public Context {
public:
  explicit PluginContext(uint32_t id) : Context(id) {}

  void onCreate() override;
  Http::FilterHeadersStatus onRequestHeaders() override;

private:
  // pre-populated serialized values for passing to peers
  std::string id_value_;
  std::string metadata_value_;
};

std::unique_ptr<Context> NewContext(uint32_t id) {
  return std::unique_ptr<Context>(new PluginContext(id));
}

class MetadataExchangeFactory : public Common::Wasm::Null::NullVmPluginFactory {
public:
  const std::string name() const override { return "envoy.wasm.metadata_exchange"; }
  std::unique_ptr<Common::Wasm::Null::NullVmPlugin> create() const override {
    return std::make_unique<Common::Wasm::Null::NullVmPlugin>(
        &NewContext);
  }
};

} // namespace MetadataExchange
} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
