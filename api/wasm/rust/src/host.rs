use crate::context::*;
use crate::types::*;
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
  get_root_context(_root_context_id).on_start(_vm_configuration_size)
}

#[no_mangle]
pub fn proxy_on_context_create(_context_id: u32, _parent_context_id: u32) {
  if _parent_context_id != 0 {
    ensure_context(_context_id, _parent_context_id).on_create();
  } else {
    ensure_root_context(_context_id);
  }
}

#[no_mangle]
pub fn proxy_on_configure(_root_context_id: u32, _vm_configuration_size: u32) -> u32 {
  match get_root_context(_root_context_id).on_configure(_vm_configuration_size) {
    true => 1,
    false => 0,
  }
}

#[no_mangle]
pub fn proxy_on_tick(_root_context_id: u32) {
  get_root_context(_root_context_id).on_tick()
}

#[no_mangle]
pub fn proxy_on_new_connection(_context_id: u32) -> u32 {
  filter_status_to_int(get_context(_context_id).on_new_connection())
}

#[no_mangle]
pub fn proxy_on_downstream_data(_context_id: u32, _data_length: u32, _end_stream: u32) -> u32 {
  filter_status_to_int(
    get_context(_context_id).on_downstream_connection(_data_length as usize, _end_stream != 0),
  )
}

// ====================== HTTP Request Handling API =============================
#[no_mangle]
pub fn proxy_on_request_headers(_context_id: u32, headers: u32) -> u32 {
  filter_header_status_to_int(get_context(_context_id).on_request_headers(headers))
}

#[no_mangle]
pub fn proxy_on_request_metadata(_context_id: u32, elements: u32) -> u32 {
  filter_metadata_status_to_int(get_context(_context_id).on_request_metadata(elements))
}

#[no_mangle]
pub fn proxy_on_request_trailers(_context_id: u32, trailers: u32) -> u32 {
  filter_trailer_status_to_int(get_context(_context_id).on_request_trailers(trailers))
}

#[no_mangle]
pub fn proxy_on_request_body(_context_id: u32, _body_buffer_length: u32, _end_stream: u32) -> u32 {
  filter_data_status_to_int(
    get_context(_context_id).on_request_body(_body_buffer_length as usize, _end_stream != 0),
  )
}

// ====================== HTTP Response Handling API =============================
#[no_mangle]
pub fn proxy_on_response_headers(_context_id: u32, headers: u32) -> u32 {
  filter_header_status_to_int(get_context(_context_id).on_response_headers(headers))
}

#[no_mangle]
pub fn proxy_on_response_metadata(_context_id: u32, elements: u32) -> u32 {
  filter_metadata_status_to_int(get_context(_context_id).on_response_metadata(elements))
}

#[no_mangle]
pub fn proxy_on_response_trailers(_context_id: u32, trailers: u32) -> u32 {
  filter_trailer_status_to_int(get_context(_context_id).on_response_trailers(trailers))
}

#[no_mangle]
pub fn proxy_on_response_body(_context_id: u32, _body_buffer_length: u32, _end_stream: u32) -> u32 {
  filter_data_status_to_int(
    get_context(_context_id).on_response_body(_body_buffer_length as usize, _end_stream != 0),
  )
}

#[no_mangle]
pub fn proxy_on_done(_context_id: u32) -> u32 {
  0
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

  // ====================== Low-Level Proxy Header/Header/Metadata API ===========================
  pub fn proxy_get_header_map_pairs(
    _type: u32,
    _ptr: *const *mut c_char,
    _size_ptr: *mut usize,
  ) -> u32;
  pub fn proxy_set_header_map_pairs(_type: u32, _ptr: *const c_char, _size: usize) -> u32;
  pub fn proxy_get_header_map_value(
    _type: u32,
    _key_ptr: *const c_char,
    _key_size_ptr: usize,
    _value_ptr: *const *mut c_char,
    _value_size_ptr: *mut usize,
  ) -> u32;
  pub fn proxy_add_header_map_value(
    _type: u32,
    _key_ptr: *const c_char,
    _key_size: usize,
    _value_ptr: *const c_char,
    _value_size: usize,
  ) -> u32;
  pub fn proxy_replace_header_map_value(
    _type: u32,
    _key_ptr: *const c_char,
    _key_size: usize,
    _value_ptr: *const c_char,
    _value_size: usize,
  ) -> u32;
  pub fn proxy_remove_header_map_value(
    _type: u32,
    _key_ptr: *const c_char,
    _key_size: usize,
  ) -> u32;
  pub fn proxy_get_header_map_size(_type: u32, _value_size_ptr: *mut usize) -> u32;
// ====================== Low-Level Proxy Header/Header/Metadata API ===========================
}
