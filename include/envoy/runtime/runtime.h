#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "envoy/common/pure.h"
#include "envoy/type/percent.pb.h"

#include "common/common/assert.h"
#include "common/singleton/threadsafe_singleton.h"

#include "absl/container/flat_hash_map.h"
#include "absl/types/optional.h"

namespace Envoy {

namespace Upstream {
class ClusterManager;
}

namespace Runtime {

/**
 * Random number generator. Implementations should be thread safe.
 */
class RandomGenerator {
public:
  virtual ~RandomGenerator() = default;

  /**
   * @return uint64_t a new random number.
   */
  virtual uint64_t random() PURE;

  /**
   * @return std::string containing uuid4 of 36 char length.
   * for example, 7c25513b-0466-4558-a64c-12c6704f37ed
   */
  virtual std::string uuid() PURE;
};

using RandomGeneratorPtr = std::unique_ptr<RandomGenerator>;

/**
 * A snapshot of runtime data.
 */
class Snapshot {
public:
  virtual ~Snapshot() = default;

  struct Entry {
    std::string raw_string_value_;
    absl::optional<uint64_t> uint_value_;
    absl::optional<double> double_value_;
    absl::optional<envoy::type::FractionalPercent> fractional_percent_value_;
    absl::optional<bool> bool_value_;
  };

  using EntryMap = absl::flat_hash_map<std::string, Entry>;

  /**
   * A provider of runtime values. One or more of these compose the snapshot's source of values,
   * where successive layers override the previous ones.
   */
  class OverrideLayer {
  public:
    virtual ~OverrideLayer() = default;

    /**
     * @return const absl::flat_hash_map<std::string, Entry>& the values in this layer.
     */
    virtual const EntryMap& values() const PURE;

    /**
     * @return const std::string& a user-friendly alias for this layer, e.g. "admin" or "disk".
     */
    virtual const std::string& name() const PURE;
  };

  using OverrideLayerConstPtr = std::unique_ptr<const OverrideLayer>;

  // Returns true if a deprecated feature is allowed.
  //
  // Fundamentally, deprecated features are boolean values.
  // They are allowed by default or with explicit configuration to "true" via runtime configuration.
  // They can be disallowed either by inclusion in the hard-coded disallowed_features[] list, or by
  // configuration of "false" in runtime config.
  virtual bool deprecatedFeatureEnabled(const std::string& key) const PURE;

  // Returns true if a runtime feature is enabled.
  //
  // Runtime features are used to easily allow switching between old and new code paths for high
  // risk changes. The intent is for the old code path to be short lived - the old code path is
  // deprecated as the feature is defaulted true, and removed with the following Envoy release.
  virtual bool runtimeFeatureEnabled(absl::string_view key) const PURE;

  /**
   * Test if a feature is enabled using the built in random generator. This is done by generating
   * a random number in the range 0-99 and seeing if this number is < the value stored in the
   * runtime key, or the default_value if the runtime key is invalid.
   * NOTE: In the current implementation, this routine may return different results each time it is
   *       called because a new random number is used each time. Callers should understand this
   *       behavior and not assume that subsequent calls using the same snapshot will be consistent.
   * @param key supplies the feature key to lookup.
   * @param default_value supplies the default value that will be used if either the feature key
   *        does not exist or it is not an integer.
   * @return true if the feature is enabled.
   */
  virtual bool featureEnabled(const std::string& key, uint64_t default_value) const PURE;

  /**
   * Test if a feature is enabled using a supplied stable random value. This variant is used if
   * the caller wants a stable result over multiple calls.
   * @param key supplies the feature key to lookup.
   * @param default_value supplies the default value that will be used if either the feature key
   *        does not exist or it is not an integer.
   * @param random_value supplies the stable random value to use for determining whether the feature
   *        is enabled.
   * @return true if the feature is enabled.
   */
  virtual bool featureEnabled(const std::string& key, uint64_t default_value,
                              uint64_t random_value) const PURE;

  /**
   * Test if a feature is enabled using a supplied stable random value and total number of buckets
   * for sampling.
   * This variant is used if the caller wants a stable result over multiple calls
   * and have more granularity for samples.
   * @param key supplies the feature key to lookup.
   * @param default_value supplies the default value that will be used if either the feature key
   *        does not exist or it is not an integer.
   * @param random_value supplies the stable random value to use for determining whether the feature
   *        is enabled.
   * @param num_buckets control max number of buckets for sampling. Sampled value will be in a range
   *        of [0, num_buckets).
   * @return true if the feature is enabled.
   */
  virtual bool featureEnabled(const std::string& key, uint64_t default_value, uint64_t random_value,
                              uint64_t num_buckets) const PURE;

