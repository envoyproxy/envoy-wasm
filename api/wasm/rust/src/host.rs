use crate::context::*;
use log::info;
use std::os::raw::c_char;

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
pub fn proxy_on_vm_start(_root_context_id: u32, _vm_configuration_size: u32) -> u32 {
  get_root_context(_root_context_id).on_start();
  1
}

#[no_mangle]
pub fn proxy_on_context_create(_root_context_id: u32, _context_id: u32) {
  if _context_id != 0 {
    ensure_context(_context_id, _root_context_id).on_create();
  } else {
    ensure_root_context(_root_context_id);
  }
}

#[no_mangle]
pub fn proxy_on_done(_context_id: u32) -> u32 {
  info!("done {}", _context_id);
  1
}

/// Low-level Proxy-WASM APIs for the host functions.
extern "C" {
  pub fn proxy_log(level: u32, message_data: *const u8, message_size: usize) -> u32;
  pub fn proxy_get_property(
    _path_ptr: *const c_char,
    _path_size: usize,
    _value_ptr_ptr: *const *mut c_char,
    _value_size_ptr: *mut usize,
  ) -> u32;
}
