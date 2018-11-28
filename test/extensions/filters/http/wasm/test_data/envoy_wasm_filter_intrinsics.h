/*
 * Intrinsic functions available to WASM filter modules.
 */
#include <string_view>

#include "envoy_wasm_intrinsics.h"


class WasmData {
public:
  WasmData(const char* data, size_t size) : data_(data), size_(size) {}
  ~WasmData() { ::free((void*)data_); }
  std::string_view view() { return {data_, size_}; }

  WasmData& operator=(const WasmData&) = delete;
  WasmData(const WasmData&) = delete;

private:
  const char* data_;
  size_t size_;
};
typedef std::unique_ptr<WasmData> WasmDataPtr;

enum class HeaderType : int { Header = 0, Trailer = 1 };

// Calls from WASM to Envoy.
extern "C" void getHeaderHandler(HeaderType type, const char* key_ptr, size_t key_size,
                                 const char** value_ptr, size_t* value_size);

inline WasmDataPtr getHeader(HeaderType type, std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  getHeaderHandler(type, key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

extern "C" void replaceHeaderHandler(HeaderType type, const char* key_ptr, size_t key_size,
                                     const char* value_ptr, size_t value_size);

inline void replaceHeader(HeaderType type, std::string_view key, std::string_view value) {
  replaceHeaderHandler(type, key.data(), key.size(), value.data(), value.size());
}


// Calls from Envoy to WASM.
extern "C" int main();

extern "C" void onCreate(int32_t context_id);
extern "C" void onStart(int32_t context_id);
extern "C" void onDestroy(int32_t context_id);
