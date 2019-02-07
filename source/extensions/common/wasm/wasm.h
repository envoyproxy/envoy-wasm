#pragma once

#include <memory>

#include "envoy/access_log/access_log.h"
#include "envoy/buffer/buffer.h"
#include "envoy/common/exception.h"
#include "envoy/config/wasm/v2/wasm.pb.validate.h"
#include "envoy/http/filter.h"
#include "envoy/server/wasm.h"
#include "envoy/thread_local/thread_local.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/assert.h"
#include "common/common/c_smart_ptr.h"
#include "common/common/logger.h"

#include "extensions/common/wasm/well_known_names.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

struct AsyncClientHandler;
class Context;
class Wasm;
class WasmVm;

using Pairs = std::vector<std::pair<absl::string_view, absl::string_view>>;
using PairsWithStringValues = std::vector<std::pair<absl::string_view, std::string>>;

using WasmCall0Void = std::function<void(Context*)>;
using WasmCall2Void = std::function<void(Context*, uint32_t, uint32_t)>;

using WasmContextCall0Void = std::function<void(Context*, uint32_t context_id)>;
using WasmContextCall7Void = std::function<void(Context*, uint32_t context_id, uint32_t, uint32_t,
                                                uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>;

using WasmContextCall0Int = std::function<uint32_t(Context*, uint32_t context_id)>;
using WasmContextCall2Int =
    std::function<uint32_t(Context*, uint32_t context_id, uint32_t, uint32_t)>;

// A context which will be the target of callbacks for a particular session
// e.g. a handler of a stream.
class Context : public Http::StreamFilter,
                public AccessLog::Instance,
                public Logger::Loggable<Logger::Id::wasm>,
                public std::enable_shared_from_this<Context> {
public:
  explicit Context(Wasm* wasm);

  Wasm* wasm() const { return wasm_; }
  WasmVm* wasmVm() const;
  Upstream::ClusterManager* clusterManager() const;
  uint32_t id() const { return id_; }
  const StreamInfo::StreamInfo& streamInfo() const;

  //
  // Calls into the WASM code (downcalls).
  //
  // VM level downcalls on Context(id == 0)
  virtual void onStart();
  virtual void onConfigure(absl::string_view configuration);
  // HTTP Filter Stream Request Downcalls on Context(id > 0)
  virtual Http::FilterHeadersStatus onRequestHeaders();
  virtual Http::FilterDataStatus onRequestBody(int body_buffer_length, bool end_of_stream);
  virtual Http::FilterTrailersStatus onRequestTrailers();
  virtual Http::FilterMetadataStatus onRequestMetadata();
  // HTTP Filter STream Response Donwcalls on Context(id > 0)
  virtual Http::FilterHeadersStatus onResponseHeaders();
  virtual Http::FilterDataStatus onResponseBody(int body_buffer_length, bool end_of_stream);
  virtual Http::FilterTrailersStatus onResponseTrailers();
  virtual Http::FilterMetadataStatus onResponseMetadata();
  // Async Response Downcalls on any Context.
  virtual void onHttpCallResponse(uint32_t token, const Pairs& response_headers,
                                  absl::string_view response_body, const Pairs& response_trailers);
  virtual void onDone();

  //
  // General Callbacks.
  //
  virtual void scriptLog(spdlog::level::level_enum level, absl::string_view message);
  virtual void setTickPeriod(std::chrono::milliseconds tick_period);
  virtual uint32_t getTotalMemory();

  //
  // AccessLog::Instance
  //
  void log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
           const Http::HeaderMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info) override;

  //
  // Http::StreamFilterBase
  //
  // Note: This calls onDone() in WASM.
  void onDestroy() override;

  //
  // Http::StreamDecoderFilter
  //
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override;
  // Note: this is not yet implementated in envoy.
  Http::FilterMetadataStatus decodeMetadata(Http::MetadataMap&& metadata_map) /* override */;
  void setDecoderFilterCallbacks(Envoy::Http::StreamDecoderFilterCallbacks& callbacks) override;

  //
  // Http::StreamEncoderFilter
  //
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override;
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override;
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap& metadata_map) override;
  void setEncoderFilterCallbacks(Envoy::Http::StreamEncoderFilterCallbacks& callbacks) override;

  //
  // HTTP Filter Callbacks
  //
  // StreamInfo
  virtual std::string getRequestStreamInfoProtocol();
  virtual std::string getResponseStreamInfoProtocol();
  // Metadata: the values are serialized ProtobufWkt::Struct
  virtual std::string getRequestMetadata(absl::string_view key);
  virtual void setRequestMetadata(absl::string_view key, absl::string_view serialized_proto_struct);
  virtual PairsWithStringValues getRequestMetadataPairs();
  virtual std::string getResponseMetadata(absl::string_view key);
  virtual void setResponseMetadata(absl::string_view key,
                                   absl::string_view serialized_proto_struct);
  virtual PairsWithStringValues getResponseMetadataPairs();

  // Continue
  virtual void continueRequest() {
    if (decoder_callbacks_)
      decoder_callbacks_->continueDecoding();
  }
  virtual void continueResponse() {
    if (encoder_callbacks_)
      encoder_callbacks_->continueEncoding();
  }

  // Shared Data
  virtual std::pair<std::string, uint32_t> getSharedData(absl::string_view key);
  virtual bool setSharedData(absl::string_view key, absl::string_view value, uint32_t cas);

  // Request Headers
  virtual void addRequestHeader(absl::string_view key, absl::string_view value);
  virtual absl::string_view getRequestHeader(absl::string_view key);
  virtual Pairs getRequestHeaderPairs();
  virtual void removeRequestHeader(absl::string_view key);
  virtual void replaceRequestHeader(absl::string_view key, absl::string_view value);

  // Request Trailers
  virtual void addRequestTrailer(absl::string_view key, absl::string_view value);
  virtual absl::string_view getRequestTrailer(absl::string_view key);
  virtual Pairs getRequestTrailerPairs();
  virtual void removeRequestTrailer(absl::string_view key);
  virtual void replaceRequestTrailer(absl::string_view key, absl::string_view value);

  // Response Headers
  virtual void addResponseHeader(absl::string_view key, absl::string_view value);
  virtual absl::string_view getResponseHeader(absl::string_view key);
  virtual Pairs getResponseHeaderPairs();
  virtual void removeResponseHeader(absl::string_view key);
  virtual void replaceResponseHeader(absl::string_view key, absl::string_view value);

  // Response Trailers
  virtual void addResponseTrailer(absl::string_view key, absl::string_view value);
  virtual absl::string_view getResponseTrailer(absl::string_view key);
  virtual Pairs getResponseTrailerPairs();
  virtual void removeResponseTrailer(absl::string_view key);
  virtual void replaceResponseTrailer(absl::string_view key, absl::string_view value);

  // Body Buffer
  virtual absl::string_view getRequestBodyBufferBytes(uint32_t start, uint32_t length);
  virtual absl::string_view getResponseBodyBufferBytes(uint32_t start, uint32_t length);

  // HTTP
  // Returns a token which will be used with the corresponding onHttpCallResponse.
  virtual uint32_t httpCall(absl::string_view cluster, const Pairs& request_headers,
                            absl::string_view request_body, const Pairs& request_trailers,
                            int timeout_millisconds);
  virtual void httpRespond(const Pairs& response_headers, absl::string_view body,
                           const Pairs& response_trailers);

  // Connection
  virtual bool isSsl();

