1.15.0 (Pending)
================


Incompatible Behavior Changes
-----------------------------
*Changes that are expected to cause an incompatibility if applicable; deployment changes are likely required*

* build: official released binary is now built on Ubuntu 18.04, requires glibc >= 2.27.

Minor Behavior Changes
----------------------
*Changes that may cause incompatibilities for some users, but should not for most*

* access loggers: applied existing buffer limits to access logs, as well as :ref:`stats <config_access_log_stats>` for logged / dropped logs. This can be reverted temporarily by setting runtime feature `envoy.reloadable_features.disallow_unbounded_access_logs` to false.
* http: fixed several bugs with applying correct connection close behavior across the http connection manager, health checker, and connection pool. This behavior may be temporarily reverted by setting runtime feature `envoy.reloadable_features.fix_connection_close` to false.
* http: fixed a bug where the upgrade header was not cleared on responses to non-upgrade requests.
  Can be reverted temporarily by setting runtime feature `envoy.reloadable_features.fix_upgrade_response` to false.
* http: stopped overwriting `date` response headers. Responses without a `date` header will still have the header properly set. This behavior can be temporarily reverted by setting `envoy.reloadable_features.preserve_upstream_date` to false.
* http: stopped adding a synthetic path to CONNECT requests, meaning unconfigured CONNECT requests will now return 404 instead of 403. This behavior can be temporarily reverted by setting `envoy.reloadable_features.stop_faking_paths` to false.
* router: allow retries of streaming or incomplete requests. This removes stat `rq_retry_skipped_request_not_complete`.
* router: allow retries by default when upstream responds with :ref:`x-envoy-overloaded <config_http_filters_router_x-envoy-overloaded_set>`.

Bug Fixes
---------
*Changes expected to improve the state of the world and are unlikely to have negative effects*

* http: fixed a bug in the grpc_http1_reverse_bridge filter where header-only requests were forwarded with a non-zero content length.
* http: fixed a bug where in some cases slash was moved from path to query string when :ref:`merging of adjacent slashes<envoy_api_field_config.filter.network.http_connection_manager.v2.HttpConnectionManager.merge_slashes>` is enabled.
* http: fixed several bugs with applying correct connection close behavior across the http connection manager, health checker, and connection pool. This behavior may be temporarily reverted by setting runtime feature `envoy.reloadable_features.fix_connection_close` to false.
* prometheus stats: fix the sort order of output lines to comply with the standard.
* upstream: fixed a bug where Envoy would panic when receiving a GRPC SERVICE_UNKNOWN status on the health check.

Removed Config or Runtime
-------------------------
*Normally occurs at the end of the* :ref:`deprecation period <deprecated>`

* http: remove legacy connection pool code and their runtime features: `envoy.reloadable_features.new_http1_connection_pool_behavior` and
  `envoy.reloadable_features.new_http2_connection_pool_behavior`.

New Features
------------

* access loggers: added GRPC_STATUS operator on logging format.
* access loggers: extened specifier for FilterStateFormatter to output :ref:`unstructured log string <config_access_log_format_filter_state>`.
* access loggers: file access logger config added :ref:`log_format <envoy_v3_api_field_extensions.access_loggers.file.v3.FileAccessLog.log_format>`.
* aggregate cluster: make route :ref:`retry_priority <envoy_v3_api_field_config.route.v3.RetryPolicy.retry_priority>` predicates work with :ref:`this cluster type <envoy_v3_api_msg_extensions.clusters.aggregate.v3.ClusterConfig>`.
* compressor: generic :ref:`compressor <config_http_filters_compressor>` filter exposed to users.
* config: added :ref:`version_text <config_cluster_manager_cds>` stat that reflects xDS version.
* decompressor: generic :ref:`decompressor <config_http_filters_decompressor>` filter exposed to users.
* dynamic forward proxy: added :ref:`SNI based dynamic forward proxy <config_network_filters_sni_dynamic_forward_proxy>` support.
* ext_authz filter: added :ref:`v2 deny_at_disable <envoy_api_field_config.filter.http.ext_authz.v2.ExtAuthz.deny_at_disable>`, :ref:`v3 deny_at_disable <envoy_v3_api_field_extensions.filters.http.ext_authz.v3.ExtAuthz.deny_at_disable>`. This allows to force deny for protected path while filter gets disabled, by setting this key to true.
* fault: added support for controlling the percentage of requests that abort, delay and response rate limits faults
  are applied to using :ref:`HTTP headers <config_http_filters_fault_injection_http_header>` to the HTTP fault filter.
* fault: added support for specifying grpc_status code in abort faults using
  :ref:`HTTP header <config_http_filters_fault_injection_http_header>` or abort fault configuration in HTTP fault filter.
* filter: add `upstram_rq_time` stats to the GPRC stats filter.
  Disabled by default and can be enabled via :ref:`enable_upstream_stats <envoy_v3_api_field_extensions.filters.http.grpc_stats.v3.FilterConfig.enable_upstream_stats>`.
* grpc: added support for Google gRPC :ref:`custom channel arguments <envoy_v3_api_field_config.core.v3.GrpcService.GoogleGrpc.channel_args>`.
* grpc-json: added support for streaming response using
  `google.api.HttpBody <https://github.com/googleapis/googleapis/blob/master/google/api/httpbody.proto>`_.
