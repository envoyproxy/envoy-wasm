#pragma once

#include "envoy/server/wasm_config.h"
#include "envoy/thread_local/thread_local.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class WasmFactoryContextImpl : public WasmFactoryContext {
public:
  WasmFactoryContextImpl(Server::Instance& server, Stats::ScopeSharedPtr scope)
      : server_(server), scope_(scope) {}

  Server::Instance& server() override { return server_; }
  Stats::ScopeSharedPtr& scope() override { return scope_; }

private:
  Server::Instance& server_;
  Stats::ScopeSharedPtr scope_;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
