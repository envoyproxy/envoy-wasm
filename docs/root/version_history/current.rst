1.16.0 (Pending)
================

Incompatible Behavior Changes
-----------------------------
*Changes that are expected to cause an incompatibility if applicable; deployment changes are likely required*

* build: added visibility rules for upstream. If these cause visibility related breakage, see notes in //BUILD.

Minor Behavior Changes
----------------------
*Changes that may cause incompatibilities for some users, but should not for most*

* compressor: always insert `Vary` headers for compressible resources even if it's decided not to compress a response due to incompatible `Accept-Encoding` value. The `Vary` header needs to be inserted to let a caching proxy in front of Envoy know that the requested resource still can be served with compression applied.
* decompressor: headers-only requests were incorrectly not advertising accept-encoding when configured to do so. This is now fixed.
* http: added :ref:`headers_to_add <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.ResponseMapper.headers_to_add>` to :ref:`local reply mapper <config_http_conn_man_local_reply>` to allow its users to add/append/override response HTTP headers to local replies.
* http: added HCM level configuration of :ref:`error handling on invalid messaging <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.stream_error_on_invalid_http_message>` which substantially changes Envoy's behavior when encountering invalid HTTP/1.1 defaulting to closing the connection instead of allowing reuse. This can temporarily be reverted by setting `envoy.reloadable_features.hcm_stream_error_on_invalid_message` to false, or permanently reverted by setting the :ref:`HCM option <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.HttpConnectionManager.stream_error_on_invalid_http_message>` to true to restore prior HTTP/1.1 beavior and setting the *new* HTTP/2 configuration :ref:`override_stream_error_on_invalid_http_message <envoy_v3_api_field_config.core.v3.Http2ProtocolOptions.override_stream_error_on_invalid_http_message>` to false to retain prior HTTP/2 behavior.
* http: changed Envoy to send error headers and body when possible. This behavior may be temporarily reverted by setting `envoy.reloadable_features.allow_response_for_timeout` to false.
* http: changed empty trailers encoding behavior by sending empty data with ``end_stream`` true (instead of sending empty trailers) for HTTP/2. This behavior can be reverted temporarily by setting runtime feature ``envoy.reloadable_features.http2_skip_encoding_empty_trailers`` to false.
* http: clarified and enforced 1xx handling. Multiple 100-continue headers are coalesced when proxying. 1xx headers other than {100, 101} are dropped.
* http: fixed the 100-continue response path to properly handle upstream failure by sending 5xx responses. This behavior can be temporarily reverted by setting `envoy.reloadable_features.allow_500_after_100` to false.
* http: the per-stream FilterState maintained by the HTTP connection manager will now provide read/write access to the downstream connection FilterState. As such, code that relies on interacting with this might
  see a change in behavior.
* logging: change default log format to `"[%Y-%m-%d %T.%e][%t][%l][%n] [%g:%#] %v"` and default value of :option:`--log-format-prefix-with-location` to `0`.
* logging: nghttp2 log messages no longer appear at trace level unless `ENVOY_NGHTTP2_TRACE` is set
  in the environment.
* router: added transport failure reason to response body when upstream reset happens. After this change, the response body will be of the form `upstream connect error or disconnect/reset before headers. reset reason:{}, transport failure reason:{}`.This behavior may be reverted by setting runtime feature `envoy.reloadable_features.http_transport_failure_reason_in_body` to false.
* router: now consumes all retry related headers to prevent them from being propagated to the upstream. This behavior may be reverted by setting runtime feature `envoy.reloadable_features.consume_all_retry_headers` to false.
* thrift_proxy: special characters {'\0', '\r', '\n'} will be stripped from thrift headers.

Bug Fixes
---------
*Changes expected to improve the state of the world and are unlikely to have negative effects*

