#pragma once

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <type_traits>

#include "envoy/http/header_map.h"

#include "common/common/non_copyable.h"
#include "common/common/utility.h"
#include "common/http/headers.h"

namespace Envoy {
namespace Http {

/**
 * These are definitions of all of the inline header access functions described inside header_map.h
 */
#define DEFINE_INLINE_HEADER_FUNCS(name)                                                           \
public:                                                                                            \
  const HeaderEntry* name() const override { return getInline(HeaderHandles::get().name); }        \
  void append##name(absl::string_view data, absl::string_view delimiter) override {                \
    appendInline(HeaderHandles::get().name, data, delimiter);                                      \
  }                                                                                                \
  void setReference##name(absl::string_view value) override {                                      \
    setReferenceInline(HeaderHandles::get().name, value);                                          \
  }                                                                                                \
  void set##name(absl::string_view value) override {                                               \
    setInline(HeaderHandles::get().name, value);                                                   \
  }                                                                                                \
  void set##name(uint64_t value) override { setInline(HeaderHandles::get().name, value); }         \
  size_t remove##name() override { return removeInline(HeaderHandles::get().name); }               \
  absl::string_view get##name##Value() const override {                                            \
    return getInlineValue(HeaderHandles::get().name);                                              \
  }

/**
 * Implementation of Http::HeaderMap. This is heavily optimized for performance. Roughly, when
 * headers are added to the map by string, we do a trie lookup to see if it's one of the O(1)
 * headers. If it is, we store a reference to it that can be accessed later directly via direct
 * method access. Most high performance paths use O(1) direct method access. In general, we try to
 * copy as little as possible and allocate as little as possible in any of the paths.
 */
class HeaderMapImpl : NonCopyable {
public:
  virtual ~HeaderMapImpl() = default;

  // The following "constructors" call virtual functions during construction and must use the
  // static factory pattern.
  static void copyFrom(HeaderMap& lhs, const HeaderMap& rhs);
  // The value_type of iterator must be pair, and the first value of them must be LowerCaseString.
  // If not, it won't be compiled successfully.
  template <class It> static void initFromInitList(HeaderMap& new_header_map, It begin, It end) {
    for (auto it = begin; it != end; ++it) {
      static_assert(std::is_same<decltype(it->first), LowerCaseString>::value,
                    "iterator must be pair and the first value of them must be LowerCaseString");
      HeaderString key_string;
      key_string.setCopy(it->first.get().c_str(), it->first.get().size());
      HeaderString value_string;
      value_string.setCopy(it->second.c_str(), it->second.size());
      new_header_map.addViaMove(std::move(key_string), std::move(value_string));
    }
  }

  // Performs a manual byte size count for test verification.
  void verifyByteSizeInternalForTest() const;

  // Note: This class does not actually implement Http::HeaderMap to avoid virtual inheritance in
  // the derived classes. Instead, it is used as a mix-in class for TypedHeaderMapImpl below. This
  // both avoid virtual inheritance and allows the concrete final header maps to use a variable
  // length member at the end.
  bool operator==(const HeaderMap& rhs) const;
  bool operator!=(const HeaderMap& rhs) const;
  void addViaMove(HeaderString&& key, HeaderString&& value);
  void addReference(const LowerCaseString& key, absl::string_view value);
  void addReferenceKey(const LowerCaseString& key, uint64_t value);
  void addReferenceKey(const LowerCaseString& key, absl::string_view value);
  void addCopy(const LowerCaseString& key, uint64_t value);
  void addCopy(const LowerCaseString& key, absl::string_view value);
  void appendCopy(const LowerCaseString& key, absl::string_view value);
  void setReference(const LowerCaseString& key, absl::string_view value);
  void setReferenceKey(const LowerCaseString& key, absl::string_view value);
  void setCopy(const LowerCaseString& key, absl::string_view value);
  uint64_t byteSize() const;
  const HeaderEntry* get(const LowerCaseString& key) const;
  void iterate(HeaderMap::ConstIterateCb cb, void* context) const;
  void iterateReverse(HeaderMap::ConstIterateCb cb, void* context) const;
  HeaderMap::Lookup lookup(const LowerCaseString& key, const HeaderEntry** entry) const;
  void clear();
  size_t remove(const LowerCaseString& key);
  size_t removePrefix(const LowerCaseString& key);
  size_t size() const { return headers_.size(); }
  bool empty() const { return headers_.empty(); }
  void dumpState(std::ostream& os, int indent_level = 0) const;

protected:
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

