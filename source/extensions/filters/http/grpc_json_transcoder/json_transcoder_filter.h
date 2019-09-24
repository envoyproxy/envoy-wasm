#pragma once

#include "envoy/api/api.h"
#include "envoy/buffer/buffer.h"
#include "envoy/config/filter/http/transcoder/v2/transcoder.pb.h"
#include "envoy/http/filter.h"
#include "envoy/http/header_map.h"
#include "envoy/json/json_object.h"

#include "common/common/logger.h"
#include "common/grpc/codec.h"
#include "common/protobuf/protobuf.h"

#include "extensions/filters/http/grpc_json_transcoder/transcoder_input_stream_impl.h"

#include "grpc_transcoding/path_matcher.h"
#include "grpc_transcoding/request_message_translator.h"
#include "grpc_transcoding/transcoder.h"
#include "grpc_transcoding/type_helper.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace GrpcJsonTranscoder {

/**
 * VariableBinding specifies a value for a single field in the request message.
 * When transcoding HTTP/REST/JSON to gRPC/proto the request message is
 * constructed using the HTTP body and the variable bindings (specified through
 * request url).
 * See https://github.com/googleapis/googleapis/blob/master/google/api/http.proto
 * for details of variable binding.
 */
struct VariableBinding {
  // The location of the field in the protobuf message, where the value
  // needs to be inserted, e.g. "shelf.theme" would mean the "theme" field
  // of the nested "shelf" message of the request protobuf message.
  std::vector<std::string> field_path;
  // The value to be inserted.
  std::string value;
};

/**
 * Global configuration for the gRPC JSON transcoder filter. Factory for the Transcoder interface.
 */
class JsonTranscoderConfig : public Logger::Loggable<Logger::Id::config> {
public:
  /**
   * constructor that loads protobuf descriptors from the file specified in the JSON config.
   * and construct a path matcher for HTTP path bindings.
   */
  JsonTranscoderConfig(
      const envoy::config::filter::http::transcoder::v2::GrpcJsonTranscoder& proto_config,
      Api::Api& api);

  /**
   * Create an instance of Transcoder interface based on incoming request
   * @param headers headers received from decoder
   * @param request_input a ZeroCopyInputStream reading from downstream request body
   * @param response_input a TranscoderInputStream reading from upstream response body
   * @param transcoder output parameter for the instance of Transcoder interface
   * @param method_descriptor output parameter for the method looked up from config
   * @return status whether the Transcoder instance are successfully created or not
   */
  ProtobufUtil::Status
  createTranscoder(const Http::HeaderMap& headers, Protobuf::io::ZeroCopyInputStream& request_input,
                   google::grpc::transcoding::TranscoderInputStream& response_input,
                   std::unique_ptr<google::grpc::transcoding::Transcoder>& transcoder,
                   const Protobuf::MethodDescriptor*& method_descriptor);

  /**
   * Converts an arbitrary protobuf message to JSON.
   */
  ProtobufUtil::Status translateProtoMessageToJson(const Protobuf::Message& message,
                                                   std::string* json_out);

  /**
   * If true, skip clearing the route cache after the incoming request has been modified.
   * This allows Envoy to select the upstream cluster based on the incoming request
   * rather than the outgoing.
   */
  bool matchIncomingRequestInfo() const;

  /**
   * If true, when trailer indicates a gRPC error and there was no HTTP body,
   * make google.rpc.Status out of gRPC status headers and use it as JSON body.
   */
  bool convertGrpcStatus() const;

private:
  /**
   * Convert method descriptor to RequestInfo that needed for transcoding library
   */
  ProtobufUtil::Status methodToRequestInfo(const Protobuf::MethodDescriptor* method,
                                           google::grpc::transcoding::RequestInfo* info);

private:
  void addFileDescriptor(const Protobuf::FileDescriptorProto& file);
  void addBuiltinSymbolDescriptor(const std::string& symbol_name);

  Protobuf::DescriptorPool descriptor_pool_;
  google::grpc::transcoding::PathMatcherPtr<const Protobuf::MethodDescriptor*> path_matcher_;
  std::unique_ptr<google::grpc::transcoding::TypeHelper> type_helper_;
  Protobuf::util::JsonPrintOptions print_options_;

  bool match_incoming_request_route_{false};
  bool ignore_unknown_query_parameters_{false};
  bool convert_grpc_status_{false};
};

using JsonTranscoderConfigSharedPtr = std::shared_ptr<JsonTranscoderConfig>;

/**
 * The filter instance for gRPC JSON transcoder.
 */
class JsonTranscoderFilter : public Http::StreamFilter, public Logger::Loggable<Logger::Id::http2> {
public:
  JsonTranscoderFilter(JsonTranscoderConfig& config);

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override;
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override;

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override {
    return Http::FilterHeadersStatus::Continue;
  }
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override;
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap&) override {
    return Http::FilterMetadataStatus::Continue;
  }
  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override;

  // Http::StreamFilterBase
  void onDestroy() override {}

private:
  bool readToBuffer(Protobuf::io::ZeroCopyInputStream& stream, Buffer::Instance& data);
  void buildResponseFromHttpBodyOutput(Http::HeaderMap& response_headers, Buffer::Instance& data);
  bool maybeConvertGrpcStatus(Grpc::Status::GrpcStatus grpc_status, Http::HeaderMap& trailers);
  bool hasHttpBodyAsOutputType();

  JsonTranscoderConfig& config_;
  std::unique_ptr<google::grpc::transcoding::Transcoder> transcoder_;
  TranscoderInputStreamImpl request_in_;
  TranscoderInputStreamImpl response_in_;
  Http::StreamDecoderFilterCallbacks* decoder_callbacks_{nullptr};
  Http::StreamEncoderFilterCallbacks* encoder_callbacks_{nullptr};
  const Protobuf::MethodDescriptor* method_{nullptr};
  Http::HeaderMap* response_headers_{nullptr};
  Grpc::Decoder decoder_;

  bool error_{false};
  bool has_http_body_output_{false};
  bool has_body_{false};
};

} // namespace GrpcJsonTranscoder
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
