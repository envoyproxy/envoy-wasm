#pragma once

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <string>

#include "envoy/http/header_map.h"

#include "common/common/non_copyable.h"
#include "common/http/headers.h"

namespace Envoy {
namespace Http {

/**
 * These are definitions of all of the inline header access functions described inside header_map.h
 * TODO(asraa): Simplify code here so macros expand into single virtual calls.
 */
#define DEFINE_INLINE_HEADER_FUNCS(name)                                                           \
public:                                                                                            \
  const HeaderEntry* name() const override { return inline_headers_.name##_; }                     \
  void append##name(absl::string_view data, absl::string_view delimiter) override {                \
    HeaderEntry& entry = maybeCreateInline(&inline_headers_.name##_, Headers::get().name);         \
    addSize(HeaderMapImpl::appendToHeader(entry.value(), data, delimiter));                        \
    verifyByteSize();                                                                              \
  }                                                                                                \
  void setReference##name(absl::string_view value) override {                                      \
    HeaderEntry& entry = maybeCreateInline(&inline_headers_.name##_, Headers::get().name);         \
    updateSize(entry.value().size(), value.size());                                                \
    entry.value().setReference(value);                                                             \
    verifyByteSize();                                                                              \
  }                                                                                                \
  void set##name(absl::string_view value) override {                                               \
    HeaderEntry& entry = maybeCreateInline(&inline_headers_.name##_, Headers::get().name);         \
    updateSize(entry.value().size(), value.size());                                                \
    entry.value().setCopy(value);                                                                  \
    verifyByteSize();                                                                              \
  }                                                                                                \
  void set##name(uint64_t value) override {                                                        \
    HeaderEntry& entry = maybeCreateInline(&inline_headers_.name##_, Headers::get().name);         \
    subtractSize(inline_headers_.name##_->value().size());                                         \
    entry.value().setInteger(value);                                                               \
    addSize(inline_headers_.name##_->value().size());                                              \
    verifyByteSize();                                                                              \
  }                                                                                                \
  void remove##name() override { removeInline(&inline_headers_.name##_); }

#define DEFINE_INLINE_HEADER_STRUCT(name) HeaderEntryImpl* name##_;

/**
 * Implementation of Http::HeaderMap. This is heavily optimized for performance. Roughly, when
 * headers are added to the map, we do a hash lookup to see if it's one of the O(1) headers.
 * If it is, we store a reference to it that can be accessed later directly. Most high performance
 * paths use O(1) direct access. In general, we try to copy as little as possible and allocate as
 * little as possible in any of the paths.
 */
class HeaderMapImpl : public HeaderMap, NonCopyable {
public:
  HeaderMapImpl();
  explicit HeaderMapImpl(
      const std::initializer_list<std::pair<LowerCaseString, std::string>>& values);
  explicit HeaderMapImpl(const HeaderMap& rhs) : HeaderMapImpl() { copyFrom(rhs); }

  /**
   * Add a header via full move. This is the expected high performance paths for codecs populating
   * a map when receiving.
   */
  void addViaMove(HeaderString&& key, HeaderString&& value);

  /**
   * For testing. Equality is based on equality of the backing list. This is an exact match
   * comparison (order matters).
   */
  bool operator==(const HeaderMapImpl& rhs) const;
  bool operator!=(const HeaderMapImpl& rhs) const;

  // Http::HeaderMap
  void addReference(const LowerCaseString& key, absl::string_view value) override;
  void addReferenceKey(const LowerCaseString& key, uint64_t value) override;
  void addReferenceKey(const LowerCaseString& key, absl::string_view value) override;
  void addCopy(const LowerCaseString& key, uint64_t value) override;
  void addCopy(const LowerCaseString& key, absl::string_view value) override;
  void appendCopy(const LowerCaseString& key, absl::string_view value) override;
  void setReference(const LowerCaseString& key, absl::string_view value) override;
  void setReferenceKey(const LowerCaseString& key, absl::string_view value) override;
  void setCopy(const LowerCaseString& key, absl::string_view value) override;
  uint64_t byteSize() const override;
  const HeaderEntry* get(const LowerCaseString& key) const override;
  void iterate(ConstIterateCb cb, void* context) const override;
  void iterateReverse(ConstIterateCb cb, void* context) const override;
  Lookup lookup(const LowerCaseString& key, const HeaderEntry** entry) const override;
  void clear() override;
  void remove(const LowerCaseString& key) override;
  void removePrefix(const LowerCaseString& key) override;
  size_t size() const override { return headers_.size(); }
  bool empty() const override { return headers_.empty(); }
  void dumpState(std::ostream& os, int indent_level = 0) const override;

protected:
  // For tests only, unoptimized, they aren't intended for regular HeaderMapImpl users.
  void copyFrom(const HeaderMap& rhs);

  struct HeaderEntryImpl : public HeaderEntry, NonCopyable {
    HeaderEntryImpl(const LowerCaseString& key);
    HeaderEntryImpl(const LowerCaseString& key, HeaderString&& value);
    HeaderEntryImpl(HeaderString&& key, HeaderString&& value);

    // HeaderEntry
    const HeaderString& key() const override { return key_; }
    void value(absl::string_view value) override;
    void value(uint64_t value) override;
    void value(const HeaderEntry& header) override;
    const HeaderString& value() const override { return value_; }
    HeaderString& value() override { return value_; }

    HeaderString key_;
    HeaderString value_;
    std::list<HeaderEntryImpl>::iterator entry_;
  };

  struct StaticLookupResponse {
    HeaderEntryImpl** entry_;
    const LowerCaseString* key_;
  };

  using EntryCb = StaticLookupResponse (*)(HeaderMapImpl&);

  /**
   * This is the static lookup table that is used to determine whether a header is one of the O(1)
   * headers. This uses a trie for lookup time at most equal to the size of the incoming string.
   */
  struct StaticLookupTable; // Defined in header_map_impl.cc.

  struct AllInlineHeaders {
    void clear() { memset(this, 0, sizeof(*this)); }

    ALL_INLINE_HEADERS(DEFINE_INLINE_HEADER_STRUCT)
  };

  /**
   * List of HeaderEntryImpl that keeps the pseudo headers (key starting with ':') in the front
   * of the list (as required by nghttp2) and otherwise maintains insertion order.
   *
   * Note: the internal iterators held in fields make this unsafe to copy and move, since the
   * reference to end() is not preserved across a move (see Notes in
   * https://en.cppreference.com/w/cpp/container/list/list). The NonCopyable will suppress both copy
   * and move constructors/assignment.
   * TODO(htuch): Maybe we want this to movable one day; for now, our header map moves happen on
   * HeaderMapPtr, so the performance impact should not be evident.
   */
  class HeaderList : NonCopyable {
  public:
    HeaderList() : pseudo_headers_end_(headers_.end()) {}

    template <class Key> bool isPseudoHeader(const Key& key) {
      return !key.getStringView().empty() && key.getStringView()[0] == ':';
    }

    template <class Key, class... Value>
    std::list<HeaderEntryImpl>::iterator insert(Key&& key, Value&&... value) {
      const bool is_pseudo_header = isPseudoHeader(key);
      std::list<HeaderEntryImpl>::iterator i =
          headers_.emplace(is_pseudo_header ? pseudo_headers_end_ : headers_.end(),
                           std::forward<Key>(key), std::forward<Value>(value)...);
      if (!is_pseudo_header && pseudo_headers_end_ == headers_.end()) {
        pseudo_headers_end_ = i;
      }
      return i;
    }

    std::list<HeaderEntryImpl>::iterator erase(std::list<HeaderEntryImpl>::iterator i) {
      if (pseudo_headers_end_ == i) {
        pseudo_headers_end_++;
      }
      return headers_.erase(i);
    }

    template <class UnaryPredicate> void remove_if(UnaryPredicate p) {
      headers_.remove_if([&](const HeaderEntryImpl& entry) {
        const bool to_remove = p(entry);
        if (to_remove) {
          if (pseudo_headers_end_ == entry.entry_) {
            pseudo_headers_end_++;
          }
        }
        return to_remove;
      });
    }

    std::list<HeaderEntryImpl>::iterator begin() { return headers_.begin(); }
    std::list<HeaderEntryImpl>::iterator end() { return headers_.end(); }
    std::list<HeaderEntryImpl>::const_iterator begin() const { return headers_.begin(); }
    std::list<HeaderEntryImpl>::const_iterator end() const { return headers_.end(); }
    std::list<HeaderEntryImpl>::const_reverse_iterator rbegin() const { return headers_.rbegin(); }
    std::list<HeaderEntryImpl>::const_reverse_iterator rend() const { return headers_.rend(); }
    size_t size() const { return headers_.size(); }
    bool empty() const { return headers_.empty(); }
    void clear() {
      headers_.clear();
      pseudo_headers_end_ = headers_.end();
    }

  private:
    std::list<HeaderEntryImpl> headers_;
    std::list<HeaderEntryImpl>::iterator pseudo_headers_end_;
  };

  void insertByKey(HeaderString&& key, HeaderString&& value);
  static uint64_t appendToHeader(HeaderString& header, absl::string_view data,
                                 absl::string_view delimiter = ",");
  HeaderEntryImpl& maybeCreateInline(HeaderEntryImpl** entry, const LowerCaseString& key);
  HeaderEntryImpl& maybeCreateInline(HeaderEntryImpl** entry, const LowerCaseString& key,
                                     HeaderString&& value);
  HeaderEntry* getExisting(const LowerCaseString& key);
  HeaderEntryImpl* getExistingInline(absl::string_view key);

  void removeInline(HeaderEntryImpl** entry);
  void updateSize(uint64_t from_size, uint64_t to_size);
  void addSize(uint64_t size);
  void subtractSize(uint64_t size);

  AllInlineHeaders inline_headers_;
  HeaderList headers_;

  // This holds the internal byte size of the HeaderMap.
  uint64_t cached_byte_size_ = 0;
  // Performs a manual byte size count.
  uint64_t byteSizeInternal() const;
  // In TestHeaderMapImpl and VerifiedHeaderMapImpl, this method is overridden to performs a
  // time-consuming manual byte size count on each operation to verify the byte size. For prod
  // HeaderMaps, this verification is skipped.
  // TODO(asraa): Move this verification out of prod code and wrap virtual Http::HeaderMap methods
  // in Http::TestHeaderMapImpl with the verification.
  virtual void verifyByteSize() {}

  ALL_INLINE_HEADERS(DEFINE_INLINE_HEADER_FUNCS)
};

using HeaderMapImplPtr = std::unique_ptr<HeaderMapImpl>;

} // namespace Http
} // namespace Envoy
