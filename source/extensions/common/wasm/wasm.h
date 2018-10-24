#pragma once

#include <memory>
#include <string>
#include <vector>

#include "envoy/common/exception.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/server/wasm.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/assert.h"
#include "common/common/c_smart_ptr.h"
#include "common/common/logger.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

class wasm_State;

class ThreadLocalState : Logger::Loggable<Logger::Id::wasm> {
public:
  ThreadLocalState(const std::string&, ThreadLocal::SlotAllocator&) {}
  // SessionPtr createSession() { return nullptr; }
  int getGlobalRef(uint64_t /*slot*/) { return 0; }
  uint64_t registerGlobal(const std::string& /*global*/) { return 0; }
  uint64_t runtimeBytesUsed() { return 0; }
  void runtimeGC() {}

private:
};

typedef int (*wasm_CFunction)(wasm_State*);
typedef std::vector<std::pair<const char*, wasm_CFunction>> ExportedFunctions;

Server::WasmPtr createWasmVm(const std::string& wasm_vm);

class WasmException : public EnvoyException {
public:
  using EnvoyException::EnvoyException;
};

class WasmVmException : public EnvoyException {
public:
  using EnvoyException::EnvoyException;
};

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
