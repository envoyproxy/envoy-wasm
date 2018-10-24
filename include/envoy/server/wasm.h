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
  class WasmCallbacks {
  public:
    virtual ~WasmCallbacks() {}
    virtual void scriptLog(spdlog::level::level_enum level, const std::string& message) PURE;
  };

  virtual ~Wasm() {}

  virtual bool initialize(const std::string& file, bool allow_precompiled) PURE;
  virtual void configure(const std::string& configuration) PURE;
  virtual void start(Event::Dispatcher& dispatcher) PURE;
  virtual void tick() PURE;

  virtual void setWasmCallbacks(WasmCallbacks& callbacks) PURE;
};

typedef std::unique_ptr<Wasm> WasmPtr;

} // namespace Server
} // namespace Envoy
