Examples
--------

Below we will use YAML representation of the config protos and a running example
of a service proxying HTTP from 127.0.0.1:10000 to 127.0.0.2:1234.

Static
^^^^^^

A minimal fully static bootstrap config is provided below:

.. code-block:: yaml

  admin:
    access_log_path: /tmp/admin_access.log
    address:
      socket_address: { address: 127.0.0.1, port_value: 9901 }

  static_resources:
    listeners:
    - name: listener_0
      address:
        socket_address: { address: 127.0.0.1, port_value: 10000 }
      filter_chains:
      - filters:
        - name: envoy.http_connection_manager
          typed_config:
            "@type": type.googleapis.com/envoy.config.filter.network.http_connection_manager.v2.HttpConnectionManager
            stat_prefix: ingress_http
            codec_type: AUTO
            route_config:
              name: local_route
              virtual_hosts:
              - name: local_service
                domains: ["*"]
                routes:
                - match: { prefix: "/" }
                  route: { cluster: some_service }
            http_filters:
            - name: envoy.router
    clusters:
    - name: some_service
      connect_timeout: 0.25s
      type: STATIC
      lb_policy: ROUND_ROBIN
      load_assignment:
        cluster_name: some_service
        endpoints:
        - lb_endpoints:
          - endpoint:
              address:
                socket_address:
                  address: 127.0.0.1
                  port_value: 1234

Mostly static with dynamic EDS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A bootstrap config that continues from the above example with :ref:`dynamic endpoint
discovery <arch_overview_dynamic_config_eds>` via an
:ref:`EDS<envoy_api_file_envoy/api/v2/eds.proto>` gRPC management server listening
on 127.0.0.1:5678 is provided below:

.. code-block:: yaml

  admin:
    access_log_path: /tmp/admin_access.log
    address:
      socket_address: { address: 127.0.0.1, port_value: 9901 }

  static_resources:
    listeners:
    - name: listener_0
      address:
        socket_address: { address: 127.0.0.1, port_value: 10000 }
      filter_chains:
      - filters:
        - name: envoy.http_connection_manager
          typed_config:
            "@type": type.googleapis.com/envoy.config.filter.network.http_connection_manager.v2.HttpConnectionManager
            stat_prefix: ingress_http
            codec_type: AUTO
            route_config:
              name: local_route
              virtual_hosts:
              - name: local_service
                domains: ["*"]
                routes:
                - match: { prefix: "/" }
                  route: { cluster: some_service }
            http_filters:
            - name: envoy.router
    clusters:
    - name: some_service
      connect_timeout: 0.25s
      lb_policy: ROUND_ROBIN
      type: EDS
      eds_cluster_config:
        eds_config:
          api_config_source:
            api_type: GRPC
            grpc_services:
              envoy_grpc:
                cluster_name: xds_cluster
    - name: xds_cluster
      connect_timeout: 0.25s
      type: STATIC
      lb_policy: ROUND_ROBIN
      http2_protocol_options: {}
      upstream_connection_options:
        # configure a TCP keep-alive to detect and reconnect to the admin
        # server in the event of a TCP socket half open connection
        tcp_keepalive: {}
      load_assignment:
        cluster_name: xds_cluster
        endpoints:
        - lb_endpoints:
          - endpoint:
              address:
                socket_address:
                  address: 127.0.0.1
                  port_value: 5678

Notice above that *xds_cluster* is defined to point Envoy at the management server. Even in
an otherwise completely dynamic configurations, some static resources need to
be defined to point Envoy at its xDS management server(s).

It's important to set appropriate :ref:`TCP Keep-Alive options <envoy_api_msg_core.TcpKeepalive>`
in the `tcp_keepalive` block. This will help detect TCP half open connections to the xDS management
server and re-establish a full connection.

In the above example, the EDS management server could then return a proto encoding of a
:ref:`DiscoveryResponse <envoy_api_msg_DiscoveryResponse>`:

.. code-block:: yaml

  version_info: "0"
  resources:
  - "@type": type.googleapis.com/envoy.api.v2.ClusterLoadAssignment
    cluster_name: some_service
    endpoints:
    - lb_endpoints:
      - endpoint:
          address:
            socket_address:
              address: 127.0.0.2
              port_value: 1234


The versioning and type URL scheme that appear above are explained in more
detail in the :ref:`streaming gRPC subscription protocol
<xds_protocol_streaming_grpc_subscriptions>`
documentation.

Dynamic
^^^^^^^

A fully dynamic bootstrap configuration, in which all resources other than
those belonging to the management server are discovered via xDS is provided
below:

.. code-block:: yaml

  admin:
    access_log_path: /tmp/admin_access.log
    address:
      socket_address: { address: 127.0.0.1, port_value: 9901 }

  dynamic_resources:
    lds_config:
      api_config_source:
        api_type: GRPC
        grpc_services:
          envoy_grpc:
            cluster_name: xds_cluster
    cds_config:
      api_config_source:
        api_type: GRPC
        grpc_services:
          envoy_grpc:
            cluster_name: xds_cluster

  static_resources:
    clusters:
    - name: xds_cluster
      connect_timeout: 0.25s
      type: STATIC
      lb_policy: ROUND_ROBIN
      http2_protocol_options: {}
      upstream_connection_options:
        # configure a TCP keep-alive to detect and reconnect to the admin
        # server in the event of a TCP socket half open connection
        tcp_keepalive: {}
      load_assignment:
        cluster_name: xds_cluster
        endpoints:
        - lb_endpoints:
          - endpoint:
              address:
                socket_address:
                  address: 127.0.0.1
                  port_value: 5678

The management server could respond to LDS requests with:

.. code-block:: yaml

  version_info: "0"
  resources:
  - "@type": type.googleapis.com/envoy.api.v2.Listener
    name: listener_0
    address:
      socket_address:
        address: 127.0.0.1
        port_value: 10000
    filter_chains:
    - filters:
      - name: envoy.http_connection_manager
        typed_config:
          "@type": type.googleapis.com/envoy.config.filter.network.http_connection_manager.v2.HttpConnectionManager
          stat_prefix: ingress_http
          codec_type: AUTO
          rds:
            route_config_name: local_route
            config_source:
              api_config_source:
                api_type: GRPC
                grpc_services:
                  envoy_grpc:
                    cluster_name: xds_cluster
          http_filters:
          - name: envoy.router

The management server could respond to RDS requests with:

.. code-block:: yaml

  version_info: "0"
  resources:
  - "@type": type.googleapis.com/envoy.api.v2.RouteConfiguration
    name: local_route
    virtual_hosts:
    - name: local_service
      domains: ["*"]
      routes:
      - match: { prefix: "/" }
        route: { cluster: some_service }

The management server could respond to CDS requests with:

.. code-block:: yaml

  version_info: "0"
  resources:
  - "@type": type.googleapis.com/envoy.api.v2.Cluster
    name: some_service
    connect_timeout: 0.25s
    lb_policy: ROUND_ROBIN
    type: EDS
    eds_cluster_config:
      eds_config:
        api_config_source:
          api_type: GRPC
          grpc_services:
            envoy_grpc:
              cluster_name: xds_cluster

The management server could respond to EDS requests with:

.. code-block:: yaml

  version_info: "0"
  resources:
  - "@type": type.googleapis.com/envoy.api.v2.ClusterLoadAssignment
    cluster_name: some_service
    endpoints:
    - lb_endpoints:
      - endpoint:
          address:
            socket_address:
              address: 127.0.0.2
              port_value: 1234
