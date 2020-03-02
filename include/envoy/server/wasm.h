#pragma once

#include <memory>

#include "envoy/common/exception.h"
#include "envoy/common/pure.h"
#include "envoy/event/dispatcher.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/logger.h"

namespace Envoy {
namespace Server {

class Wasm {
public:
  virtual ~Wasm() {}
};

using WasmSharedPtr = std::shared_ptr<Wasm>;

class WasmService {
public:
  WasmService(WasmSharedPtr singleton) : singleton_(std::move(singleton)) {}
  WasmService(ThreadLocal::SlotPtr tls_slot) : tls_slot_(std::move(tls_slot)) {}

private:
  WasmSharedPtr singleton_;
  ThreadLocal::SlotPtr tls_slot_;
};

using WasmServicePtr = std::unique_ptr<WasmService>;
using CreateWasmServiceCallback = std::function<void(WasmServicePtr)>;

} // namespace Server
} // namespace Envoy
