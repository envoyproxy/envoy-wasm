#pragma once

#include <functional>
#include <string>
#include <vector>

#include "envoy/common/time.h"
#include "envoy/config/core/v3/base.pb.h"
#include "envoy/formatter/substitution_formatter.h"
#include "envoy/stream_info/stream_info.h"

#include "common/common/utility.h"

#include "absl/container/flat_hash_map.h"
#include "absl/types/optional.h"

namespace Envoy {
namespace Formatter {

/**
 * Access log format parser.
 */
class SubstitutionFormatParser {
public:
  static std::vector<FormatterProviderPtr> parse(const std::string& format);

private:
  /**
   * Parse a header format rule of the form: %REQ(X?Y):Z% .
   * Will populate a main_header and an optional alternative header if specified.
   * See doc:
   * docs/root/configuration/access_log.rst#format-rules
   */
  static void parseCommandHeader(const std::string& token, const size_t start,
                                 std::string& main_header, std::string& alternative_header,
                                 absl::optional<size_t>& max_length);

  /**
   * General parse command utility. Will parse token from start position. Token is expected to end
   * with ')'. An optional ":max_length" may be specified after the closing ')' char. Token may
   * contain multiple values separated by "separator" string. First value will be populated in
   * "main" and any additional sub values will be set in the vector "subitems". For example token
   * of: "com.test.my_filter:test_object:inner_key):100" with separator of ":" will set the
   * following:
   * - main: com.test.my_filter
   * - subitems: {test_object, inner_key}
   * - max_length: 100
   *
   * @param token the token to parse
   * @param start the index to start parsing from
   * @param separator separator between values
   * @param main the first value
   * @param sub_items any additional values
   * @param max_length optional max_length will be populated if specified
   *
   * TODO(glicht) Rewrite with a parser library. See:
   * https://github.com/envoyproxy/envoy/issues/2967
   */
  static void parseCommand(const std::string& token, const size_t start,
                           const std::string& separator, std::string& main,
                           std::vector<std::string>& sub_items, absl::optional<size_t>& max_length);

  // the indexes of where the parameters for each directive is expected to begin
  static const size_t ReqParamStart{sizeof("REQ(") - 1};
  static const size_t RespParamStart{sizeof("RESP(") - 1};
  static const size_t TrailParamStart{sizeof("TRAILER(") - 1};
  static const size_t StartTimeParamStart{sizeof("START_TIME(") - 1};
};

/**
 * Util class for access log format.
 */
class SubstitutionFormatUtils {
public:
  static FormatterPtr defaultSubstitutionFormatter();
  static const std::string& protocolToString(const absl::optional<Http::Protocol>& protocol);
  static const std::string getHostname();

private:
  SubstitutionFormatUtils();

  static const std::string DEFAULT_FORMAT;
};

/**
 * Composite formatter implementation.
 */
class FormatterImpl : public Formatter {
public:
  FormatterImpl(const std::string& format);

  // Formatter::format
  std::string format(const Http::RequestHeaderMap& request_headers,
                     const Http::ResponseHeaderMap& response_headers,
                     const Http::ResponseTrailerMap& response_trailers,
                     const StreamInfo::StreamInfo& stream_info,
                     absl::string_view local_reply_body) const override;

private:
  std::vector<FormatterProviderPtr> providers_;
};

class JsonFormatterImpl : public Formatter {
public:
  JsonFormatterImpl(const ProtobufWkt::Struct& format_mapping, bool preserve_types)
      : preserve_types_(preserve_types), json_output_format_(toFormatMap(format_mapping)) {}

  // Formatter::format
  std::string format(const Http::RequestHeaderMap& request_headers,
                     const Http::ResponseHeaderMap& response_headers,
                     const Http::ResponseTrailerMap& response_trailers,
                     const StreamInfo::StreamInfo& stream_info,
                     absl::string_view local_reply_body) const override;

private:
  struct JsonFormatMapWrapper;
  using JsonFormatMapValue =
      absl::variant<const std::vector<FormatterProviderPtr>, const JsonFormatMapWrapper>;
  // Although not required for JSON, it is nice to have the order of properties
  // preserved between the format and the log entry, thus std::map.
  using JsonFormatMap = std::map<std::string, JsonFormatMapValue>;
  using JsonFormatMapPtr = std::unique_ptr<JsonFormatMap>;
  struct JsonFormatMapWrapper {
    JsonFormatMapPtr value_;
  };

  const bool preserve_types_;
  const JsonFormatMapWrapper json_output_format_;

  ProtobufWkt::Struct toStruct(const Http::RequestHeaderMap& request_headers,
                               const Http::ResponseHeaderMap& response_headers,
                               const Http::ResponseTrailerMap& response_trailers,
                               const StreamInfo::StreamInfo& stream_info,
                               absl::string_view local_reply_body) const;
  JsonFormatMapWrapper toFormatMap(const ProtobufWkt::Struct& json_format) const;
};

/**
 * FormatterProvider for string literals. It ignores headers and stream info and returns string by
 * which it was initialized.
 */
class PlainStringFormatter : public FormatterProvider {
public:
  PlainStringFormatter(const std::string& str);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;

private:
  ProtobufWkt::Value str_;
};

/**
 * FormatterProvider for local_reply_body. It returns the string from `local_reply_body` argument.
 */
class LocalReplyBodyFormatter : public FormatterProvider {
public:
  LocalReplyBodyFormatter() = default;

