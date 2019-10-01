package envoywasmsdk

type RootContext interface {
	OnStart()
}

type StreamContext interface {
	OnCreate()
	OnRequestHeaders() FilterHeadersStatus
	OnResponseHeaders() FilterHeadersStatus
	OnDelete()
}

type RootContextFactory interface {
	CreateRootContext() RootContext
}

type StreamContextFactory interface {
	CreateStreamContext(id uint32) StreamContext
}

type Registry struct {}

var RootCtx RootContext
var StreamCtxMap map[uint32]StreamContext
var RootCtxFty RootContextFactory
var StreamCtxFty StreamContextFactory

func RegisterFactory(rcf RootContextFactory, scf StreamContextFactory) Registry {
	RootCtxFty = rcf
	StreamCtxFty = scf
	return Registry{}
}

func ensureRootContext() RootContext {
  if RootCtxFty == nil {
	  return nil
  }
  if RootCtx != nil {
	  return RootCtx
  }
  return RootCtxFty.CreateRootContext()
}

func ensureStreamContext(id uint32) StreamContext {
	if StreamCtxMap == nil {
		StreamCtxMap = make(map[uint32]StreamContext)
	}
	if v, ok := StreamCtxMap[id]; ok {
		return v
	}
	StreamCtxMap[id] = StreamCtxFty.CreateStreamContext(id)
	return StreamCtxMap[id]
}

func getStreamContext(id uint32) StreamContext {
	if v, ok := StreamCtxMap[id]; ok {
		return v
	}
	return nil
}
