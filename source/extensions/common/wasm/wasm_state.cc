#include "extensions/common/wasm/wasm_state.h"

#include "flatbuffers/reflection.h"
#include "tools/flatbuffers_backed_impl.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Wasm {

google::api::expr::runtime::CelValue WasmState::exprValue(Protobuf::Arena* arena) const {
  if (initialized_) {
    switch (type_) {
    case WasmType::String:
      return google::api::expr::runtime::CelValue::CreateString(&value_);
    case WasmType::Bytes:
    case WasmType::Protobuf:
      return google::api::expr::runtime::CelValue::CreateBytes(&value_);
    case WasmType::FlatBuffers:
      return google::api::expr::runtime::CelValue::CreateMap(
          google::api::expr::runtime::CreateFlatBuffersBackedObject(
              reinterpret_cast<const uint8_t*>(value_.data()),
              *reflection::GetSchema(schema_.data()), arena));
    }
  }
  return google::api::expr::runtime::CelValue::CreateNull();
}
ProtobufTypes::MessagePtr WasmState::serializeAsProto() const {
  auto any = std::make_unique<ProtobufWkt::Any>();

  if (type_ != WasmType::Protobuf) {
    ProtobufWkt::BytesValue value;
    value.set_value(value_);
    any->PackFrom(value);
  } else {
    // The Wasm extension serialized in its own type.
    any->set_type_url(std::string(schema_));
    any->set_value(value_);
  }

  return any;
}

} // namespace Wasm
} // namespace Common
} // namespace Extensions
} // namespace Envoy
