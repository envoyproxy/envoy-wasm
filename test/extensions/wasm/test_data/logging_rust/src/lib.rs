// Include exports from Proxy-WASM at link-time.
#[allow(unused_imports)]
use proxy_wasm;

use log::{debug, error, info, trace, warn};

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onConfigure(_context_id : u32, _plugin_configuration_size: u32) -> u32 {
    warn!("warn configure-test");
    1
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onStart(_context_id : u32, _vm_configuration_size : u32) -> u32 {
    trace!("test trace logging");
    debug!("test debug logging");
    error!("test error logging");
    1
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onTick(_context_id : u32) {
    info!("test tick logging");
}
