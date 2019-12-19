/// Logger that integrates with host's logging system.
pub struct Logger;

static LOGGER: Logger = Logger;

impl Logger {
    pub fn init() -> Result<(), log::SetLoggerError> {
        log::set_logger(&LOGGER).map(|()| log::set_max_level(log::LevelFilter::Trace))
    }

    fn proxywasm_loglevel(level: log::Level) -> u32 {
        match level {
            log::Level::Trace => 0,
            log::Level::Debug => 1,
            log::Level::Info => 2,
            log::Level::Warn => 3,
            log::Level::Error => 4,
        }
    }
}

impl log::Log for Logger {
    fn enabled(&self, _metadata: &log::Metadata) -> bool {
        true
    }

    fn log(&self, record: &log::Record) {
        let level = Logger::proxywasm_loglevel(record.level());
        let message = record.args().to_string();
        unsafe {
            host::proxy_log(level, message.as_ptr(), message.len());
        }
    }

    fn flush(&self) {}
}

/// Always hook into host's logging system.
#[no_mangle]
fn _start() {
    Logger::init().unwrap();
}

/// Allow host to allocate memory.
#[no_mangle]
fn malloc(size: usize) -> *mut u8 {
    let mut vec: Vec<u8> = Vec::with_capacity(size);
    unsafe {
        vec.set_len(size);
    }
    let slice = vec.into_boxed_slice();
    Box::into_raw(slice) as *mut u8
}

/// Allow host to free memory.
#[no_mangle]
fn free(ptr: *mut u8) {
    if !ptr.is_null() {
        unsafe {
            Box::from_raw(ptr);
        }
    }
}

#[no_mangle]
fn proxy_on_configure(_root_context_id: u32, _configuration_size: u32) -> u32 {
    1
}

#[no_mangle]
fn proxy_on_create(_context_id: u32, _root_context_id: u32) {
}

#[no_mangle]
fn proxy_on_downstream_connection_close(_context_id: u32, _peer_type: u32) {
}

#[no_mangle]
fn proxy_on_downstream_data(_ontext_id: u32, _data_length: u32, _end_of_stream: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_new_connection(_context_id: u32) -> i32  {
    1
}

#[no_mangle]
fn proxy_on_request_body(_context_id: u32, _body_buffer_length: u32, _end_of_stream: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_request_headers(_context_id: u32, _headers: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_request_metadata(_context_id: u32, _elements: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_request_trailers(_context_id: u32, _trailers: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_response_body(_context_id: u32, _body_buffer_length: u32, _end_of_stream: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_response_headers(_context_id: u32, _headers: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_response_metadata(_context_id: u32, _elements: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_response_trailers(_context_id: u32, _trailers: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_start(_root_context_id: u32, _vm_configuration_size: u32) -> u32 {
    1
}

#[no_mangle]
fn proxy_on_tick(_root_context_id: u32) {
}

#[no_mangle]
fn proxy_on_upstream_connection_close(_context_id: u32, _peer_type: u32) {
}

#[no_mangle]
fn proxy_on_upstream_data(_context_id: u32, _data_length: u32, _end_of_stream: u32) -> i32 {
    1
}

#[no_mangle]
fn proxy_on_validate_configuration(_root_context_id: u32, _configuration_size: u32) -> u32 {
    0
}


/// Low-level Proxy-WASM APIs for the host functions.
pub mod host {
    extern "C" {
        pub fn proxy_log(level: u32, message_data: *const u8, message_size: usize) -> u32;
    }
}
