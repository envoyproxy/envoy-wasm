// NOLINT(namespace-envoy)
#include <string>

#include "proxy_wasm_intrinsics.h"

class ExampleContext : public Context {
public:
  explicit ExampleContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterStatus onNewConnection() override;
  FilterStatus onDownstreamData(size_t data_length, bool end_stream) override;
  FilterStatus onUpstreamData(size_t data_length, bool end_stream) override;
  void onDownstreamConnectionClose(PeerType peer_type) override;
  void onUpstreamConnectionClose(PeerType peer_type) override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext));

FilterStatus ExampleContext::onNewConnection() {
  logTrace("onNewConnection " + std::to_string(id()));
  return FilterStatus::Continue;
}

FilterStatus ExampleContext::onDownstreamData(size_t data_length, bool end_stream) {
  WasmDataPtr data = getBufferBytes(BufferType::NetworkDownstreamData, 0, data_length);
  logTrace("onDownstreamData " + std::to_string(id()) + " len=" + std::to_string(data_length) +
           " end_stream=" + std::to_string(end_stream) + "\n" + std::string(data->view()));
  return FilterStatus::Continue;
}

FilterStatus ExampleContext::onUpstreamData(size_t data_length, bool end_stream) {
  WasmDataPtr data = getBufferBytes(BufferType::NetworkUpstreamData, 0, data_length);
  logTrace("onUpstreamData " + std::to_string(id()) + " len=" + std::to_string(data_length) +
           " end_stream=" + std::to_string(end_stream) + "\n" + std::string(data->view()));
  return FilterStatus::Continue;
}

void ExampleContext::onDownstreamConnectionClose(PeerType peer_type) {
  logTrace("onDownstreamConnectionClose " + std::to_string(id()) + " " +
           std::to_string(static_cast<uint32_t>(peer_type)));
}

void ExampleContext::onUpstreamConnectionClose(PeerType peer_type) {
  logTrace("onUpstreamConnectionClose " + std::to_string(id()) + " " +
           std::to_string(static_cast<uint32_t>(peer_type)));
}
