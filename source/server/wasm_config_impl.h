#pragma once

#include "envoy/server/wasm_config.h"
#include "envoy/thread_local/thread_local.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContextImpl : public WasmFactoryContext {
public:
  WasmFactoryContextImpl(Event::Dispatcher& dispatcher, ThreadLocal::SlotAllocator& tls, Api::Api& api) : dispatcher_(dispatcher), tls_(tls), api_(api) {}

  Event::Dispatcher& dispatcher() override { return dispatcher_; }
  ThreadLocal::SlotAllocator& threadLocal() override { return tls_; }
  Api::Api& api() override { return api_; }

private:
  Event::Dispatcher& dispatcher_;
  ThreadLocal::SlotAllocator& tls_;
  Api::Api& api_;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
