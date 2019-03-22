// Include exports from Proxy-WASM at link-time.
#[allow(unused_imports)]
use proxy_wasm;

use log::{debug, error, info, trace, warn};

#[no_mangle]
#[allow(non_snake_case)]
pub fn _proxy_onConfigure(message: &str) {
    warn!("warn {}", message);
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn _proxy_onStart() {
    trace!("test trace logging");
    debug!("test debug logging");
    error!("test error logging");
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn _proxy_onTick() {
    info!("test tick logging");
}
