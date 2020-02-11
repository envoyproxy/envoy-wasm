use crate::host::*;
use crate::types::*;
use log::{info, warn};
use std::collections::HashMap;
use std::convert::TryFrom;
use std::mem::transmute;
use std::os::raw::c_char;
use std::ptr::null_mut;

pub struct WasmData {
  data: *const c_char,
  len: usize,
}

impl WasmData {
  pub fn to_string(&self) -> String {
    unsafe { String::from_raw_parts(self.data as *mut u8, self.len, self.len) }
  }
}

pub fn buffer_size(_pairs: &HashMap<String, String>) -> usize {
  let mut size = 4;
  size += 8 * _pairs.len(); // It is required to add the length of key and value.
  for (k, v) in _pairs {
    size += k.len();
    size += 1; // Null-termination symbol
    size += v.len();
    size += 1; // Null-termination symbol
  }
  size
}

pub fn buffer_into_hashmap(_buffer_ptr: *mut u8, _size: usize) -> HashMap<String, String> {
  let mut result: HashMap<String, String> = HashMap::new();
  unsafe {
    let buffer = Vec::from_raw_parts(_buffer_ptr, _size, _size);
    // read 4 bytes to get hashmap size illustrated as usize
    let pairs_size = transmute::<[u8; 4], u32>([
      buffer[0] as u8,
      buffer[1] as u8,
      buffer[2] as u8,
      buffer[3] as u8,
    ]);
    let mut byte_lengthes = Vec::<(u32, u32)>::new();
    let mut key_value_index_starter = 0;
    let mut key_size = 0;
    for i in 0..(2 * pairs_size) {
      if i % 2 == 0 {
        // read 4 bytes to get key size illustrated as usize
        key_size = transmute::<[u8; 4], u32>([
          buffer[4 + 4 * i as usize] as u8,
          buffer[5 + 4 * i as usize] as u8,
          buffer[6 + 4 * i as usize] as u8,
          buffer[7 + 4 * i as usize] as u8,
        ]);
      } else {
        // read 4 bytes to get value size illustrated as usize
        let value_size = transmute::<[u8; 4], u32>([
          buffer[4 + 4 * i as usize] as u8,
          buffer[5 + 4 * i as usize] as u8,
          buffer[6 + 4 * i as usize] as u8,
          buffer[7 + 4 * i as usize] as u8,
        ]);
        if key_size != 0 && value_size != 0 {
          byte_lengthes.push((key_size, value_size));
          key_size = 0;
        }
        key_value_index_starter = (7 + 4 * i) + 1;
      }
    }
    for (key_size, value_size) in byte_lengthes {
      let key_str = String::from_utf8(
        buffer[key_value_index_starter as usize..(key_value_index_starter + key_size) as usize]
          .to_vec(),
      )
      .unwrap();
      key_value_index_starter += key_size + 1;
      let value_str = String::from_utf8(
        buffer[key_value_index_starter as usize..(key_value_index_starter + value_size) as usize]
          .to_vec(),
      )
      .unwrap();
      key_value_index_starter += value_size + 1;
      result.insert(key_str, value_str);
    }
  }
  result
}

pub fn hashmap_into_buffer(_pairs: &HashMap<String, String>, _buffer: &mut Vec<c_char>) {
  let mut index = 0;
  // write length of pairs
  let pairs_len = _pairs.len() as u32;
  for b in pairs_len.to_be_bytes().iter() {
    _buffer[index] = *b as i8;
    index += 1;
  }
  // write length of keys and values
  for (key, value) in _pairs {
    let key_len = key.len();
    for b in key_len.to_be_bytes().iter() {
      _buffer[index] = *b as i8;
      index += 1;
    }
    let value_len = value.len();
    for b in value_len.to_be_bytes().iter() {
      _buffer[index] = *b as i8;
      index += 1;
    }
  }
  // write value of pairs
  for (key, value) in _pairs {
    for b in key.as_bytes().iter() {
      _buffer[index] = *b as i8;
      index += 1;
    }
    _buffer[index] = '\0' as i8;
    index += 1;
    for b in value.as_bytes().iter() {
      _buffer[index] = *b as i8;
      index += 1;
    }
    _buffer[index] = '\0' as i8;
    index += 1;
  }
}