  /**
   * This is the static lookup table that is used to determine whether a header is one of the O(1)
   * headers. This uses a trie for lookup time at most equal to the size of the incoming string.
   */
  struct StaticLookupResponse {
    HeaderEntryImpl** entry_;
    const LowerCaseString* key_;
  };

  /**
   * Base class for a static lookup table that converts a string key into an O(1) header.
   */
  template <class Interface>
  struct StaticLookupTable
      : public TrieLookupTable<std::function<StaticLookupResponse(HeaderMapImpl&)>> {
    StaticLookupTable();

    void finalizeTable() {
      CustomInlineHeaderRegistry::finalize<Interface::header_map_type>();
      auto& headers = CustomInlineHeaderRegistry::headers<Interface::header_map_type>();
      size_ = headers.size();
      for (const auto& header : headers) {
        this->add(header.first.get().c_str(), [&header](HeaderMapImpl& h) -> StaticLookupResponse {
          return {&h.inlineHeaders()[header.second], &header.first};
        });
      }
    }

    static size_t size() {
      // The size of the lookup table is finalized when the singleton lookup table is created. This
      // allows for late binding of custom headers as well as envoy header prefix changes. This
      // does mean that once the first header map is created of this type, no further changes are
      // possible.
      // TODO(mattklein123): If we decide to keep this implementation, it is conceivable that header
      // maps could be created by an API factory that is owned by the listener/HCM, thus making
      // O(1) header delivery over xDS possible.
      return ConstSingleton<StaticLookupTable>::get().size_;
    }

    static absl::optional<StaticLookupResponse> lookup(HeaderMapImpl& header_map,
                                                       absl::string_view key) {
      const auto& entry = ConstSingleton<StaticLookupTable>::get().find(key);
      if (entry != nullptr) {
        return entry(header_map);
      } else {
        return absl::nullopt;
      }
    }

    size_t size_;
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
  size_t removeInline(HeaderEntryImpl** entry);
  void updateSize(uint64_t from_size, uint64_t to_size);
  void addSize(uint64_t size);
  void subtractSize(uint64_t size);
  virtual absl::optional<StaticLookupResponse> staticLookup(absl::string_view) PURE;
  virtual void clearInline() PURE;
  virtual HeaderEntryImpl** inlineHeaders() PURE;

  HeaderList headers_;
  // This holds the internal byte size of the HeaderMap.
  uint64_t cached_byte_size_ = 0;
};

/**
 * Typed derived classes for all header map types. This class implements the actual typed
 * interface and for the majority of methods just passes through to the HeaderMapImpl mix-in. Per
 * above, this avoids virtual inheritance.
 */
template <class Interface> class TypedHeaderMapImpl : public HeaderMapImpl, public Interface {
public:
  // Implementation of Http::HeaderMap that passes through to HeaderMapImpl.
  bool operator==(const HeaderMap& rhs) const override { return HeaderMapImpl::operator==(rhs); }
  bool operator!=(const HeaderMap& rhs) const override { return HeaderMapImpl::operator!=(rhs); }
  void addViaMove(HeaderString&& key, HeaderString&& value) override {
    HeaderMapImpl::addViaMove(std::move(key), std::move(value));
  }
  void addReference(const LowerCaseString& key, absl::string_view value) override {
    HeaderMapImpl::addReference(key, value);
  }
  void addReferenceKey(const LowerCaseString& key, uint64_t value) override {
    HeaderMapImpl::addReferenceKey(key, value);
  }
  void addReferenceKey(const LowerCaseString& key, absl::string_view value) override {
    HeaderMapImpl::addReferenceKey(key, value);
  }
  void addCopy(const LowerCaseString& key, uint64_t value) override {
    HeaderMapImpl::addCopy(key, value);
  }
  void addCopy(const LowerCaseString& key, absl::string_view value) override {
    HeaderMapImpl::addCopy(key, value);
  }
  void appendCopy(const LowerCaseString& key, absl::string_view value) override {
    HeaderMapImpl::appendCopy(key, value);
  }
  void setReference(const LowerCaseString& key, absl::string_view value) override {
    HeaderMapImpl::setReference(key, value);
  }
  void setReferenceKey(const LowerCaseString& key, absl::string_view value) override {
    HeaderMapImpl::setReferenceKey(key, value);
  }
  void setCopy(const LowerCaseString& key, absl::string_view value) override {
    HeaderMapImpl::setCopy(key, value);
  }
  uint64_t byteSize() const override { return HeaderMapImpl::byteSize(); }
  const HeaderEntry* get(const LowerCaseString& key) const override {
    return HeaderMapImpl::get(key);
  }
  void iterate(HeaderMap::ConstIterateCb cb, void* context) const override {
    HeaderMapImpl::iterate(cb, context);
  }
  void iterateReverse(HeaderMap::ConstIterateCb cb, void* context) const override {
    HeaderMapImpl::iterateReverse(cb, context);
  }
  HeaderMap::Lookup lookup(const LowerCaseString& key, const HeaderEntry** entry) const override {
    return HeaderMapImpl::lookup(key, entry);
  }
  void clear() override { HeaderMapImpl::clear(); }
  size_t remove(const LowerCaseString& key) override { return HeaderMapImpl::remove(key); }
  size_t removePrefix(const LowerCaseString& key) override {
    return HeaderMapImpl::removePrefix(key);
  }
  size_t size() const override { return HeaderMapImpl::size(); }
  bool empty() const override { return HeaderMapImpl::empty(); }
  void dumpState(std::ostream& os, int indent_level = 0) const override {
    HeaderMapImpl::dumpState(os, indent_level);
  }

  // Generic custom header functions for each fully typed interface. To avoid accidental issues,
  // the Handle type is different for each interface, which is why these functions live here vs.
  // inside HeaderMapImpl.
  using Handle = CustomInlineHeaderRegistry::Handle<Interface::header_map_type>;
  const HeaderEntry* getInline(Handle handle) const override {
    ASSERT(handle.it_->second < inlineHeadersSize());
    return constInlineHeaders()[handle.it_->second];
  }
  void appendInline(Handle handle, absl::string_view data, absl::string_view delimiter) override {
    ASSERT(handle.it_->second < inlineHeadersSize());
    HeaderEntry& entry = maybeCreateInline(&inlineHeaders()[handle.it_->second], handle.it_->first);
    addSize(HeaderMapImpl::appendToHeader(entry.value(), data, delimiter));
  }
  void setReferenceInline(Handle handle, absl::string_view value) override {
    ASSERT(handle.it_->second < inlineHeadersSize());
    HeaderEntry& entry = maybeCreateInline(&inlineHeaders()[handle.it_->second], handle.it_->first);
    updateSize(entry.value().size(), value.size());
    entry.value().setReference(value);
  }
  void setInline(Handle handle, absl::string_view value) override {
    ASSERT(handle.it_->second < inlineHeadersSize());
    HeaderEntry& entry = maybeCreateInline(&inlineHeaders()[handle.it_->second], handle.it_->first);
    updateSize(entry.value().size(), value.size());
    entry.value().setCopy(value);
  }
  void setInline(Handle handle, uint64_t value) override {
    ASSERT(handle.it_->second < inlineHeadersSize());
    HeaderEntry& entry = maybeCreateInline(&inlineHeaders()[handle.it_->second], handle.it_->first);
    subtractSize(entry.value().size());
    entry.value().setInteger(value);
    addSize(entry.value().size());
  }
  size_t removeInline(Handle handle) override {
    ASSERT(handle.it_->second < inlineHeadersSize());
    return HeaderMapImpl::removeInline(&inlineHeaders()[handle.it_->second]);
  }
  static size_t inlineHeadersSize() {
    return StaticLookupTable<Interface>::size() * sizeof(HeaderEntryImpl*);
  }

protected:
  absl::optional<StaticLookupResponse> staticLookup(absl::string_view key) override {
    return StaticLookupTable<Interface>::lookup(*this, key);
  }
  virtual const HeaderEntryImpl* const* constInlineHeaders() const PURE;
};

#define DEFINE_HEADER_HANDLE(name)                                                                 \
  Handle name =                                                                                    \
      CustomInlineHeaderRegistry::getInlineHeader<header_map_type>(Headers::get().name).value();

/**
 * Concrete implementation of RequestHeaderMap which allows for variable custom registered inline
 * headers.
 */
class RequestHeaderMapImpl final : public TypedHeaderMapImpl<RequestHeaderMap>,
                                   public InlineStorage {
public:
  static std::unique_ptr<RequestHeaderMapImpl> create() {
    return std::unique_ptr<RequestHeaderMapImpl>(new (inlineHeadersSize()) RequestHeaderMapImpl());
  }

  INLINE_REQ_HEADERS(DEFINE_INLINE_HEADER_FUNCS)
  INLINE_REQ_RESP_HEADERS(DEFINE_INLINE_HEADER_FUNCS)

protected:
  // NOTE: Because inline_headers_ is a variable size member, it must be the last member in the
  // most derived class. This forces the definition of the following three functions to also be
  // in the most derived class and thus duplicated. There may be a way to consolidate thus but it's
  // not clear and can be deferred for now.
  void clearInline() override { memset(inline_headers_, 0, inlineHeadersSize()); }
  const HeaderEntryImpl* const* constInlineHeaders() const override { return inline_headers_; }
  HeaderEntryImpl** inlineHeaders() override { return inline_headers_; }

private:
  struct HeaderHandleValues {
    INLINE_REQ_HEADERS(DEFINE_HEADER_HANDLE)
    INLINE_REQ_RESP_HEADERS(DEFINE_HEADER_HANDLE)
  };

  using HeaderHandles = ConstSingleton<HeaderHandleValues>;

  RequestHeaderMapImpl() { clearInline(); }

  HeaderEntryImpl* inline_headers_[];
};

/**
 * Concrete implementation of RequestTrailerMap which allows for variable custom registered inline
 * headers.
 */
class RequestTrailerMapImpl final : public TypedHeaderMapImpl<RequestTrailerMap>,
                                    public InlineStorage {
public:
  static std::unique_ptr<RequestTrailerMapImpl> create() {
    return std::unique_ptr<RequestTrailerMapImpl>(new (inlineHeadersSize())
                                                      RequestTrailerMapImpl());
  }

protected:
  // See comment in RequestHeaderMapImpl.
  void clearInline() override { memset(inline_headers_, 0, inlineHeadersSize()); }
  const HeaderEntryImpl* const* constInlineHeaders() const override { return inline_headers_; }
  HeaderEntryImpl** inlineHeaders() override { return inline_headers_; }

private:
  RequestTrailerMapImpl() { clearInline(); }

  HeaderEntryImpl* inline_headers_[];
};

/**
 * Concrete implementation of ResponseHeaderMap which allows for variable custom registered inline
 * headers.
 */
class ResponseHeaderMapImpl final : public TypedHeaderMapImpl<ResponseHeaderMap>,
                                    public InlineStorage {
public:
  static std::unique_ptr<ResponseHeaderMapImpl> create() {
    return std::unique_ptr<ResponseHeaderMapImpl>(new (inlineHeadersSize())
                                                      ResponseHeaderMapImpl());
  }

  INLINE_RESP_HEADERS(DEFINE_INLINE_HEADER_FUNCS)
  INLINE_REQ_RESP_HEADERS(DEFINE_INLINE_HEADER_FUNCS)
  INLINE_RESP_HEADERS_TRAILERS(DEFINE_INLINE_HEADER_FUNCS)

protected:
  // See comment in RequestHeaderMapImpl.
  void clearInline() override { memset(inline_headers_, 0, inlineHeadersSize()); }
  const HeaderEntryImpl* const* constInlineHeaders() const override { return inline_headers_; }
  HeaderEntryImpl** inlineHeaders() override { return inline_headers_; }

private:
  struct HeaderHandleValues {
    INLINE_RESP_HEADERS(DEFINE_HEADER_HANDLE)
    INLINE_REQ_RESP_HEADERS(DEFINE_HEADER_HANDLE)
    INLINE_RESP_HEADERS_TRAILERS(DEFINE_HEADER_HANDLE)
  };

  using HeaderHandles = ConstSingleton<HeaderHandleValues>;

  ResponseHeaderMapImpl() { clearInline(); }

  HeaderEntryImpl* inline_headers_[];
};

/**
 * Concrete implementation of ResponseTrailerMap which allows for variable custom registered
 * inline headers.
 */
class ResponseTrailerMapImpl final : public TypedHeaderMapImpl<ResponseTrailerMap>,
                                     public InlineStorage {
public:
  static std::unique_ptr<ResponseTrailerMapImpl> create() {
    return std::unique_ptr<ResponseTrailerMapImpl>(new (inlineHeadersSize())
                                                       ResponseTrailerMapImpl());
  }

  INLINE_RESP_HEADERS_TRAILERS(DEFINE_INLINE_HEADER_FUNCS)

protected:
  // See comment in RequestHeaderMapImpl.
  void clearInline() override { memset(inline_headers_, 0, inlineHeadersSize()); }
  const HeaderEntryImpl* const* constInlineHeaders() const override { return inline_headers_; }
  HeaderEntryImpl** inlineHeaders() override { return inline_headers_; }

private:
  struct HeaderHandleValues {
    INLINE_RESP_HEADERS_TRAILERS(DEFINE_HEADER_HANDLE)
  };

  using HeaderHandles = ConstSingleton<HeaderHandleValues>;

  ResponseTrailerMapImpl() { clearInline(); }

  HeaderEntryImpl* inline_headers_[];
};

template <class T>
std::unique_ptr<T>
createHeaderMap(const std::initializer_list<std::pair<LowerCaseString, std::string>>& values) {
  auto new_header_map = T::create();
  HeaderMapImpl::initFromInitList(*new_header_map, values.begin(), values.end());
  return new_header_map;
}

template <class T, class It> std::unique_ptr<T> createHeaderMap(It begin, It end) {
  auto new_header_map = T::create();
  HeaderMapImpl::initFromInitList(*new_header_map, begin, end);
  return new_header_map;
}

template <class T> std::unique_ptr<T> createHeaderMap(const HeaderMap& rhs) {
  // TODO(mattklein123): Use of this function allows copying a request header map into a response
  // header map, etc. which is probably not what we want. Unfortunately, we do this on purpose in
  // a few places when dealing with gRPC headers/trailers conversions so it's not trivial to remove.
  // We should revisit this to figure how to make this a bit safer as a non-intentional conversion
  // may have surprising results with different O(1) headers, implementations, etc.
  auto new_header_map = T::create();
  HeaderMapImpl::copyFrom(*new_header_map, rhs);
  return new_header_map;
}

struct EmptyHeaders {
  RequestHeaderMapPtr request_headers = RequestHeaderMapImpl::create();
  ResponseHeaderMapPtr response_headers = ResponseHeaderMapImpl::create();
  ResponseTrailerMapPtr response_trailers = ResponseTrailerMapImpl::create();
};

using StaticEmptyHeaders = ConstSingleton<EmptyHeaders>;

class HeaderMapImplUtility {
public:
  struct HeaderMapImplInfo {
    // Human readable name for the header map used in info logging.
    std::string name_;
    // The byte size of the header map including both fixed space as well as variable space used
    // by the registered custom headers.
    size_t size_;
    // All registered custom headers for the header map.
    std::vector<std::string> registered_headers_;
  };

  /**
   * Fetch detailed information about each header map implementation for use in logging.
   */
  static std::vector<HeaderMapImplInfo> getAllHeaderMapImplInfo();
};

} // namespace Http
} // namespace Envoy
