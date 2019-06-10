.. _config_http_filters_wasm:

WebAssembly
===========

Overview
--------

TODO: add text about high level design principal and feature set.

This `video talk <https://youtu.be/XdWmm_mtVXI>`_ is a great introduction about architecture of WebAssembly integration.

Configuration
-------------

* :ref:`v2 API reference <envoy_api_msg_config.filter.http.wasm.v2.Wasm>`
* This filter should be configured with name *envoy.wasm*.

Example
-------

An example C++ WASM filter could be found `here <https://github.com/envoyproxy/envoy-wasm/tree/19b9fd9a22e27fcadf61a06bf6aac03b735418e6/examples/wasm>`_.

To implement a WASM filter:

* Create a `filter context class <https://github.com/envoyproxy/envoy-wasm/blob/19b9fd9a22e27fcadf61a06bf6aac03b735418e6/examples/wasm/envoy_filter_http_wasm_example.cc#L7>`_ which inherits the `base context class <https://github.com/envoyproxy/envoy-wasm/blob/19b9fd9a22e27fcadf61a06bf6aac03b735418e6/api/wasm/cpp/proxy_wasm_impl.h#L225>`_.
* Override :ref:`context API <config_http_filters_wasm_context_object_api>` methods to handle corresponding initialization and stream events from host.
* Implement a `NewContext method <https://github.com/envoyproxy/envoy-wasm/blob/19b9fd9a22e27fcadf61a06bf6aac03b735418e6/examples/wasm/envoy_filter_http_wasm_example.cc#L22>`_ for the construction of filter :ref:`context object <config_http_filters_wasm_context_object>`.

.. _config_http_filters_wasm_context_object:

Context object
--------------

WASM module is running in a stack-based virtual machine and its memory is isolated from the host environment. 
All interactions between host and WASM module are through functions and callbacks wrapped by context object. 

At bootstrap time, a root context with id 0 is created. 
The root context has the same lifetime as the VM/runtime instance and acts as a target for any interactions which happen at initial setup.
It is also used for interactions that outlive a request. 

At request time, a context with incremental id from 1 is created for each stream.
Stream context has the same lifetime as the stream itself and acts as a target for interactions that are local to that stream.

.. image:: /_static/wasm_context.svg
  :width: 100%

.. _config_http_filters_wasm_context_object_api:

Context object API
------------------

onConfigure
^^^^^^^^^^^

.. code-block:: cpp

    void onConfigure(std::unique_ptr<WasmData> configuration)

Called when host loads the WASM module. *configuration* is passed in using :ref:`WasmData <config_http_filters_wasm_WasmData>`.
If the VM that the module running in has not been configured, `onConfigure` is called first with :ref:`VM config <envoy_api_field_config.wasm.v2.VmConfig.initial_configuration>`,
then a second call will be invoked to pass in :ref:`module config <envoy_api_field_config.wasm.v2.WasmConfig.configuration>`.
*onConfigure* will only be called in :ref:`root context <config_http_filters_wasm_context_object>`.

If :ref:`VM is shared <config_http_filters_wasm_vm_sharing>` by multiple modules and has already been configured via other WASM filter in the chain, `onConfigure` will only be called once with module config. 

onStart
^^^^^^^

.. code-block:: cpp

    void onStart()

Called after finishing loading WASM module and before serving any stream events.
*onStart* will only be called in :ref:`root context <config_http_filters_wasm_context_object>`.

The following methods are called in order during the lifetime of a stream.

onCreate
^^^^^^^^

.. code-block:: cpp

    void onCreate()

Called at the beginning of filter chain iteration. Indicates creation of the new stream context.

.. _config_http_filters_wasm_context_object_api_onrequestheaders:

onRequestHeaders
^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void onRequestHeaders()

Called when headers are decoded. Request Headers could be fetched and manipulated by :ref:`request header API <config_http_filters_wasm_request_header_api>`.

Returns `FilterHeadersStatus <https://github.com/envoyproxy/envoy/blob/5d3214d4d8e1d77937f0f1278d3ac816d9a3d888/include/envoy/http/filter.h#L27>`_ 
to determine how filter chain iteration proceeds.

.. _config_http_filters_wasm_context_object_api_onrequestbody:

onRequestBody
^^^^^^^^^^^^^

