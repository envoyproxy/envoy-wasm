#include "extensions/filters/http/grpc_json_transcoder/json_transcoder_filter.h"

#include <memory>
#include <unordered_set>

#include "envoy/common/exception.h"
#include "envoy/http/filter.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/enum_to_int.h"
#include "common/common/utility.h"
#include "common/grpc/common.h"
#include "common/http/headers.h"
#include "common/http/utility.h"
#include "common/protobuf/protobuf.h"
#include "common/protobuf/utility.h"

#include "google/api/annotations.pb.h"
#include "google/api/http.pb.h"
#include "google/api/httpbody.pb.h"
#include "grpc_transcoding/json_request_translator.h"
#include "grpc_transcoding/path_matcher_utility.h"
#include "grpc_transcoding/response_to_json_translator.h"

using Envoy::Protobuf::FileDescriptorSet;
using Envoy::Protobuf::io::ZeroCopyInputStream;
using Envoy::ProtobufUtil::Status;
using Envoy::ProtobufUtil::error::Code;
using google::api::HttpRule;
using google::grpc::transcoding::JsonRequestTranslator;
using google::grpc::transcoding::PathMatcherBuilder;
using google::grpc::transcoding::PathMatcherUtility;
using google::grpc::transcoding::RequestInfo;
using google::grpc::transcoding::ResponseToJsonTranslator;
using google::grpc::transcoding::Transcoder;
using google::grpc::transcoding::TranscoderInputStream;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace GrpcJsonTranscoder {

struct RcDetailsValues {
  // The gRPC json transcoder filter failed to transcode when processing request headers.
  // This will generally be accompanied by details about the transcoder failure.
  const std::string GrpcTranscodeFailedEarly = "early_grpc_json_transcode_failure";
  // The gRPC json transcoder filter failed to transcode when processing the request body.
  // This will generally be accompanied by details about the transcoder failure.
  const std::string GrpcTranscodeFailed = "grpc_json_transcode_failure";
};
using RcDetails = ConstSingleton<RcDetailsValues>;

namespace {

const Http::LowerCaseString& trailerHeader() {
  CONSTRUCT_ON_FIRST_USE(Http::LowerCaseString, "trailer");
}

// Transcoder:
// https://github.com/grpc-ecosystem/grpc-httpjson-transcoding/blob/master/src/include/grpc_transcoding/transcoder.h
// implementation based on JsonRequestTranslator & ResponseToJsonTranslator
class TranscoderImpl : public Transcoder {
public:
  /**
   * Construct a transcoder implementation
   * @param request_translator a JsonRequestTranslator that does the request translation
   * @param response_translator a ResponseToJsonTranslator that does the response translation
   */
  TranscoderImpl(std::unique_ptr<JsonRequestTranslator> request_translator,
                 std::unique_ptr<ResponseToJsonTranslator> response_translator)
      : request_translator_(std::move(request_translator)),
        response_translator_(std::move(response_translator)),
        request_stream_(request_translator_->Output().CreateInputStream()),
        response_stream_(response_translator_->CreateInputStream()) {}

  // Transcoder
  ::google::grpc::transcoding::TranscoderInputStream* RequestOutput() override {
    return request_stream_.get();
  }
  ProtobufUtil::Status RequestStatus() override { return request_translator_->Output().Status(); }

  ZeroCopyInputStream* ResponseOutput() override { return response_stream_.get(); }
  ProtobufUtil::Status ResponseStatus() override { return response_translator_->Status(); }

private:
  std::unique_ptr<JsonRequestTranslator> request_translator_;
  std::unique_ptr<ResponseToJsonTranslator> response_translator_;
  std::unique_ptr<TranscoderInputStream> request_stream_;
  std::unique_ptr<TranscoderInputStream> response_stream_;
};

} // namespace

JsonTranscoderConfig::JsonTranscoderConfig(
    const envoy::config::filter::http::transcoder::v2::GrpcJsonTranscoder& proto_config,
    Api::Api& api) {
  FileDescriptorSet descriptor_set;

  switch (proto_config.descriptor_set_case()) {
  case envoy::config::filter::http::transcoder::v2::GrpcJsonTranscoder::kProtoDescriptor:
    if (!descriptor_set.ParseFromString(
            api.fileSystem().fileReadToEnd(proto_config.proto_descriptor()))) {
      throw EnvoyException("transcoding_filter: Unable to parse proto descriptor");
    }
    break;
  case envoy::config::filter::http::transcoder::v2::GrpcJsonTranscoder::kProtoDescriptorBin:
    if (!descriptor_set.ParseFromString(proto_config.proto_descriptor_bin())) {
      throw EnvoyException("transcoding_filter: Unable to parse proto descriptor");
    }
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  for (const auto& file : descriptor_set.file()) {
    addFileDescriptor(file);
  }

  convert_grpc_status_ = proto_config.convert_grpc_status();
  if (convert_grpc_status_) {
    addBuiltinSymbolDescriptor("google.protobuf.Any");
    addBuiltinSymbolDescriptor("google.rpc.Status");
  }

  PathMatcherBuilder<const Protobuf::MethodDescriptor*> pmb;
  std::unordered_set<std::string> ignored_query_parameters;
  for (const auto& query_param : proto_config.ignored_query_parameters()) {
    ignored_query_parameters.insert(query_param);
  }

  for (const auto& service_name : proto_config.services()) {
    auto service = descriptor_pool_.FindServiceByName(service_name);
    if (service == nullptr) {
      throw EnvoyException("transcoding_filter: Could not find '" + service_name +
                           "' in the proto descriptor");
    }
    for (int i = 0; i < service->method_count(); ++i) {
      auto method = service->method(i);

      HttpRule http_rule;
      if (method->options().HasExtension(google::api::http)) {
        http_rule = method->options().GetExtension(google::api::http);
      } else if (proto_config.auto_mapping()) {
        auto post = "/" + service->full_name() + "/" + method->name();
        http_rule.set_post(post);
        http_rule.set_body("*");
      }

      if (!PathMatcherUtility::RegisterByHttpRule(pmb, http_rule, ignored_query_parameters,
                                                  method)) {
        throw EnvoyException("transcoding_filter: Cannot register '" + method->full_name() +
                             "' to path matcher");
      }
    }
  }

  path_matcher_ = pmb.Build();

  type_helper_ = std::make_unique<google::grpc::transcoding::TypeHelper>(
      Protobuf::util::NewTypeResolverForDescriptorPool(Grpc::Common::typeUrlPrefix(),
                                                       &descriptor_pool_));

  const auto& print_config = proto_config.print_options();
  print_options_.add_whitespace = print_config.add_whitespace();
  print_options_.always_print_primitive_fields = print_config.always_print_primitive_fields();
  print_options_.always_print_enums_as_ints = print_config.always_print_enums_as_ints();
  print_options_.preserve_proto_field_names = print_config.preserve_proto_field_names();

  match_incoming_request_route_ = proto_config.match_incoming_request_route();
  ignore_unknown_query_parameters_ = proto_config.ignore_unknown_query_parameters();
}

void JsonTranscoderConfig::addFileDescriptor(const Protobuf::FileDescriptorProto& file) {
  if (descriptor_pool_.BuildFile(file) == nullptr) {
    throw EnvoyException("transcoding_filter: Unable to build proto descriptor pool");
  }
}

void JsonTranscoderConfig::addBuiltinSymbolDescriptor(const std::string& symbol_name) {
  if (descriptor_pool_.FindFileContainingSymbol(symbol_name) != nullptr) {
    return;
  }

  auto* builtin_pool = Protobuf::DescriptorPool::generated_pool();
  if (!builtin_pool) {
    return;
  }

  Protobuf::DescriptorPoolDatabase pool_database(*builtin_pool);
  Protobuf::FileDescriptorProto file_proto;
  pool_database.FindFileContainingSymbol(symbol_name, &file_proto);
  addFileDescriptor(file_proto);
}

bool JsonTranscoderConfig::matchIncomingRequestInfo() const {
  return match_incoming_request_route_;
}

bool JsonTranscoderConfig::convertGrpcStatus() const { return convert_grpc_status_; }

ProtobufUtil::Status JsonTranscoderConfig::createTranscoder(
    const Http::HeaderMap& headers, ZeroCopyInputStream& request_input,
    google::grpc::transcoding::TranscoderInputStream& response_input,
    std::unique_ptr<Transcoder>& transcoder, const Protobuf::MethodDescriptor*& method_descriptor) {
  if (Grpc::Common::hasGrpcContentType(headers)) {
    return ProtobufUtil::Status(Code::INVALID_ARGUMENT,
                                "Request headers has application/grpc content-type");
  }
  const std::string method(headers.Method()->value().getStringView());
  std::string path(headers.Path()->value().getStringView());
  std::string args;

  const size_t pos = path.find('?');
  if (pos != std::string::npos) {
    args = path.substr(pos + 1);
    path = path.substr(0, pos);
  }

  struct RequestInfo request_info;
  std::vector<VariableBinding> variable_bindings;
  method_descriptor =
      path_matcher_->Lookup(method, path, args, &variable_bindings, &request_info.body_field_path);
  if (!method_descriptor) {
    return ProtobufUtil::Status(Code::NOT_FOUND, "Could not resolve " + path + " to a method");
  }

  auto status = methodToRequestInfo(method_descriptor, &request_info);
  if (!status.ok()) {
    return status;
  }

  for (const auto& binding : variable_bindings) {
    google::grpc::transcoding::RequestWeaver::BindingInfo resolved_binding;
    status = type_helper_->ResolveFieldPath(*request_info.message_type, binding.field_path,
                                            &resolved_binding.field_path);
    if (!status.ok()) {
      if (ignore_unknown_query_parameters_) {
        continue;
      }
      return status;
    }

    resolved_binding.value = binding.value;

    request_info.variable_bindings.emplace_back(std::move(resolved_binding));
  }

  std::unique_ptr<JsonRequestTranslator> request_translator{
      new JsonRequestTranslator(type_helper_->Resolver(), &request_input, request_info,
                                method_descriptor->client_streaming(), true)};

  const auto response_type_url =
      Grpc::Common::typeUrl(method_descriptor->output_type()->full_name());
  std::unique_ptr<ResponseToJsonTranslator> response_translator{new ResponseToJsonTranslator(
      type_helper_->Resolver(), response_type_url, method_descriptor->server_streaming(),
      &response_input, print_options_)};

  transcoder = std::make_unique<TranscoderImpl>(std::move(request_translator),
                                                std::move(response_translator));
  return ProtobufUtil::Status();
}

ProtobufUtil::Status
JsonTranscoderConfig::methodToRequestInfo(const Protobuf::MethodDescriptor* method,
                                          google::grpc::transcoding::RequestInfo* info) {
  auto request_type_url = Grpc::Common::typeUrl(method->input_type()->full_name());
  info->message_type = type_helper_->Info()->GetTypeByTypeUrl(request_type_url);
  if (info->message_type == nullptr) {
    ENVOY_LOG(debug, "Cannot resolve input-type: {}", method->input_type()->full_name());
    return ProtobufUtil::Status(Code::NOT_FOUND,
                                "Could not resolve type: " + method->input_type()->full_name());
  }

  return ProtobufUtil::Status();
}

ProtobufUtil::Status
JsonTranscoderConfig::translateProtoMessageToJson(const Protobuf::Message& message,
                                                  std::string* json_out) {
  return ProtobufUtil::BinaryToJsonString(
      type_helper_->Resolver(), Grpc::Common::typeUrl(message.GetDescriptor()->full_name()),
      message.SerializeAsString(), json_out, print_options_);
}

JsonTranscoderFilter::JsonTranscoderFilter(JsonTranscoderConfig& config) : config_(config) {}

Http::FilterHeadersStatus JsonTranscoderFilter::decodeHeaders(Http::HeaderMap& headers,
                                                              bool end_stream) {
  const auto status =
      config_.createTranscoder(headers, request_in_, response_in_, transcoder_, method_);

  if (!status.ok()) {
    // If transcoder couldn't be created, it might be a normal gRPC request, so the filter will
    // just pass-through the request to upstream.
    return Http::FilterHeadersStatus::Continue;
  }
  has_http_body_output_ = !method_->server_streaming() && hasHttpBodyAsOutputType();

  headers.removeContentLength();
  headers.insertContentType().value().setReference(Http::Headers::get().ContentTypeValues.Grpc);
  headers.insertEnvoyOriginalPath().value(*headers.Path());
  headers.insertPath().value("/" + method_->service()->full_name() + "/" + method_->name());
  headers.insertMethod().value().setReference(Http::Headers::get().MethodValues.Post);
  headers.insertTE().value().setReference(Http::Headers::get().TEValues.Trailers);

  if (!config_.matchIncomingRequestInfo()) {
    decoder_callbacks_->clearRouteCache();
  }

  if (end_stream) {
    request_in_.finish();

    const auto& request_status = transcoder_->RequestStatus();
    if (!request_status.ok()) {
      ENVOY_LOG(debug, "Transcoding request error {}", request_status.ToString());
      error_ = true;
      decoder_callbacks_->sendLocalReply(
          Http::Code::BadRequest,
          absl::string_view(request_status.error_message().data(),
                            request_status.error_message().size()),
          nullptr, absl::nullopt,
          absl::StrCat(RcDetails::get().GrpcTranscodeFailedEarly, "{",
                       MessageUtil::CodeEnumToString(request_status.code()), "}"));

      return Http::FilterHeadersStatus::StopIteration;
    }

    Buffer::OwnedImpl data;
    readToBuffer(*transcoder_->RequestOutput(), data);

    if (data.length() > 0) {
      decoder_callbacks_->addDecodedData(data, true);
    }
  }
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterDataStatus JsonTranscoderFilter::decodeData(Buffer::Instance& data, bool end_stream) {
  ASSERT(!error_);

  if (!transcoder_) {
    return Http::FilterDataStatus::Continue;
  }

  request_in_.move(data);

  if (end_stream) {
    request_in_.finish();
  }

  readToBuffer(*transcoder_->RequestOutput(), data);

  const auto& request_status = transcoder_->RequestStatus();

  if (!request_status.ok()) {
    ENVOY_LOG(debug, "Transcoding request error {}", request_status.ToString());
    error_ = true;
    decoder_callbacks_->sendLocalReply(
        Http::Code::BadRequest,
        absl::string_view(request_status.error_message().data(),
                          request_status.error_message().size()),
        nullptr, absl::nullopt,
        absl::StrCat(RcDetails::get().GrpcTranscodeFailed, "{",
                     MessageUtil::CodeEnumToString(request_status.code()), "}"));

    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
  return Http::FilterDataStatus::Continue;
}

Http::FilterTrailersStatus JsonTranscoderFilter::decodeTrailers(Http::HeaderMap&) {
  ASSERT(!error_);

  if (!transcoder_) {
    return Http::FilterTrailersStatus::Continue;
  }

  request_in_.finish();

  Buffer::OwnedImpl data;
  readToBuffer(*transcoder_->RequestOutput(), data);

  if (data.length()) {
    decoder_callbacks_->addDecodedData(data, true);
  }
  return Http::FilterTrailersStatus::Continue;
}

void JsonTranscoderFilter::setDecoderFilterCallbacks(
    Http::StreamDecoderFilterCallbacks& callbacks) {
  decoder_callbacks_ = &callbacks;
}

Http::FilterHeadersStatus JsonTranscoderFilter::encodeHeaders(Http::HeaderMap& headers,
                                                              bool end_stream) {
  if (!Grpc::Common::isGrpcResponseHeader(headers, end_stream)) {
    error_ = true;
  }

  if (error_ || !transcoder_) {
    return Http::FilterHeadersStatus::Continue;
  }

  response_headers_ = &headers;

  if (end_stream) {

    if (method_->server_streaming()) {
      // When there is no body in a streaming response, a empty JSON array is
      // returned by default. Set the content type correctly.
      headers.insertContentType().value().setReference(Http::Headers::get().ContentTypeValues.Json);
    }

    // In gRPC wire protocol, headers frame with end_stream is a trailers-only response.
    // The return value from encodeTrailers is ignored since it is always continue.
    encodeTrailers(headers);

    return Http::FilterHeadersStatus::Continue;
  }

  headers.insertContentType().value().setReference(Http::Headers::get().ContentTypeValues.Json);
  if (!method_->server_streaming()) {
    return Http::FilterHeadersStatus::StopIteration;
  }

  return Http::FilterHeadersStatus::Continue;
}

Http::FilterDataStatus JsonTranscoderFilter::encodeData(Buffer::Instance& data, bool end_stream) {
  if (error_ || !transcoder_) {
    return Http::FilterDataStatus::Continue;
  }

  has_body_ = true;

  // TODO(dio): Add support for streaming case.
  if (has_http_body_output_) {
    buildResponseFromHttpBodyOutput(*response_headers_, data);
    return Http::FilterDataStatus::StopIterationAndBuffer;
  }

  response_in_.move(data);

  if (end_stream) {
    response_in_.finish();
  }

  readToBuffer(*transcoder_->ResponseOutput(), data);

  if (!method_->server_streaming() && !end_stream) {
    // Buffer until the response is complete.
    return Http::FilterDataStatus::StopIterationAndBuffer;
  }
  // TODO(lizan): Check ResponseStatus

  return Http::FilterDataStatus::Continue;
}

Http::FilterTrailersStatus JsonTranscoderFilter::encodeTrailers(Http::HeaderMap& trailers) {
  if (error_ || !transcoder_) {
    return Http::FilterTrailersStatus::Continue;
  }

  response_in_.finish();

  const absl::optional<Grpc::Status::GrpcStatus> grpc_status =
      Grpc::Common::getGrpcStatus(trailers);
  if (grpc_status && maybeConvertGrpcStatus(*grpc_status, trailers)) {
    return Http::FilterTrailersStatus::Continue;
  }

  Buffer::OwnedImpl data;
  readToBuffer(*transcoder_->ResponseOutput(), data);

  if (data.length()) {
    encoder_callbacks_->addEncodedData(data, true);
  }

  if (method_->server_streaming()) {
    // For streaming case, the headers are already sent, so just continue here.
    return Http::FilterTrailersStatus::Continue;
  }

  // If there was no previous headers frame, this |trailers| map is our |response_headers_|,
  // so there is no need to copy headers from one to the other.
  bool is_trailers_only_response = response_headers_ == &trailers;

  if (!grpc_status || grpc_status.value() == Grpc::Status::GrpcStatus::InvalidCode) {
    response_headers_->Status()->value(enumToInt(Http::Code::ServiceUnavailable));
  } else {
    response_headers_->Status()->value(Grpc::Utility::grpcToHttpStatus(grpc_status.value()));
    if (!is_trailers_only_response) {
      response_headers_->insertGrpcStatus().value(enumToInt(grpc_status.value()));
    }
  }

  if (!is_trailers_only_response) {
    // Copy the grpc-message header if it exists.
    const Http::HeaderEntry* grpc_message_header = trailers.GrpcMessage();
    if (grpc_message_header) {
      response_headers_->insertGrpcMessage().value(*grpc_message_header);
    }
  }

  // remove Trailer headers if the client connection was http/1
  if (encoder_callbacks_->streamInfo().protocol() < Http::Protocol::Http2) {
    response_headers_->remove(trailerHeader());
  }

  response_headers_->insertContentLength().value(
      encoder_callbacks_->encodingBuffer() ? encoder_callbacks_->encodingBuffer()->length() : 0);
  return Http::FilterTrailersStatus::Continue;
}

void JsonTranscoderFilter::setEncoderFilterCallbacks(
    Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

// TODO(lizan): Incorporate watermarks to bound buffer sizes
bool JsonTranscoderFilter::readToBuffer(Protobuf::io::ZeroCopyInputStream& stream,
                                        Buffer::Instance& data) {
  const void* out;
  int size;
  while (stream.Next(&out, &size)) {
    data.add(out, size);

    if (size == 0) {
      return true;
    }
  }
  return false;
}

void JsonTranscoderFilter::buildResponseFromHttpBodyOutput(Http::HeaderMap& response_headers,
                                                           Buffer::Instance& data) {
  std::vector<Grpc::Frame> frames;
  decoder_.decode(data, frames);
  if (frames.empty()) {
    return;
  }

  google::api::HttpBody http_body;
  for (auto& frame : frames) {
    if (frame.length_ > 0) {
      Buffer::ZeroCopyInputStreamImpl stream(std::move(frame.data_));
      http_body.ParseFromZeroCopyStream(&stream);
      const auto& body = http_body.data();

      data.add(body);

      response_headers.insertContentType().value(http_body.content_type());
      response_headers.insertContentLength().value(body.size());
      return;
    }
  }
}

bool JsonTranscoderFilter::maybeConvertGrpcStatus(Grpc::Status::GrpcStatus grpc_status,
                                                  Http::HeaderMap& trailers) {
  if (!config_.convertGrpcStatus()) {
    return false;
  }

  // Send a serialized status only if there was no body.
  if (has_body_) {
    return false;
  }

  if (grpc_status == Grpc::Status::GrpcStatus::Ok ||
      grpc_status == Grpc::Status::GrpcStatus::InvalidCode) {
    return false;
  }

  auto status_details = Grpc::Common::getGrpcStatusDetailsBin(trailers);
  if (!status_details) {
    // If no rpc.Status object was sent in the grpc-status-details-bin header,
    // construct it from the grpc-status and grpc-message headers.
    status_details.emplace();
    status_details->set_code(grpc_status);

    auto grpc_message_header = trailers.GrpcMessage();
    if (grpc_message_header) {
      auto message = grpc_message_header->value().getStringView();
      status_details->set_message(message.data(), message.size());
    }
  }

  std::string json_status;
  auto translate_status = config_.translateProtoMessageToJson(*status_details, &json_status);
  if (!translate_status.ok()) {
    ENVOY_LOG(debug, "Transcoding status error {}", translate_status.ToString());
    return false;
  }

  response_headers_->Status()->value(Grpc::Utility::grpcToHttpStatus(grpc_status));

  bool is_trailers_only_response = response_headers_ == &trailers;
  if (is_trailers_only_response) {
    // Drop the gRPC status headers, we already have them in the JSON body.
    response_headers_->removeGrpcStatus();
    response_headers_->removeGrpcMessage();
    response_headers_->remove(Http::Headers::get().GrpcStatusDetailsBin);
  }

  // remove Trailer headers if the client connection was http/1
  if (encoder_callbacks_->streamInfo().protocol() < Http::Protocol::Http2) {
    response_headers_->remove(trailerHeader());
  }

  response_headers_->insertContentType().value().setReference(
      Http::Headers::get().ContentTypeValues.Json);

  response_headers_->insertContentLength().value(json_status.length());

  Buffer::OwnedImpl status_data(json_status);
  encoder_callbacks_->addEncodedData(status_data, false);
  return true;
}

bool JsonTranscoderFilter::hasHttpBodyAsOutputType() {
  return method_->output_type()->full_name() == google::api::HttpBody::descriptor()->full_name();
}

} // namespace GrpcJsonTranscoder
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