protected:
  friend struct AsyncClientHandler;

  void onAsyncClientSuccess(uint32_t token, Envoy::Http::MessagePtr& response);
  void onAsyncClientFailure(uint32_t token, Http::AsyncClient::FailureReason reason);

  Wasm* const wasm_;
  const uint32_t id_;
  bool destroyed_ = false;

  // Async callback support.
  uint32_t next_async_token_ = 1;
  // MB: must be a node-type map as we take persistent references to the entries.
  std::map<uint32_t, AsyncClientHandler> http_request_;
  Envoy::Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Envoy::Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};

  // HTTP Filter state.
  Http::HeaderMap* request_headers_{};
  Http::HeaderMap* response_headers_{};
  Buffer::Instance* requestBodyBuffer_{};
  Buffer::Instance* responseBodyBuffer_{};
  bool request_end_of_stream_{};
  bool response_end_of_stream_{};
  Http::HeaderMap* request_trailers_{};
  Http::HeaderMap* response_trailers_{};
  Http::MetadataMap* request_metadata_{};
  Http::MetadataMap* response_metadata_{};

  const StreamInfo::StreamInfo* access_log_stream_info_{};
  const Http::HeaderMap* access_log_request_headers_{};
  const Http::HeaderMap* access_log_response_headers_{};
  const Http::HeaderMap* access_log_request_trailers_{}; // unused
  const Http::HeaderMap* access_log_response_trailers_{};
};

struct AsyncClientHandler : public Http::AsyncClient::Callbacks {
  // Http::AsyncClient::Callbacks
  void onSuccess(Envoy::Http::MessagePtr&& response) override;
  void onFailure(Http::AsyncClient::FailureReason reason) override;

  Context* context;
  uint32_t token;
  Http::AsyncClient::Request* request;
};