.. code-block:: cpp
   
    FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) 

Called when request body is decoded. *body_buffer_length* is used to indicate size of decoded request body. 
*end_of_stream* indicates if this is the last data frame. Request body could be fetched by :ref:`body API <config_http_filters_wasm_body_api>`.

Returns `FilterDataStatus <https://github.com/envoyproxy/envoy/blob/5d3214d4d8e1d77937f0f1278d3ac816d9a3d888/include/envoy/http/filter.h#L66>`_
to determine how filter chain iteration proceeds.

.. _config_http_filters_wasm_context_object_api_onrequesttrailers:

onRequestTrailers
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    FilterTrailersStatus onRequestTrailers()

Called when request trailers are decoded. Request trailers could be fetched and manipulated by :ref:`request trailer API <config_http_filters_wasm_response_trailer_api>`.

Returns `FilterTrailerStatus <https://github.com/envoyproxy/envoy/blob/5d3214d4d8e1d77937f0f1278d3ac816d9a3d888/include/envoy/http/filter.h#L104>`_
to determine how filter chain iteration proceeds.

.. _config_http_filters_wasm_context_object_api_onresponseheaders:

onResponseHeaders
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void onResponseHeaders()

Called when headers are decoded. Response headers could be fetched and manipulated by :ref:`response header API <config_http_filters_wasm_response_header_api>`.

Returns `FilterHeadersStatus <https://github.com/envoyproxy/envoy/blob/5d3214d4d8e1d77937f0f1278d3ac816d9a3d888/include/envoy/http/filter.h#L27>`_
to determine how filter chain iteration proceeds.

.. _config_http_filters_wasm_context_object_api_onresponsebody:

onResponseBody
^^^^^^^^^^^^^^

.. code-block:: cpp
   
    FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) 

Called when response body is decoded. *body_buffer_length* is used to indicate size of decoded response body.
*end_of_stream* indicates if this is the last data frame.
Response body could be fetched by :ref:`body API <config_http_filters_wasm_body_api>`.

Returns `FilterDataStatus <https://github.com/envoyproxy/envoy/blob/5d3214d4d8e1d77937f0f1278d3ac816d9a3d888/include/envoy/http/filter.h#L66>`_
to determine how filter chain iteration proceeds.

.. _config_http_filters_wasm_context_object_api_onresponsetrailers:

onResponseTrailers
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    FilterTrailersStatus onResponseTrailers()

Called when response trailers are decoded. Response trailers could be fetched and manipulated :ref:`response trailer API <config_http_filters_wasm_response_trailer_api>`.

Returns FilterTrailerStatus `FilterTrailerStatus <https://github.com/envoyproxy/envoy/blob/5d3214d4d8e1d77937f0f1278d3ac816d9a3d888/include/envoy/http/filter.h#L104>`_
to determine how filter chain iteration proceeds.

onDone
^^^^^^

.. code-block:: cpp

    void onDone()

Called after stream is ended or reset. All stream info will not be changed any more and is safe for access logging.

.. _config_http_filters_wasm_context_object_api_onlog:

onLog
^^^^^

.. code-block:: cpp

    void onLog()

Called to log any stream info. Several types of stream info are available from API: 
Request headers could be fetched by :ref:`request header API <config_http_filters_wasm_request_header_api>`.
Response headers could be fetched by :ref:`response header API <config_http_filters_wasm_response_header_api>`.
Response trailers could be fetched by :ref:`response trailer API <config_http_filters_wasm_response_trailer_api>`.
Streaminfo could be fetched by :ref:`streaminfo API <config_http_filters_wasm_streaminfo_api>`.

onDelete
^^^^^^^^

.. code-block:: cpp

    void onDelete()

Called after logging is done. This call indicates no more handler will be called on the stream context and it is up for deconstruction, 
The stream context needs to make sure all async events are cleaned up, such as network calls, timers.

Context object is also used to handle timer event.

.. _config_http_filters_wasm_context_object_api_ontick:

onTick
^^^^^^

.. code-block:: cpp

    void onTick()

Called when a timer is set and fired. Timer could be set by :ref:`setTickPeriodMilliseconds <config_http_filters_wasm_setTickPeriodMilliseconds>`.

The following methods on context object are supported.

httpCall
^^^^^^^^

