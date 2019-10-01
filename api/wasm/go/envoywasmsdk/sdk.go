package envoywasmsdk

/*
 	High level API
 */
func Log(level LogLevel, msg string) WasmResult {
	return _proxy_log(level, &msg, int32(len(msg)))
}

func GetHeaderMapValue(t HeaderMapType, key string) string {
	var valAddress *byte
	var valSize int32
	result := _proxy_getHeaderMapValue(t, &key, int32(len(key)), &valAddress, &valSize)
	if result != Ok {
		Log(WARN, "failed to get header value!")
		return ""
	}
	if (result == Ok) {
		if val, ok := exportedHeap[valAddress]; ok {
			return string(val)
		}
	}
	Log(WARN, "failed to get header value!")
	return ""
}

func AddHeaderMapValue(t HeaderMapType, key, val string) {
	result := _proxy_addHeaderMapValue(t, &key, int32(len(key)), &val, int32(len(val)))
	if (result != Ok) {
	    Log(WARN, "failed to get header value!")
	}
}
