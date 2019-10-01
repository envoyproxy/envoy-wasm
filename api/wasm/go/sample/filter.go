package main

import (
	sdk "envoywasmsdk"
)

type SampleStreamContextFactory struct {}
type SampleRootContextFactory struct {}

type SampleStreamContext struct {
	id uint32
}

type SampleRootContext struct {
}

func (sf SampleRootContextFactory) CreateRootContext() sdk.RootContext {
	return SampleRootContext{}
}

func (sf SampleStreamContextFactory) CreateStreamContext(ctxID uint32) sdk.StreamContext {
	return SampleStreamContext{id: ctxID}
}

func (rc SampleRootContext) OnStart() {
	sdk.Log(sdk.INFO, "called onStart")
}

func (sc SampleStreamContext) OnCreate() {
	sdk.Log(sdk.INFO, "called onCreate")
}

func (sc SampleStreamContext) OnRequestHeaders() sdk.FilterHeadersStatus {
	sdk.Log(sdk.INFO, "request headers:")
	sdk.Log(sdk.INFO, "user-agent: " + sdk.GetHeaderMapValue(sdk.RequestHeaders, "user-agent"))
	sdk.Log(sdk.INFO, "authority: " + sdk.GetHeaderMapValue(sdk.RequestHeaders, ":authority"))
	sdk.Log(sdk.INFO, "path: " + sdk.GetHeaderMapValue(sdk.RequestHeaders, ":path"))
	sdk.Log(sdk.INFO, "method: " + sdk.GetHeaderMapValue(sdk.RequestHeaders, ":method"))
	sdk.Log(sdk.INFO, "scheme: " + sdk.GetHeaderMapValue(sdk.RequestHeaders, ":scheme"))
	return sdk.Continue
}

func (sc SampleStreamContext) OnResponseHeaders() sdk.FilterHeadersStatus {
	sdk.AddHeaderMapValue(sdk.ResponseHeaders, "test-key", "test-val")
	return sdk.Continue
}

func (sc SampleStreamContext) OnDelete() {
	sdk.Log(sdk.INFO, "called onDelete")
}

var registry = sdk.RegisterFactory(
	SampleRootContextFactory{}, SampleStreamContextFactory{})

func main() {}
