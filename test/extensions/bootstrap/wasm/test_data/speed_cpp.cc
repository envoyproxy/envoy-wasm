// NOLINT(namespace-envoy)
#include <google/protobuf/util/json_util.h>

#include <string>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics_full.h"
// Required Proxy-Wasm ABI version.
extern "C" PROXY_WASM_KEEPALIVE void proxy_abi_version_0_1_0() {}
#else
#include "envoy/config/core/v3/grpc_service.pb.h"
using envoy::config::core::v3::GrpcService;
#include "include/proxy-wasm/null_plugin.h"
#endif

START_WASM_PLUGIN(WasmSpeedCpp)

int xDoNotRemove = 0;

google::protobuf::Arena arena;

google::protobuf::Struct args;
google::protobuf::Struct* args_arena =
    google::protobuf::Arena::CreateMessage<google::protobuf::Struct>(&arena);
const std::string configuration = R"EOF(
  {
    "NAME":"test_pod",
    "NAMESPACE":"test_namespace",
    "LABELS": {
        "app": "productpage",
        "version": "v1",
        "pod-template-hash": "84975bc778"
    },
    "OWNER":"test_owner",
    "WORKLOAD_NAME":"test_workload",
    "PLATFORM_METADATA":{
        "gcp_project":"test_project",
        "gcp_cluster_location":"test_location",
        "gcp_cluster_name":"test_cluster"
    },
    "ISTIO_VERSION":"istio-1.4",
    "MESH_ID":"test-mesh"
  }
  )EOF";

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

void modify_metadata_test() {
  auto path = getRequestHeader(":path");
  addRequestHeader("newheader", "newheadervalue");
  auto server = getRequestHeader("server");
  replaceRequestHeader("server", "envoy-wasm");
  replaceRequestHeader("envoy-wasm", "server");
  removeRequestHeader("newheader");
}

void modify_metadata1000_test() {
  for (int x = 0; x < 1000; x++) {
    auto path = getRequestHeader(":path");
    addRequestHeader("newheader", "newheadervalue");
    auto server = getRequestHeader("server");
    replaceRequestHeader("server", "envoy-wasm");
    replaceRequestHeader("envoy-wasm", "server");
    removeRequestHeader("newheader");
  }
}

void json_serialize_test() { google::protobuf::util::JsonStringToMessage(configuration, &args); }

void json_serialize_arena_test() {
  google::protobuf::util::JsonStringToMessage(configuration, args_arena);
}

void json_deserialize_test() {
  std::string json;
  google::protobuf::util::MessageToJsonString(args, &json);
}

void json_deserialize_arena_test() {
  std::string json;
  google::protobuf::util::MessageToJsonString(*args_arena, &json);
}

WASM_EXPORT(uint32_t, proxy_on_vm_start, (uint32_t, uint32_t configuration_size)) {
  const char* configuration_ptr = nullptr;
  size_t size;
  proxy_get_buffer_bytes(WasmBufferType::VmConfiguration, 0, configuration_size, &configuration_ptr,
                         &size);
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
  } else if (configuration == "modify_metadata") {
    test_fn = &modify_metadata_test;
  } else if (configuration == "modify_metadata1000") {
    test_fn = &modify_metadata1000_test;
  } else if (configuration == "json_serialize") {
    test_fn = &json_serialize_test;
  } else if (configuration == "json_serialize_arena") {
    test_fn = &json_serialize_arena_test;
  } else if (configuration == "json_deserialize") {
    test_fn = &json_deserialize_test;
  } else if (configuration == "json_deserialize_arena") {
    test_fn = &json_deserialize_arena_test;
  } else {
    std::string message = "on_start " + configuration;
    proxy_log(LogLevel::info, message.c_str(), message.size());
  }
  ::free(const_cast<void*>(reinterpret_cast<const void*>(configuration_ptr)));
  return 1;
}

WASM_EXPORT(void, proxy_on_tick, (uint32_t)) { (*test_fn)(); }

END_WASM_PLUGIN
