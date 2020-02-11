use crate::payload_wrapper::*;
use crate::types::*;
use log::error;
use std::collections::HashMap;

// ====================== Request Header Processing API ===========================
pub fn get_request_header_pairs() -> HashMap<String, String> {
  match get_header_map_pairs(HeaderMapType::RequestHeaders) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

#[deprecated(note = "assertion failure when called proxy_set_header_map_pairs")]
pub fn set_request_header_pairs(_pairs: &HashMap<String, String>) -> WasmResult {
  set_header_map_pairs(HeaderMapType::RequestHeaders, _pairs)
}

pub fn get_request_header(key: String) -> Box<WasmData> {
  match get_header_map_value(HeaderMapType::RequestHeaders, key) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

pub fn add_request_header(key: String, value: String) -> WasmResult {
  add_header_map_value(HeaderMapType::RequestHeaders, key, value)
}

pub fn replace_request_header(key: String, value: String) -> WasmResult {
  replace_header_map_value(HeaderMapType::RequestHeaders, key, value)
}

pub fn remove_request_header(key: String) -> WasmResult {
  remove_header_map_value(HeaderMapType::RequestHeaders, key)
}

pub fn get_request_header_size() -> u32 {
  get_header_map_value_size(HeaderMapType::RequestHeaders)
}
// ====================== Request Header Processing API ===========================

// ====================== Response Header Processing API ===========================
#[deprecated(note = "assertion failure when called proxy_set_header_map_pairs")]
pub fn set_response_header_pairs(_pairs: &HashMap<String, String>) -> WasmResult {
  set_header_map_pairs(HeaderMapType::ResponseHeaders, _pairs)
}

pub fn get_response_header_pairs() -> HashMap<String, String> {
  match get_header_map_pairs(HeaderMapType::ResponseHeaders) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

pub fn get_response_header(key: String) -> Box<WasmData> {
  match get_header_map_value(HeaderMapType::ResponseHeaders, key) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

pub fn add_response_header(key: String, value: String) -> WasmResult {
  add_header_map_value(HeaderMapType::ResponseHeaders, key, value)
}

pub fn replace_response_header(key: String, value: String) -> WasmResult {
  replace_header_map_value(HeaderMapType::ResponseHeaders, key, value)
}

pub fn remove_response_header(key: String) -> WasmResult {
  remove_header_map_value(HeaderMapType::ResponseHeaders, key)
}

pub fn get_response_header_size() -> u32 {
  get_header_map_value_size(HeaderMapType::ResponseHeaders)
}
// ====================== Response Header Processing API ===========================

// ====================== Request Trailer Processing API ===========================
#[deprecated(note = "assertion failure when called proxy_set_header_map_pairs")]
pub fn set_request_trailer_pairs(_pairs: &HashMap<String, String>) -> WasmResult {
  set_header_map_pairs(HeaderMapType::RequestTrailers, _pairs)
}

pub fn get_request_trailer_pairs() -> HashMap<String, String> {
  match get_header_map_pairs(HeaderMapType::RequestTrailers) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

pub fn get_request_trailer(key: String) -> Box<WasmData> {
  match get_header_map_value(HeaderMapType::RequestTrailers, key) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

pub fn add_request_trailer(key: String, value: String) -> WasmResult {
  add_header_map_value(HeaderMapType::RequestTrailers, key, value)
}

pub fn replace_request_trailer(key: String, value: String) -> WasmResult {
  replace_header_map_value(HeaderMapType::RequestTrailers, key, value)
}

pub fn remove_request_trailer(key: String) -> WasmResult {
  remove_header_map_value(HeaderMapType::RequestTrailers, key)
}

pub fn get_request_trailer_size() -> u32 {
  get_header_map_value_size(HeaderMapType::RequestTrailers)
}
// ====================== Request Trailer Processing API ===========================

// ====================== Response Trailer Processing API ===========================
#[deprecated(note = "assertion failure when called proxy_set_header_map_pairs")]
pub fn set_response_trailer_pairs(_pairs: &HashMap<String, String>) -> WasmResult {
  set_header_map_pairs(HeaderMapType::ResponseTrailers, _pairs)
}

pub fn get_response_trailer_pairs() -> HashMap<String, String> {
  match get_header_map_pairs(HeaderMapType::ResponseTrailers) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

pub fn get_response_trailer(key: String) -> Box<WasmData> {
  match get_header_map_value(HeaderMapType::ResponseTrailers, key) {
    Ok(t) => t,
    Err(e) => {
      error!("{}", e);
      panic!();
    }
  }
}

pub fn add_response_trailer(key: String, value: String) -> WasmResult {
  add_header_map_value(HeaderMapType::ResponseTrailers, key, value)
}

pub fn replace_response_trailer(key: String, value: String) -> WasmResult {
  replace_header_map_value(HeaderMapType::ResponseTrailers, key, value)
}

pub fn remove_response_trailer(key: String) -> WasmResult {
  remove_header_map_value(HeaderMapType::ResponseTrailers, key)
}

pub fn get_response_trailer_size() -> u32 {
  get_header_map_value_size(HeaderMapType::ResponseTrailers)
}
// ====================== Response Trailer Processing API ===========================
