#pragma once

#include <memory>

#include "envoy/common/exception.h"
#include "envoy/common/pure.h"
#include "envoy/event/dispatcher.h"

#include "common/common/logger.h"

namespace Envoy {
namespace Server {

class Wasm {
public:
  virtual ~Wasm() {}
};

using WasmSharedPtr = std::shared_ptr<Wasm>;
using CreateWasmCallback = std::function<void(WasmSharedPtr)>;

} // namespace Server
} // namespace Envoy
