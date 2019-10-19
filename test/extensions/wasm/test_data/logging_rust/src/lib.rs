// Include exports from Proxy-WASM at link-time.
#[allow(unused_imports)]
use proxy_wasm;

use log::{debug, error, info, trace, warn};

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onConfigure(_context_id : u32, message: &str) -> u32 {
    warn!("warn {}", message);
    1
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onStart(_context_id : u32, _root_id_ptr : u32, _root_id_size : u32, _vm_configuration_ptr : u32, _vm_configuration_size : u32) {
    trace!("test trace logging");
    debug!("test debug logging");
    error!("test error logging");
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onTick(_context_id : u32) {
    info!("test tick logging");
}