// Wasm execution instance. Manages the Envoy side of the Wasm interface.
class Wasm : public Envoy::Server::Wasm,
             public AccessLog::Instance,
             public ThreadLocal::ThreadLocalObject,
             public Logger::Loggable<Logger::Id::wasm>,
             public std::enable_shared_from_this<Wasm> {
public:
  Wasm(absl::string_view vm, absl::string_view id);
  Wasm(const Wasm& other);
  ~Wasm() {}

  void setDispatcher(Event::Dispatcher& dispatcher) { dispatcher_ = &dispatcher; }
  void setClusterManager(Upstream::ClusterManager& clusterManager) {
    clusterManager_ = &clusterManager;
  }

  bool initialize(const std::string& code, absl::string_view name, bool allow_precompiled);
  void configure(absl::string_view configuration);
  void start();

  const std::string& context_id_filter_state_data_name() {
    return context_id_filter_state_data_name_;
  }
  absl::string_view id() const { return id_; }
  WasmVm* wasmVm() const { return wasm_vm_.get(); }
  Context* generalContext() const { return general_context_.get(); }
  Upstream::ClusterManager* clusterManager() const { return clusterManager_; }

  std::shared_ptr<Context> createContext() { return std::make_shared<Context>(this); }

  void setTickPeriod(std::chrono::milliseconds tick_period);
  void tickHandler();

  uint32_t allocContextId();

  //
  // AccessLog::Instance
  //
  void log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
           const Http::HeaderMap* response_trailers, const StreamInfo::StreamInfo& stream_info);

  // For testing.
  void setGeneralContext(std::shared_ptr<Context> context) {
    general_context_ = std::move(context);
  }

private:
  friend class Context;

  void getFunctions();

  Event::Dispatcher* dispatcher_ = nullptr;
  Upstream::ClusterManager* clusterManager_ = nullptr;
  std::string id_;
  std::string context_id_filter_state_data_name_;
  uint32_t next_context_id_ = 0;
  std::unique_ptr<WasmVm> wasm_vm_;
  std::shared_ptr<Context> general_context_; // Context unrelated to any specific stream.
  std::function<void(Common::Wasm::Context*)> tick_;
  std::chrono::milliseconds tick_period_;
  Event::TimerPtr timer_;

  // Calls into the VM.
  WasmCall0Void onStart_;
  WasmCall2Void onConfigure_;
  WasmCall0Void onTick_;

  // Calls into the VM with a context.
  WasmContextCall0Int onRequestHeaders_;
  WasmContextCall2Int onRequestBody_;
  WasmContextCall0Int onRequestTrailers_;
  WasmContextCall0Int onRequestMetadata_;

  WasmContextCall0Int onResponseHeaders_;
  WasmContextCall2Int onResponseBody_;
  WasmContextCall0Int onResponseTrailers_;
  WasmContextCall0Int onResponseMetadata_;

  WasmContextCall7Void onHttpCallResponse_;

  WasmContextCall0Void onLog_;
  WasmContextCall0Void onDone_;
};

inline WasmVm* Context::wasmVm() const { return wasm_->wasmVm(); }
inline Upstream::ClusterManager* Context::clusterManager() const { return wasm_->clusterManager(); }

inline const ProtobufWkt::Struct& getMetadata(Http::StreamFilterCallbacks* callbacks) {
  if (callbacks->route() == nullptr || callbacks->route()->routeEntry() == nullptr) {
    return ProtobufWkt::Struct::default_instance();
  }
  const auto& metadata = callbacks->route()->routeEntry()->metadata();
  const auto filter_it = metadata.filter_metadata().find(HttpFilters::HttpFilterNames::get().Wasm);
  if (filter_it == metadata.filter_metadata().end()) {
    return ProtobufWkt::Struct::default_instance();
  }
  return filter_it->second;
}

// Wasm VM instance. Provides the low level WASM interface.
class WasmVm : public Logger::Loggable<Logger::Id::wasm> {
public:
  virtual ~WasmVm() {}
  virtual absl::string_view vm() PURE;

  // Whether or not the VM implementation supports cloning.
  virtual bool clonable() PURE;
  // Make a thread-specific copy. This may not be supported by the underlying VM system in which
  // case it will return nullptr and the caller will need to create a new VM from scratch.
  virtual std::unique_ptr<WasmVm> clone() PURE;

  // Load the WASM code from a file. Return true on success.
  virtual bool initialize(const std::string& code, absl::string_view id,
                          bool allow_precompiled) PURE;

  // Call the 'start' function or main() if there is no start funcition.
  virtual void start(Context*) PURE;

  // Allocate a block of memory in the VM and return the pointer to use as a call arguments.
  virtual void* allocMemory(uint32_t size, uint32_t* pointer) PURE;
  // Convert a block of memory in the VM to a string_view.
  virtual absl::string_view getMemory(uint32_t pointer, uint32_t size) PURE;
  // Set a block of memory in the VM, returns true on success, false if the pointer/size is invalid.
  virtual bool setMemory(uint32_t pointer, uint32_t size, void* data) PURE;

