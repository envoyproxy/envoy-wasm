#include <memory>

#include "envoy/common/exception.h"
#include "envoy/common/pure.h"
#include "envoy/event/dispatcher.h"

#include "extensions/wasm/wasm.h"

#include "extensions/common/wasm/wasm.h"

namespace Envoy {
namespace Extensions {
namespace Wasm {
  
Wasm::Wasm(absl::string_view vm) {
  wasm_vm_ = Common::Wasm::createWasmVm(vm);
  registerCallback(wasm_vm_.get(), "scriptLog", &Context::scriptLogHandler);
  registerCallback(wasm_vm_.get(), "setTickPeriodMilliseconds", &Context::setTickPeriodMillisecondsHandler);
}

bool Wasm::initialize(absl::string_view file, bool allow_precompiled) {
  auto ok = wasm_vm_->initialize(file, allow_precompiled);
  getFunction(wasm_vm_.get(), "_configure", &configure_);
  getFunction(wasm_vm_.get(), "_tick", &tick_);
  context_.reset(new Context(this));
  return ok;
}

void Wasm::configure(absl::string_view configuration) {
  if (configure_) {
    auto address = wasm_vm_->copyString(configuration);
    configure_(callingContext(), address, configuration.size());
  }
}

void Wasm::start(Event::Dispatcher& dispatcher, std::chrono::milliseconds tick_period) {
  tick_period_ = tick_period;
  wasm_vm_->start(callingContext());
  if (tick_ && tick_period_.count() > 0) {
    timer_ = dispatcher.createTimer([this]() { this->tickHandler(); });
    timer_->enableTimer(tick_period_);
  }
}

void Wasm::tickHandler() {
  if (tick_) {
    tick_(callingContext());
    if (timer_) {
      timer_->enableTimer(tick_period_);
    }
  }
}

} // namespace Wasm
} // namespace Extensions
} // namespace Envoy
