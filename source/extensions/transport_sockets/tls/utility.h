#pragma once

#include <string>
#include <vector>

#include "common/common/utility.h"

#include "absl/types/optional.h"
#include "openssl/ssl.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {
namespace Utility {

/**
 * Retrieves the serial number of a certificate.
 * @param cert the certificate
 * @return std::string the serial number field of the certificate. Returns "" if
 *         there is no serial number.
 */
std::string getSerialNumberFromCertificate(X509& cert);

/**
 * Retrieves the subject alternate names of a certificate of type DNS.
 * @param cert the certificate
 * @param type type of subject alternate name either GEN_DNS or GEN_URI
 * @return std::vector returns the list of subject alternate names.
 */
std::vector<std::string> getSubjectAltNames(X509& cert, int type);

/**
 * Retrieves the issuer from certificate.
 * @param cert the certificate
 * @return std::string the issuer field for the certificate.
 */
std::string getIssuerFromCertificate(X509& cert);

/**
 * Retrieves the subject from certificate.
 * @param cert the certificate
 * @return std::string the subject field for the certificate.
 */
std::string getSubjectFromCertificate(X509& cert);

/**
 * Returns the days until this certificate is valid.
 * @param cert the certificate
 * @param time_source the time source to use for current time calculation.
 * @return the number of days till this certificate is valid.
 */
int32_t getDaysUntilExpiration(const X509* cert, TimeSource& time_source);

/**
 * Returns the time from when this certificate is valid.
 * @param cert the certificate.
 * @return time from when this certificate is valid.
 */
SystemTime getValidFrom(const X509& cert);

/**
 * Returns the time when this certificate expires.
 * @param cert the certificate.
 * @return time after which the certificate expires.
 */
SystemTime getExpirationTime(const X509& cert);

/**
 * Returns the last crypto error from ERR_get_error(), or `absl::nullopt`
 * if the error stack is empty.
 * @return std::string error message
 */
absl::optional<std::string> getLastCryptoError();

} // namespace Utility
} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
