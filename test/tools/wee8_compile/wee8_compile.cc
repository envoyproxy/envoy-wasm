// NOLINT(namespace-envoy)

#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "v8-version.h"
#include "wasm-api/wasm.hh"

namespace v8 {
namespace internal {
extern bool FLAG_wasm_opt;
} // namespace internal
} // namespace v8

uint32_t parseVarint(const byte_t*& pos, const byte_t* end) {
  uint32_t n = 0;
  uint32_t shift = 0;
  byte_t b;

  do {
    if (pos + 1 > end) {
      return static_cast<uint32_t>(-1);
    }
    b = *pos++;
    n += (b & 0x7f) << shift;
    shift += 7;
  } while ((b & 0x80) != 0);

  return n;
}

wasm::vec<byte_t> getVarint(uint32_t value) {
  byte_t bytes[5];
  int pos = 0;

  while (pos < 5) {
    if ((value & ~0x7F) == 0) {
      bytes[pos++] = static_cast<uint8_t>(value);
      break;
    }

    bytes[pos++] = static_cast<uint8_t>(value & 0x7F) | 0x80;
    value >>= 7;
  }

  auto vec = wasm::vec<byte_t>::make_uninitialized(pos);
  ::memcpy(vec.get(), bytes, pos);

  return vec;
}

wasm::vec<byte_t> readWasmModule(const char* path, const std::string& name) {
  // Open binary file.
  auto file = std::ifstream(path, std::ios::binary);
  file.seekg(0, std::ios_base::end);
  auto size = file.tellg();
  file.seekg(0);
  auto content = wasm::vec<byte_t>::make_uninitialized(size);
  file.read(content.get(), size);
  file.close();

  if (file.fail()) {
    std::cerr << "ERROR: Failed to read the input file from: " << path << std::endl;
    return wasm::vec<byte_t>::invalid();
  }

  // Parse custom sections to see if precompiled module already exists.
  const byte_t* end = content.get() + content.size();
  const byte_t* pos = content.get() + 8; // skip header
  while (pos < end) {
    if (pos + 1 > end) {
      std::cerr << "ERROR: Failed to parse corrupted Wasm module from: " << path << std::endl;
      return wasm::vec<byte_t>::invalid();
    }
    auto type = *pos++;
    auto rest = parseVarint(pos, end);
    if (rest == static_cast<uint32_t>(-1) || pos + rest > end) {
      std::cerr << "ERROR: Failed to parse corrupted Wasm module from: " << path << std::endl;
      return wasm::vec<byte_t>::invalid();
    }
    if (type == 0 /* custom section */) {
      auto start = pos;
      auto len = parseVarint(pos, end);
      if (len == static_cast<uint32_t>(-1) || pos + len > end) {
        std::cerr << "ERROR: Failed to parse corrupted Wasm module from: " << path << std::endl;
        return wasm::vec<byte_t>::invalid();
      }
      pos += len;
      rest -= (pos - start);
      if (len == name.size() && ::memcmp(pos - len, name.data(), len) == 0) {
        std::cerr << "ERROR: Wasm module: " << path << " already contains precompiled module."
                  << std::endl;
        return wasm::vec<byte_t>::invalid();
      }
    }
    pos += rest;
  }

  return content;
}

wasm::vec<byte_t> serializeWasmModule(const char* path, const wasm::vec<byte_t>& content) {
  // Enable Wasm optimizations.
  v8::internal::FLAG_wasm_opt = true;

  const auto engine = wasm::Engine::make();
  if (engine == nullptr) {
    std::cerr << "ERROR: Failed to start V8." << std::endl;
    return wasm::vec<byte_t>::invalid();
  }

  const auto store = wasm::Store::make(engine.get());
  if (store == nullptr) {
    std::cerr << "ERROR: Failed to create V8 isolate." << std::endl;
    return wasm::vec<byte_t>::invalid();
  }

  const auto module = wasm::Module::make(store.get(), content);
  if (module == nullptr) {
    std::cerr << "ERROR: Failed to instantiate WebAssembly module from: " << path << std::endl;
    return wasm::vec<byte_t>::invalid();
  }

  // TODO(PiotrSikora): figure out how to hook the completion callback.
  sleep(3);

  return module->serialize();
}

bool writeWasmModule(const char* path, const wasm::vec<byte_t>& module,
                     const std::string& section_name, const wasm::vec<byte_t>& serialized) {
  auto file = std::fstream(path, std::ios::out | std::ios::binary);
  file.write(module.get(), module.size());
  char section_type = '\0'; // custom section
  file.write(&section_type, 1);
  auto section_name_len = getVarint(section_name.size());
  auto section_size = getVarint(section_name_len.size() + section_name.size() + serialized.size());
  file.write(section_size.get(), section_size.size());
  file.write(section_name_len.get(), section_name_len.size());
  file.write(section_name.data(), section_name.size());
  file.write(serialized.get(), serialized.size());
  file.close();

  if (file.fail()) {
    std::cerr << "ERROR: Failed to write the output file to: " << path << std::endl;
    return false;
  }

  const auto total_size = module.size() + 1 + section_size.size() + section_name_len.size() +
                          section_name.size() + serialized.size();
  std::cout << "Written " << total_size << " bytes (bytecode: " << module.size() << " bytes,"
            << " precompiled: " << serialized.size() << " bytes)." << std::endl;
  return true;
}

#if defined(__linux__) && defined(__x86_64__)
#define WEE8_PLATFORM "linux_x86_64"
#elif defined(__APPLE__) && defined(__x86_64__)
#define WEE8_PLATFORM "macos_x86_64"
#else
#define WEE8_PLATFORM ""
#endif

int main(int argc, char* argv[]) {
  if (sizeof(WEE8_PLATFORM) - 1 == 0) {
    std::cerr << "Unsupported platform." << std::endl;
    return EXIT_FAILURE;
  }

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <input> <output>" << std::endl;
    return EXIT_FAILURE;
  }

  const std::string section_name = "precompiled_wee8_v" + std::to_string(V8_MAJOR_VERSION) + "." +
                                   std::to_string(V8_MINOR_VERSION) + "." +
                                   std::to_string(V8_BUILD_NUMBER) + "." +
                                   std::to_string(V8_PATCH_LEVEL) + "_" + WEE8_PLATFORM;

  const auto module = readWasmModule(argv[1], section_name);
  if (!module) {
    return EXIT_FAILURE;
  }

  const auto serialized = serializeWasmModule(argv[1], module);
  if (!serialized) {
    return EXIT_FAILURE;
  }

  if (!writeWasmModule(argv[2], module, section_name, serialized)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
