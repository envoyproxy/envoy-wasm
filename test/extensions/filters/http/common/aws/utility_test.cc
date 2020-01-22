#include "extensions/filters/http/common/aws/utility.h"

#include "test/test_common/utility.h"

#include "gtest/gtest.h"

using testing::ElementsAre;
using testing::Pair;

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Common {
namespace Aws {
namespace {

// Headers must be in alphabetical order by virtue of std::map
TEST(UtilityTest, CanonicalizeHeadersInAlphabeticalOrder) {
  Http::TestHeaderMapImpl headers{
      {"d", "d_value"}, {"f", "f_value"}, {"b", "b_value"},
      {"e", "e_value"}, {"c", "c_value"}, {"a", "a_value"},
  };
  const auto map = Utility::canonicalizeHeaders(headers);
  EXPECT_THAT(map, ElementsAre(Pair("a", "a_value"), Pair("b", "b_value"), Pair("c", "c_value"),
                               Pair("d", "d_value"), Pair("e", "e_value"), Pair("f", "f_value")));
}

// HTTP pseudo-headers should be ignored
TEST(UtilityTest, CanonicalizeHeadersSkippingPseudoHeaders) {
  Http::TestHeaderMapImpl headers{
      {":path", "path_value"},
      {":method", "GET"},
      {"normal", "normal_value"},
  };
  const auto map = Utility::canonicalizeHeaders(headers);
  EXPECT_THAT(map, ElementsAre(Pair("normal", "normal_value")));
}

// Repeated headers are joined with commas
TEST(UtilityTest, CanonicalizeHeadersJoiningDuplicatesWithCommas) {
  Http::TestHeaderMapImpl headers{
      {"a", "a_value1"},
      {"a", "a_value2"},
      {"a", "a_value3"},
  };
  const auto map = Utility::canonicalizeHeaders(headers);
  EXPECT_THAT(map, ElementsAre(Pair("a", "a_value1,a_value2,a_value3")));
}

// We canonicalize the :authority header as host
TEST(UtilityTest, CanonicalizeHeadersAuthorityToHost) {
  Http::TestHeaderMapImpl headers{
      {":authority", "authority_value"},
  };
  const auto map = Utility::canonicalizeHeaders(headers);
  EXPECT_THAT(map, ElementsAre(Pair("host", "authority_value")));
}

// Ports 80 and 443 are omitted from the host headers
TEST(UtilityTest, CanonicalizeHeadersRemovingDefaultPortsFromHost) {
  Http::TestHeaderMapImpl headers_port80{
      {":authority", "example.com:80"},
  };
  const auto map_port80 = Utility::canonicalizeHeaders(headers_port80);
  EXPECT_THAT(map_port80, ElementsAre(Pair("host", "example.com")));

  Http::TestHeaderMapImpl headers_port443{
      {":authority", "example.com:443"},
  };
  const auto map_port443 = Utility::canonicalizeHeaders(headers_port443);
  EXPECT_THAT(map_port443, ElementsAre(Pair("host", "example.com")));
}

// Whitespace is trimmed from headers
TEST(UtilityTest, CanonicalizeHeadersTrimmingWhitespace) {
  Http::TestHeaderMapImpl headers{
      {"leading", "    leading value"},
      {"trailing", "trailing value    "},
      {"internal", "internal    value"},
      {"all", "    all    value    "},
  };
  const auto map = Utility::canonicalizeHeaders(headers);
  EXPECT_THAT(map,
              ElementsAre(Pair("all", "all value"), Pair("internal", "internal value"),
                          Pair("leading", "leading value"), Pair("trailing", "trailing value")));
}

// Verify the format of a minimalist canonical request
TEST(UtilityTest, MinimalCanonicalRequest) {
  std::map<std::string, std::string> headers;
  const auto request = Utility::createCanonicalRequest("GET", "", headers, "content-hash");
  EXPECT_EQ(R"(GET
/



content-hash)",
            request);
}

TEST(UtilityTest, CanonicalRequestWithQueryString) {
  const std::map<std::string, std::string> headers;
  const auto request = Utility::createCanonicalRequest("GET", "?query", headers, "content-hash");
  EXPECT_EQ(R"(GET
/
query


content-hash)",
            request);
}

TEST(UtilityTest, CanonicalRequestWithHeaders) {
  const std::map<std::string, std::string> headers = {
      {"header1", "value1"},
      {"header2", "value2"},
      {"header3", "value3"},
  };
  const auto request = Utility::createCanonicalRequest("GET", "", headers, "content-hash");
  EXPECT_EQ(R"(GET
/

header1:value1
header2:value2
header3:value3

header1;header2;header3
content-hash)",
            request);
}

// Verify headers are joined with ";"
TEST(UtilityTest, JoinCanonicalHeaderNames) {
  std::map<std::string, std::string> headers = {
      {"header1", "value1"},
      {"header2", "value2"},
      {"header3", "value3"},
  };
  const auto names = Utility::joinCanonicalHeaderNames(headers);
  EXPECT_EQ("header1;header2;header3", names);
}

// Verify we return "" when there are no headers
TEST(UtilityTest, JoinCanonicalHeaderNamesWithEmptyMap) {
  std::map<std::string, std::string> headers;
  const auto names = Utility::joinCanonicalHeaderNames(headers);
  EXPECT_EQ("", names);
}

} // namespace
} // namespace Aws
} // namespace Common
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