.. code-block:: cpp

    void httpCall(StringView cluster, 
                  const HeaderStringPairs& request_headers,
                  StringView request_body, 
                  const HeaderStringPairs& request_trailers,
                  uint32_t timeout_milliseconds,
                  HttpCallCallback callback)

Makes an HTTP call to an upstream host. 

*cluster* is a string which maps to a configured cluster manager cluster. 
*request_headers* is a vector of key/value pairs to send. Note that the *:method*, *:path*, and *:authority* headers must be set. 
*request_body* is an optional string of body data to send. timeout is an integer that specifies the call timeout in milliseconds. 
*timeout_milliseconds* is an unsigned integer as timeout period for the http call in milliseconds.
*callback* is the callback function to be called when the HTTP request finishes.

.. note::
    If the call outlives the stream context, *httpCall* should be called within :ref:`root context <config_http_filters_wasm_context_object>`.

.. _config_http_filters_wasm_context_object_api_grpcSimpleCall:

grpcSimpleCall
^^^^^^^^^^^^^^

.. code-block:: cpp

    template<typename Response> 
    void grpcSimpleCall(StringView service, 
                        StringView service_name,
                        StringView method_name, 
                        const google::protobuf::MessageLite &request, 
                        uint32_t timeout_milliseconds,
                        std::function<void(Response&& response)> success_callback,
                        std::function<void(GrpcStatus status, StringView error_message)> failure_callback)

Makes a unary gRPC call to an upstream host.

*service* is a serialized proto string of :ref:`gRPC service <envoy_api_msg_core.GrpcService>` for gRPC client initialization.
*service_name* and *method_name* indicates the target gRPC service and method name.
*request* is a `lite proto message <https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.message_lite>`_ that gRPC service accepts as request.
*timeout_milliseconds* is an unsigned integer as timeout period for the gRPC call in milliseconds.
*success_callback* is the callback function that will be called when gRPC call succeeds. *response* is the returned message from gRPC service.
*failure_callback* is the callback function that will be invoked when gRPC call fails. *status* is the returned gRPC status code. *error_message* is detailed error message extracted from gRPC response.

.. note::
    if the call outlives the stream context, *grpcSimpleCall* should be called within :ref:`root context <config_http_filters_wasm_context_object>`.

grpcCallHandler
^^^^^^^^^^^^^^^

.. code-block:: cpp

    void grpcCallHandler(
        StringView service,
        StringView service_name,
        StringView method_name,
        const google::protobuf::MessageLite &request,
        uint32_t timeout_milliseconds,
        std::unique_ptr<GrpcCallHandlerBase> handler)

Makes a unary gRPC call to an upstream host.

Similar to :ref:`grpcSimpleCall <config_http_filters_wasm_context_object_api_grpcSimpleCall>` for gRPC client initialization,
but uses :ref:`GrpcCallHandler <config_http_filters_wasm_GrpcCallHandler>` as target for callback and fine grained control on the call.

grpcStreamHandler
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void grpcStreamHandler(StringView service,
                           StringView service_name,
                           StringView method_name,
                           std::unique_ptr<GrpcStreamHandlerBase> handler)

Makes an gRPC stream to an upstream host.

*service* is a serialized proto string of :ref:`gRPC service <envoy_api_msg_core.GrpcService>` for gRPC client initialization.
*service_name* and *method_name* indicates the target gRPC service and method name.
*handler* (:ref:`GrpcStreamHandler <config_http_filters_wasm_GrpcStreamHandler>`) is used to control the stream and
as target for gRPC stream callbacks.

.. note::
    if the stream call outlives the per request context, *grpcStreamHandler* should be called within :ref:`root context <config_http_filters_wasm_context_object>`.

Application log API
-------------------

log*
^^^^

.. code-block:: cpp

    void LogTrace(const std::string& logMessage)
    void LogDebug(const std::string& logMessage)
    void LogInfo(const std::string& logMessage)
    void LogWarn(const std::string& logMessage)
    void LogError(const std::string& logMessage)
    void LogCritical(const std::string& logMessage)

Logs a message using Envoy's application logging. *logMessage* is a string to log.

.. _config_http_filters_wasm_header_api:

Header API
----------

.. _config_http_filters_wasm_request_header_api:

addRequestHeader
^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void addRequestHeader(StringView key, StringView value)

