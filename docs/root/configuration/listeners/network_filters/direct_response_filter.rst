.. _config_network_filters_direct_response:

Direct response
===============

The direct response filter is a trivial network filter used to respond
immediately to new downstream connections with an optional canned response. It
can be used, for example, as a terminal filter in filter chains to collect
telemetry for blocked traffic. This filter should be configured with the name
*envoy.filters.network.direct_response*.

* :ref:`v3 API reference <envoy_v3_api_field_config.listener.v3.Filter.name>`
