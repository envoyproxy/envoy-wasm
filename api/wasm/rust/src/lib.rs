mod context;
mod envoy_log;
mod host;

use crate::envoy_log::Logger;
use context::*;
use lazy_static::*;
use log::info;
use std::sync::{Arc, Mutex};

// =============== RootContext ============================
struct SampleRootContext {}

impl RootContext for SampleRootContext {
    fn on_start(&self) -> u32 {
        info!("Hello Envoy!");
        0
    }
}

struct SampleRootContextFactory {}

impl RootContextFactory for SampleRootContextFactory {
    fn create(&self) -> Arc<Mutex<dyn RootContext + Sync + Send>> {
        Arc::new(Mutex::new(SampleRootContext {}))
    }
}
// ========================================================

// =================== Context ============================
struct SampleContext {
    pub root_context: Arc<Mutex<dyn RootContext + Sync + Send>>,
}

impl Context for SampleContext {
    fn on_create(&self) {
        info!("Hello Envoy Create!");
    }
}

struct SampleContextFactory {}

impl ContextFactory for SampleContextFactory {
    fn create(
        &self,
        _root_context: Arc<Mutex<dyn RootContext + Sync + Send>>,
    ) -> Arc<Mutex<dyn Context + Sync + Send>> {
        Arc::new(Mutex::new(SampleContext {
            root_context: _root_context,
        }))
    }
}
// ========================================================

lazy_static! {
    static ref SAMPLE_ROOT_CONTEXT_FACTORY: Box<dyn RootContextFactory + Sync> =
        Box::new(SampleRootContextFactory {});
    static ref SAMPLE_CONTEXT_FACTORY: Box<dyn ContextFactory + Sync> =
        Box::new(SampleContextFactory {});
}

#[no_mangle]
fn _start() {
    Logger::init().unwrap();
    register_factory(
        "my_root_id",
        &SAMPLE_CONTEXT_FACTORY,
        &SAMPLE_ROOT_CONTEXT_FACTORY,
    );
}