Adds a new request header with the key and value if header does not exist, or append the value if header exists.
This method is effective only when called in :ref:`onRequestHeader <config_http_filters_wasm_context_object_api_onrequestheaders>`.

replaceRequestHeader
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void replaceRequestHeader(StringView key, StringView value)

Replaces the value of an existing request header with the given key, or create a new request header with the key and value if not existing.
This method is effective only when called in :ref:`onRequestHeader <config_http_filters_wasm_context_object_api_onrequestheaders>`.

removeRequestHeader
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void removeRequestHeader(StringView key)

Removes request header with the given key. No-op if the request header does not exist.
This method is effective only when called in :ref:`onRequestHeader <config_http_filters_wasm_context_object_api_onrequestheaders>`.

setRequestHeaderPairs
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void setRequestHeaderPairs(const HeaderStringPairs &pairs)

Sets request headers with the given header pairs. For each header key value pair, it acts the same way as replaceRequestHeader.
This method is effective only when called in :ref:`onRequestHeader <config_http_filters_wasm_context_object_api_onrequestheaders>`.

getRequestHeader
^^^^^^^^^^^^^^^^

.. code-block:: cpp

    WasmDataPtr getRequestHeader(StringView key)

Gets value of header with the given key. Returns empty string if header does not exist. 
This method is effective only when called in :ref:`onRequestHeader <config_http_filters_wasm_context_object_api_onrequestheaders>` and
:ref:`onLog <config_http_filters_wasm_context_object_api_onlog>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which contains the header value data.

getRequestHeaderPairs
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    WasmDataPtr getRequestHeaderPairs()

Gets all header pairs. This method is effective only when called in :ref:`onRequestHeader <config_http_filters_wasm_context_object_api_onrequestheaders>` and
:ref:`onLog <config_http_filters_wasm_context_object_api_onlog>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which contains header pairs data.

.. _config_http_filters_wasm_response_header_api:

addResponseHeader
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void addResponseHeader(StringView key, StringView value)

Adds a new response header with the key and value if header does not exist, or append the value if header exists.
This method is effective only when called in :ref:`onResponseHeader <config_http_filters_wasm_context_object_api_onresponseheaders>`.

replaceResponseHeader
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void replaceResponseHeader(StringView key, StringView value)

Replaces the value of an existing response header with the given key, or create a new response header with the key and value if not existing.
This method is effective only when called in :ref:`onResponseHeader <config_http_filters_wasm_context_object_api_onresponseheaders>`.

removeResponseHeader
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void removeResponseHeader(StringView key)

Removes response header with the given key. No-op if the response header does not exist.
This method is effective only when called in :ref:`onResponseHeader <config_http_filters_wasm_context_object_api_onresponseheaders>`.

setResponseHeaderPairs
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void setResponseHeaderPairs(const HeaderStringPairs &pairs)

Sets response headers with the given header pairs. For each header key value pair, it acts the same way as replaceResponseHeader.
This method is effective only when called in :ref:`onResponseHeader <config_http_filters_wasm_context_object_api_onresponseheaders>`.

getResponseHeader
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   WasmDataPtr getResponseHeader(StringView key)

Gets value of header with the given key. Returns empty string if header does not exist.
This method is effective only when called in :ref:`onResponseHeader <config_http_filters_wasm_context_object_api_onresponseheaders>` and
:ref:`onLog <config_http_filters_wasm_context_object_api_onlog>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the header value.

getResponseHeaderPairs
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   WasmDataPtr getResponseHeaderPairs()

Gets all header pairs. This method is effective only when called in :ref:`onResponseHeader <config_http_filters_wasm_context_object_api_onresponseheaders>` and
:ref:`onLog <config_http_filters_wasm_context_object_api_onlog>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the header pairs.

.. _config_http_filters_wasm_response_trailer_api:

addRequestTrailer
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void addRequestTrailer(StringView key, StringView value)

Adds a new request trailer with the key and value if trailer does not exist, or append the value if trailer exists.
This method is effective only when called in :ref:`onRequestTrailers <config_http_filters_wasm_context_object_api_onrequesttrailers>`.

replaceRequestTrailer
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void replaceRequestTrailer(StringView key, StringView value)

