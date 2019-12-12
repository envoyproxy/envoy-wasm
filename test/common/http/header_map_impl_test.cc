#include <memory>
#include <string>

#include "common/http/header_map_impl.h"
#include "common/http/header_utility.h"

#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

using ::testing::InSequence;

namespace Envoy {
namespace Http {

class VerifiedHeaderMapImpl : public HeaderMapImpl {
public:
  VerifiedHeaderMapImpl() = default;
  explicit VerifiedHeaderMapImpl(
      const std::initializer_list<std::pair<LowerCaseString, std::string>>& values)
      : HeaderMapImpl(values) {}

  void verifyByteSize() override { ASSERT(cached_byte_size_ == byteSizeInternal()); }
};

TEST(HeaderStringTest, All) {
  // Static LowerCaseString constructor
  {
    LowerCaseString static_string("hello");
    HeaderString string(static_string);
    EXPECT_EQ("hello", string.getStringView());
    EXPECT_EQ(static_string.get(), string.getStringView());
    EXPECT_EQ(5U, string.size());
  }

  // Static LowerCaseString operators
  {
    LowerCaseString banana("banana");
    LowerCaseString lemon("lemon");
    EXPECT_TRUE(banana < lemon);
    EXPECT_TRUE(banana != lemon);
    EXPECT_TRUE(banana == banana);
  }

  // Static std::string constructor
  {
    std::string static_string("HELLO");
    HeaderString string(static_string);
    EXPECT_EQ("HELLO", string.getStringView());
    EXPECT_EQ(static_string, string.getStringView());
    EXPECT_EQ(5U, string.size());
  }

  // Static move constructor
  {
    std::string static_string("HELLO");
    HeaderString string1(static_string);
    HeaderString string2(std::move(string1));
    EXPECT_EQ("HELLO", string2.getStringView());
    EXPECT_EQ(static_string, string1.getStringView()); // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(static_string, string2.getStringView());
    EXPECT_EQ(5U, string1.size());
    EXPECT_EQ(5U, string2.size());
  }

  // Inline move constructor
  {
    HeaderString string;
    string.setCopy("hello");
    EXPECT_EQ(HeaderString::Type::Inline, string.type());
    HeaderString string2(std::move(string));
    EXPECT_TRUE(string.empty()); // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(HeaderString::Type::Inline, string.type());
    EXPECT_EQ(HeaderString::Type::Inline, string2.type());
    string.append("world", 5);
    EXPECT_EQ("world", string.getStringView());
    EXPECT_EQ(5UL, string.size());
    EXPECT_EQ("hello", string2.getStringView());
    EXPECT_EQ(5UL, string2.size());
  }

  // Dynamic move constructor
  {
    std::string large(4096, 'a');
    HeaderString string;
    string.setCopy(large);
    EXPECT_EQ(HeaderString::Type::Dynamic, string.type());
    HeaderString string2(std::move(string));
    EXPECT_TRUE(string.empty()); // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(HeaderString::Type::Inline, string.type());
    EXPECT_EQ(HeaderString::Type::Dynamic, string2.type());
    string.append("b", 1);
    EXPECT_EQ("b", string.getStringView());
    EXPECT_EQ(1UL, string.size());
    EXPECT_EQ(large, string2.getStringView());
    EXPECT_EQ(4096UL, string2.size());
  }

  // Static to inline number.
  {
    std::string static_string("HELLO");
    HeaderString string(static_string);
    string.setInteger(5);
    EXPECT_EQ(HeaderString::Type::Inline, string.type());
    EXPECT_EQ("5", string.getStringView());
  }

  // Static to inline string.
  {
    std::string static_string("HELLO");
    HeaderString string(static_string);
    string.setCopy(static_string);
    EXPECT_EQ(HeaderString::Type::Inline, string.type());
    EXPECT_EQ("HELLO", string.getStringView());
  }

  // Static clear() does nothing.
  {
    std::string static_string("HELLO");
    HeaderString string(static_string);
    EXPECT_EQ(HeaderString::Type::Reference, string.type());
    string.clear();
    EXPECT_EQ(HeaderString::Type::Reference, string.type());
    EXPECT_EQ("HELLO", string.getStringView());
  }

  // Static to append.
  {
    std::string static_string("HELLO");
    HeaderString string(static_string);
    EXPECT_EQ(HeaderString::Type::Reference, string.type());
    string.append("a", 1);
    EXPECT_EQ("HELLOa", string.getStringView());
  }

  // Copy inline
  {
    HeaderString string;
    string.setCopy("hello");
    EXPECT_EQ("hello", string.getStringView());
    EXPECT_EQ(5U, string.size());
  }

  // Copy dynamic
  {
    HeaderString string;
    std::string large_value(4096, 'a');
    string.setCopy(large_value);
    EXPECT_EQ(large_value, string.getStringView());
    EXPECT_NE(large_value.c_str(), string.getStringView().data());
    EXPECT_EQ(4096U, string.size());
  }

  // Copy twice dynamic
  {
    HeaderString string;
    std::string large_value1(4096, 'a');
    string.setCopy(large_value1);
    std::string large_value2(2048, 'b');
    string.setCopy(large_value2);
    EXPECT_EQ(large_value2, string.getStringView());
    EXPECT_NE(large_value2.c_str(), string.getStringView().data());
    EXPECT_EQ(2048U, string.size());
  }

  // Copy twice dynamic with reallocate
  {
    HeaderString string;
    std::string large_value1(4096, 'a');
    string.setCopy(large_value1);
    std::string large_value2(16384, 'b');
    string.setCopy(large_value2);
    EXPECT_EQ(large_value2, string.getStringView());
    EXPECT_NE(large_value2.c_str(), string.getStringView().data());
    EXPECT_EQ(16384U, string.size());
  }

  // Copy twice inline to dynamic
  {
    HeaderString string;
    std::string large_value1(16, 'a');
    string.setCopy(large_value1);
    std::string large_value2(16384, 'b');
    string.setCopy(large_value2);
    EXPECT_EQ(large_value2, string.getStringView());
    EXPECT_NE(large_value2.c_str(), string.getStringView().data());
    EXPECT_EQ(16384U, string.size());
  }

  // Copy, exactly filling inline capacity
  //
  // ASAN does not catch the clobber in the case where the code writes one past the
  // end of the inline buffer. To ensure coverage the next block checks that setCopy
  // is not introducing a NUL in a way that does not rely on an actual clobber getting
  // detected.
  {
    HeaderString string;
    std::string large(128, 'z');
    string.setCopy(large);
    EXPECT_EQ(string.type(), HeaderString::Type::Inline);
    EXPECT_EQ(string.getStringView(), large);
  }

  // Ensure setCopy does not add NUL.
  {
    HeaderString string;
    std::string large(128, 'z');
    string.setCopy(large);
    EXPECT_EQ(string.type(), HeaderString::Type::Inline);
    EXPECT_EQ(string.getStringView(), large);
    std::string small(1, 'a');
    string.setCopy(small);
    EXPECT_EQ(string.type(), HeaderString::Type::Inline);
    EXPECT_EQ(string.getStringView(), small);
    // If we peek past the valid first character of the
    // header string_view it should still be 'z' and not '\0'.
    // We know this peek is OK since the memory is much larger
    // than two bytes.
    EXPECT_EQ(string.getStringView().data()[1], 'z');
  }

  // Copy, exactly filling dynamic capacity
  //
  // ASAN should catch a write one past the end of the dynamic buffer. This test
  // forces a dynamic buffer with one copy and then fills it with the next.
  {
    HeaderString string;
    // Force Dynamic with setCopy of inline buffer size + 1.
    std::string large1(129, 'z');
    string.setCopy(large1);
    EXPECT_EQ(string.type(), HeaderString::Type::Dynamic);
    const void* dynamic_buffer_address = string.getStringView().data();
    // Dynamic capacity in setCopy is 2x required by the size.
    // So to fill it exactly setCopy with a total of 258 chars.
    std::string large2(258, 'z');
    string.setCopy(large2);
    EXPECT_EQ(string.type(), HeaderString::Type::Dynamic);
    // The actual buffer address should be the same as it was after
    // setCopy(large1), ensuring no reallocation occurred.
    EXPECT_EQ(string.getStringView().data(), dynamic_buffer_address);
    EXPECT_EQ(string.getStringView(), large2);
  }

  // Append, small buffer to dynamic
  {
    HeaderString string;
    std::string test(128, 'a');
    string.append(test.c_str(), test.size());
    EXPECT_EQ(HeaderString::Type::Inline, string.type());
    string.append("a", 1);
    EXPECT_EQ(HeaderString::Type::Dynamic, string.type());
    test += 'a';
    EXPECT_EQ(test, string.getStringView());
  }

  // Append into inline twice, then shift to dynamic.
  {
    HeaderString string;
    string.append("hello", 5);
    EXPECT_EQ("hello", string.getStringView());
    EXPECT_EQ(5U, string.size());
    string.append("world", 5);
    EXPECT_EQ("helloworld", string.getStringView());
    EXPECT_EQ(10U, string.size());
    std::string large(4096, 'a');
    string.append(large.c_str(), large.size());
    large = "helloworld" + large;
    EXPECT_EQ(large, string.getStringView());
    EXPECT_EQ(4106U, string.size());
  }

  // Append, realloc dynamic.
  {
    HeaderString string;
    std::string large(129, 'a');
    string.append(large.c_str(), large.size());
    EXPECT_EQ(HeaderString::Type::Dynamic, string.type());
    std::string large2 = large + large;
    string.append(large2.c_str(), large2.size());
    large += large2;
    EXPECT_EQ(large, string.getStringView());
    EXPECT_EQ(387U, string.size());
  }

  // Append, realloc close to limit with small buffer.
  {
    HeaderString string;
    std::string large(129, 'a');
    string.append(large.c_str(), large.size());
    EXPECT_EQ(HeaderString::Type::Dynamic, string.type());
    std::string large2(120, 'b');
    string.append(large2.c_str(), large2.size());
    std::string large3(32, 'c');
    string.append(large3.c_str(), large3.size());
    EXPECT_EQ((large + large2 + large3), string.getStringView());
    EXPECT_EQ(281U, string.size());
  }

  // Append, exactly filling dynamic capacity
  //
  // ASAN should catch a write one past the end of the dynamic buffer. This test
  // forces a dynamic buffer with one copy and then fills it with the next.
  {
    HeaderString string;
    // Force Dynamic with setCopy of inline buffer size + 1.
    std::string large1(129, 'z');
    string.setCopy(large1);
    EXPECT_EQ(string.type(), HeaderString::Type::Dynamic);
    const void* dynamic_buffer_address = string.getStringView().data();
    // Dynamic capacity in setCopy is 2x required by the size.
    // So to fill it exactly append 129 chars for a total of 258 chars.
    std::string large2(129, 'z');
    string.append(large2.c_str(), large2.size());
    EXPECT_EQ(string.type(), HeaderString::Type::Dynamic);
    // The actual buffer address should be the same as it was after
    // setCopy(large1), ensuring no reallocation occurred.
    EXPECT_EQ(string.getStringView().data(), dynamic_buffer_address);
    EXPECT_EQ(string.getStringView(), large1 + large2);
  }

  // Set integer, inline
  {
    HeaderString string;
    string.setInteger(123456789);
    EXPECT_EQ("123456789", string.getStringView());
    EXPECT_EQ(9U, string.size());
  }

  // Set integer, dynamic
  {
    HeaderString string;
    std::string large(129, 'a');
    string.append(large.c_str(), large.size());
    string.setInteger(123456789);
    EXPECT_EQ("123456789", string.getStringView());
    EXPECT_EQ(9U, string.size());
    EXPECT_EQ(HeaderString::Type::Dynamic, string.type());
  }

  // Set static, switch to dynamic, back to static.
  {
    const std::string static_string = "hello world";
    HeaderString string;
    string.setReference(static_string);
    EXPECT_EQ(string.getStringView(), static_string);
    EXPECT_EQ(11U, string.size());
    EXPECT_EQ(HeaderString::Type::Reference, string.type());

    const std::string large(129, 'a');
    string.setCopy(large);
    EXPECT_NE(string.getStringView().data(), large.c_str());
    EXPECT_EQ(HeaderString::Type::Dynamic, string.type());

    string.setReference(static_string);
    EXPECT_EQ(string.getStringView(), static_string);
    EXPECT_EQ(11U, string.size());
    EXPECT_EQ(HeaderString::Type::Reference, string.type());
  }

  // getString
  {
    std::string static_string("HELLO");
    HeaderString headerString1(static_string);
    absl::string_view retString1 = headerString1.getStringView();
    EXPECT_EQ("HELLO", retString1);
    EXPECT_EQ(5U, retString1.size());

    HeaderString headerString2;
    absl::string_view retString2 = headerString2.getStringView();
    EXPECT_EQ(0U, retString2.size());
  }
}

TEST(HeaderMapImplTest, InlineInsert) {
  VerifiedHeaderMapImpl headers;
  EXPECT_TRUE(headers.empty());
  EXPECT_EQ(0, headers.size());
  EXPECT_EQ(nullptr, headers.Host());
  headers.setHost("hello");
  EXPECT_FALSE(headers.empty());
  EXPECT_EQ(1, headers.size());
  EXPECT_EQ(":authority", headers.Host()->key().getStringView());
  EXPECT_EQ("hello", headers.Host()->value().getStringView());
  EXPECT_EQ("hello", headers.get(Headers::get().Host)->value().getStringView());
}

TEST(HeaderMapImplTest, InlineAppend) {
  {
    VerifiedHeaderMapImpl headers;
    // Create via header and append.
    headers.setVia("");
    headers.appendVia("1.0 fred", ",");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred");
    headers.appendVia("1.1 nowhere.com", ",");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred,1.1 nowhere.com");
  }
  {
    // Append to via header without explicitly creating first.
    VerifiedHeaderMapImpl headers;
    headers.appendVia("1.0 fred", ",");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred");
    headers.appendVia("1.1 nowhere.com", ",");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred,1.1 nowhere.com");
  }
  {
    // Custom delimiter.
    VerifiedHeaderMapImpl headers;
    headers.setVia("");
    headers.appendVia("1.0 fred", ", ");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred");
    headers.appendVia("1.1 nowhere.com", ", ");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred, 1.1 nowhere.com");
  }
  {
    // Append and then later set.
    VerifiedHeaderMapImpl headers;
    headers.appendVia("1.0 fred", ",");
    headers.appendVia("1.1 nowhere.com", ",");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred,1.1 nowhere.com");
    headers.setVia("2.0 override");
    EXPECT_EQ(headers.Via()->value().getStringView(), "2.0 override");
  }
  {
    // Set and then append. This mimics how GrpcTimeout is set.
    VerifiedHeaderMapImpl headers;
    headers.setGrpcTimeout(42);
    EXPECT_EQ(headers.GrpcTimeout()->value().getStringView(), "42");
    headers.appendGrpcTimeout("s", "");
    EXPECT_EQ(headers.GrpcTimeout()->value().getStringView(), "42s");
  }
}

TEST(HeaderMapImplTest, MoveIntoInline) {
  VerifiedHeaderMapImpl headers;
  HeaderString key;
  key.setCopy(Headers::get().CacheControl.get());
  HeaderString value;
  value.setCopy("hello");
  headers.addViaMove(std::move(key), std::move(value));
  EXPECT_EQ("cache-control", headers.CacheControl()->key().getStringView());
  EXPECT_EQ("hello", headers.CacheControl()->value().getStringView());

  HeaderString key2;
  key2.setCopy(Headers::get().CacheControl.get());
  HeaderString value2;
  value2.setCopy("there");
  headers.addViaMove(std::move(key2), std::move(value2));
  EXPECT_EQ("cache-control", headers.CacheControl()->key().getStringView());
  EXPECT_EQ("hello,there", headers.CacheControl()->value().getStringView());
}

TEST(HeaderMapImplTest, Remove) {
  VerifiedHeaderMapImpl headers;

  // Add random header and then remove by name.
  LowerCaseString static_key("hello");
  std::string ref_value("value");
  headers.addReference(static_key, ref_value);
  EXPECT_EQ("value", headers.get(static_key)->value().getStringView());
  EXPECT_EQ(HeaderString::Type::Reference, headers.get(static_key)->value().type());
  EXPECT_EQ(1UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.remove(static_key);
  EXPECT_EQ(nullptr, headers.get(static_key));
  EXPECT_EQ(0UL, headers.size());
  EXPECT_TRUE(headers.empty());

  // Add and remove by inline.
  headers.setContentLength(5);
  EXPECT_EQ("5", headers.ContentLength()->value().getStringView());
  EXPECT_EQ(1UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.removeContentLength();
  EXPECT_EQ(nullptr, headers.ContentLength());
  EXPECT_EQ(0UL, headers.size());
  EXPECT_TRUE(headers.empty());

  // Add inline and remove by name.
  headers.setContentLength(5);
  EXPECT_EQ("5", headers.ContentLength()->value().getStringView());
  EXPECT_EQ(1UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.remove(Headers::get().ContentLength);
  EXPECT_EQ(nullptr, headers.ContentLength());
  EXPECT_EQ(0UL, headers.size());
  EXPECT_TRUE(headers.empty());
}

TEST(HeaderMapImplTest, RemoveRegex) {
  // These will match.
  LowerCaseString key1 = LowerCaseString("X-prefix-foo");
  LowerCaseString key3 = LowerCaseString("X-Prefix-");
  LowerCaseString key5 = LowerCaseString("x-prefix-eep");
  // These will not.
  LowerCaseString key2 = LowerCaseString(" x-prefix-foo");
  LowerCaseString key4 = LowerCaseString("y-x-prefix-foo");

  VerifiedHeaderMapImpl headers;
  headers.addReference(key1, "value");
  headers.addReference(key2, "value");
  headers.addReference(key3, "value");
  headers.addReference(key4, "value");
  headers.addReference(key5, "value");

  // Test removing the first header, middle headers, and the end header.
  headers.removePrefix(LowerCaseString("x-prefix-"));
  EXPECT_EQ(nullptr, headers.get(key1));
  EXPECT_NE(nullptr, headers.get(key2));
  EXPECT_EQ(nullptr, headers.get(key3));
  EXPECT_NE(nullptr, headers.get(key4));
  EXPECT_EQ(nullptr, headers.get(key5));

  // Remove all headers.
  headers.removePrefix(LowerCaseString(""));
  EXPECT_EQ(nullptr, headers.get(key2));
  EXPECT_EQ(nullptr, headers.get(key4));

  // Add inline and remove by regex
  headers.setContentLength(5);
  EXPECT_EQ("5", headers.ContentLength()->value().getStringView());
  EXPECT_EQ(1UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.removePrefix(LowerCaseString("content"));
  EXPECT_EQ(nullptr, headers.ContentLength());
}

TEST(HeaderMapImplTest, SetRemovesAllValues) {
  VerifiedHeaderMapImpl headers;

  LowerCaseString key1("hello");
  LowerCaseString key2("olleh");
  std::string ref_value1("world");
  std::string ref_value2("planet");
  std::string ref_value3("globe");
  std::string ref_value4("earth");
  std::string ref_value5("blue marble");

  headers.addReference(key1, ref_value1);
  headers.addReference(key2, ref_value2);
  headers.addReference(key1, ref_value3);
  headers.addReference(key1, ref_value4);

  using MockCb = testing::MockFunction<void(const std::string&, const std::string&)>;

  {
    MockCb cb;

    InSequence seq;
    EXPECT_CALL(cb, Call("hello", "world"));
    EXPECT_CALL(cb, Call("olleh", "planet"));
    EXPECT_CALL(cb, Call("hello", "globe"));
    EXPECT_CALL(cb, Call("hello", "earth"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }

  headers.setReference(key1, ref_value5); // set moves key to end

  {
    MockCb cb;

    InSequence seq;
    EXPECT_CALL(cb, Call("olleh", "planet"));
    EXPECT_CALL(cb, Call("hello", "blue marble"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }
}

TEST(HeaderMapImplTest, DoubleInlineAdd) {
  {
    VerifiedHeaderMapImpl headers;
    const std::string foo("foo");
    const std::string bar("bar");
    headers.addReference(Headers::get().ContentLength, foo);
    headers.addReference(Headers::get().ContentLength, bar);
    EXPECT_EQ("foo,bar", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
  {
    VerifiedHeaderMapImpl headers;
    headers.addReferenceKey(Headers::get().ContentLength, "foo");
    headers.addReferenceKey(Headers::get().ContentLength, "bar");
    EXPECT_EQ("foo,bar", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
  {
    VerifiedHeaderMapImpl headers;
    headers.addReferenceKey(Headers::get().ContentLength, 5);
    headers.addReferenceKey(Headers::get().ContentLength, 6);
    EXPECT_EQ("5,6", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
  {
    VerifiedHeaderMapImpl headers;
    const std::string foo("foo");
    headers.addReference(Headers::get().ContentLength, foo);
    headers.addReferenceKey(Headers::get().ContentLength, 6);
    EXPECT_EQ("foo,6", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
}

// Per https://github.com/envoyproxy/envoy/issues/7488 make sure we don't
// combine set-cookie headers
TEST(HeaderMapImplTest, DoubleCookieAdd) {
  VerifiedHeaderMapImpl headers;
  const std::string foo("foo");
  const std::string bar("bar");
  const LowerCaseString& set_cookie = Http::Headers::get().SetCookie;
  headers.addReference(set_cookie, foo);
  headers.addReference(set_cookie, bar);
  EXPECT_EQ(2UL, headers.size());

  std::vector<absl::string_view> out;
  Http::HeaderUtility::getAllOfHeader(headers, "set-cookie", out);
  ASSERT_EQ(out.size(), 2);
  ASSERT_EQ(out[0], "foo");
  ASSERT_EQ(out[1], "bar");
}

TEST(HeaderMapImplTest, DoubleInlineSet) {
  VerifiedHeaderMapImpl headers;
  headers.setReferenceKey(Headers::get().ContentType, "blah");
  headers.setReferenceKey(Headers::get().ContentType, "text/html");
  EXPECT_EQ("text/html", headers.ContentType()->value().getStringView());
  EXPECT_EQ(1UL, headers.size());
}

TEST(HeaderMapImplTest, AddReferenceKey) {
  VerifiedHeaderMapImpl headers;
  LowerCaseString foo("hello");
  headers.addReferenceKey(foo, "world");
  EXPECT_NE("world", headers.get(foo)->value().getStringView().data());
  EXPECT_EQ("world", headers.get(foo)->value().getStringView());
}

TEST(HeaderMapImplTest, SetReferenceKey) {
  VerifiedHeaderMapImpl headers;
  LowerCaseString foo("hello");
  headers.setReferenceKey(foo, "world");
  EXPECT_NE("world", headers.get(foo)->value().getStringView().data());
  EXPECT_EQ("world", headers.get(foo)->value().getStringView());

  headers.setReferenceKey(foo, "monde");
  EXPECT_NE("monde", headers.get(foo)->value().getStringView().data());
  EXPECT_EQ("monde", headers.get(foo)->value().getStringView());
}

TEST(HeaderMapImplTest, SetCopy) {
  VerifiedHeaderMapImpl headers;
  LowerCaseString foo("hello");
  headers.setCopy(foo, "world");
  EXPECT_EQ("world", headers.get(foo)->value().getStringView());

  // Overwrite value.
  headers.setCopy(foo, "monde");
  EXPECT_EQ("monde", headers.get(foo)->value().getStringView());

  // Add another foo header.
  headers.addCopy(foo, "monde2");
  EXPECT_EQ(headers.size(), 2);

  // Only the first foo header is overridden.
  headers.setCopy(foo, "override-monde");
  EXPECT_EQ(headers.size(), 2);

  using MockCb = testing::MockFunction<void(const std::string&, const std::string&)>;
  MockCb cb;

  InSequence seq;
  EXPECT_CALL(cb, Call("hello", "override-monde"));
  EXPECT_CALL(cb, Call("hello", "monde2"));
  headers.iterate(
      [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
        static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                         std::string(header.value().getStringView()));
        return HeaderMap::Iterate::Continue;
      },
      &cb);

  // Test setting an empty string and then overriding.
  headers.remove(foo);
  EXPECT_EQ(headers.size(), 0);
  const std::string empty;
  headers.setCopy(foo, empty);
  EXPECT_EQ(headers.size(), 1);
  headers.setCopy(foo, "not-empty");
  EXPECT_EQ(headers.get(foo)->value().getStringView(), "not-empty");

  // Use setCopy with inline headers both indirectly and directly.
  headers.clear();
  EXPECT_EQ(headers.size(), 0);
  headers.setCopy(Headers::get().Path, "/");
  EXPECT_EQ(headers.size(), 1);
  EXPECT_EQ(headers.Path()->value().getStringView(), "/");
  headers.setPath("/foo");
  EXPECT_EQ(headers.size(), 1);
  EXPECT_EQ(headers.Path()->value().getStringView(), "/foo");
}

TEST(HeaderMapImplTest, AddCopy) {
  VerifiedHeaderMapImpl headers;

  // Start with a string value.
  std::unique_ptr<LowerCaseString> lcKeyPtr(new LowerCaseString("hello"));
  headers.addCopy(*lcKeyPtr, "world");

  const HeaderString& value = headers.get(*lcKeyPtr)->value();

  EXPECT_EQ("world", value.getStringView());
  EXPECT_EQ(5UL, value.size());

  lcKeyPtr.reset();

  const HeaderString& value2 = headers.get(LowerCaseString("hello"))->value();

  EXPECT_EQ("world", value2.getStringView());
  EXPECT_EQ(5UL, value2.size());
  EXPECT_EQ(value.getStringView(), value2.getStringView());
  EXPECT_EQ(1UL, headers.size());

  // Repeat with an int value.
  //
  // addReferenceKey and addCopy can both add multiple instances of a
  // given header, so we need to delete the old "hello" header.
  // Test that removing will return 0 byte size.
  headers.remove(LowerCaseString("hello"));
  EXPECT_EQ(headers.byteSize(), 0);

  // Build "hello" with string concatenation to make it unlikely that the
  // compiler is just reusing the same string constant for everything.
  lcKeyPtr = std::make_unique<LowerCaseString>(std::string("he") + "llo");
  EXPECT_STREQ("hello", lcKeyPtr->get().c_str());

  headers.addCopy(*lcKeyPtr, 42);

  const HeaderString& value3 = headers.get(*lcKeyPtr)->value();

  EXPECT_EQ("42", value3.getStringView());
  EXPECT_EQ(2UL, value3.size());

  lcKeyPtr.reset();

  const HeaderString& value4 = headers.get(LowerCaseString("hello"))->value();

  EXPECT_EQ("42", value4.getStringView());
  EXPECT_EQ(2UL, value4.size());
  EXPECT_EQ(1UL, headers.size());

  // Here, again, we'll build yet another key string.
  LowerCaseString lcKey3(std::string("he") + "ll" + "o");
  EXPECT_STREQ("hello", lcKey3.get().c_str());

  EXPECT_EQ("42", headers.get(lcKey3)->value().getStringView());
  EXPECT_EQ(2UL, headers.get(lcKey3)->value().size());

  LowerCaseString cache_control("cache-control");
  headers.addCopy(cache_control, "max-age=1345");
  EXPECT_EQ("max-age=1345", headers.get(cache_control)->value().getStringView());
  EXPECT_EQ("max-age=1345", headers.CacheControl()->value().getStringView());
  headers.addCopy(cache_control, "public");
  EXPECT_EQ("max-age=1345,public", headers.get(cache_control)->value().getStringView());
  headers.addCopy(cache_control, "");
  EXPECT_EQ("max-age=1345,public", headers.get(cache_control)->value().getStringView());
  headers.addCopy(cache_control, 123);
  EXPECT_EQ("max-age=1345,public,123", headers.get(cache_control)->value().getStringView());
  headers.addCopy(cache_control, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ("max-age=1345,public,123,18446744073709551615",
            headers.get(cache_control)->value().getStringView());
}

TEST(HeaderMapImplTest, Equality) {
  VerifiedHeaderMapImpl headers1;
  VerifiedHeaderMapImpl headers2;
  EXPECT_EQ(headers1, headers2);

  headers1.addCopy(LowerCaseString("hello"), "world");
  EXPECT_FALSE(headers1 == headers2);

  headers2.addCopy(LowerCaseString("foo"), "bar");
  EXPECT_FALSE(headers1 == headers2);
}

TEST(HeaderMapImplTest, LargeCharInHeader) {
  VerifiedHeaderMapImpl headers;
  LowerCaseString static_key("\x90hello");
  std::string ref_value("value");
  headers.addReference(static_key, ref_value);
  EXPECT_EQ("value", headers.get(static_key)->value().getStringView());
}

TEST(HeaderMapImplTest, Iterate) {
  VerifiedHeaderMapImpl headers;
  headers.addCopy(LowerCaseString("hello"), "world");
  headers.addCopy(LowerCaseString("foo"), "xxx");
  headers.addCopy(LowerCaseString("world"), "hello");
  LowerCaseString foo_key("foo");
  headers.setReferenceKey(foo_key, "bar"); // set moves key to end

  using MockCb = testing::MockFunction<void(const std::string&, const std::string&)>;
  MockCb cb;

  InSequence seq;
  EXPECT_CALL(cb, Call("hello", "world"));
  EXPECT_CALL(cb, Call("world", "hello"));
  EXPECT_CALL(cb, Call("foo", "bar"));
  headers.iterate(
      [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
        static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                         std::string(header.value().getStringView()));
        return HeaderMap::Iterate::Continue;
      },
      &cb);
}

TEST(HeaderMapImplTest, IterateReverse) {
  VerifiedHeaderMapImpl headers;
  headers.addCopy(LowerCaseString("hello"), "world");
  headers.addCopy(LowerCaseString("foo"), "bar");
  LowerCaseString world_key("world");
  headers.setReferenceKey(world_key, "hello");

  using MockCb = testing::MockFunction<void(const std::string&, const std::string&)>;
  MockCb cb;

  InSequence seq;
  EXPECT_CALL(cb, Call("world", "hello"));
  EXPECT_CALL(cb, Call("foo", "bar"));
  // no "hello"
  headers.iterateReverse(
      [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
        static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                         std::string(header.value().getStringView()));
        if (header.key().getStringView() != "foo") {
          return HeaderMap::Iterate::Continue;
        } else {
          return HeaderMap::Iterate::Break;
        }
      },
      &cb);
}

TEST(HeaderMapImplTest, Lookup) {
  VerifiedHeaderMapImpl headers;
  headers.addCopy(LowerCaseString("hello"), "world");
  headers.setContentLength(5);

  // Lookup is not supported for non predefined inline headers.
  {
    const HeaderEntry* entry;
    EXPECT_EQ(HeaderMap::Lookup::NotSupported, headers.lookup(LowerCaseString{"hello"}, &entry));
    EXPECT_EQ(nullptr, entry);
  }

  // Lookup returns the entry of a predefined inline header if it exists.
  {
    const HeaderEntry* entry;
    EXPECT_EQ(HeaderMap::Lookup::Found, headers.lookup(Headers::get().ContentLength, &entry));
    EXPECT_EQ("5", entry->value().getStringView());
  }

  // Lookup returns HeaderMap::Lookup::NotFound if a predefined inline header does not exist.
  {
    const HeaderEntry* entry;
    EXPECT_EQ(HeaderMap::Lookup::NotFound, headers.lookup(Headers::get().Host, &entry));
    EXPECT_EQ(nullptr, entry);
  }
}

TEST(HeaderMapImplTest, Get) {
  {
    const VerifiedHeaderMapImpl headers{{Headers::get().Path, "/"},
                                        {LowerCaseString("hello"), "world"}};
    EXPECT_EQ("/", headers.get(LowerCaseString(":path"))->value().getStringView());
    EXPECT_EQ("world", headers.get(LowerCaseString("hello"))->value().getStringView());
    EXPECT_EQ(nullptr, headers.get(LowerCaseString("foo")));
  }

  {
    VerifiedHeaderMapImpl headers{{Headers::get().Path, "/"}, {LowerCaseString("hello"), "world"}};
    // There is not HeaderMap method to set a header and copy both the key and value.
    headers.setReferenceKey(LowerCaseString(":path"), "/new_path");
    EXPECT_EQ("/new_path", headers.get(LowerCaseString(":path"))->value().getStringView());
    LowerCaseString foo("hello");
    headers.setReferenceKey(foo, "world2");
    EXPECT_EQ("world2", headers.get(foo)->value().getStringView());
    EXPECT_EQ(nullptr, headers.get(LowerCaseString("foo")));
  }
}

TEST(HeaderMapImplTest, TestAppendHeader) {
  // Test appending to a string with a value.
  {
    VerifiedHeaderMapImpl headers;
    LowerCaseString foo("key1");
    headers.addCopy(foo, "some;");
    headers.appendCopy(foo, "test");
    EXPECT_EQ(headers.get(foo)->value().getStringView(), "some;,test");
  }

  // Test appending to an empty string.
  {
    VerifiedHeaderMapImpl headers;
    LowerCaseString key2("key2");
    headers.appendCopy(key2, "my tag data");
    EXPECT_EQ(headers.get(key2)->value().getStringView(), "my tag data");
  }

  // Test empty data case.
  {
    VerifiedHeaderMapImpl headers;
    LowerCaseString key3("key3");
    headers.addCopy(key3, "empty");
    headers.appendCopy(key3, "");
    EXPECT_EQ(headers.get(key3)->value().getStringView(), "empty");
  }
  // Regression test for appending to an empty string with a short string, then
  // setting integer.
  {
    VerifiedHeaderMapImpl headers;
    const std::string empty;
    headers.setPath(empty);
    // Append with default delimiter.
    headers.appendPath(" ", ",");
    headers.setPath(0);
    EXPECT_EQ("0", headers.Path()->value().getStringView());
    EXPECT_EQ(1U, headers.Path()->value().size());
  }
  // Test append for inline headers using this method and append##name.
  {
    VerifiedHeaderMapImpl headers;
    headers.addCopy(Headers::get().Via, "1.0 fred");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred");
    headers.appendCopy(Headers::get().Via, "1.1 p.example.net");
    EXPECT_EQ(headers.Via()->value().getStringView(), "1.0 fred,1.1 p.example.net");
    headers.appendVia("1.1 new.example.net", ",");
    EXPECT_EQ(headers.Via()->value().getStringView(),
              "1.0 fred,1.1 p.example.net,1.1 new.example.net");
  }
}

TEST(HeaderMapImplDeathTest, TestHeaderLengthChecks) {
  HeaderString value;
  value.setCopy("some;");
  EXPECT_DEATH_LOG_TO_STDERR(value.append(nullptr, std::numeric_limits<uint32_t>::max()),
                             "Trying to allocate overly large headers.");

  std::string source("hello");
  HeaderString reference;
  reference.setReference(source);
  EXPECT_DEATH_LOG_TO_STDERR(reference.append(nullptr, std::numeric_limits<uint32_t>::max()),
                             "Trying to allocate overly large headers.");
}

TEST(HeaderMapImplTest, PseudoHeaderOrder) {
  using MockCb = testing::MockFunction<void(const std::string&, const std::string&)>;
  MockCb cb;

  {
    LowerCaseString foo("hello");
    Http::VerifiedHeaderMapImpl headers{};
    EXPECT_EQ(0UL, headers.size());
    EXPECT_TRUE(headers.empty());

    headers.addReferenceKey(foo, "world");
    EXPECT_EQ(1UL, headers.size());
    EXPECT_FALSE(headers.empty());

    headers.setReferenceKey(Headers::get().ContentType, "text/html");
    EXPECT_EQ(2UL, headers.size());
    EXPECT_FALSE(headers.empty());

    // Pseudo header gets inserted before non-pseudo headers
    headers.setReferenceKey(Headers::get().Method, "PUT");
    EXPECT_EQ(3UL, headers.size());
    EXPECT_FALSE(headers.empty());

    InSequence seq;
    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call("hello", "world"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removal of the header before which pseudo-headers are inserted
    headers.remove(foo);
    EXPECT_EQ(2UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Next pseudo-header goes after other pseudo-headers, but before normal headers
    headers.setReferenceKey(Headers::get().Path, "/test");
    EXPECT_EQ(3UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removing the last normal header
    headers.remove(Headers::get().ContentType);
    EXPECT_EQ(2UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Adding a new pseudo-header after removing the last normal header
    headers.setReferenceKey(Headers::get().Host, "host");
    EXPECT_EQ(3UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));
    EXPECT_CALL(cb, Call(":authority", "host"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Adding the first normal header
    headers.setReferenceKey(Headers::get().ContentType, "text/html");
    EXPECT_EQ(4UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));
    EXPECT_CALL(cb, Call(":authority", "host"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removing all pseudo-headers
    headers.remove(Headers::get().Path);
    headers.remove(Headers::get().Method);
    headers.remove(Headers::get().Host);
    EXPECT_EQ(1UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removing all headers
    headers.remove(Headers::get().ContentType);
    EXPECT_EQ(0UL, headers.size());
    EXPECT_TRUE(headers.empty());

    // Adding a lone pseudo-header
    headers.setReferenceKey(Headers::get().Status, "200");
    EXPECT_EQ(1UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":status", "200"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }

  // Starting with a normal header
  {
    Http::VerifiedHeaderMapImpl headers{{Headers::get().ContentType, "text/plain"},
                                        {Headers::get().Method, "GET"},
                                        {Headers::get().Path, "/"},
                                        {LowerCaseString("hello"), "world"},
                                        {Headers::get().Host, "host"}};

    InSequence seq;
    EXPECT_CALL(cb, Call(":method", "GET"));
    EXPECT_CALL(cb, Call(":path", "/"));
    EXPECT_CALL(cb, Call(":authority", "host"));
    EXPECT_CALL(cb, Call("content-type", "text/plain"));
    EXPECT_CALL(cb, Call("hello", "world"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }

  // Starting with a pseudo-header
  {
    Http::VerifiedHeaderMapImpl headers{{Headers::get().Path, "/"},
                                        {Headers::get().ContentType, "text/plain"},
                                        {Headers::get().Method, "GET"},
                                        {LowerCaseString("hello"), "world"},
                                        {Headers::get().Host, "host"}};

    InSequence seq;
    EXPECT_CALL(cb, Call(":path", "/"));
    EXPECT_CALL(cb, Call(":method", "GET"));
    EXPECT_CALL(cb, Call(":authority", "host"));
    EXPECT_CALL(cb, Call("content-type", "text/plain"));
    EXPECT_CALL(cb, Call("hello", "world"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }
}

// Validate that TestHeaderMapImpl copy construction and assignment works. This is a
// regression for where we were missing a valid copy constructor and had the
// default (dangerous) move semantics takeover.
TEST(HeaderMapImplTest, TestHeaderMapImplyCopy) {
  TestHeaderMapImpl foo;
  foo.addCopy(LowerCaseString("foo"), "bar");
  auto headers = std::make_unique<TestHeaderMapImpl>(foo);
  EXPECT_EQ("bar", headers->get(LowerCaseString("foo"))->value().getStringView());
  TestHeaderMapImpl baz{{"foo", "baz"}};
  baz = *headers;
  EXPECT_EQ("bar", baz.get(LowerCaseString("foo"))->value().getStringView());
  const TestHeaderMapImpl& baz2 = baz;
  baz = baz2;
  EXPECT_EQ("bar", baz.get(LowerCaseString("foo"))->value().getStringView());
}

TEST(HeaderMapImplTest, TestInlineHeaderAdd) {
  VerifiedHeaderMapImpl foo;
  foo.addCopy(LowerCaseString(":path"), "GET");
  EXPECT_EQ(foo.size(), 1);
  EXPECT_TRUE(foo.Path() != nullptr);
}

TEST(HeaderMapImplTest, ClearHeaderMap) {
  VerifiedHeaderMapImpl headers;
  LowerCaseString static_key("hello");
  std::string ref_value("value");

  // Add random header and then clear.
  headers.addReference(static_key, ref_value);
  EXPECT_EQ("value", headers.get(static_key)->value().getStringView());
  EXPECT_EQ(HeaderString::Type::Reference, headers.get(static_key)->value().type());
  EXPECT_EQ(1UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.clear();
  EXPECT_EQ(nullptr, headers.get(static_key));
  EXPECT_EQ(0UL, headers.size());
  EXPECT_EQ(headers.byteSize(), 0);
  EXPECT_TRUE(headers.empty());

  // Add inline and clear.
  headers.setContentLength(5);
  EXPECT_EQ("5", headers.ContentLength()->value().getStringView());
  EXPECT_EQ(1UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.clear();
  EXPECT_EQ(nullptr, headers.ContentLength());
  EXPECT_EQ(0UL, headers.size());
  EXPECT_EQ(headers.byteSize(), 0);
  EXPECT_TRUE(headers.empty());

  // Add mixture of headers.
  headers.addReference(static_key, ref_value);
  headers.setContentLength(5);
  headers.addCopy(static_key, "new_value");
  EXPECT_EQ(3UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.clear();
  EXPECT_EQ(nullptr, headers.ContentLength());
  EXPECT_EQ(0UL, headers.size());
  EXPECT_EQ(headers.byteSize(), 0);
  EXPECT_TRUE(headers.empty());
}

// Validates byte size is properly accounted for in different inline header setting scenarios.
TEST(HeaderMapImplTest, InlineHeaderByteSize) {
  {
    VerifiedHeaderMapImpl headers;
    std::string foo = "foo";
    headers.setHost(foo);
    EXPECT_EQ(headers.byteSize(), 13);
  }
  {
    // Overwrite an inline headers with set.
    VerifiedHeaderMapImpl headers;
    std::string foo = "foo";
    headers.setHost(foo);
    std::string big_foo = "big_foo";
    headers.setHost(big_foo);
    EXPECT_EQ(headers.byteSize(), 17);
  }
  {
    // Overwrite an inline headers with setReference and clear.
    VerifiedHeaderMapImpl headers;
    std::string foo = "foo";
    headers.setHost(foo);
    std::string big_foo = "big_foo";
    headers.setReferenceHost(big_foo);
    EXPECT_EQ(headers.byteSize(), 17);
    headers.removeHost();
    EXPECT_EQ(headers.byteSize(), 0);
  }
  {
    // Overwrite an inline headers with set integer value.
    VerifiedHeaderMapImpl headers;
    uint64_t status = 200;
    headers.setStatus(status);
    EXPECT_EQ(headers.byteSize(), 10);
    uint64_t newStatus = 500;
    headers.setStatus(newStatus);
    EXPECT_EQ(headers.byteSize(), 10);
    headers.removeStatus();
    EXPECT_EQ(headers.byteSize(), 0);
  }
  {
    // Set an inline header, remove, and rewrite.
    VerifiedHeaderMapImpl headers;
    uint64_t status = 200;
    headers.setStatus(status);
    EXPECT_EQ(headers.byteSize(), 10);
    headers.removeStatus();
    EXPECT_EQ(headers.byteSize(), 0);
    uint64_t newStatus = 500;
    headers.setStatus(newStatus);
    EXPECT_EQ(headers.byteSize(), 10);
  }
}

} // namespace Http
} // namespace Envoy
