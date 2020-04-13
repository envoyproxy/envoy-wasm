#include <string>
#include <unordered_map>

#ifndef NULL_PLUGIN
#include "proxy_wasm_intrinsics.h"
#else
#include "extensions/common/wasm/null/null_plugin.h"
#include "absl/base/casts.h"
#endif

#include "source/extensions/common/wasm/declare_property.pb.h"

// NOLINT(namespace-envoy)
START_WASM_PLUGIN(ExamplePlugin)

// For performance testing: see wasm_speed_test.cc.
class PluginRootContext : public RootContext {
public:
  PluginRootContext(uint32_t id, StringView root_id) : RootContext(id, root_id) {}
  void onTick() override;
};

void PluginRootContext::onTick() {
  uint64_t t;
  if (WasmResult::Ok != proxy_get_current_time_nanoseconds(&t)) {
    logError(std::string("bad proxy_get_current_time_nanoseconds result"));
  }
  {
    std::string function = "declare_property";
    envoy::source::extensions::common::wasm::DeclarePropertyArguments args;
    args.set_name("structured_state");
    args.set_type(envoy::source::extensions::common::wasm::WasmType::FlatBuffers);
    args.set_span(envoy::source::extensions::common::wasm::LifeSpan::DownstreamConnection);
    // simple table {i : int64}
    static const char bfbsData[192] = {
        0x18, 0x00, 0x00, 0x00, 0x42, 0x46, 0x42, 0x53, 0x10, 0x00, 0x1C, 0x00, 0x04, 0x00, 0x08,
        0x00, 0x0C, 0x00, 0x10, 0x00, 0x14, 0x00, 0x18, 0x00, 0x10, 0x00, 0x00, 0x00, 0x30, 0x00,
        0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x34,
        0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x10, 0x00, 0x04, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x08,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
        0x0D, 0x00, 0x00, 0x00, 0x57, 0x61, 0x73, 0x6D, 0x2E, 0x43, 0x6F, 0x6D, 0x6D, 0x6F, 0x6E,
        0x2E, 0x54, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x12, 0x00, 0x08, 0x00, 0x0C, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0C,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x07, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00};
    args.set_schema(bfbsData, 192);
    std::string in;
    args.SerializeToString(&in);
    char* out = nullptr;
    size_t out_size = 0;
    if (WasmResult::Ok != proxy_call_foreign_function(function.data(), function.size(), in.data(),
                                                      in.size(), &out, &out_size)) {
      logError("declare_property failed");
    }
    ::free(out);
  }
}

class PluginContext : public Context {
public:
  explicit PluginContext(uint32_t id, RootContext* root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t headers) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  void onLog() override;
  void onDone() override;
};
static RegisterContextFactory register_PluginContext(CONTEXT_FACTORY(PluginContext),
                                                     ROOT_FACTORY(PluginRootContext));

FilterHeadersStatus PluginContext::onRequestHeaders(uint32_t) {
  logDebug(std::string("onRequestHeaders ") + std::to_string(id()));
  auto path = getRequestHeader(":path");
  logInfo(std::string("header path ") + std::string(path->view()));
  if (addRequestHeader("newheader", "newheadervalue") != WasmResult::Ok) {
    logInfo("addRequestHeader failed");
  }
  replaceRequestHeader("server", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus PluginContext::onRequestBody(size_t body_buffer_length, bool /* end_of_stream */) {
  auto body = getBufferBytes(WasmBufferType::HttpRequestBody, 0, body_buffer_length);
  logError(std::string("onRequestBody ") + std::string(body->view()));
  return FilterDataStatus::Continue;
}

void PluginContext::onLog() {
  setFilterState("wasm_state", "wasm_value");
  auto path = getRequestHeader(":path");
  if (path->view() == "/test_context") {
    logWarn("request.path: " + getProperty({"request", "path"}).value()->toString());
    logWarn("node.metadata: " +
            getProperty({"node", "metadata", "istio.io/metadata"}).value()->toString());
    logWarn("metadata: " + getProperty({"metadata", "filter_metadata", "envoy.filters.http.wasm",
                                        "wasm_request_get_key"})
                               .value()
                               ->toString());
    int64_t responseCode;
    if (getValue({"response", "code"}, &responseCode)) {
      logWarn("response.code: " + absl::StrCat(responseCode));
    }
    logWarn("state: " + getProperty({"wasm_state"}).value()->toString());
  } else {
    logWarn("onLog " + std::to_string(id()) + " " + std::string(path->view()));
  }

  // Wasm state property set and read validation for {i: 1337}
  {
    static const char data[24] = {0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
                                  0x0c, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
                                  0x39, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if (WasmResult::Ok != setFilterState("structured_state", StringView(data, 24))) {
      logWarn("setProperty(structured_state) failed");
    }
    int64_t value = 0;
    if (!getValue({"structured_state", "i"}, &value)) {
      logWarn("getProperty(structured_state) failed");
    }
    if (value != 1337) {
      logWarn("getProperty(structured_state) returned " + std::to_string(value));
    }
  }
}

void PluginContext::onDone() { logWarn("onDone " + std::to_string(id())); }

END_WASM_PLUGIN