  /**
   * Test if a feature is enabled using the built in random generator. This is done by generating a
   * random number between 0 and the fractional percent denominator and seeing if this number is <
   * the numerator value stored in the runtime key. The default_value's numerator/denominator is
   * used if the runtime key is invalid.
   *
   * If the runtime value for the provided runtime key is provided as an integer, it is assumed that
   * the value is the numerator and the denominator is 100.
   *
   * NOTE: In the current implementation, this routine may return different results each time it is
   *       called because a new random number is used each time. Callers should understand this
   *       behavior and not assume that subsequent calls using the same snapshot will be consistent.
   * @param key supplies the feature key to lookup.
   * @param default_value supplies the default value that will be used if either the feature key
   *        does not exist or it is not a fractional percent.
   * @return true if the feature is enabled.
   */
  virtual bool featureEnabled(const std::string& key,
                              const envoy::type::FractionalPercent& default_value) const PURE;

  /**
   * Test if a feature is enabled using a supplied stable random value. This variant is used if
   * the caller wants a stable result over multiple calls.
   *
   * If the runtime value for the provided runtime key is provided as an integer, it is assumed that
   * the value is the numerator and the denominator is 100.
   *
   * @param key supplies the feature key to lookup.
   * @param default_value supplies the default value that will be used if either the feature key
   *        does not exist or it is not a fractional percent.
   * @param random_value supplies the stable random value to use for determining whether the feature
   *        is enabled.
   * @return true if the feature is enabled.
   */
  virtual bool featureEnabled(const std::string& key,
                              const envoy::type::FractionalPercent& default_value,
                              uint64_t random_value) const PURE;

  /**
   * Fetch raw runtime data based on key.
   * @param key supplies the key to fetch.
   * @return const std::string& the value or empty string if the key does not exist.
   */
  virtual const std::string& get(const std::string& key) const PURE;

  /**
   * Fetch an integer runtime key. Runtime keys larger than ~2^53 may not be accurately converted
   * into integers and will return default_value.
   * @param key supplies the key to fetch.
   * @param default_value supplies the value to return if the key does not exist or it does not
   *        contain an integer.
   * @return uint64_t the runtime value or the default value.
   */
  virtual uint64_t getInteger(const std::string& key, uint64_t default_value) const PURE;

  /**
   * Fetch a double runtime key.
   * @param key supplies the key to fetch.
   * @param default_value supplies the value to return if the key does not exist or it does not
   *        contain a double.
   * @return double the runtime value or the default value.
   */
  virtual double getDouble(const std::string& key, double default_value) const PURE;

  /**
   * Fetch a boolean runtime key.
   * @param key supplies the key to fetch.
   * @param default_value supplies the value to return if the key does not exist or it does not
   *        contain a boolean.
   * @return bool the runtime value or the default value.
   */
  virtual bool getBoolean(absl::string_view key, bool default_value) const PURE;

  /**
   * Fetch the OverrideLayers that provide values in this snapshot. Layers are ordered from bottom
   * to top; for instance, the second layer's entries override the first layer's entries, and so on.
   * Any layer can add a key in addition to overriding keys in layers below. The layer vector is
   * safe only for the lifetime of the Snapshot.
   * @return const std::vector<OverrideLayerConstPtr>& the raw map of loaded values.
   */
  virtual const std::vector<OverrideLayerConstPtr>& getLayers() const PURE;
};

/**
 * Loads runtime snapshots from storage (local disk, etc.).
 */
class Loader {
public:
  virtual ~Loader() = default;

  /**
   * Post-construction initialization. Runtime will be generally available after
   * the constructor is finished, with the exception of dynamic RTDS layers,
   * which require ClusterManager.
   * @param cm cluster manager reference.
   */
  virtual void initialize(Upstream::ClusterManager& cm) PURE;

  /**
   * @return const Snapshot& the current snapshot. This reference is safe to use for the duration of
   *         the calling routine, but may be overwritten on a future event loop cycle so should be
   *         fetched again when needed. This may only be called from worker threads.
   */
  virtual const Snapshot& snapshot() PURE;

  /**
   * @return shared_ptr<const Snapshot> the current snapshot. This function may safely be called
   *         from non-worker threads.
   */
  virtual std::shared_ptr<const Snapshot> threadsafeSnapshot() PURE;

  /**
   * Merge the given map of key-value pairs into the runtime's state. To remove a previous merge for
   * a key, use an empty string as the value.
   * @param values the values to merge
   */
  virtual void mergeValues(const std::unordered_map<std::string, std::string>& values) PURE;
};

using LoaderPtr = std::unique_ptr<Loader>;

// To make the runtime generally accessible, we make use of the dreaded
// singleton class. For Envoy, the runtime will be created and cleaned up by the
// Server::InstanceImpl initialize() and destructor, respectively.
//
// This makes it possible for call sites to easily make use of runtime values to
// determine if a given feature is on or off, as well as various deprecated configuration
// protos being enabled or disabled by default.
using LoaderSingleton = InjectableSingleton<Loader>;
using ScopedLoaderSingleton = ScopedInjectableLoader<Loader>;

} // namespace Runtime
} // namespace Envoy
