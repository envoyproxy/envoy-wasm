use crate::host::*;
use crate::types::*;
use lazy_static::lazy_static;
use std::collections::HashMap;
use std::ffi::CString;
use std::os::raw::c_char;
use std::ptr::null_mut;
use std::sync::{Arc, Mutex};

pub trait RootContext {
  fn on_start(&self, _configuration_size: u32) -> u32 {
    0
  }
  fn on_configure(&self, _configuration_size: u32) -> u32 {
    0
  }
  fn on_tick(&self) {}
}

pub trait Context {
  fn on_create(&self);
  fn on_new_connection(&self) -> FilterStatus {
    FilterStatus::Continue
  }
  fn on_downstream_connection(&self, _data_length: usize, _is_stream_end: bool) -> FilterStatus {
    FilterStatus::Continue
  }
  fn on_request_headers(&self, _headers: u32) -> FilterHeadersStatus {
    FilterHeadersStatus::Continue
  }
  fn on_request_metadata(&self, _element: u32) -> FilterMetadataStatus {
    FilterMetadataStatus::Continue
  }
  fn on_request_trailers(&self, _trailers: u32) -> FilterTrailersStatus {
    FilterTrailersStatus::Continue
  }
  fn on_request_body(&self, _body_buffer_length: usize, _is_stream_end: bool) -> FilterDataStatus {
    FilterDataStatus::Continue
  }
  fn on_response_headers(&self, _headers: u32) -> FilterHeadersStatus {
    FilterHeadersStatus::Continue
  }
  fn on_response_metadata(&self, _element: u32) -> FilterMetadataStatus {
    FilterMetadataStatus::Continue
  }
  fn on_response_trailers(&self, _trailers: u32) -> FilterTrailersStatus {
    FilterTrailersStatus::Continue
  }
  fn on_response_body(&self, _body_buffer_length: usize, _is_stream_end: bool) -> FilterDataStatus {
    FilterDataStatus::Continue
  }
}

pub trait RootContextFactory {
  fn create(&self) -> Arc<dyn RootContext + Sync + Send>;
}
pub trait ContextFactory {
  fn create(
    &self,
    _root_context: Arc<dyn RootContext + Sync + Send>,
  ) -> Arc<dyn Context + Sync + Send>;
}

lazy_static! {
  static ref ROOT_CONTEXT_FACTORY_MAP: Mutex<HashMap<&'static str, &'static Box<dyn RootContextFactory + Sync + Send>>> =
    Mutex::new(HashMap::new());
  static ref CONTEXT_FACTORY_MAP: Mutex<HashMap<&'static str, &'static Box<dyn ContextFactory + Sync + Send>>> =
    Mutex::new(HashMap::new());
  static ref ROOT_CONTEXT_MAP: Mutex<HashMap<u32, Arc<dyn RootContext + Sync + Send>>> =
    Mutex::new(HashMap::new());
  static ref CONTEXT_MAP: Mutex<HashMap<u32, Arc<dyn Context + Sync + Send>>> =
    Mutex::new(HashMap::new());
}

pub fn register_factory(
  _root_id: &'static str,
  _cf: &'static Box<dyn ContextFactory + Sync + Send>,
  _rcf: &'static Box<dyn RootContextFactory + Sync + Send>,
) {
  ROOT_CONTEXT_FACTORY_MAP
    .lock()
    .unwrap()
    .insert(_root_id, _rcf);
  CONTEXT_FACTORY_MAP.lock().unwrap().insert(_root_id, _cf);
}

fn current_root_id_str() -> String {
  let path = CString::new("plugin_root_id").unwrap();
  let root_id: *mut c_char = null_mut::<c_char>();
  let root_id_size_ptr = Box::into_raw(Box::new(0));
  unsafe {
    proxy_get_property(
      path.as_ptr(),
      path.as_bytes().len(),
      &root_id,
      root_id_size_ptr,
    );
    // Is it correct to fill capacity with the length of allocated length of root_id_str?
    String::from_raw_parts(root_id as *mut u8, *root_id_size_ptr, *root_id_size_ptr)
  }
}

pub fn ensure_root_context(root_context_id: u32) -> Arc<dyn RootContext + Sync + Send> {
  let root_context = match ROOT_CONTEXT_MAP.lock().unwrap().get(&root_context_id) {
    Some(x) => Arc::clone(x),
    None => {
      let root_id_str = current_root_id_str();
      let root_context = match ROOT_CONTEXT_FACTORY_MAP
        .lock()
        .unwrap()
        .get(&root_id_str.as_ref())
      {
        Some(root_factory) => root_factory.create(),
        None => unimplemented!(),
      };
      root_context
    }
  };
  let mut locked_root_context_map = ROOT_CONTEXT_MAP.lock().unwrap();
  if !locked_root_context_map.contains_key(&root_context_id) {
    locked_root_context_map.insert(root_context_id, Arc::clone(&root_context));
  }
  root_context
}

pub fn ensure_context(context_id: u32, root_context_id: u32) -> Arc<dyn Context + Sync + Send> {
  let root_id_str = current_root_id_str();
  let context = match CONTEXT_MAP.lock().unwrap().get(&context_id) {
    Some(x) => Arc::clone(x),
    None => {
      let root_context = match ROOT_CONTEXT_MAP.lock().unwrap().get(&root_context_id) {
        Some(root_context) => Arc::clone(root_context),
        None => unimplemented!(),
      };
      let context = match CONTEXT_FACTORY_MAP
        .lock()
        .unwrap()
        .get(&root_id_str.as_ref())
      {
        Some(factory) => factory.create(root_context),
        None => unimplemented!(),
      };
      context
    }
  };
  let mut locked_context_map = CONTEXT_MAP.lock().unwrap();
  if !locked_context_map.contains_key(&context_id) {
    locked_context_map.insert(context_id, Arc::clone(&context));
  }
  context
}

pub fn get_context(context_id: u32) -> Arc<dyn Context + Sync + Send> {
  match CONTEXT_MAP.lock().unwrap().get(&context_id) {
    Some(x) => Arc::clone(x),
    None => unimplemented!(),
  }
}

pub fn get_root_context(root_context_id: u32) -> Arc<dyn RootContext + Sync + Send> {
  match ROOT_CONTEXT_MAP.lock().unwrap().get(&root_context_id) {
    Some(x) => Arc::clone(x),
    None => unimplemented!(),
  }
}
