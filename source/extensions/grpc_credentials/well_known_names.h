#pragma once

#include <string>

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Extensions {
namespace GrpcCredentials {

/**
 * Well-known gRPC Credentials names.
 * NOTE: New gRPC Credentials should use the well known name: envoy.grpc_credentials.name.
 */
class GrpcCredentialsNameValues {
public:
  // Access Token Example.
  const std::string AccessTokenExample = "envoy.grpc_credentials.access_token_example";
  // File Based Metadata credentials
  const std::string FileBasedMetadata = "envoy.grpc_credentials.file_based_metadata";
  // AWS IAM
  const std::string AwsIam = "envoy.grpc_credentials.aws_iam";
};

using GrpcCredentialsNames = ConstSingleton<GrpcCredentialsNameValues>;

} // namespace GrpcCredentials
} // namespace Extensions
} // namespace Envoy