Replaces the value of an existing request trailer with the given key, or create a new request trailer with the key and value if not existing.
This method is effective only when called in :ref:`onRequestTrailers <config_http_filters_wasm_context_object_api_onrequesttrailers>`.

removeRequestTrailer
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void removeRequestTrailer(StringView key)

Removes request trailer with the given key. No-op if the request trailer does not exist.
This method is effective only when called in :ref:`onRequestTrailers <config_http_filters_wasm_context_object_api_onrequesttrailers>`.

setRequestTrailerPairs
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    void setRequestTrailerPairs(const HeaderStringPairs &pairs)

Sets request trailers with the given trailer pairs. For each trailer key value pair,it acts the same way as replaceRequestHeader.
This method is effective only when called in :ref:`onRequestTrailers <config_http_filters_wasm_context_object_api_onrequesttrailers>`.

getRequestTrailer
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    WasmDataPtr getRequestTrailer(StringView key)

Gets value of trailer with the given key. Returns empty string if trailer does not exist.
This method is effective only when called in :ref:`onRequestTrailers <config_http_filters_wasm_context_object_api_onrequesttrailers>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the trailer value.

getRequestTrailerPairs
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    WasmDataPtr getRequestTrailerPairs()

Gets all trailer pairs. This method is effective only when called in :ref:`onRequestTrailers <config_http_filters_wasm_context_object_api_onrequesttrailers>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the trailer pairs.

addResponseTrailer
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void addResponseTrailer(StringView key, StringView value)

Adds a new response trailer with the key and value if trailer does not exist, or append the value if trailer exists.
This method is effective only when called in :ref:`onResponseTrailer <config_http_filters_wasm_context_object_api_onresponsetrailers>`.

replaceResponseTrailer
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void replaceResponseTrailer(StringView key, StringView value)

Replaces the value of an existing response trailer with the given key, or create a new response trailer with the key and value if not existing.
This method is effective only when called in :ref:`onResponseTrailer <config_http_filters_wasm_context_object_api_onresponsetrailers>`.

removeResponseTrailer
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void removeResponseTrailer(StringView key)

Removes response trailer with the given key. No-op if the response trailer does not exist.
This method is effective only when called in :ref:`onResponseTrailer <config_http_filters_wasm_context_object_api_onresponsetrailers>`.

setResponseTrailerPairs
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void setResponseTrailerPairs(const TrailerStringPairs &pairs)

Sets response trailers with the given trailer pairs. For each trailer key value pair, it acts the same way as replaceResponseTrailer.
This method is effective only when called in :ref:`onResponseTrailer <config_http_filters_wasm_context_object_api_onresponsetrailers>`.

getResponseTrailer
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   WasmDataPtr getResponseTrailer(StringView key)

Gets value of trailer with the given key. Returns empty string if trailer does not exist.
This method is effective only when called in :ref:`onResponseTrailer <config_http_filters_wasm_context_object_api_onresponsetrailers>` and
:ref:`onLog <config_http_filters_wasm_context_object_api_onlog>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the trailer value.

getResponseTrailerPairs
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   WasmDataPtr getResponseTrailerPairs()

Gets all trailer pairs. This method is effective only when called in :ref:`onResponseTrailer <config_http_filters_wasm_context_object_api_onresponsetrailers>` and
:ref:`onLog <config_http_filters_wasm_context_object_api_onlog>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the trailer pairs.

.. _config_http_filters_wasm_body_api:

Body API
--------

getRequestBodyBufferBytes
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   WasmDataPtr getRequestBodyBufferBytes(size_t start, size_t length)

