// Include exports from Proxy-WASM at link-time.
#[allow(unused_imports)]
use proxy_wasm;

// Required Proxy-Wasm ABI version.
#[no_mangle]
pub fn proxy_abi_version_0_1_0() {}

use log::{debug, error, info, trace, warn};

extern "C" {
  fn proxy_done() -> u32;
}

#[no_mangle]
pub fn proxy_on_configure(_context_id : u32, _plugin_configuration_size: u32) -> u32 {
    warn!("warn configure-test");
    1
}

#[no_mangle]
pub fn proxy_on_vm_start(_context_id : u32, _vm_configuration_size : u32) -> u32 {
    trace!("test trace logging");
    debug!("test debug logging");
    error!("test error logging");
    1
}

#[no_mangle]
pub fn proxy_on_tick(_context_id : u32) {
    info!("test tick logging");
    unsafe {
        proxy_done();
    }
}

#[no_mangle]
pub fn proxy_on_done(_context_id : u32) -> u32 {
    info!("onDone logging");
    0
}

#[no_mangle]
pub fn proxy_on_delete(_context_id : u32) {
    info!("onDelete logging");
}
