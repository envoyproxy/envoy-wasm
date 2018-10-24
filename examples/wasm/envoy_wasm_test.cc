#include <string>

#include "envoy_wasm_intrinsics.h"

extern "C" void configure() {}
extern "C" int main() {
  logDebug(std::string("test debug") + " logging");
  logInfo(std::string("test info") + " logging");
  return 0;
}