Returns buffered request body. This copies segment of request body. *start* is an integer and supplies the body buffer start index to copy. 
*length* is an integer and supplies the buffer length to copy. This method is effective when calling from :ref:`onRequestBody <config_http_filters_wasm_context_object_api_onrequestbody>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the request body data.

getResponseBodyBufferBytes
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   WasmDataPtr getResponseBodyBufferBytes(size_t start, size_t length)

Returns buffered response body. This copies segment of response body. *start* is an integer and supplies the body buffer start index to copy.
*length* is an integer and supplies the buffer length to copy. This method is effective when calling from :ref:`onResponseBody <config_http_filters_wasm_context_object_api_onresponsebody>`.

Returns :ref:`WasmData <config_http_filters_wasm_WasmData>` pointer which holds the response body data.

Metadata API
------------

TODO: Add metadata related API

.. _config_http_filters_wasm_streaminfo_api:

StreamInfo API
--------------

getProtocol
^^^^^^^^^^^

.. code-block:: cpp

   WasmDataPtr getProtocol(StreamType type)

Returns the string representation of HTTP protocol used by the current request.
The possible values are: HTTP/1.0, HTTP/1.1, and HTTP/2.
*type* is the stream type with two possible values: StreamType::Request and StreamType::Response.
The string protocol is returned as :ref:`WasmData <config_http_filters_wasm_WasmData>`.

Timer API
---------

Timer API is used to set a timer and get current timestamp.

.. _config_http_filters_wasm_setTickPeriodMilliseconds:

setTickPeriodMilliseconds
^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: cpp

    void setTickPeriodMilliseconds(uint32_t millisecond)

Set a timer. *millisecond* is tick interval in millisecond. :ref:`onTick <config_http_filters_wasm_context_object_api_ontick>` will be invoked when timer fires.

.. note::
    Only one timer could be set for each WASM module, so it needs to be multiplexed by events with different tick intervals.

getCurrentTimeNanoseconds
^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: cpp

    uint64 getCurrentTimeNanoseconds()

Returns timestamp of now in nanosecond precision.

Stats API
----------

The following objects are supported to export stats from WASM module to host stats sink.

.. _config_http_filters_wasm_Counter:

Counter
^^^^^^^

New
~~~

.. code-block:: cpp
    
    static Counter<Tags...>* New(StringView name, MetricTagDescriptor<Tags>... fieldnames)

Create a new counter with the given metric name and tag names. Example code to create a counter metric:

.. code-block:: cpp

    auto c = Counter<std::string, int, bool>::New(
                 "test_counter", "string_tag", "int_tag", "bool_tag");

Returns a pointer to counter object.

increment
^^^^^^^^^

.. code-block:: cpp

    void increment(int64_t offset, Tags... tags)

Increments a counter. *offset* is the value the counter incremented by.
*tags* is a list of tag values to identify a specific counter.
Example code to increment the aforementioned counter:

.. code-block:: cpp

    c->increment(1, "test_tag", 7, true)

get
^^^

.. code-block:: cpp

    uint64_t get(Tags... tags)

Returns value of a counter. *tags* is a list of tag values to identify a specific counter. 
Example code to get value of a counter:

.. code-block:: cpp

    c->get("test_tag", 7, true);

resolve
^^^^^^^

.. code-block:: cpp

    SimpleCounter resolve(Tags... f)

Resolves counter object to a specific counter for a list of tag values.

Returns a :ref:`SimpleCounter <config_http_filters_wasm_SimpleCounter>`
resolved from the counter object, so that tag values do not need to be specified
in every increment call. Example code:

.. code-block:: cpp

    auto simple_counter = c->resolve("test_tag", 7, true);

.. _config_http_filters_wasm_SimpleCounter:

SimpleCounter
^^^^^^^^^^^^^

*SimpleCounter* is resolved from a :ref:`Counter <config_http_filters_wasm_Counter>` object with predetermined tag values.

increment
^^^^^^^^^

.. code-block:: cpp

    void increment(int64_t offset)

Increment a counter. *offset* is the value counter incremented by. 

get
^^^

.. code-block:: cpp

    uint64_t get()

Returns current value of a counter.

.. _config_http_filters_wasm_Gauge:

Gauge
^^^^^

New
~~~

.. code-block:: cpp
    
    static Gauge<Tags...>* New(StringView name, MetricTagDescriptor<Tags>... fieldnames)

Create a new gauge with the given metric name and tag names. Example code to create a gauge metric:

.. code-block:: cpp

    auto c = Gauge<std::string, int, bool>::New(
                 "test_gauge", "string_tag", "int_tag", "bool_tag");

Returns a pointer to Gauge object.

record
^^^^^^

.. code-block:: cpp

    void record(int64_t offset, Tags... tags)

Records current value of a gauge. *offset* is the value to set for current gauge.
*tags* is a list of tag values to identify a specific gauge.
Example code to record value of a gauge metric:

.. code-block:: cpp

    c->record(1, "test_tag", 7, true)

get
^^^

.. code-block:: cpp

    uint64_t get(Tags... tags)

Returns value of a gauge. *tags* is a list of tag values to identify a specific gauge.
Example code to get value of a gauge:

.. code-block:: cpp

    c->get("test_tag", 7, true);

resolve
^^^^^^^

.. code-block:: cpp

    SimpleGauge resolve(Tags... f)

Resolves gauge object to a specific gauge for a list of tag values.

Returns a :ref:`SimpleGauge <config_http_filters_wasm_SimpleGauge>`
resolved from the gauge object, so that tag values do not need to be specified
in every record call. Example code:

.. code-block:: cpp

    auto simple_gauge = c->resolve("test_tag", 7, true);

.. _config_http_filters_wasm_SimpleGauge:

SimpleGauge
^^^^^^^^^^^

*SimpleGauge* is resolved from a :ref:`Gauge <config_http_filters_wasm_Gauge>` object with predetermined tag values.

record
^^^^^^

.. code-block:: cpp

    void record(int64_t offset)

Records current value of a gauge. *offset* is the value to set for current gauge.

get
^^^

.. code-block:: cpp

    uint64_t get()

Returns current value of a gauge.

.. _config_http_filters_wasm_Histogram:

Histogram
^^^^^^^^^

New
~~~

.. code-block:: cpp

    static Histogram<Tags...>* New(StringView name, MetricTagDescriptor<Tags>... fieldnames)

Create a new histogram object with the given metric name and tag names.
Example code to create a histogram metric:

.. code-block:: cpp

    auto h = Histogram<std::string, int, bool>::New(
                 "test_histogram", "string_tag", "int_tag", "bool_tag");

Returns a pointer to Histogram object.

record
^^^^^^

.. code-block:: cpp

    void record(int64_t offset, Tags... tags)

Records a value in histogram stats. *offset* is the value to be recorded.
*tags* is a list of tag values to identify a specific histogram.
Example code to add a new value into histogram:

.. code-block:: cpp

    h->record(1, "test_tag", 7, true)

resolve
^^^^^^^

.. code-block:: cpp

    SimpleHistogram resolve(Tags... f)

Resolves histogram object to a specific histogram for a list of tag values.

Returns a :ref:`SimpleHistogram <config_http_filters_wasm_SimpleHistogram>`
resolved from the histogram object, so that tag values do not need to be specified
in every record call. Example code:

.. code-block:: cpp

    auto simple_histogram = c->resolve("test_tag", 7, true);

.. _config_http_filters_wasm_SimpleHistogram:

SimpleHistogram
^^^^^^^^^^^^^^^

*SimpleHistogram* is resolved from a :ref:`Histogram <config_http_filters_wasm_Histogram>` object with predetermined tag values.

record
^^^^^^

.. code-block:: cpp

    void record(int64_t offset)

Records a value in histogram. *offset* is the value to be recorded.

Data Structure
--------------

.. _config_http_filters_wasm_GrpcCallHandler:

GrpcCallHandler
^^^^^^^^^^^^^^^

Base class for gRPC unary call handler. Subclass should specify response message type
and override necessary callbacks.
Example code to create a call handler using *google::protobuf::Empty* as response message.

.. code-block:: cpp

    class CallHandler : public GrpcCallHandler<google::protobuf::Empty> {
      public:
        void onSuccess(google::protobuf::Empty&& response) {
            /* override onSuccess code */
        }
        /*
            more callbacks such as onFailure, onCreateInitialMetadata
        */
    };

To initialize a handler, pass in a pointer to :ref:`context object <config_http_filters_wasm_context_object>`
that this call should attach to. For example, passing in root context:

.. code-block:: cpp

    auto handler = std::make_unique<CallHandler>(&root_context);

Note the context object needs to outlive the call.
*handler* is also used for WASM module to interact with the stream, such as canceling the call.

onSuccess
~~~~~~~~~

.. code-block:: cpp

    void onSuccess(Message&& response)

Called when the async gRPC request succeeds. No further callbacks will be invoked.

onFailure
~~~~~~~~~

.. code-block:: cpp

    void onFailure(GrpcStatus status, std::unique_ptr<WasmData> error_message)

Called when the async gRPC request fails. No further callbacks will be invoked.
*status* is returned grpc status. *error_message* is the gRPC status message or empty string if not present.

onCreateInitialMetadata
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

    void onCreateInitialMetadata()

Called when populating the headers to send with initial metadata. TODO: how to add metadata?

cancel
~~~~~~

.. code-block:: cpp

    void cancel()

Signals that the request should be cancelled. No further callbacks will be invoked.

.. _config_http_filters_wasm_GrpcStreamHandler:

GrpcStreamHandler
^^^^^^^^^^^^^^^^^

Base class for gRPC stream handler. Subclass should specify stream message type and override callbacks.
Example code to create a stream handler using *google::protobuf::Struct* as request message
and *google::protobuf::Any* response message:

.. code-block:: cpp

    class StreamHandler : public GrpcStreamHandler<google::protobuf::Struct, google::protobuf::Any> {
      public:
        void onReceive(google::protobuf::Any&& message) {
            /* override onReceive code */
        }
        /*
            more callbacks such as onCreateInitialMetadat, onReceiveTrailingMetadata, onReceive, onRemoteClose
        */
    };

To initialize a handler, pass in a pointer to :ref:`context object <config_http_filters_wasm_context_object>`
that this stream should attach to. For example, passing in root context:

.. code-block:: cpp

    auto handler = std::make_unique<StreamHandler>(&root_context);

Note the context object needs to outlive the stream.
*handler* is also used for WASM module to interact with the stream, such as sending message,
closing and resetting stream.

send
~~~~

.. code-block:: cpp

    void send(const Request& message, bool end_of_stream)

Sends a request message to the stream. *end_of_stream* indicates if this is the last message to send. 
With *end_of_stream* as true, callbacks can still occur.

close
~~~~~

.. code-block:: cpp

    void close()

Close the stream locally and send an empty DATA frame to the remote. No further methods may be
invoked on the stream object, but callbacks may still be received until the stream is closed
remotely.

reset
~~~~~

.. code-block:: cpp
  
    void reset()

Close the stream locally and remotely (as needed). No further methods may be invoked on the
handler object and no further callbacks will be invoked.

onCreateInitialMetadata
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp
  
    void onCreateInitialMetadata()

Called when populating the headers to send with initial metadata. TODO: how to add initial metadata?

onReceiveInitialMetadata
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp
  
    void onReceiveInitialMetadata()

Called when initial metadata is received. This will be called with empty metadata on a
trailers-only response, followed by onReceiveTrailingMetadata() with the trailing metadata.
. TODO: how to get initial metadata?

onReceiveTrailingMetadata
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

    void onReceiveTrailingMetadata()

Called when trailing metadata is received. This will also be called on non-Ok grpc-status
stream termination.

onReceive
~~~~~~~~~

.. code-block:: cpp

    void onReceive(Response&& message)

Called when an async gRPC message is received.

onRemoteClose
~~~~~~~~~~~~~

.. code-block:: cpp

    void onRemoteClose(GrpcStatus status, std::unique_ptr<WasmData> error_message)

Called when the remote closes or an error occurs on the gRPC stream. The stream is
considered remotely closed after this invocation and no further callbacks will be
invoked. In addition, no further stream operations are permitted. *status* is the
grpc status, *error_message* is the gRPC status error message or empty string if not present.

.. _config_http_filters_wasm_WasmData:

WasmData
^^^^^^^^

WasmData is used to represent data passed into WASM module from host. It is like string view,
which holds a pointer to start of the data and a size. It also supports several methods to access
the data.

data
~~~~

.. code-block:: cpp

    const char* data()

Returns the start pointer of the data.

view
~~~~

.. code-block:: cpp

    StringView view()

Returns data as a string view constructed with the start pointer and the size.

toString
~~~~~~~~

.. code-block:: cpp

    std::string toString()

Returns data as a string by converting the string view to string.

pairs
~~~~~

.. code-block:: cpp

    std::vector<std::pair<StringView, StringView>> pairs()

Returns a vector of string view pair parsed from the data.

proto
~~~~~

.. code-block:: cpp

    template<typename T> T proto()

Returns a proto message parsed from the data based on the specified proto type.

Out of tree WASM module
-----------------------

TODO: add an example about out of tree WASM module example

.. _config_http_filters_wasm_vm_sharing:

VM Sharing
----------

TODO: add instruction about vm sharing