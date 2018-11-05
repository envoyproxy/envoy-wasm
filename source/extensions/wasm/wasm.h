#pragma once

#include <stdio.h>

#include <chrono>
#include <memory>

#include "envoy/common/exception.h"
#include "envoy/common/pure.h"
#include "envoy/event/dispatcher.h"

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {

class Wasm;

class Context : public Common::Wasm::Context {
  public:
    explicit Context(Wasm* wasm);
    ~Context() override {}

    // Common::Wasm::Context WASM callbacks.
    void scriptLog(spdlog::level::level_enum level, absl::string_view message) override;
    // Local WASM callbacks.
    virtual void setTickPeriodMilliseconds(uint32_t tick_period_milliseconds);

  private:
    Wasm* wasm_ = nullptr;
};

class Wasm : public Server::Wasm {
  public:
    explicit Wasm(absl::string_view vm);
    ~Wasm() override {}

    bool initialize(absl::string_view file, bool allow_precomiled) override;
    void configure(absl::string_view configuration) override;
    void start(Event::Dispatcher& dispatcher, std::chrono::milliseconds tick_period) override;

    Common::Wasm::Context *callingContext() { return context_.get(); }
    Common::Wasm::WasmVm *wasm_vm() { return wasm_vm_.get(); }

    void setTickPeriodMilliseconds(uint32_t tick_period_milliseconds) {
      tick_period_ = std::chrono::milliseconds(tick_period_milliseconds);
    }

    // For testing only.
    void tickHandler();
    void setContext(Context *context) {
      context_.reset(context);
    }

  private:
    std::unique_ptr<Common::Wasm::WasmVm> wasm_vm_;
    std::unique_ptr<Context> context_;
    std::function<void(Common::Wasm::Context*, uint32_t configuration_ptr, uint32_t configuration_size)> configure_;
    std::function<void(Common::Wasm::Context*)> tick_;
    std::chrono::milliseconds tick_period_;
    Event::TimerPtr timer_;
};

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
