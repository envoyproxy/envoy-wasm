package envoywasmsdk

var exportedHeap map[*byte][]byte

/*
	Low level imported APIs
 */
func _proxy_log(level LogLevel, logMessage *string, messageSize int32) WasmResult
func _proxy_getHeaderMapValue(t HeaderMapType, key *string, keySize int32, valAddress **byte, valSize *int32) WasmResult
func _proxy_addHeaderMapValue(t HeaderMapType, key *string, keySize int32, val *string, valSize int32) WasmResult


/*
	Exported APIs
 */
//go:export _malloc
func malloc(size int32) *byte {
	if (exportedHeap == nil) {
		exportedHeap = make(map[*byte][]byte)
	}
	b := make([]byte, size)
	exportedHeap[&b[0]] = b
	return &b[0]
}

//go:export _free
func free(b *byte) {
	delete(exportedHeap, b)
}

//go:export _proxy_onStart
func proxy_onStart(rootContextID, rootIDPtr, rootIDSize,
    vmConfigurationPtr, vmConfigurationSize uint32) {
	ensureRootContext().OnStart()
}

//go:export _proxy_onCreate
func proxy_onCreate(ctxID, rootCtxID uint32) {
	ensureStreamContext(ctxID).OnCreate()
}

//go:export _proxy_onDelete
func proxy_onDelete(ctxID uint32) {
	getStreamContext(ctxID).OnDelete()
}

//go:export _proxy_onRequestHeaders
func proxy_onRequestHeaders(ctxID uint32) FilterHeadersStatus {
	return getStreamContext(ctxID).OnRequestHeaders()
}

//go:export _proxy_onResponseHeaders
func proxy_onResponseHeaders(ctxID uint32) FilterHeadersStatus {
	return getStreamContext(ctxID).OnResponseHeaders()
}
