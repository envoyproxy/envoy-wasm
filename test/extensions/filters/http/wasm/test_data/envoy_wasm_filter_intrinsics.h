/*
 * Intrinsic functions available to WASM filter modules.
 */

#include "envoy_wasm_intrinsics.h"

class WasmData {
  WasmData(const char* data, size_t size) : data_(data), size_(size) {}
  ~WasmData() { ::free(data_); }
  std::string_view view() { return {data_, size_}; }

  WasmData& operator=(const WasmData&) = delete;
  WasmData(const WasmData&) = delete;

private:
  const char* data_;
  size_t size_;
};
typedef std::unique_ptr<WasmData> WasmDataPtr;

enum class HeaderType : int { Header = 0, Trailer = 1 };

extern "C" void getHeaderHandler(HeaderType type, const char* key_ptr, size_t key_size,
                                 const char** value_ptr, size_t* value_size);

inline WasmDataPtr getHeader(HeaderType type, absl::string_view key) {
  char* value_ptr = nullptr;
  size_t value_size = 0;
  getHeaderHandler(type, key.c_ptr(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

extern "C" void replaceHeaderHandler(HeaderType type, const char* key_ptr, size_t key_size,
                                     const char* value_ptr, size_t value_size);

inline void replaceHeader(HeaderType type, absl::string_view key, absl::string_view value) {
  replaceHeaderHandler(type, key.c_ptr(), key.size(), value.c_ptr(), value.size());
}