* grpc-json: send a `x-envoy-original-method` header to grpc services.
* gzip filter: added option to set zlib's next output buffer size.
* health checks: allow configuring health check transport sockets by specifying :ref:`transport socket match criteria <envoy_v3_api_field_config.core.v3.HealthCheck.transport_socket_match_criteria>`.
* http: added :ref:`local_reply config <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.local_reply_config>` to http_connection_manager to customize :ref:`local reply <config_http_conn_man_local_reply>`.
* http: added :ref:`stripping port from host header <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.strip_matching_host_port>` support.
* http: added support for proxying CONNECT requests, terminating CONNECT requests, and converting raw TCP streams into HTTP/2 CONNECT requests. See :ref:`upgrade documentation<arch_overview_upgrades>` for details.
* listener: added in place filter chain update flow for tcp listener update which doesn't close connections if the corresponding network filter chain is equivalent during the listener update.
  Can be disabled by setting runtime feature `envoy.reloadable_features.listener_in_place_filterchain_update` to false.
  Also added additional draining filter chain stat for :ref:`listener manager <config_listener_manager_stats>` to track the number of draining filter chains and the number of in place update attempts.
* logger: added :ref:`--log-format-prefix-with-location <operations_cli>` command line option to prefix '%v' with file path and line number.
* lrs: added new *envoy_api_field_service.load_stats.v2.LoadStatsResponse.send_all_clusters* field
  in LRS response, which allows management servers to avoid explicitly listing all clusters it is
  interested in; behavior is allowed based on new "envoy.lrs.supports_send_all_clusters" capability
  in :ref:`client_features<envoy_v3_api_field_config.core.v3.Node.client_features>` field.
* network filters: added a :ref:`postgres proxy filter <config_network_filters_postgres_proxy>`.
* network filters: added a :ref:`rocketmq proxy filter <config_network_filters_rocketmq_proxy>`.
* request_id: added to :ref:`always_set_request_id_in_response setting <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.always_set_request_id_in_response>`
  to set :ref:`x-request-id <config_http_conn_man_headers_x-request-id>` header in response even if
  tracing is not forced.
* router: add support for RESPONSE_FLAGS and RESPONSE_CODE_DETAILS :ref:`header formatters
  <config_http_conn_man_headers_custom_request_headers>`.
* router: allow Rate Limiting Service to be called in case of missing request header for a descriptor if the :ref:`skip_if_absent <envoy_v3_api_field_config.route.v3.RateLimit.Action.RequestHeaders.skip_if_absent>` field is set to true.
* router: more fine grained internal redirect configs are added to the :ref:`internal_redirect_policy
  <envoy_v3_api_field_config.route.v3.RouteAction.internal_redirect_policy>` field.
* runtime: add new gauge :ref:`deprecated_feature_seen_since_process_start <runtime_stats>` that gets reset across hot restarts.
* stats: added the option to :ref:`report counters as deltas <envoy_v3_api_field_config.metrics.v3.MetricsServiceConfig.report_counters_as_deltas>` to the metrics service stats sink.
* tracing: tracing configuration has been made fully dynamic and every HTTP connection manager
  can now have a separate :ref:`tracing provider <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.Tracing.provider>`.
* udp: :ref:`udp_proxy <config_udp_listener_filters_udp_proxy>` filter has been upgraded to v3 and is no longer considered alpha.

Deprecated
----------

* Tracing provider configuration as part of :ref:`bootstrap config <envoy_v3_api_field_config.bootstrap.v3.Bootstrap.tracing>`
  has been deprecated in favor of configuration as part of :ref:`HTTP connection manager
  <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.Tracing.provider>`.
* The :ref:`HTTP Gzip filter <config_http_filters_gzip>` has been deprecated in favor of
  :ref:`Compressor <config_http_filters_compressor>`.
* The * :ref:`GoogleRE2.max_program_size<envoy_v3_api_field_type.matcher.v3.RegexMatcher.GoogleRE2.max_program_size>`
  field is now deprecated. Management servers are expected to validate regexp program sizes
  instead of expecting the client to do it.
* The :ref:`internal_redirect_action <envoy_v3_api_field_config.route.v3.RouteAction.internal_redirect_action>`
  field and :ref:`max_internal_redirects <envoy_v3_api_field_config.route.v3.RouteAction.max_internal_redirects>` field
  are now deprecated. This changes the implemented default cross scheme redirect behavior.
  All cross scheme redirect are disallowed by default. To restore
  the previous behavior, set allow_cross_scheme_redirect=true and use
  :ref:`safe_cross_scheme<envoy_v3_api_msg_extensions.internal_redirect.safe_cross_scheme.v3.SafeCrossSchemeConfig>`,
  in :ref:`predicates <envoy_v3_api_field_config.route.v3.InternalRedirectPolicy.predicates>`.
* File access logger fields :ref:`format <envoy_v3_api_field_extensions.access_loggers.file.v3.FileAccessLog.format>`, :ref:`json_format <envoy_v3_api_field_extensions.access_loggers.file.v3.FileAccessLog.json_format>` and :ref:`typed_json_format <envoy_v3_api_field_extensions.access_loggers.file.v3.FileAccessLog.typed_json_format>` are deprecated in favor of :ref:`log_format <envoy_v3_api_field_extensions.access_loggers.file.v3.FileAccessLog.log_format>`.
* A warning is now logged when v2 xDS api is used. This behavior can be temporarily disabled by setting `envoy.reloadable_features.enable_deprecated_v2_api_warning` to `false`.
