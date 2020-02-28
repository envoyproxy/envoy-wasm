// NOLINT(namespace-envoy)
#include <string>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics.h"
// Required Proxy-Wasm ABI version.
extern "C" PROXY_WASM_KEEPALIVE void proxy_abi_version_0_1_0() {}
#else
#include "extensions/common/wasm/null/null_plugin.h"
#endif

START_WASM_PLUGIN(WasmSpeedCpp)

int xDoNotRemove = 0;

void (*test_fn)() = nullptr;

void empty_test() {}

void get_current_time_test() {
  uint64_t t;
  if (WasmResult::Ok != proxy_get_current_time_nanoseconds(&t)) {
    logError("bad result from getCurrentTimeNanoseconds");
  }
}

void string_test() {
  std::string s = "foo";
  s += "bar";
  xDoNotRemove = s.size();
}

void string1000_test() {
  for (int x = 0; x < 1000; x++) {
    std::string s = "foo";
    s += "bar";
    xDoNotRemove += s.size();
  }
}

void get_property_test() {
  std::string property = "plugin_root_id";
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  auto result = proxy_get_property(property.data(), property.size(), &value_ptr, &value_size);
  if (WasmResult::Ok != result) {
    logError("bad result for getProperty");
  }
  ::free(reinterpret_cast<void*>(const_cast<char*>(value_ptr)));
}

void grpc_service_test() {
  std::string value = "foo";
  GrpcService grpc_service;
  grpc_service.mutable_envoy_grpc()->set_cluster_name(value);
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
}

void grpc_service1000_test() {
  std::string value = "foo";
  for (int x = 0; x < 1000; x++) {
    GrpcService grpc_service;
    grpc_service.mutable_envoy_grpc()->set_cluster_name(value);
    std::string grpc_service_string;
    grpc_service.SerializeToString(&grpc_service_string);
  }
}

WASM_EXPORT(uint32_t, proxy_on_vm_start, (uint32_t, uint32_t)) {
  const char* configuration_ptr = nullptr;
  size_t size;
  proxy_get_configuration(&configuration_ptr, &size);
  std::string configuration(configuration_ptr, size);
  if (configuration == "empty") {
    test_fn = &empty_test;
  } else if (configuration == "get_current_time") {
    test_fn = &get_current_time_test;
  } else if (configuration == "string") {
    test_fn = &string_test;
  } else if (configuration == "string1000") {
    test_fn = &string1000_test;
  } else if (configuration == "get_property") {
    test_fn = &get_property_test;
  } else if (configuration == "grpc_service") {
    test_fn = &grpc_service_test;
  } else if (configuration == "grpc_service1000") {
    test_fn = &grpc_service1000_test;
  } else {
    std::string message = "on_start " + configuration;
    proxy_log(LogLevel::info, message.c_str(), message.size());
  }
  ::free(const_cast<void*>(reinterpret_cast<const void*>(configuration_ptr)));
  return 1;
}

WASM_EXPORT(void, proxy_on_tick, (uint32_t)) { (*test_fn)(); }

END_WASM_PLUGIN
