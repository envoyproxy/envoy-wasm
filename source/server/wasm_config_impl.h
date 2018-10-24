#pragma once

#include "envoy/server/wasm_config.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContextImpl : public WasmFactoryContext {
public:
  WasmFactoryContextImpl(Event::Dispatcher& dispatcher) : dispatcher_(dispatcher) {}

  Event::Dispatcher& dispatcher() override { return dispatcher_; }

private:
  Event::Dispatcher& dispatcher_;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
