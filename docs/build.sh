#!/bin/bash

. tools/shell_utils.sh

set -e

# We need to set ENVOY_DOCS_VERSION_STRING and ENVOY_DOCS_RELEASE_LEVEL for Sphinx.
# We also validate that the tag and version match at this point if needed.
if [ -n "$CIRCLE_TAG" ]
then
  # Check the git tag matches the version number in the VERSION file.
  VERSION_NUMBER=$(cat VERSION)
  if [ "v${VERSION_NUMBER}" != "${CIRCLE_TAG}" ]; then
    echo "Given git tag does not match the VERSION file content:"
    echo "${CIRCLE_TAG} vs $(cat VERSION)"
    exit 1
  fi
  # Check the version_history.rst contains current release version.
  grep --fixed-strings "$VERSION_NUMBER" docs/root/intro/version_history.rst \
    || (echo "Git tag not found in version_history.rst" && exit 1)

  # Now that we now there is a match, we can use the tag.
  export ENVOY_DOCS_VERSION_STRING="tag-$CIRCLE_TAG"
  export ENVOY_DOCS_RELEASE_LEVEL=tagged
  export ENVOY_BLOB_SHA="$CIRCLE_TAG"
else
  BUILD_SHA=$(git rev-parse HEAD)
  VERSION_NUM=$(cat VERSION)
  export ENVOY_DOCS_VERSION_STRING="${VERSION_NUM}"-"${BUILD_SHA:0:6}"
  export ENVOY_DOCS_RELEASE_LEVEL=pre-release
  export ENVOY_BLOB_SHA="$BUILD_SHA"
fi

SCRIPT_DIR=$(dirname "$0")
API_DIR=$(dirname "$dir")/api
BUILD_DIR=build_docs
[[ -z "${DOCS_OUTPUT_DIR}" ]] && DOCS_OUTPUT_DIR=generated/docs
[[ -z "${GENERATED_RST_DIR}" ]] && GENERATED_RST_DIR=generated/rst

rm -rf "${DOCS_OUTPUT_DIR}"
mkdir -p "${DOCS_OUTPUT_DIR}"

rm -rf "${GENERATED_RST_DIR}"
mkdir -p "${GENERATED_RST_DIR}"

source_venv "$BUILD_DIR"
pip install -r "${SCRIPT_DIR}"/requirements.txt

bazel build ${BAZEL_BUILD_OPTIONS} @envoy_api//docs:protos --aspects \
  tools/protodoc/protodoc.bzl%proto_doc_aspect --output_groups=rst --action_env=CPROFILE_ENABLED \
  --action_env=ENVOY_BLOB_SHA --spawn_strategy=standalone --host_force_python=PY2

