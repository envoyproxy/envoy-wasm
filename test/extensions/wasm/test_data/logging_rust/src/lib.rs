// Include exports from Proxy-WASM at link-time.
#[allow(unused_imports)]
use proxy_wasm;

use log::{debug, error, info, trace, warn};

extern "C" {
  fn proxy_done() -> u32;
}

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
    unsafe {
        proxy_done();
    }
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onDone(_context_id : u32) -> u32 {
    info!("onDone logging");
    0
}

#[no_mangle]
#[allow(non_snake_case)]
pub fn proxy_onDelete(_context_id : u32) {
    info!("onDelete logging");
}
