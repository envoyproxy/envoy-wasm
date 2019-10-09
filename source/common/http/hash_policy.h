#pragma once

#include "envoy/api/v2/route/route.pb.h"
#include "envoy/http/hash_policy.h"

namespace Envoy {
namespace Http {

/**
 * Implementation of HashPolicy that reads from the proto route config and only currently supports
 * hashing on an HTTP header.
 */
class HashPolicyImpl : public HashPolicy {
public:
  explicit HashPolicyImpl(
      absl::Span<const envoy::api::v2::route::RouteAction::HashPolicy* const> hash_policy);

  // Http::HashPolicy
  absl::optional<uint64_t> generateHash(const Network::Address::Instance* downstream_addr,
                                        const HeaderMap& headers,
                                        const AddCookieCallback add_cookie) const override;

  class HashMethod {
  public:
    virtual ~HashMethod() = default;
    virtual absl::optional<uint64_t> evaluate(const Network::Address::Instance* downstream_addr,
                                              const HeaderMap& headers,
                                              const AddCookieCallback add_cookie) const PURE;

    // If the method is a terminal method, ignore rest of the hash policy chain.
    virtual bool terminal() const PURE;
  };

  using HashMethodPtr = std::unique_ptr<HashMethod>;

private:
  std::vector<HashMethodPtr> hash_impls_;
};

} // namespace Http
} // namespace Envoy
