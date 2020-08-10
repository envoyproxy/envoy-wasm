use log::{debug, info, warn};
use proxy_wasm::traits::{Context, HttpContext};
use proxy_wasm::types::*;

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_http_context(|_, _| -> Box<dyn HttpContext> { Box::new(TestStream) });
}

struct TestStream;

impl Context for TestStream {}

impl HttpContext for TestStream {
    fn on_http_request_headers(&mut self, _: usize) -> Action {
        if self.get_shared_data("shared_data_key_bad") == (None, None) {
            debug!("get of bad key not found");
        }
        self.set_shared_data("shared_data_key1", Some(b"shared_data_value0"), None)
            .unwrap();
        self.set_shared_data("shared_data_key1", Some(b"shared_data_value1"), None)
            .unwrap();
        self.set_shared_data("shared_data_key2", Some(b"shared_data_value2"), None)
            .unwrap();
        if let (_, Some(cas)) = self.get_shared_data("shared_data_key2") {
            match self.set_shared_data(
                "shared_data_key2",
                Some(b"shared_data_value3"),
                Some(cas + 1),
            ) {
                Err(Status::CasMismatch) => info!("set CasMismatch"),
                _ => panic!(),
            };
        }
        Action::Continue
    }

    fn on_log(&mut self) {
        if self.get_shared_data("shared_data_key_bad") == (None, None) {
            debug!("second get of bad key not found");
        }
        if let (Some(value), _) = self.get_shared_data("shared_data_key1") {
            debug!("get 1 {}", String::from_utf8(value).unwrap());
        }
        if let (Some(value), _) = self.get_shared_data("shared_data_key2") {
            warn!("get 2 {}", String::from_utf8(value).unwrap());
        }
    }
}
