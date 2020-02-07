use crate::host::*;
use lazy_static::lazy_static;
use std::collections::HashMap;
use std::ffi::CString;
use std::os::raw::c_char;
use std::ptr::null_mut;
use std::sync::{Arc, Mutex};

pub trait RootContext {
  fn on_start(&self) -> u32;
}
pub trait Context {
  fn on_create(&self);
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
  let mut locked_map = ROOT_CONTEXT_MAP.lock().unwrap();
  if !locked_map.contains_key(&root_context_id) {
    locked_map.insert(root_context_id, Arc::clone(&root_context));
  }
  root_context
}

pub fn ensure_context(context_id: u32, root_context_id: u32) -> Arc<dyn Context + Sync + Send> {
  match CONTEXT_MAP.lock().unwrap().get(&context_id) {
    Some(x) => Arc::clone(x),
    None => {
      let root_id_str = current_root_id_str();
      let context = match CONTEXT_FACTORY_MAP
        .lock()
        .unwrap()
        .get(&root_id_str.as_ref())
      {
        Some(factory) => {
          let context = match ROOT_CONTEXT_MAP.lock().unwrap().get(&root_context_id) {
            Some(root_context) => factory.create(Arc::clone(root_context)),
            None => {
              // Can't find specified root_context_id
              // Create new ROOT_CONTEXT
              let root_context = match ROOT_CONTEXT_FACTORY_MAP
                .lock()
                .unwrap()
                .get(&root_id_str.as_ref())
              {
                Some(root_factory) => root_factory.create(),
                None => unimplemented!(),
              };
              factory.create(root_context)
            }
          };
          context
        }
        None => unimplemented!(), // can't find speficied context_id
      };
      context
    }
  }
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
