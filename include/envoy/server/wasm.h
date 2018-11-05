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

  virtual bool initialize(absl::string_view file, bool allow_precompiled) PURE;
  virtual void configure(absl::string_view configuration) PURE;
  virtual void start(Event::Dispatcher& dispatcher, std::chrono::milliseconds tick_period) PURE;
};

typedef std::unique_ptr<Wasm> WasmPtr;

} // namespace Server
} // namespace Envoy
