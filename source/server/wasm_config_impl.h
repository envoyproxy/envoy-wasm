#pragma once

#include "envoy/server/wasm_config.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContextImpl : public WasmFactoryContext {
public:
  WasmFactoryContextImpl(Event::Dispatcher& dispatcher, Api::Api& api)
      : dispatcher_(dispatcher), api_(api) {}

  Event::Dispatcher& dispatcher() override { return dispatcher_; }

  Api::Api& api() override { return api_; }

private:
  Event::Dispatcher& dispatcher_;
  Api::Api& api_;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