  // Convenience functions.

  // Allocate a null-terminated string in the VM and return the pointer to use as a call arguments.
  uint32_t copyString(absl::string_view s) {
    uint32_t pointer;
    uint8_t* m = static_cast<uint8_t*>(allocMemory((s.size() + 1), &pointer));
    if (s.size() > 0)
      memcpy(m, s.data(), s.size());
    m[s.size()] = 0;
    return pointer;
  }

  // Copy the data in 's' into the VM along with the pointer-size pair. Returns true on success.
  bool copyToPointerSize(absl::string_view s, uint32_t ptr_ptr, uint32_t size_ptr) {
    uint32_t pointer = 0;
    uint32_t size = s.size();
    void* p = nullptr;
    if (size > 0) {
      p = allocMemory(size, &pointer);
      if (!p)
        return false;
      memcpy(p, s.data(), size);
    }
    if (!setMemory(ptr_ptr, sizeof(uint32_t), &pointer))
      return false;
    if (!setMemory(size_ptr, sizeof(uint32_t), &size))
      return false;
    return true;
  }

  bool copyToPointerSize(const Buffer::Instance& buffer, uint32_t start, uint32_t length,
                         uint32_t ptr_ptr, uint32_t size_ptr) {
    uint32_t size = buffer.length();
    if (size < start + length)
      return false;
    auto nslices = buffer.getRawSlices(nullptr, 0);
    auto slices = std::make_unique<Buffer::RawSlice[]>(nslices + 10 /* pad for evbuffer overrun */);
    auto actual_slices = buffer.getRawSlices(&slices[0], nslices);
    uint32_t pointer = 0;
    char* p = static_cast<char*>(allocMemory(length, &pointer));
    auto s = start;
    auto l = length;
    if (!p)
      return false;
    for (uint64_t i = 0; i < actual_slices; i++) {
      if (slices[i].len_ <= s) {
        s -= slices[i].len_;
        continue;
      }
      auto ll = l;
      if (ll > s + slices[i].len_)
        ll = s + slices[i].len_;
      memcpy(p, static_cast<char*>(slices[i].mem_) + s, ll);
      l -= ll;
      if (l <= 0)
        break;
      s = 0;
      p += ll;
    }
    if (!setMemory(ptr_ptr, sizeof(int32_t), &pointer))
      return false;
    if (!setMemory(size_ptr, sizeof(int32_t), &length))
      return false;
    return true;
  }
};

// Create a new WASM VM of the give type (e.g. "envoy.wasm.vm.wavm").
std::unique_ptr<WasmVm> createWasmVm(absl::string_view vm);

// Create a new Wasm VM not attached to any thread. Note: 'id' may be empty if this VM will not be
// shared by APIs (e.g. HTTP Filter + AccessLog).
std::unique_ptr<Wasm> createWasm(absl::string_view id,
                                 const envoy::config::wasm::v2::VmConfig& vm_config, Api::Api& api);
// Create a ThreadLocal VM from an existing VM (e.g. from createWasm() above).
std::shared_ptr<Wasm> createThreadLocalWasm(Wasm& base_wasm,
                                            const envoy::config::wasm::v2::VmConfig& vm_config,
                                            Event::Dispatcher& dispatcher,
                                            absl::string_view configuration, Api::Api& api);
// Get an existing ThreadLocal VM matching 'id'.
std::shared_ptr<Wasm> getThreadLocalWasm(absl::string_view id, absl::string_view configuration);

class WasmException : public EnvoyException {
public:
  using EnvoyException::EnvoyException;
};

class WasmVmException : public EnvoyException {
public:
  using EnvoyException::EnvoyException;
};

inline Context::Context(Wasm* wasm) : wasm_(wasm), id_(wasm->allocContextId()) {}

// Forward declarations for VM implemenations.
template <typename R, typename... Args>
void registerCallbackWavm(WasmVm* vm, absl::string_view functionName, R (*)(Args...));
template <typename R, typename... Args>
void getFunctionWavm(WasmVm* vm, absl::string_view functionName,
                     std::function<R(Context*, Args...)>*);

template <typename R, typename... Args>
void registerCallback(WasmVm* vm, absl::string_view functionName, R (*f)(Args...)) {
  if (vm->vm() == WasmVmNames::get().Wavm) {
    registerCallbackWavm(vm, functionName, f);
  } else {
    throw WasmVmException("unsupoorted wasm vm");
  }
}

template <typename F> void getFunction(WasmVm* vm, absl::string_view functionName, F* function) {
  if (vm->vm() == WasmVmNames::get().Wavm) {
    getFunctionWavm(vm, functionName, function);
  } else {
    throw WasmVmException("unsupoorted wasm vm");
  }
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