pub fn export_hashmap(_pairs: &HashMap<String, String>) -> (*mut c_char, usize) {
  let buffer_size = buffer_size(_pairs);
  let mut alloced_buffer = Vec::<c_char>::with_capacity(buffer_size);
  unsafe {
    alloced_buffer.set_len(buffer_size);
  }
  hashmap_into_buffer(_pairs, &mut alloced_buffer);
  (
    Box::into_raw(alloced_buffer.into_boxed_slice()) as *mut c_char,
    buffer_size,
  )
}

// ======================= Low-Level Proxy API Wrapper =============================

pub fn get_header_map_pairs(htype: HeaderMapType) -> Result<HashMap<String, String>, String> {
  let type_num = header_map_type_to_int(htype);
  let data_ptr: *mut c_char = null_mut::<c_char>();
  let size_ptr = Box::into_raw(Box::new(0));
  unsafe {
    let code = proxy_get_header_map_pairs(type_num, &data_ptr, size_ptr);
    match WasmResult::try_from(code) {
      Ok(r) => match r {
        WasmResult::Ok => {
          let header_map = buffer_into_hashmap(data_ptr as *mut u8, *size_ptr);
          Ok(header_map)
        }
        _ => Err(r.to_string()),
      },
      Err(e) => Err(e),
    }
  }
}

pub fn set_header_map_pairs(htype: HeaderMapType, _pairs: &HashMap<String, String>) -> WasmResult {
  let type_num = header_map_type_to_int(htype);
  let (buffer, size) = export_hashmap(_pairs);
  unsafe {
    info!(
      "data {}",
      String::from_raw_parts(buffer as *mut u8, size, size)
    );
  }
  unsafe {
    let code = proxy_set_header_map_pairs(type_num, buffer, size);
    match WasmResult::try_from(code) {
      Ok(r) => r,
      Err(e) => {
        warn!("failed to convert: {}", e);
        WasmResult::InternalFailure
      }
    }
  }
}

pub fn get_header_map_value(htype: HeaderMapType, key: String) -> Result<Box<WasmData>, String> {
  let type_num = header_map_type_to_int(htype);
  let data_ptr: *mut c_char = null_mut::<c_char>();
  let size_ptr = Box::into_raw(Box::new(0));
  unsafe {
    let code = proxy_get_header_map_value(
      type_num,
      key.as_ptr() as *const c_char,
      key.bytes().len(),
      &data_ptr,
      size_ptr,
    );
    match WasmResult::try_from(code) {
      Ok(r) => match r {
        WasmResult::Ok => Ok(Box::new(WasmData {
          data: data_ptr,
          len: *size_ptr,
        })),
        _ => Err(r.to_string()),
      },
      Err(e) => Err(e),
    }
  }
}

pub fn add_header_map_value(htype: HeaderMapType, key: String, value: String) -> WasmResult {
  let type_num = header_map_type_to_int(htype);
  unsafe {
    let code = proxy_add_header_map_value(
      type_num,
      key.as_ptr() as *const c_char,
      key.as_bytes().len(),
      value.as_ptr() as *const c_char,
      value.as_bytes().len(),
    );
    match WasmResult::try_from(code) {
      Ok(r) => r,
      Err(e) => panic!(format!("failed to convert: {}", e)),
    }
  }
}

pub fn replace_header_map_value(htype: HeaderMapType, key: String, value: String) -> WasmResult {
  let type_num = header_map_type_to_int(htype);
  unsafe {
    let code = proxy_replace_header_map_value(
      type_num,
      key.as_ptr() as *const c_char,
      key.as_bytes().len(),
      value.as_ptr() as *const c_char,
      value.as_bytes().len(),
    );
    match WasmResult::try_from(code) {
      Ok(r) => r,
      Err(e) => panic!(format!("failed to convert: {}", e)),
    }
  }
}

pub fn remove_header_map_value(htype: HeaderMapType, key: String) -> WasmResult {
  let type_num = header_map_type_to_int(htype);
  unsafe {
    let code = proxy_remove_header_map_value(
      type_num,
      key.as_ptr() as *const c_char,
      key.as_bytes().len(),
    );
    match WasmResult::try_from(code) {
      Ok(r) => r,
      Err(e) => panic!(format!("failed to convert: {}", e)),
    }
  }
}

pub fn get_header_map_value_size(htype: HeaderMapType) -> u32 {
  let type_num = header_map_type_to_int(htype);
  let size_ptr = Box::into_raw(Box::new(0));
  unsafe { proxy_get_header_map_size(type_num, size_ptr) }
}

// ======================= Low-Level Proxy API Wrapper =============================
