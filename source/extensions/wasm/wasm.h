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


class Wasm : public Server::Wasm {
  public:
    explicit Wasm(absl::string_view vm);
    ~Wasm() override {}

    bool initialize(absl::string_view file, bool allow_precomiled) override;
    void configure(absl::string_view configuration) override;
    void start(Event::Dispatcher& dispatcher, std::chrono::milliseconds tick_period) override;

    void setWasmCallbacks(Server::Wasm::WasmCallbacks& callbacks) override {
      context_->setWasmCallbacks(callbacks);
    }

    Common::Wasm::Context *callingContext() { return context_.get(); }
    Common::Wasm::WasmVm *wasm_vm() { return wasm_vm_.get(); }

    // For testing only.
    void tickHandler();

  private:
    class WasmCallbacks : public Server::Wasm::WasmCallbacks, public Common::Wasm::WasmVmCallbacks {
      public:
        WasmCallbacks() = default;
        ~WasmCallbacks() override {}

        virtual void setTickPeriodMilliseconds(uint32_t period) PURE;
        virtual void scriptLog(spdlog::level::level_enum level, absl::string_view ssage) PURE;
    };

    class Context : public Common::Wasm::Context, public WasmCallbacks {
      public:
        Context(Wasm* wasm) : Common::Wasm::Context(wasm->wasm_vm()), wasm_(wasm) {}
        ~Context() override {}

        // Server::Wasm::WasmCallbacks implementations.
        void scriptLog(spdlog::level::level_enum level, absl::string_view message) override {

          if (wasm_callbacks_) {
            wasm_callbacks_->scriptLog(level, message);
            return;
          }
          Common::Wasm::Context::scriptLog(level, message);
        }
        void setTickPeriodMilliseconds(uint32_t tick_period_milliseconds) override {
          if (wasm_callbacks_) {
            wasm_callbacks_->setTickPeriodMilliseconds(tick_period_milliseconds);
            return;
          }
          wasm_->setTickPeriodMilliseconds(tick_period_milliseconds);
        }

        // Server::Wasm::WasmCallbacks testing hooks.
        void setWasmCallbacks(Server::Wasm::WasmCallbacks& callbacks) {
          wasm_callbacks_ = &callbacks;
        }
        void scriptLogHandler(spdlog::level::level_enum level, absl::string_view message) {
          wasm_callbacks_->scriptLog(level, message);
        }
        void setTickPeriodMillisecondsHandler(uint32_t tick_period_milliseconds) {
          wasm_callbacks_->setTickPeriodMilliseconds(tick_period_milliseconds);
        }

      private:
        Wasm* wasm_ = nullptr;
        Server::Wasm::WasmCallbacks* wasm_callbacks_ = nullptr;;
    };

    void setTickPeriodMilliseconds(uint32_t tick_period_milliseconds) {
      tick_period_ = std::chrono::milliseconds(tick_period_milliseconds);
    }

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
