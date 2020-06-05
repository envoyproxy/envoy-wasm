load(":envoy_http_archive.bzl", "envoy_http_archive")
load(":repository_locations.bzl", "REPOSITORY_LOCATIONS")

def api_dependencies():
    envoy_http_archive(
        "bazel_skylib",
        locations = REPOSITORY_LOCATIONS,
    )
    envoy_http_archive(
        "com_envoyproxy_protoc_gen_validate",
        locations = REPOSITORY_LOCATIONS,
    )
    envoy_http_archive(
        name = "com_google_googleapis",
        locations = REPOSITORY_LOCATIONS,
    )
    envoy_http_archive(
        name = "com_github_cncf_udpa",
        locations = REPOSITORY_LOCATIONS,
    )

    envoy_http_archive(
        name = "prometheus_metrics_model",
        locations = REPOSITORY_LOCATIONS,
        build_file_content = PROMETHEUSMETRICS_BUILD_CONTENT,
    )
    envoy_http_archive(
        name = "opencensus_proto",
        locations = REPOSITORY_LOCATIONS,
    )
    envoy_http_archive(
        name = "rules_proto",
        locations = REPOSITORY_LOCATIONS,
    )
    envoy_http_archive(
        name = "com_github_openzipkin_zipkinapi",
        locations = REPOSITORY_LOCATIONS,
        build_file_content = ZIPKINAPI_BUILD_CONTENT,
    )

PROMETHEUSMETRICS_BUILD_CONTENT = """
load("@envoy_api//bazel:api_build_system.bzl", "api_cc_py_proto_library")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

api_cc_py_proto_library(
    name = "client_model",
    srcs = [
        "metrics.proto",
    ],
    visibility = ["//visibility:public"],
)

go_proto_library(
    name = "client_model_go_proto",
    importpath = "github.com/prometheus/client_model/go",
    proto = ":client_model",
    visibility = ["//visibility:public"],
)
"""

OPENCENSUSTRACE_BUILD_CONTENT = """
load("@envoy_api//bazel:api_build_system.bzl", "api_cc_py_proto_library")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

api_cc_py_proto_library(
    name = "trace_model",
    srcs = [
        "trace.proto",
    ],
    visibility = ["//visibility:public"],
)

go_proto_library(
    name = "trace_model_go_proto",
    importpath = "trace_model",
    proto = ":trace_model",
    visibility = ["//visibility:public"],
)
"""

ZIPKINAPI_BUILD_CONTENT = """

load("@envoy_api//bazel:api_build_system.bzl", "api_cc_py_proto_library")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

api_cc_py_proto_library(
    name = "zipkin",
    srcs = [
        "zipkin-jsonv2.proto",
        "zipkin.proto",
    ],
    visibility = ["//visibility:public"],
)

go_proto_library(
    name = "zipkin_go_proto",
    proto = ":zipkin",
    visibility = ["//visibility:public"],
)
"""