* csrf: fixed issues with regards to origin and host header parsing.
* dynamic_forward_proxy: only perform DNS lookups for routes to Dynamic Forward Proxy clusters since other cluster types handle DNS lookup themselves.
* fault: fixed an issue with `active_faults` gauge not being decremented for when abort faults were injected.
* grpc-web: fixed an issue with failing HTTP/2 requests on some browsers. Notably, WebKit-based browsers (https://bugs.webkit.org/show_bug.cgi?id=210108), Internet Explorer 11, and Edge (pre-Chromium).
* rocketmq_proxy network-level filter: fixed an issue involving incorrect header lengths. In debug mode it causes crash and in release mode it causes underflow.

Removed Config or Runtime
-------------------------
*Normally occurs at the end of the* :ref:`deprecation period <deprecated>`

* http: removed legacy header sanitization and the runtime guard `envoy.reloadable_features.strict_header_validation`.
* http: removed legacy transfer-encoding enforcement and runtime guard `envoy.reloadable_features.reject_unsupported_transfer_encodings`.
* http: removed configurable strict host validation and runtime guard `envoy.reloadable_features.strict_authority_validation`.

New Features
------------
* access log: added a :ref:`dynamic metadata filter<envoy_v3_api_msg_config.accesslog.v3.MetadataFilter>` for access logs, which filters whether to log based on matching dynamic metadata.
* access log: added support for :ref:`%DOWNSTREAM_PEER_FINGERPRINT_1% <config_access_log_format_response_flags>` as a response flag.
* build: enable building envoy :ref:`arm64 images <arm_binaries>` by buildx tool in x86 CI platform.
* dynamic_forward_proxy: added :ref:`use_tcp_for_dns_lookups<envoy_v3_api_field_extensions.common.dynamic_forward_proxy.v3.DnsCacheConfig.use_tcp_for_dns_lookups>` option to use TCP for DNS lookups in order to match the DNS options for :ref:`Clusters<envoy_v3_api_msg_config.cluster.v3.Cluster>`.
* ext_authz filter: added support for emitting dynamic metadata for both :ref:`HTTP <config_http_filters_ext_authz_dynamic_metadata>` and :ref:`network <config_network_filters_ext_authz_dynamic_metadata>` filters.
* grpc-json: support specifying `response_body` field in for `google.api.HttpBody` message.
* http: added support for :ref:`%DOWNSTREAM_PEER_FINGERPRINT_1% <config_http_conn_man_headers_custom_request_headers>` as custom header.
* http: introduced new HTTP/1 and HTTP/2 codec implementations that will remove the use of exceptions for control flow due to high risk factors and instead use error statuses. The old behavior is used by default, but the new codecs can be enabled for testing by setting the runtime feature `envoy.reloadable_features.new_codec_behavior` to true. The new codecs will be in development for one month, and then enabled by default while the old codecs are deprecated.
* load balancer: added a :ref:`configuration<envoy_v3_api_msg_config.cluster.v3.Cluster.LeastRequestLbConfig>` option to specify the active request bias used by the least request load balancer.
* lua: added Lua APIs to access :ref:`SSL connection info <config_http_filters_lua_ssl_socket_info>` object.
* postgres network filter: :ref:`metadata <config_network_filters_postgres_proxy_dynamic_metadata>` is produced based on SQL query.
* ratelimit: added :ref:`enable_x_ratelimit_headers <envoy_v3_api_msg_extensions.filters.http.ratelimit.v3.RateLimit>` option to enable `X-RateLimit-*` headers as defined in `draft RFC <https://tools.ietf.org/id/draft-polli-ratelimit-headers-03.html>`_.
* rbac filter: added a log action to the :ref:`RBAC filter <envoy_v3_api_msg_config.rbac.v3.RBAC>` which sets dynamic metadata to inform access loggers whether to log.
* router: added new
  :ref:`envoy-ratelimited<config_http_filters_router_retry_policy-envoy-ratelimited>`
  retry policy, which allows retrying envoy's own rate limited responses.
* router: added new :ref:`host_rewrite_path_regex <envoy_v3_api_field_config.route.v3.RouteAction.host_rewrite_path_regex>`
  option, which allows rewriting Host header based on path.
* signal: added support for calling fatal error handlers without envoy's signal handler, via FatalErrorHandler::callFatalErrorHandlers().
* stats: added optional histograms to :ref:`cluster stats <config_cluster_manager_cluster_stats_request_response_sizes>`
  that track headers and body sizes of requests and responses.
* stats: allow configuring histogram buckets for stats sinks and admin endpoints that support it.
* tap: added :ref:`generic body matcher<envoy_v3_api_msg_config.tap.v3.HttpGenericBodyMatch>` to scan http requests and responses for text or hex patterns.
* tcp: switched the TCP connection pool to the new "shared" connection pool, sharing a common code base with HTTP and HTTP/2. Any unexpected behavioral changes can be temporarily reverted by setting `envoy.reloadable_features.new_tcp_connection_pool` to false.
* watchdog: support randomizing the watchdog's kill timeout to prevent synchronized kills via a maximium jitter parameter :ref:`max_kill_timeout_jitter<envoy_v3_api_field_config.bootstrap.v3.Watchdog.max_kill_timeout_jitter>`.
* xds: added :ref:`extension config discovery<envoy_v3_api_msg_config.core.v3.ExtensionConfigSource>` support for HTTP filters.

Deprecated
----------
* The :ref:`track_timeout_budgets <envoy_v3_api_field_config.cluster.v3.Cluster.track_timeout_budgets>`
  field has been deprecated in favor of `timeout_budgets` part of an :ref:`Optional Configuration <envoy_v3_api_field_config.cluster.v3.Cluster.track_cluster_stats>`.
* tap: the :ref:`match_config <envoy_v3_api_field_config.tap.v3.TapConfig.match_config>` field has been deprecated in favor of
  :ref:`match <envoy_v3_api_field_config.tap.v3.TapConfig.match>` field.
