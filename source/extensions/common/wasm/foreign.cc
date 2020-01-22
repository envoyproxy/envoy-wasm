#include "extensions/common/wasm/foreign.h"

#include "zlib.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

class CompressFactory : public ForeignFunctionFactory {
public:
  std::string name() const override { return "compress"; }
  WasmForeignFunction create() const override {
    WasmForeignFunction f = [](Wasm&, absl::string_view arguments,
                               std::function<void*(size_t size)> alloc_result) -> WasmResult {
      unsigned long dest_len = compressBound(arguments.size());
      std::unique_ptr<unsigned char[]> b(new unsigned char[dest_len]);
      if (compress(b.get(), &dest_len, reinterpret_cast<const unsigned char*>(arguments.data()),
                   arguments.size()) != Z_OK) {
        return WasmResult::SerializationFailure;
      }
      auto result = alloc_result(dest_len);
      memcpy(result, b.get(), dest_len);
      return WasmResult::Ok;
    };
    return f;
  }
};
static Registry::RegisterFactory<CompressFactory, ForeignFunctionFactory> register_compress_;

class UncompressFactory : public ForeignFunctionFactory {
public:
  std::string name() const override { return "uncompress"; }
  WasmForeignFunction create() const override {
    WasmForeignFunction f = [](Wasm&, absl::string_view arguments,
                               std::function<void*(size_t size)> alloc_result) -> WasmResult {
      unsigned long dest_len = arguments.size() * 2 + 2; // output estimate.
      while (1) {
        std::unique_ptr<unsigned char[]> b(new unsigned char[dest_len]);
        auto r =
            uncompress(b.get(), &dest_len, reinterpret_cast<const unsigned char*>(arguments.data()),
                       arguments.size());
        if (r == Z_OK) {
          auto result = alloc_result(dest_len);
          memcpy(result, b.get(), dest_len);
          return WasmResult::Ok;
        }
        if (r != Z_MEM_ERROR) {
          return WasmResult::SerializationFailure;
        }
        dest_len = dest_len * 2;
      }
    };
    return f;
  }
};
static Registry::RegisterFactory<UncompressFactory, ForeignFunctionFactory> register_uncompress_;

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