# These are the protos we want to put in docs, this list will grow.
# TODO(htuch): Factor this out of this script.
PROTO_RST="
  /envoy/admin/v2alpha/certs/envoy/admin/v2alpha/certs.proto.rst
  /envoy/admin/v2alpha/clusters/envoy/admin/v2alpha/clusters.proto.rst
  /envoy/admin/v2alpha/config_dump/envoy/admin/v2alpha/config_dump.proto.rst
  /envoy/admin/v2alpha/listeners/envoy/admin/v2alpha/listeners.proto.rst
  /envoy/admin/v2alpha/memory/envoy/admin/v2alpha/memory.proto.rst
  /envoy/admin/v2alpha/clusters/envoy/admin/v2alpha/metrics.proto.rst
  /envoy/admin/v2alpha/mutex_stats/envoy/admin/v2alpha/mutex_stats.proto.rst
  /envoy/admin/v2alpha/server_info/envoy/admin/v2alpha/server_info.proto.rst
  /envoy/admin/v2alpha/tap/envoy/admin/v2alpha/tap.proto.rst
  /envoy/api/v2/core/address/envoy/api/v2/core/address.proto.rst
  /envoy/api/v2/core/base/envoy/api/v2/core/base.proto.rst
  /envoy/api/v2/core/http_uri/envoy/api/v2/core/http_uri.proto.rst
  /envoy/api/v2/core/config_source/envoy/api/v2/core/config_source.proto.rst
  /envoy/api/v2/core/grpc_service/envoy/api/v2/core/grpc_service.proto.rst
  /envoy/api/v2/core/health_check/envoy/api/v2/core/health_check.proto.rst
  /envoy/api/v2/core/protocol/envoy/api/v2/core/protocol.proto.rst
  /envoy/api/v2/discovery/envoy/api/v2/discovery.proto.rst
  /envoy/api/v2/auth/cert/envoy/api/v2/auth/cert.proto.rst
  /envoy/api/v2/eds/envoy/api/v2/eds.proto.rst
  /envoy/api/v2/endpoint/endpoint/envoy/api/v2/endpoint/endpoint.proto.rst
  /envoy/api/v2/cds/envoy/api/v2/cds.proto.rst
  /envoy/api/v2/cluster/outlier_detection/envoy/api/v2/cluster/outlier_detection.proto.rst
  /envoy/api/v2/cluster/circuit_breaker/envoy/api/v2/cluster/circuit_breaker.proto.rst
  /envoy/api/v2/cluster/filter/envoy/api/v2/cluster/filter.proto.rst
  /envoy/api/v2/rds/envoy/api/v2/rds.proto.rst
  /envoy/api/v2/route/route/envoy/api/v2/route/route.proto.rst
  /envoy/api/v2/srds/envoy/api/v2/srds.proto.rst
  /envoy/api/v2/lds/envoy/api/v2/lds.proto.rst
  /envoy/api/v2/listener/listener/envoy/api/v2/listener/listener.proto.rst
  /envoy/api/v2/ratelimit/ratelimit/envoy/api/v2/ratelimit/ratelimit.proto.rst
  /envoy/config/accesslog/v2/als/envoy/config/accesslog/v2/als.proto.rst
  /envoy/config/accesslog/v2/file/envoy/config/accesslog/v2/file.proto.rst
  /envoy/config/bootstrap/v2/bootstrap/envoy/config/bootstrap/v2/bootstrap.proto.rst
  /envoy/config/cluster/dynamic_forward_proxy/v2alpha/cluster/envoy/config/cluster/dynamic_forward_proxy/v2alpha/cluster.proto.rst
  /envoy/config/cluster/redis/redis_cluster/envoy/config/cluster/redis/redis_cluster.proto.rst
  /envoy/config/common/dynamic_forward_proxy/v2alpha/dns_cache/envoy/config/common/dynamic_forward_proxy/v2alpha/dns_cache.proto.rst
  /envoy/config/common/tap/v2alpha/common/envoy/config/common/tap/v2alpha/common.proto.rst
  /envoy/config/ratelimit/v2/rls/envoy/config/ratelimit/v2/rls.proto.rst
  /envoy/config/metrics/v2/metrics_service/envoy/config/metrics/v2/metrics_service.proto.rst
  /envoy/config/metrics/v2/stats/envoy/config/metrics/v2/stats.proto.rst
  /envoy/config/trace/v2/trace/envoy/config/trace/v2/trace.proto.rst
  /envoy/config/filter/accesslog/v2/accesslog/envoy/config/filter/accesslog/v2/accesslog.proto.rst
  /envoy/config/filter/fault/v2/fault/envoy/config/filter/fault/v2/fault.proto.rst
  /envoy/config/filter/http/buffer/v2/buffer/envoy/config/filter/http/buffer/v2/buffer.proto.rst
  /envoy/config/filter/http/csrf/v2/csrf/envoy/config/filter/http/csrf/v2/csrf.proto.rst
  /envoy/config/filter/http/dynamic_forward_proxy/v2alpha/dynamic_forward_proxy/envoy/config/filter/http/dynamic_forward_proxy/v2alpha/dynamic_forward_proxy.proto.rst
  /envoy/config/filter/http/ext_authz/v2/ext_authz/envoy/config/filter/http/ext_authz/v2/ext_authz.proto.rst
  /envoy/config/filter/http/fault/v2/fault/envoy/config/filter/http/fault/v2/fault.proto.rst
  /envoy/config/filter/http/gzip/v2/gzip/envoy/config/filter/http/gzip/v2/gzip.proto.rst
  /envoy/config/filter/http/health_check/v2/health_check/envoy/config/filter/http/health_check/v2/health_check.proto.rst
  /envoy/config/filter/http/header_to_metadata/v2/header_to_metadata/envoy/config/filter/http/header_to_metadata/v2/header_to_metadata.proto.rst
  /envoy/config/filter/http/ip_tagging/v2/ip_tagging/envoy/config/filter/http/ip_tagging/v2/ip_tagging.proto.rst
  /envoy/config/filter/http/jwt_authn/v2alpha/jwt_authn/envoy/config/filter/http/jwt_authn/v2alpha/config.proto.rst
  /envoy/config/filter/http/lua/v2/lua/envoy/config/filter/http/lua/v2/lua.proto.rst
  /envoy/config/filter/http/original_src/v2alpha1/original_src/envoy/config/filter/http/original_src/v2alpha1/original_src.proto.rst
  /envoy/config/filter/http/rate_limit/v2/rate_limit/envoy/config/filter/http/rate_limit/v2/rate_limit.proto.rst
  /envoy/config/filter/http/rbac/v2/rbac/envoy/config/filter/http/rbac/v2/rbac.proto.rst
  /envoy/config/filter/http/router/v2/router/envoy/config/filter/http/router/v2/router.proto.rst
  /envoy/config/filter/http/squash/v2/squash/envoy/config/filter/http/squash/v2/squash.proto.rst
  /envoy/config/filter/http/tap/v2alpha/tap/envoy/config/filter/http/tap/v2alpha/tap.proto.rst
  /envoy/config/filter/http/transcoder/v2/transcoder/envoy/config/filter/http/transcoder/v2/transcoder.proto.rst
  /envoy/config/filter/listener/original_src/v2alpha1/original_src/envoy/config/filter/listener/original_src/v2alpha1/original_src.proto.rst
  /envoy/config/filter/network/dubbo_proxy/v2alpha1/dubbo_proxy/envoy/config/filter/network/dubbo_proxy/v2alpha1/dubbo_proxy.proto.rst
  /envoy/config/filter/network/dubbo_proxy/v2alpha1/dubbo_proxy/envoy/config/filter/network/dubbo_proxy/v2alpha1/route.proto.rst
  /envoy/config/filter/dubbo/router/v2alpha1/router/envoy/config/filter/dubbo/router/v2alpha1/router.proto.rst
  /envoy/config/filter/network/client_ssl_auth/v2/client_ssl_auth/envoy/config/filter/network/client_ssl_auth/v2/client_ssl_auth.proto.rst
  /envoy/config/filter/network/ext_authz/v2/ext_authz/envoy/config/filter/network/ext_authz/v2/ext_authz.proto.rst
  /envoy/config/filter/network/http_connection_manager/v2/http_connection_manager/envoy/config/filter/network/http_connection_manager/v2/http_connection_manager.proto.rst
  /envoy/config/filter/network/mongo_proxy/v2/mongo_proxy/envoy/config/filter/network/mongo_proxy/v2/mongo_proxy.proto.rst
  /envoy/config/filter/network/rate_limit/v2/rate_limit/envoy/config/filter/network/rate_limit/v2/rate_limit.proto.rst
  /envoy/config/filter/network/rbac/v2/rbac/envoy/config/filter/network/rbac/v2/rbac.proto.rst
  /envoy/config/filter/network/redis_proxy/v2/redis_proxy/envoy/config/filter/network/redis_proxy/v2/redis_proxy.proto.rst
  /envoy/config/filter/network/tcp_proxy/v2/tcp_proxy/envoy/config/filter/network/tcp_proxy/v2/tcp_proxy.proto.rst
  /envoy/config/filter/network/thrift_proxy/v2alpha1/thrift_proxy/envoy/config/filter/network/thrift_proxy/v2alpha1/thrift_proxy.proto.rst
  /envoy/config/filter/network/thrift_proxy/v2alpha1/thrift_proxy/envoy/config/filter/network/thrift_proxy/v2alpha1/route.proto.rst
  /envoy/config/filter/thrift/rate_limit/v2alpha1/rate_limit/envoy/config/filter/thrift/rate_limit/v2alpha1/rate_limit.proto.rst
  /envoy/config/filter/thrift/router/v2alpha1/router/envoy/config/filter/thrift/router/v2alpha1/router.proto.rst
  /envoy/config/grpc_credential/v2alpha/aws_iam/envoy/config/grpc_credential/v2alpha/aws_iam.proto.rst
  /envoy/config/health_checker/redis/v2/redis/envoy/config/health_checker/redis/v2/redis.proto.rst
  /envoy/config/overload/v2alpha/overload/envoy/config/overload/v2alpha/overload.proto.rst
  /envoy/config/rbac/v2/rbac/envoy/config/rbac/v2/rbac.proto.rst
  /envoy/config/resource_monitor/fixed_heap/v2alpha/fixed_heap/envoy/config/resource_monitor/fixed_heap/v2alpha/fixed_heap.proto.rst
  /envoy/config/resource_monitor/injected_resource/v2alpha/injected_resource/envoy/config/resource_monitor/injected_resource/v2alpha/injected_resource.proto.rst
  /envoy/config/transport_socket/tap/v2alpha/tap/envoy/config/transport_socket/tap/v2alpha/tap.proto.rst
  /envoy/data/accesslog/v2/accesslog/envoy/data/accesslog/v2/accesslog.proto.rst
  /envoy/data/core/v2alpha/health_check_event/envoy/data/core/v2alpha/health_check_event.proto.rst
  /envoy/data/tap/v2alpha/common/envoy/data/tap/v2alpha/common.proto.rst
  /envoy/data/tap/v2alpha/transport/envoy/data/tap/v2alpha/transport.proto.rst
  /envoy/data/tap/v2alpha/http/envoy/data/tap/v2alpha/http.proto.rst
  /envoy/data/tap/v2alpha/wrapper/envoy/data/tap/v2alpha/wrapper.proto.rst
  /envoy/data/cluster/v2alpha/outlier_detection_event/envoy/data/cluster/v2alpha/outlier_detection_event.proto.rst
  /envoy/service/accesslog/v2/als/envoy/service/accesslog/v2/als.proto.rst
  /envoy/service/auth/v2/external_auth/envoy/service/auth/v2/attribute_context.proto.rst
  /envoy/service/auth/v2/external_auth/envoy/service/auth/v2/external_auth.proto.rst
  /envoy/service/discovery/v2/rtds/envoy/service/discovery/v2/rtds.proto.rst
  /envoy/service/ratelimit/v2/rls/envoy/service/ratelimit/v2/rls.proto.rst
  /envoy/service/tap/v2alpha/common/envoy/service/tap/v2alpha/common.proto.rst
  /envoy/type/http_status/envoy/type/http_status.proto.rst
  /envoy/type/percent/envoy/type/percent.proto.rst
  /envoy/type/range/envoy/type/range.proto.rst
  /envoy/type/matcher/metadata/envoy/type/matcher/metadata.proto.rst
  /envoy/type/matcher/value/envoy/type/matcher/value.proto.rst
  /envoy/type/matcher/number/envoy/type/matcher/number.proto.rst
  /envoy/type/matcher/string/envoy/type/matcher/string.proto.rst
  /envoy/config/filter/http/wasm/v2/wasm/envoy/config/filter/http/wasm/v2/wasm.proto.rst
  /envoy/config/wasm/v2/wasm/envoy/config/wasm/v2/wasm.proto.rst
"

# Dump all the generated RST so they can be added to PROTO_RST easily.
find -L bazel-bin/external/envoy_api -name "*.proto.rst"

# Only copy in the protos we care about and know how to deal with in protodoc.
for p in $PROTO_RST
do
  DEST="${GENERATED_RST_DIR}/api-v2/$(sed -e 's#/envoy\/.*/envoy/##' <<< "$p")"
  mkdir -p "$(dirname "${DEST}")"
  cp -f bazel-bin/external/envoy_api/"${p}" "$(dirname "${DEST}")"
  [ -n "${CPROFILE_ENABLED}" ] && cp -f bazel-bin/"${p}".profile "$(dirname "${DEST}")"
done

mkdir -p ${GENERATED_RST_DIR}/api-docs

cp -f $API_DIR/xds_protocol.rst "${GENERATED_RST_DIR}/api-docs/xds_protocol.rst"

rsync -rav  $API_DIR/diagrams "${GENERATED_RST_DIR}/api-docs"

rsync -av "${SCRIPT_DIR}"/root/ "${SCRIPT_DIR}"/conf.py "${GENERATED_RST_DIR}"

sphinx-build -W --keep-going -b html "${GENERATED_RST_DIR}" "${DOCS_OUTPUT_DIR}"
