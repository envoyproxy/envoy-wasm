use proxy_wasm::traits::{Context, RootContext};
use proxy_wasm::types::*;
use proxy_wasm::hostcalls;

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_root_context(|context_id| -> Box<dyn RootContext> {
        Box::new(AccessLogger { context_id })
    });
}

struct AccessLogger {
    context_id: u32
}

impl RootContext for AccessLogger {
    fn on_log(&mut self) {
        {
            let request_headers = hostcalls::get_map(MapType::HttpRequestHeaders).unwrap();
            log::trace!("onLog {}: {} request headers", self.context_id, request_headers.len());
            for (name, value) in &request_headers {
                log::trace!(" {} = {}", name, value);
            }
        }
        {
            let response_headers = hostcalls::get_map(MapType::HttpResponseHeaders).unwrap();
            log::trace!("onLog {}: {} response headers", self.context_id, response_headers.len());
            for (name, value) in &response_headers {
                log::trace!(" {} = {}", name, value);
            }
        }
        {
            let response_trailers = hostcalls::get_map(MapType::HttpResponseTrailers).unwrap();
            log::trace!("onLog {}: {} response trailers", self.context_id, response_trailers.len());
            for (name, value) in &response_trailers {
                log::trace!(" {} = {}", name, value);
            }
        }
        {
            log::trace!("onLog {}: stream info", self.context_id);

            if let Some(value) = hostcalls::get_property(vec!["upstream", "transport_failure_reason"]).unwrap() {
                let string_value = String::from_utf8(value).unwrap();
                log::trace!(" {} = {}", "upstream.transport_failure_reason", string_value);
            }
        }
    }
}
impl Context for AccessLogger {}