  // Formatter::format
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view local_reply_body) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view local_reply_body) const override;
};

class HeaderFormatter {
public:
  HeaderFormatter(const std::string& main_header, const std::string& alternative_header,
                  absl::optional<size_t> max_length);

protected:
  std::string format(const Http::HeaderMap& headers) const;
  ProtobufWkt::Value formatValue(const Http::HeaderMap& headers) const;

private:
  const Http::HeaderEntry* findHeader(const Http::HeaderMap& headers) const;

  Http::LowerCaseString main_header_;
  Http::LowerCaseString alternative_header_;
  absl::optional<size_t> max_length_;
};

/**
 * FormatterProvider for request headers.
 */
class RequestHeaderFormatter : public FormatterProvider, HeaderFormatter {
public:
  RequestHeaderFormatter(const std::string& main_header, const std::string& alternative_header,
                         absl::optional<size_t> max_length);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap& request_headers, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;
};

/**
 * FormatterProvider for response headers.
 */
class ResponseHeaderFormatter : public FormatterProvider, HeaderFormatter {
public:
  ResponseHeaderFormatter(const std::string& main_header, const std::string& alternative_header,
                          absl::optional<size_t> max_length);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap& response_headers,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;
};

/**
 * FormatterProvider for response trailers.
 */
class ResponseTrailerFormatter : public FormatterProvider, HeaderFormatter {
public:
  ResponseTrailerFormatter(const std::string& main_header, const std::string& alternative_header,
                           absl::optional<size_t> max_length);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap& response_trailers,
                     const StreamInfo::StreamInfo&, absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;
};

/**
 * FormatterProvider for grpc-status
 */
class GrpcStatusFormatter : public FormatterProvider, HeaderFormatter {
public:
  GrpcStatusFormatter(const std::string& main_header, const std::string& alternative_header,
                      absl::optional<size_t> max_length);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap& response_headers,
                     const Http::ResponseTrailerMap& response_trailers,
                     const StreamInfo::StreamInfo&, absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;
};

/**
 * FormatterProvider based on StreamInfo fields.
 */
class StreamInfoFormatter : public FormatterProvider {
public:
  StreamInfoFormatter(const std::string& field_name);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;

  class FieldExtractor {
  public:
    virtual ~FieldExtractor() = default;

    virtual std::string extract(const StreamInfo::StreamInfo&) const PURE;
    virtual ProtobufWkt::Value extractValue(const StreamInfo::StreamInfo&) const PURE;
  };
  using FieldExtractorPtr = std::unique_ptr<FieldExtractor>;

  enum class StreamInfoAddressFieldExtractionType { WithPort, WithoutPort, JustPort };

private:
  FieldExtractorPtr field_extractor_;
};

/**
 * Base formatter for formatting Metadata objects
 */
class MetadataFormatter {
public:
  MetadataFormatter(const std::string& filter_namespace, const std::vector<std::string>& path,
                    absl::optional<size_t> max_length);

protected:
  std::string formatMetadata(const envoy::config::core::v3::Metadata& metadata) const;
  ProtobufWkt::Value formatMetadataValue(const envoy::config::core::v3::Metadata& metadata) const;

private:
  std::string filter_namespace_;
  std::vector<std::string> path_;
  absl::optional<size_t> max_length_;
};

/**
 * FormatterProvider for DynamicMetadata from StreamInfo.
 */
class DynamicMetadataFormatter : public FormatterProvider, MetadataFormatter {
public:
  DynamicMetadataFormatter(const std::string& filter_namespace,
                           const std::vector<std::string>& path, absl::optional<size_t> max_length);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;
};

/**
 * FormatterProvider for FilterState from StreamInfo.
 */
class FilterStateFormatter : public FormatterProvider {
public:
  FilterStateFormatter(const std::string& key, absl::optional<size_t> max_length,
                       bool serialize_as_string);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;

private:
  const Envoy::StreamInfo::FilterState::Object*
  filterState(const StreamInfo::StreamInfo& stream_info) const;

  std::string key_;
  absl::optional<size_t> max_length_;

  bool serialize_as_string_;
};

/**
 * FormatterProvider for request start time from StreamInfo.
 */
class StartTimeFormatter : public FormatterProvider {
public:
  StartTimeFormatter(const std::string& format);

  // FormatterProvider
  std::string format(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                     const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                     absl::string_view) const override;
  ProtobufWkt::Value formatValue(const Http::RequestHeaderMap&, const Http::ResponseHeaderMap&,
                                 const Http::ResponseTrailerMap&, const StreamInfo::StreamInfo&,
                                 absl::string_view) const override;

private:
  const Envoy::DateFormatter date_formatter_;
};

} // namespace Formatter
} // namespace Envoy
