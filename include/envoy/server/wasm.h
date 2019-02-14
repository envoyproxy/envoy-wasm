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

typedef std::shared_ptr<Wasm> WasmSharedPtr;

} // namespace Server
} // namespace Envoy
